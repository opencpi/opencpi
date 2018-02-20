/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstring>
#include "OcpiOsMisc.h"
#include "OcpiUtilValue.h"
#include "OcpiUtilMisc.h"
#include "CmdOption.h"

// Limitations:
//  No support for multiple letter options in one flag (dubious UI in any case)
//  No support for arguments out of order so you can't apply flags multiple ways for different files
//    This requires the more complex interface of getopt etc.
//  No support for different flags affecting the same value
//    e.g. --quiet vs. --verbose
namespace OCPI {
  namespace Util {
    namespace OA = OCPI::API;
    BaseCommandOptions::
    BaseCommandOptions(Member *members, unsigned nMembers, const char *help, const char **defaults)
      : m_argvCount(0), m_options(members), m_seen(new bool[nMembers]), m_defaults(defaults),
        m_nOptions(nMembers), m_error(""), m_help(help)
    {
      Member *m = members;
      m_names.resize(nMembers);
      for (unsigned n = 0; n < m_nOptions; n++, m++) {
	if (!m->m_default)
	  m->m_default = new Value(*m);
	m_seen[n] = false;
	m_names[n] = m->m_name;
	for (unsigned i = 0; i < m_names[n].length(); i++)
	  if (m_names[n][i] == '_')
	    m_names[n][i] = '-';
      }
    }
    BaseCommandOptions::
    ~BaseCommandOptions() {
      delete [] m_seen;
    }
    const char *BaseCommandOptions::
    setError(const char *err) {
      m_error = err;
      fprintf(stderr, "Error in command options: %s\n", err);
      return err;
    }
    const char *BaseCommandOptions::
    doValue(Member &m, const char *argValue, const char **&a_argv) {
      Value &v = *m.m_default;
      size_t ordinal = &m - m_options;
      bool seen = m_seen[ordinal];
      if (!m.m_isSequence && seen)
	return setError(esprintf("Multiple '%s' options are present, which is not allowed",
				 m_names[ordinal].c_str()));
      m_seen[ordinal] = true;
      if (!m.m_isSequence && m.m_baseType == OA::OCPI_Bool) {
	if (argValue)
	  return setError(esprintf("Extraneous value found in option: '%s'", *a_argv));
	v.m_Bool = m.m_default ? !m.m_default->m_Bool : true;
      } else {
	if (!argValue) {
	  if (!a_argv[1])
	    return setError(esprintf("Missing value for option: '%s'", *a_argv));
	  argValue = a_argv[1];
	  a_argv++;
	}
	const char *err;
	if ((err = v.parse(argValue, NULL, m.m_isSequence))) // seen)))
	  return setError(esprintf("When parsing option '%s', value '%s' invalid: %s",
				   *a_argv, argValue, err));
      }
      return NULL;
    }
    const char *BaseCommandOptions::
    setArgv(const char **ap) {
      m_beforeArgv = ap++;
      const char *err;
      if (!ap[0]) {
	usage();
	return "No arguments specified";
      }
      bool help = false, debug = false;
      for (;ap[0] && ap[0][0] == '-'; ap++) {
	if (ap[0][1] == '-') {
	  if (ap[0][2] == '\0') {
	    ap++;
	    break;
	  }
	  // Long option
	  const char *arg = &ap[0][2];
	  if (!strcasecmp(arg, "help")) {
	    help = true;
	    goto cont2;
	  }
	  if (!strcasecmp(arg, "debug")) {
	    debug = true;
	    goto cont2;
	  }
	  const char *eq = strchr(arg, '=');
	  size_t len = eq ? eq - arg : strlen(arg);
	  Member *m = m_options;
	  for (unsigned n = 0; n < m_nOptions; n++, m++)
	    if (len == m_names[n].size() && !strncasecmp(arg, m_names[n].c_str(), len)) {
	      if ((err = doValue(*m, eq ? eq + 1 : NULL, ap)))
		return err;
	      goto cont2;
	    }
	} else {
	  const char *arg = &ap[0][1];
	  Member *m = m_options;
	  for (unsigned n = 0; n < m_nOptions; n++, m++)
	    if (!strncmp(arg, m->m_abbrev.c_str(), 1)) {
	      if ((err = doValue(*m, arg[1] ? &arg[1] : NULL, ap)))
		return err;
	      goto cont2;
	    }
	}
	return setError(esprintf("Unknown option name/letter in option '%s'", *ap));
      cont2:;
      }
      m_argv = ap;
      while (*ap) ap++;
      m_argvCount = ap - m_argv;
      if (debug) {
	Member *m = m_options;
	for (unsigned n = 0; n < m_nOptions; n++, m++)
	  if (m->m_default->m_parsed) {
	    std::string val;
	    m->m_default->unparse(val);
	    fprintf(stderr, "%s(%s): %s\n", m_names[n].c_str(), m->m_abbrev.c_str(),
		    val.c_str());
	  }
      }
      if (help) {
	usage();
	return "Help requested";
      }
      return NULL;
    }
    int BaseCommandOptions::
    usage() {
      const char *slash = strrchr(m_beforeArgv[0], '/');
      if (slash)
	slash++;
      else
	slash = m_beforeArgv[0];
      std::string exec;
      OCPI::OS::getExecFile(exec);
      fprintf(stderr,
	      "Usage for %s (executed from %s):\n"
	      "Options are:\n", slash, exec.c_str());
      Member *m = m_options;
      size_t width = 10;
      for (unsigned n = 0; n < m_nOptions; n++, m++)
	if (m_names[n].size() > width)
	  width = m_names[n].size();
      m = m_options;
      fprintf(stderr, "  %-*s Letter  Datatype  Multiple? Description\n", (int)width, "Long name");
      for (unsigned n = 0; n < m_nOptions; n++, m++) {
	size_t nlwidth = 1000;
	const char *nl = strchr(m->m_description.c_str(), '\n');
	if (nl)
	  nlwidth = nl - m->m_description.c_str();
	fprintf(stderr, "  %-*s   %-6s  %-8s   %s   %-.*s.", (int)width, m_names[n].c_str(),
		m->m_abbrev.size() ? m->m_abbrev.c_str() : "<none>",
		baseTypeNames[m->m_baseType],
		m->m_isSequence ? "Yes" : "No ", (int)nlwidth, m->m_description.c_str());
	while (nl) {
	  const char *start = ++nl;
	  nl = strchr(start, '\n');
	  nlwidth = nl ? nl - start : 1000;
	  fprintf(stderr, "\n%*s%-.*s", (int)width + 30, "", (int)nlwidth, start);
	}
	if (m_defaults[n])
	  fprintf(stderr, "  Default: %s", m_defaults[n]);
	fprintf(stderr, "\n");
      }
      fprintf(stderr,
	      "Long options are of the form:   --<long-name>[=<value>]\n"
	      "Short options are of the form:  -<letter>[<value>] or -<letter>  <value>\n"
	      "Boolean options have no value; their presence indicates 'true'.  Other types require values.\n"
	      "\n%s", m_help);
      return 1;
    }
    int BaseCommandOptions::
    main(const char **initargv, int (*themain)(const char **a)) {
      try {
	return setArgv(initargv) ? 1 : themain(argv());
      } catch (std::string &e) {
	exitbad(e.c_str());
      } catch (const char *e) {
	exitbad(e);
      } catch (std::exception &e) {
	exitbad(e.what());
      } catch (...) {
	exitbad("Unexpected exception");
      }
      return 0; // not reached.
    }
    void BaseCommandOptions::
    exitbad(const char *e) {
      fprintf(stderr, "Exiting for exception: %s\n", e);
      exit(1);
    }
    void BaseCommandOptions::
    bad(const char *fmt, ...) {
      va_list ap;
      va_start(ap, fmt);
      std::string e = "Exiting for problem: ";
      formatAddV(e, fmt, ap);
      if (m_error.size())
	formatAdd(e, ": %s", m_error.c_str());
      va_end(ap);
      throw e;
    }
  }
}

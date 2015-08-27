/*
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */

// -*- c++ -*-

#ifndef CommandOption_H
#define CommandOption_H

#include <assert.h>
#include "OcpiUtilDataTypes.h"
#include "OcpiUtilValue.h"

namespace OCPI {
  namespace Util {
    class BaseCommandOptions {
      const char **m_beforeArgv, **m_argv; // what is left after options
    protected:
      Member *m_options;
      bool *m_seen;
      const char **m_defaults;
      //      Value *m_values;
      unsigned m_nOptions;
      std::string m_error;
      const char *m_help;
      BaseCommandOptions(Member *members, unsigned nMembers, const char *help, const char **defaults);
      const char *setError(const char *);
      const char *doValue(Member &m, const char *value, const char **&argv);
    public:
      void usage();
      const char *setArgv(const char **argv);
      const char **argv() const { return m_argv; }
      std::string &error() { return m_error; }
      int main(const char **argv, int (*main)(const char **argv));
      void exitbad(const char *e);
      void bad(const char *fmt, ...);
    };
  }
}
#ifdef OCPI_OPTIONS
#define CMD_OPTION_S(n,b,d,t,v) CMD_OPTION(n,b,d,t,v)
namespace {
#if !defined(OCPI_OPTIONS_CLASS_NAME)
#define OCPI_OPTIONS_CLASS_NAME CommandOptions
#endif
#if !defined(OCPI_OPTIONS_NAME)
#define OCPI_OPTIONS_NAME options
#endif
#if !defined(OCPI_OPTIONS_HELP)
#define OCPI_OPTIONS_HELP ""
#endif
  class OCPI_OPTIONS_CLASS_NAME : public OCPI::Util::BaseCommandOptions {
    // Define the enumeration of options, and the limit
    enum Option {
#define CMD_OPTION(n,b,t,v,d) Option_##n,
     OCPI_OPTIONS
#undef CMD_OPTION
     CMD_OPTION_LIMIT_
    };
    static OCPI::Util::Member s_options[CMD_OPTION_LIMIT_];
    static const char *s_defaults[CMD_OPTION_LIMIT_];
#undef  CMD_OPTION_S
    //    static OCPI::Util::Value s_values[CMD_OPTION_LIMIT_];
  public:
#define CMD_OPTION(n,b,t,v,d)						\
    OCPI::API::t n() const { return m_options[Option_##n].m_default->m_##t; }
#define CMD_OPTION_S(n,b,t,v,d)		   \
    OCPI::API::t *n(size_t &num) const {                         \
      OCPI::Util::Value *val_ = m_options[Option_##n].m_default; \
      assert(val_);                                              \
      assert(val_->m_vt);                                        \
      assert(val_->m_vt->m_isSequence);                          \
      num = val_->m_nElements;                                   \
      return val_->m_p##t;                                       \
    }
    OCPI_OPTIONS
#undef CMD_OPTION
#undef CMD_OPTION_S
    OCPI_OPTIONS_CLASS_NAME()
      : BaseCommandOptions(s_options, CMD_OPTION_LIMIT_, OCPI_OPTIONS_HELP, s_defaults) {
    };
  };
#define CMD_OPTION(n,b,t,v,d) OCPI::Util::Member(#n,#b,d,OCPI::API::OCPI_##t,false,v),
#define CMD_OPTION_S(n,b,t,v,d) OCPI::Util::Member(#n,#b,d,OCPI::API::OCPI_##t,true,v),
  OCPI::Util::Member OCPI_OPTIONS_CLASS_NAME::s_options[CMD_OPTION_LIMIT_] = { OCPI_OPTIONS };
#undef CMD_OPTION
#undef CMD_OPTION_S
#define CMD_OPTION(n,b,t,v,d) v,
#define CMD_OPTION_S(n,b,t,v,d) v,
  const char *OCPI_OPTIONS_CLASS_NAME::s_defaults[CMD_OPTION_LIMIT_] = { OCPI_OPTIONS };
#undef CMD_OPTION
#undef CMD_OPTION_S
  OCPI_OPTIONS_CLASS_NAME OCPI_OPTIONS_NAME;
}
#endif
#endif

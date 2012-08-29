
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
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


#include <string.h>
#include <stdio.h>
#include "OcpiOsAssert.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilMisc.h"
#include "sca_props.h"
#include "idl2ifr.h"

namespace OCPI { namespace SCA {
    namespace OU = OCPI::Util;

// Return a single string, to be freed by caller, or NULL on error.
 char *
encode_props(Property *properties, unsigned nprops, unsigned size,
             Port *ports, unsigned nports,
             Test *tests, unsigned ntests)
{
  Property *p;
  unsigned length, n, nmems, ntestps;
  
  // Compute length of string encoding and allocate.
  for (p = properties, length = 0, nmems = 0, n = 0; n < nprops; n++, p++) {
    length += strlen(p->name) + 1;
    ocpiAssert(p->num_members);
    nmems += p->num_members;
  }

  Port *port;
  for (port = ports, n = 0; n < nports; n++, port++)
    length += strlen(port->name) + 1;

  Test *test;
  for (test = tests, ntestps=0, n=0; n < ntests; n++, test++)
    ntestps += test->numInputs + test->numResults;
    
  // This is pretty lame...
  unsigned total =
    2 + // header delimiter and final null
    5 * 11 + // 3 values in header
    nprops * (4 * 11 + 9 + 2) + // 4 values, 9 booleans, + delimeter
    nmems * (1 + 11) + // one char and one number per member
    nports * 2 + // 2 booleans, + delimiter
    ntests * (3 * 11 + 3) + // 3 values and 3 delimiters per test, plus ...
    ntestps * (11 + 1) +    // 1 value and 1 delimiter per test parameter
    length; // strings
  
  char *props = (char *)malloc(total);
  if (!props)
    return NULL;
  char *cp = props;
  // header
  cp += sprintf(cp, "%u/%u/%u/%u/%u/%u/%u$", nports, nprops, size, nmems, length, ntests, ntestps);
  // Encode properties.
  for (p = properties, n = 0; n < nprops; n++, p++) {
    ocpiAssert(!strchr(p->name, '~'));
    cp += sprintf(cp, "%s~%lu/%lu/%lu/%lu/%c%c%c%c%c%c%c%c%c|", 
                  p->name, p->num_members, p->sequence_size, p->offset, p->data_offset,
                  p->is_sequence ? '1' : '0',
                  p->is_struct ? '1' : '0',
                  p->is_readable ? '1' : '0',
                  p->is_writable ? '1' : '0',
                  p->read_error ? '1' : '0',
                  p->write_error ? '1' : '0',
                  p->read_sync ? '1' : '0',
                  p->write_sync ? '1' : '0',
                  p->is_test ? '1' : '0');
    // Now do the structure members (or the single simple data type)
    unsigned nm = p->num_members;
    for (SimpleType *t = p->types; nm--; t++)
      cp += sprintf(cp, "%c%lu/",'a'+ (unsigned)t->data_type, t->size);
    // Terminate the property
    *cp++ = '$';
  }

  // Encode ports
  for (port = ports, n = 0; n < nports; n++, port++) {
    ocpiAssert(!strchr(port->name, '~'));
    cp += sprintf(cp, "%s~%c%c|", port->name, port->provider + '0', port->twoway +'0');
  }

  // Encode tests
  for (test = tests, n = 0; n < ntests; n++, test++) {
    unsigned int pi, *v;
    cp += sprintf (cp, "%u/%u/%u|", test->testId, test->numInputs, test->numResults);
    for (pi=0, v=test->inputValues; pi<test->numInputs; pi++, v++) {
      cp += sprintf (cp, "%u/", *v);
    }
    *cp++ = '|';
    for (pi=0, v=test->resultValues; pi<test->numResults; pi++, v++) {
      cp += sprintf (cp, "%u/", *v);
    }
    *cp++ = '$';
  }

  *cp++ = 0;
  ocpiAssert(cp - props <= (int)total);
  return props;
}
static const char *names[] = {
  0,
#define SCA_DATA_TYPE(l,u,c,y)  #u,
SCA_DATA_TYPES
#undef SCA_DATA_TYPE
};
    
static void emit_props(FILE *f, Property *props, unsigned nProps, bool impl) {
  bool first = true;
  if (nProps) {
    Property *p = props;
    for (unsigned n = 0; n < nProps; n++, p++) {
      if (p->is_impl != impl)
	continue;
      if (first) {
	fprintf(f, "  <Properties>\n");
	first = false;
      }
      fprintf(f, "    <Property Name=\"%s\"", p->name);
      if (p->is_readable)
	fprintf(f, " Readable=\"true\"");
      if (p->is_writable)
	fprintf(f, " Writable=\"true\"");
      if (p->is_sequence)
	fprintf(f, " SequenceLength=\"%lu\"", p->sequence_size);
      if (p->is_struct) {
	fprintf(f, " Type=\"Struct\">\n");
	SimpleType *s = p->types;
	for (unsigned m = 0; m < p->num_members; m++, s++) {
	  fprintf(f, "      <Member Name=\"m%d\" Type=\"%s\"", m,
		  s->data_type == SCA_octet ? "UChar" : names[s->data_type]);
	  if (s->data_type == SCA_string)
	    fprintf(f, " StringLength=\"%lu\"", s->size);
	  fprintf(f, "/>\n");
	}
	fprintf(f, "    </Property>\n");
      } else {
	fprintf(f, " Type=\"%s\"",
		p->types->data_type == SCA_octet ? "UChar" : names[p->types->data_type]);
	if (p->types->data_type == SCA_string)
	  fprintf(f, " StringLength=\"%lu\"", p->types->size);
	fprintf(f, "/>\n");
      }
    }
    if (!first)
      fprintf(f, "  </Properties>\n");
  }
}
 static const char *
 doPort(FILE *f, OCPI::SCA::Port *p, unsigned n, const char *repo, bool debug)
{
  ( void ) n;
  ( void ) debug;
  char *rid;
  asprintf(&rid, "\n%s\n", p->repid);
  const char *cp = strcasestr(repo, rid);
  if (!cp)
    return OU::esprintf("Didn't find interface definition for REPID: %s in IDL files provided", p->repid);
  cp += strlen(rid);
  cp = strchr(cp, '\n'); // skip interface name
  if (!cp)
    return "Corrupted interface repository";
  cp++;
  const char *end = strstr(cp, "</Protocol>\n");
  if (!end)
    return "Corrupted interface repository";
  end = strchr(end, '\n');
  if (!end)
    return "Corrupted interface repository";
  fwrite(cp, 1, end + 1 - cp, f);
#if 0
  fprintf(f, "    <Protocol>\n");
  for (unsigned i = 0; i < nops; ++i) {
    CORBA::OperationDef_var op =
      CORBA::OperationDef::_narrow (operations[i].in());
    bool twoway = op->mode() == CORBA::OP_NORMAL;
    if (twoway)
      p->twoway = true;
    fprintf(f, "      <Operation Name=\"%s\"%s", op->name(), twoway ? "Twoway=\"true\"" : "");
    CORBA::ParDescriptionSeq_var args = op->params();
    unsigned nargs = args->length();
    if (!nargs)
      fprintf(f, "/>\n");
    else {
      fprintf(f, ">\n");
      for (unsigned j = 0; j < nargs; j++) {
	CORBA::ParameterDescription *arg = &args[j];
	CORBA::TypeCode *type = arg->type.in();
	CORBA::TCKind kind = type->kind();
	// Bypass typedefs
	while (kind == CORBA::tk_alias) {
	  type = type->content_type();
	  kind = type->kind();
	}
	fprintf(f, "        <Argument Name=\"%s\"",
		(const char *)arg->name);	
	if (kind == CORBA::tk_sequence || kind == CORBA::tk_array) {
	  fprintf(f, " %s=\"%lu\"",
		  kind == CORBA::tk_sequence ? "SequenceLength" : "ArrayLength",
		  (unsigned long)type->length());
	  do {
	    type = type->content_type();
	    kind = type->kind();
	  } while (kind != CORBA::tk_alias);
	}
	// We'll order these in the order of our type system
	const char *ocpiType;
	switch (kind) {
	case CORBA::tk_boolean:
	  ocpiType = "Boolean";
	  break;
	case CORBA::tk_char:
	  ocpiType = "Char";
	  break;
	case CORBA::tk_double:
	  ocpiType = "Double";
	  break;
	case CORBA::tk_float:
	  ocpiType = "Float";
	  break;
	case CORBA::tk_short:
	  ocpiType = "Short";
	  break;
	case CORBA::tk_long:
	  ocpiType = "Long";
	  break;
	case CORBA::tk_octet: // unsigned char
	  ocpiType = "UChar";
	  break;
	case CORBA::tk_ulong:
	case CORBA::tk_enum: // CDR says marshalled as ulong
	  ocpiType = "Ulong";
	  break;
	case CORBA::tk_ushort:
	  ocpiType = "Ushort";
	  break;
	case CORBA::tk_longlong:
	  ocpiType = "Longlong";
	  break;
	case CORBA::tk_ulonglong:
	  ocpiType = "Ulonglong";
	  break;
	case CORBA::tk_string:
	case CORBA::tk_objref: // modeled as a string
	  ocpiType = "String";
	  fprintf(f, " StringLength=\"%lu\"", (unsigned long)type->length());	
	  break;
	case CORBA::tk_struct:
	  ocpiType = "Struct";
	  break;
	  // The rest are not allowed
	case CORBA::tk_array: // 2d or more not supported
	case CORBA::tk_sequence: // 2d or more not supported
	case CORBA::tk_null:
	case CORBA::tk_void:
	case CORBA::tk_any:
	case CORBA::tk_TypeCode:
	case CORBA::tk_Principal:
	case CORBA::tk_union:
	case CORBA::tk_alias:
	case CORBA::tk_except:
	case CORBA::tk_longdouble:
	case CORBA::tk_wchar:
	case CORBA::tk_wstring:
	case CORBA::tk_fixed:
	case CORBA::tk_value:
	case CORBA::tk_value_box:
	case CORBA::tk_native:
	case CORBA::tk_abstract_interface:
	case CORBA::tk_local_interface:
	case CORBA::tk_component:
	case CORBA::tk_home:
	case CORBA::tk_event:
	default:
	  return OU::esprintf("Argument data type unsupported for parameter \"%s\" of operation \"%s\" type %d",
			  (const char *)op->name(), (const char *)arg->name, kind);
	}
	fprintf(f, " Type=\"%s\"/>\n", ocpiType);	

	// All other info depends on kind.
	if (debug)
	  printf("arg %s type kind %d\n", (const char *)arg->name, kind);
      }
      fprintf(f, "      </Operation>\n");
    }

    if (debug)
      printf("op %d: %s(%d)\n", i, op->name(), twoway);
  }
  fprintf(f, "    </Protocol>\n");
#endif
  return 0;
}
const char *emit_ocpi_xml(const char *specFile, const char *implFile,
			  const char *specName, const char *implName,
			  const char *parentFile, const char *model,
			  char *idlFiles[], bool debug,
			  Property *properties, unsigned nProps,
			  Port *ports, unsigned nPorts,
			  Test *tests, unsigned nTests)
{
  ( void ) tests;
  ( void ) nTests;
#if 0
  // We use the repository to get idl info back
  CORBA::Repository_var repo;
#else
  char *repo;
#endif
  // read all the idl files into the interface repository
  const char *err = idl2ifr(idlFiles, repo);
  if (err)
    return err;
  FILE *f = fopen(specFile, "w");
  if (f == NULL)
    return "can't open output spec file";
  time_t now = time(0);
  char *ct = ctime(&now);
  ct[strlen(ct) - 1] = '\0';
  struct tm *local = localtime(&now);
  // FIXME: share this code with wip_HDL.c openOutput etc.
  fprintf(f,
	  "<?xml version=\"1.0\"?>\n"
	  "<!-- THIS FILE WAS GENERATED ON %s %s\n"
	  "     BASED ON THE FILE: %s\n"
	  "     YOU PROBABLY SHOULD NOT EDIT IT\n"
	  " -->\n",
	  ct, local->tm_zone, parentFile);
  fprintf(f,
	  "<ComponentSpec Name=\"%s\">\n", specName);
  emit_props(f, properties, nProps, false);
  Port *p = ports;
  for (unsigned n = 0; n < nPorts; n++, p++) {
    fprintf(f, "  <DataInterfaceSpec Name=\"%s\" Producer=\"%s\">\n",
	    p->name, p->provider ? "false" : "true");
    if ((err = doPort(f, p, n, repo, debug)))
      return err;
    // Here is where we put the idl-based info
    fprintf(f, "  </DataInterfaceSpec>\n");
  }
  
  fprintf(f,
	  "</ComponentSpec>\n");
  fclose(f);
  f = fopen(implFile, "w");
  if (f == NULL)
    return "can't open output impl file";
  fprintf(f,
	  "<?xml version=\"1.0\"?>\n"
	  "<!-- THIS FILE WAS GENERATED ON %s %s\n"
	  "     BASED ON THE FILE: %s\n"
	  "     YOU MAY WANT TO EDIT IT to add implementation-specific aspects\n"
	  " -->\n",
	  ct, local->tm_zone, parentFile);
  const char *specFileName = strrchr(specFile, '/');
  fprintf(f,
	  "<%sImplementation Name=\"%s\">\n"
	  "  <xi:include href=\"%s\">\n",
	  model, implName, specFileName ? specFileName + 1 : specFile);
  emit_props(f, properties, nProps, true);

  fprintf(f,
	  "</%sImplementation>\n", model);
  
  fclose(f);
  return 0;
}
}} //namespace

// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

#include <string.h>
#include <stdio.h>
#include <CpiOsAssert.h>
#include "sca_props.h"


namespace CPI { namespace SCA {

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
    cpiAssert(p->num_members);
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
    cpiAssert(!strchr(p->name, '~'));
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
    cpiAssert(!strchr(port->name, '~'));
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
  cpiAssert(cp - props <= (int)total);
  return props;
}
}} //namespace

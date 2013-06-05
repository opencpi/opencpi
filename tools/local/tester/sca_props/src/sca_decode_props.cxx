
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


/*
 * Decode "magic property string".
 *
 * Revision History:
 *
 *     06/15/2009 - Frank Pilhofer
 *                  Fix coverity warnings.
 *
 *     02/20/2009 - Frank Pilhofer
 *                  Add support for test properties.
 *
 *     10/17/2008 - Frank Pilhofer
 *                  Avoid assertions with side effects.
 *
 *     10/02/2008 - Jim Kulp
 *                  Initial version.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <OcpiOsAssert.h>
#include "sca_props.h"


namespace OCPI { namespace SCA {

// return an array of properties, to be freed (as one thing) by caller,
// or NULL on error
    bool
decode_props(const char *props,
             Property **propsp, size_t *npropsp, size_t *sizep,
             Port **portsp, size_t *nportsp,
             Test **testsp, size_t *ntestsp)
{
  size_t nprops, namesize, nmembers, size, nports, ntests, ntestps, n;
  const char *cp;

  n = sscanf (props, "%zu/%zu/%zu/%zu/%zu/%zu/%zu$",
              &nports, &nprops, &size, &nmembers, &namesize,
              &ntests, &ntestps);

  if (n == 5) { // For backwards compatibility
    ntests = ntestps = 0;
  }
  else if (n != 7) {
    return true;
  }

  if (!(cp = strchr(props, '$'))) {
    return true;
  }
  cp++;

  // Compute allocation to hold all properties, names, types etc.
  size_t memSize = (nprops * sizeof(Property) +
		    nmembers * sizeof(SimpleType) +
		    nports * sizeof(Port) + 
		    ntests * sizeof(Test) +
		    ntestps * sizeof(unsigned int) +
		    namesize);
  Property *properties = (Property *) malloc (memSize);
  if (!properties)
    return true;

  SimpleType *s = (SimpleType *)(properties + nprops);
  Port *ports = (Port *)(s + nmembers);
  Test *tests = (Test *)(ports + nports);
  unsigned int *testps = (unsigned int *)(tests + ntests);
  char *names = (char *)(testps + ntestps);

  // Decode each property
  Property *p = properties;
  char c[10];
  for (n = nprops; n; p++, n--) {
    if (!*cp || sscanf(cp, "%[^~]~%lu/%lu/%lu/%lu/%[^|]|",
                       names, &p->num_members, &p->sequence_size, &p->offset, &p->data_offset, c) != 6)
      break;
    p->name = names;
    names += strlen(names) + 1;
    ocpiAssert ((unsigned int ) (names - (char *) properties) <= memSize);
    p->types = s;
    p->is_sequence = c[0] == '1';
    p->is_struct = c[1] == '1';
    p->is_readable = c[2] == '1';
    p->is_writable = c[3] == '1';
    p->read_error = c[4] == '1';
    p->write_error = c[5] == '1';
    p->read_sync = c[6] == '1';
    p->write_sync = c[7] == '1';
    p->is_test = c[8] == '1';
    if (!(cp = strchr(cp, '|')))
      break;
    cp++;
    for (nmembers = p->num_members; nmembers; nmembers--) {
      char c;
      if (sscanf(cp, "%c%lu/", &c, &s->size) != 2)
        break;
      s->data_type = (DataType)(c - 'a');
      s++;
      if (!(cp = strchr(cp, '/')))
        break;
      cp++;
    }
    if (nmembers || *cp++ != '$')
      break;
  } 
  if (n) {
    free(properties);
    return true;
  }

  // Decode each port
  Port *port = ports;
  for (n = nports; n; n--, port++) {
    if (!*cp || sscanf(cp, "%[^~]~%[^|]|", names, c) != 2)
      break;
    port->name = names;
    names += strlen(names) + 1;
    port->provider = c[0] == '1';
    port->twoway = c[1] == '1';
    if (!(cp = strchr(cp, '|')))
      break;
    cp++;
  }
  if (n) {
    free(properties);
    return true;
  }

  // Decode each test
  Test *test = tests;
  for (n = ntests; n; n--, test++) {
    if (!*cp || sscanf (cp, "%u/%u/%u|", &test->testId, &test->numInputs, &test->numResults) != 3) {
      break;
    }
    if (!(cp = strchr (cp, '|'))) {
      break;
    }
    cp++;

    test->inputValues = testps;
    for (nmembers = test->numInputs; nmembers; nmembers--, testps++) {
      if (sscanf (cp, "%u/", testps) != 1) {
        break;
      }
      if (!(cp = strchr (cp, '/'))) {
        break;
      }
      cp++;
    }

    if (!cp || *cp++ != '|') {
      break;
    }

    test->resultValues = testps;
    for (nmembers = test->numResults; nmembers; nmembers--, testps++) {
      if (sscanf (cp, "%u/", testps) != 1) {
        break;
      }
      if (!(cp = strchr (cp, '/'))) {
        break;
      }
      cp++;
    }

    if (!cp || *cp++ != '$') {
      break;
    }
  }

  if (n) {
    free (properties);
    return true;
  }

  *portsp = ports;
  *propsp = properties;
  *testsp = tests;
  *nportsp = nports;
  *npropsp = nprops;
  *ntestsp = ntests;
  *sizep = size;

  return false;
}
}} //namespace

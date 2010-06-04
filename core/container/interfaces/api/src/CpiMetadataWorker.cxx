/*
 * Configure worker properties according to the property sheet.
 *
 * Revision History:
 *
 *     05/06/2009 - Frank Pilhofer
 *                  RCC updates (interpretation of maximum string size to
 *                  exclude the null character).
 *
 *     02/25/2009 - Frank Pilhofer
 *                  Add support for test properties.
 *                  Add accessor for list of ports.
 *                  Constify accessors.
 *
 *     11/13/2008 - Frank Pilhofer
 *                  Add missing getPort operation.
 *
 *     10/22/2008 - Frank Pilhofer
 *                  String size includes the null character.
 *
 *     10/17/2008 - Frank Pilhofer
 *                  Don't place code with side effects in assertions.
 *                  Integrate with WCI API updates.
 *
 *     10/02/2008 - Jim Kulp
 *                  Initial version.
 */

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <CpiOsAssert.h>
#include "CpiMetadataWorker.h"
#include <CpiContainerMisc.h>

namespace CPI {
  namespace CC = CPI::Container;
  namespace Metadata {

    Worker::Worker(const char *workerMetadata) 
      : myProps( NULL )
    {
      if ( ! workerMetadata ) {
        return;
      }
      if (decode(workerMetadata))
        throw CC::ApiError("Worker xml metadata invalid", 0);
    }
    Worker::Worker(ezxml_t xml) {
      if (decode(xml))
        throw CC::ApiError("Worker encoded metadata invalid", 0);
    }
    Worker::~Worker() {
      if ( myProps ) {
        delete [] (char *)myProps;
      }
    }
    unsigned Worker::whichProperty(const char *id) {
      Property *p = myProps;
      for (unsigned n=0; n < nProps; n++, p++)
        if (!strcmp(p->name, id))
          return n;
      throw CC::ApiError("Unknown property: \"", id, "\"", 0);
    }
    Property &Worker::findProperty(const char *id) {
      return *(myProps + whichProperty(id));
    }
    Port *Worker::findPort(const char *id) {
      Port *p = myPorts;
      for (unsigned int n=nPorts; n; n--, p++) {
        if ( strcmp(p->name,id)==0 )
          return p;
      }
      return 0;
    }
    Test &Worker::findTest(unsigned int testId) {
      assert(0); static Test *t; return *t;
    }
    uint8_t Property::tsize[CPI_data_type_limit + 1] = {
      0,// for CPI_NONE
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) bits/CHAR_BIT,
      CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE
      0
    };

    // return an array of properties, to be freed (as one thing) by caller,
    // or NULL on error
    bool
    Worker::decode(const char *metadata)
    {
      unsigned nameSize, nMembers, nTestPs, n;
      char *cp;

      n = sscanf (metadata, "%u/%u/%u/%u/%u/%u/%u$",
                  &nPorts, &nProps, &size, &nMembers, &nameSize,
                  &nTests, &nTestPs);

      if (n == 5) { // For backwards compatibility
        nTests = nTestPs = 0;
      }
      else if (n != 7) {
        return true;
      }

      if (!(cp = strchr(metadata, '$'))) {
        return true;
      }
      cp++;

      // Compute allocation to hold all properties, names, types etc.
      unsigned int memSize = (nProps * sizeof(Property) +
                              nMembers * sizeof(Property::SimpleType) +
                              nPorts * sizeof(Port) + 
                              nTests * sizeof(Test) +
                              nTestPs * sizeof(unsigned int) +
                              nameSize);
      Property *properties = (Property *) new char[memSize];
      Property::SimpleType *s = (Property::SimpleType *)(properties + nProps);
      Port *ports = (Port *)(s + nMembers);
      Test *tests = (Test *)(ports + nPorts);
      unsigned int *testps = (unsigned int *)(tests + nTests);
      char *names = (char *)(testps + nTestPs);

      // Decode each property
      Property *p = properties;
      char c[10];
      for (unsigned n = 0; n <  nProps; p++, n++) {
        if (!*cp || sscanf(cp, "%[^~]~%lu/%lu/%lu/%lu/%[^|]|",
                           names, &p->numMembers, &p->sequence_size, &p->offset, &p->data_offset, c) != 6)
          break;
        p->name = names;
        p->ordinal = n;
        names += strlen(names) + 1;
        cpiAssert ((unsigned)(names - (char *) properties) <= memSize);
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
        for (nMembers = p->numMembers; nMembers; nMembers--) {
          char c;
          if (sscanf(cp, "%c%lu/", &c, &s->size) != 2)
            break;
          s->type = (Property::Type)(c - 'a');
          s++;
          if (!(cp = strchr(cp, '/')))
            break;
          cp++;
        }
        if (nMembers || *cp++ != '$')
          break;
      } 
      if (n) {
        delete [] (char*)properties;
        return true;
      }

      // Decode each port
      Port *port = ports;
      for (n = nPorts; n; n--, port++) {
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
        delete [] (char*)properties;
        return true;
      }

      // Decode each test
      Test *test = tests;
      for (n = nTests; n; n--, test++) {
        if (!*cp || sscanf (cp, "%u/%u/%u|", &test->testId, &test->numInputs, &test->numResults) != 3) {
          break;
        }
        if (!(cp = strchr (cp, '|'))) {
          break;
        }
        cp++;

        test->inputValues = testps;
        for (nMembers = test->numInputs; nMembers; nMembers--, testps++) {
          if (sscanf (cp, "%u/", testps) != 1) {
            break;
          }
          if (!(cp = strchr (cp, '/'))) {
            break;
          }
          cp++;
        }

        if (*cp++ != '|') {
          break;
        }

        test->resultValues = testps;
        for (nMembers = test->numResults; nMembers; nMembers--, testps++) {
          if (sscanf (cp, "%u/", testps) != 1) {
            break;
          }
          if (!(cp = strchr (cp, '/'))) {
            break;
          }
          cp++;
        }

        if (*cp++ != '$') {
          break;
        }
      }

      if (n) {
        delete [] (char*)properties;
        return true;
      }

      myPorts = ports;
      myProps = properties;
      myTests = tests;
      return false;
    }
    static bool decodeMember(ezxml_t x, Property::SimpleType *s, unsigned &maxAlign) {
      static const char *tnames[] = {
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) #pretty,
        "None",
        CPI_PROPERTY_DATA_TYPES
        0
      };
      const char *type = ezxml_attr(x, "type");
      if (!type)
        return true;
      for (unsigned i = 0; tnames[i]; i++)
        if (strcmp(tnames[i], type) == 0) {
          s->type = (Property::Type)i;
          const char *a = ezxml_attr(x, "size");
          s->size = a ? strtoul(a, NULL, 10) : 1;
          if (Property::tsize[i] > maxAlign)
            maxAlign = Property::tsize[i];
          return false;
        }
      return true;
    }
    static bool getBoolean(ezxml_t x, const char *name) {
      const char *a = ezxml_attr(x, name);
      if (a) {
        if (!strcmp(a, "true") || !strcmp(a, "1"))
          return true;
        else if (!strcmp(a, "false")  || !strcmp(a, "0"))
          return false;
        throw CC::ApiError("Bad boolean value: \n", a, "\"", 0);
      } else
        return false; // booleans default to false;
    }
    inline void roundUp(unsigned offset, unsigned n) {
      offset += n - 1;
      offset &= ~(n - 1);
    }
    bool Property::decode(ezxml_t x, SimpleType *&s) {
      name = ezxml_attr(x, "name");
      if (!name)
          return true;
      const char *a = ezxml_attr(x, "sequenceSize");
      if (a) {
        is_sequence = true;
        sequence_size = strtoul(a, NULL, 10);
      } else {
        is_sequence = false;
        sequence_size = 1;
      }
      types = s;
      maxAlign = 4;
      ezxml_t y = ezxml_child(x, "member");
      if (y) {
        is_struct = true;
        unsigned i;
        for (i = 0; y; i++, y = ezxml_next(y))
          if (decodeMember(y, s++, maxAlign))
            return true;
        numMembers = i;
      } else {
        is_struct = false;
        if (decodeMember(x, s++, maxAlign))
          return true;
        numMembers = 1;
      }
      is_readable = getBoolean(x, "readable");
      is_writable = getBoolean(x, "writable");
      read_error = getBoolean(x, "readError");
      write_error = getBoolean(x, "writeError");
      read_sync = getBoolean(x, "readSync");
      write_sync = getBoolean(x, "writeSync");
      return false;
    }
    // Alignment rules are:
    // Every property aligned at least at 4 byte boundary.
    // Every property aligned at largest required alignment (which might be 64 bits).
    // Sequence count is aligned at largest rquired alignment (which might be 64 bits).
    // Structure members aligned per member requirements, no 4 byte requirement.
    // Sequence of structures: each struct in sequence aligned per max member alignment.
    void Property::align(unsigned theOrdinal, unsigned &theOffset) {
      ordinal = theOrdinal;
      roundUp(theOffset, maxAlign);
      offset = theOffset;
      if (is_sequence)
        theOffset += maxAlign;
      SimpleType *st = types;
      unsigned dataOffset = theOffset;
      for (unsigned m = 0; m < numMembers; m++, st++) {
        roundUp(theOffset, tsize[st->type]);
        if (st->type == CPI_String)
          theOffset += st->size + 1;
        else
          theOffset += tsize[st->type];
      }
      if (is_sequence) {
        roundUp(theOffset, maxAlign);
        theOffset += dataOffset + sequence_size * (theOffset - dataOffset);
      }
      size = theOffset - offset;
    }
    bool Port::decode(ezxml_t x, int pid) {
      myXml = x;
      name = ezxml_attr(x, "name");

      printf("Port %s has pid = %d\n", name, pid );

      m_pid = pid;
      if ( name == NULL )
        return true;
      twoway = getBoolean(x, "twoWay");
      provider = getBoolean(x, "provider");


      bool found;
      int n = CC::getAttrNum(x, "minBufferSize", true, &found);
      if (found)
	minBufferSize = n;
      n = CC::getAttrNum(x, "maxBufferSize", true, &found);
      if (found)
	maxBufferSize = n; // no max if not specified.
      n = CC::getAttrNum(x, "minNumBuffers", true, &found);
      if (found)
	minBufferCount = n;

      return false;
    }
    // Decode based on XML, determining offsets
    bool Worker::decode(ezxml_t xml) {
      ezxml_t x;
      unsigned nMembers;
      // First pass - just count for allocation
      nProps = nPorts = nTests = nMembers = 0;
      for (x = ezxml_child(xml, "property"); x; x = ezxml_next(x)) {
        nProps++;      
        ezxml_t y = ezxml_child(xml, "member");
        if (y)
          do 
            nMembers++;
          while ((y = ezxml_next(y)));
        else
          nMembers++;
      }
      for (x = ezxml_child(xml, "port"); x; x = ezxml_next(x))
        nPorts++;      
      unsigned int memSize = (nProps * sizeof(Property) +
                              nMembers * sizeof(Property::SimpleType) +
                              nPorts * sizeof(Port));
      Property *prop = myProps = (Property *) new char[memSize];
      Property::SimpleType *s = (Property::SimpleType *)(myProps + nProps);
      Port *port = myPorts = (Port *)(s + nMembers);
      // Second pass - decode all information
      for (x = ezxml_child(xml, "property"); x; x = ezxml_next(x), prop++)
        if (prop->decode(x, s))
          throw CC::ApiError("Invalid xml property description", 0); // can't decode property
      // Third pass - determine offsets and total sizes
      unsigned offset = 0;
      prop = myProps;
      for (unsigned i = 0; i < nProps; i++, prop++)
        prop->align(i, offset);
      // Ports at this level are unidirectional? Or do we support the pairing at this point?
      int pid=0;
      for (x = ezxml_child(xml, "port"); x; x = ezxml_next(x), port++) {
        if (port->decode(x,pid++)) {
          throw CC::ApiError("Invalid xml port description", 0);
        }
      }
      // Control operations
      const char *ops = ezxml_attr(xml, "controlOperations");
#define CONTROL_OP(x, c, t, s1, s2, s3)  has##c = ops && strstr(ops, #x) != NULL;
      CPI_CONTROL_OPS
#undef CONTROL_OP
        return false;
    }
  }
}


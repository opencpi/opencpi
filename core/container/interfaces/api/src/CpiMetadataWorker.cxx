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
#include "CpiOsAssert.h"
#include "CpiMetadataWorker.h"
#include "CpiContainerMisc.h"
#include "CpiUtilEzxml.h"

namespace CPI {
  namespace CC = CPI::Container;
  namespace CP = CPI::Util::Prop;
  namespace CE = CPI::Util::EzXml;
  namespace Metadata {

    Worker::~Worker() {
	if (myPorts)
	  delete [] myPorts;
	if (myProps)
	  delete [] myProps;
	if (myTests)
	  delete [] myTests;
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
       (void)testId;
      assert(0); static Test *t; return *t;
    }
    bool Port::decode(ezxml_t x, int pid) {
      myXml = x;
      name = ezxml_cattr(x, "name");

      printf("Port %s has pid = %d\n", name, pid );

      m_pid = pid;
      if ( name == NULL )
        return true;
      if (CE::getBoolean(x, "twoWay", &twoway) ||
	  CE::getBoolean(x, "provider", &provider))
	return true;
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
    Worker::Worker(ezxml_t xml)
      : myProps(0), myPorts(0), myTests(0), nProps(0), nPorts(0), nTests(0),
	size(0)
    {
      ezxml_t x;
      // First pass - just count for allocation
      for (x = ezxml_cchild(xml, "property"); x; x = ezxml_next(x)) 
        nProps++;
      for (x = ezxml_cchild(xml, "port"); x; x = ezxml_next(x))
        nPorts++;
      if (nProps)
	myProps = new Property[nProps];
      if (nPorts)
	myPorts = new Port[nPorts];
      const char *err;
      // Second pass - decode all information
      Property *prop = myProps;
      unsigned offset = 0;
      bool readableConfigs, writableConfigs, sub32Configs; // all unused
      for (x = ezxml_cchild(xml, "property"); x; x = ezxml_next(x), prop++)
        if ((err = prop->parse(x, offset, readableConfigs, writableConfigs,
			       sub32Configs, true)))
          throw CC::ApiError("Invalid xml property description:", err, NULL);
      // Ports at this level are unidirectional? Or do we support the pairing at this point?
      unsigned pid = 0;
      Port *p = myPorts;
      for (x = ezxml_cchild(xml, "port"); x; x = ezxml_next(x), p++)
        if (p->decode(x, pid++))
          throw CC::ApiError("Invalid xml port description", 0);
    }
  }
}


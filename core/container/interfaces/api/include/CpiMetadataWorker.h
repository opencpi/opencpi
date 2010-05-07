/*

 Definitions for worker metadata encoding,decoding.
 Offline, in tools, this information is encoded into a string format 
 suitable for program level arguments (argv).  All properties are encoded into
 a single string.  This relieves runtime of any parsing overhead (time or space)
 or dependencies on external parsing libraries.

 The "scope" of this property support is configuration properties for CP289
 components.  Thus it is not (yet) intended to support SCA GPP components.

 This file defines the binary (non-string) format of SCA component properties,
 as well as the functions to encode (binary to string) and decode 
 (string to binary).

 
*/
#ifndef CPI_WORKER_METADATA_H
#define CPI_WORKER_METADATA_H

#include <CpiOsAssert.h>
#include <CpiUtilException.h>
#include "CpiMetadataProperty.h"
#include "ezxml.h"

#define CPI_CONTROL_OPS                                                        \
  CONTROL_OP(initialize,     Initialize,     INITIALIZED, EXISTS,      NONE,        NONE) \
  CONTROL_OP(start,          Start,          OPERATING,   SUSPENDED,   INITIALIZED, NONE) \
  CONTROL_OP(stop,           Stop,           SUSPENDED,   OPERATING,   NONE,        NONE) \
  CONTROL_OP(release,        Release,        EXISTS,      INITIALIZED, OPERATING,   SUSPENDED) \
  CONTROL_OP(beforeQuery,    BeforeQuery,    NONE,        INITIALIZED, OPERATING,   SUSPENDED) \
  CONTROL_OP(afterConfigure, AfterConfigure, NONE,        INITIALIZED, OPERATING,   SUSPENDED) \
  CONTROL_OP(test,           Test,           NONE,        INITIALIZED, NONE,        NONE) \
  /**/

namespace CPI {
  namespace Metadata {

    class Port {
      friend class Worker;
      // Describe a port
      bool decode(ezxml_t x, int pid);
    public:
      const char *   name;
      int            m_pid;
      bool           provider;
      bool           twoway;


      static const unsigned
        DEFAULT_NBUFFERS = 2,
        DEFAULT_BUFFER_SIZE = 2*1024;
      uint32_t minBufferSize;
      uint32_t minBufferCount;
      uint32_t maxBufferSize;

      ezxml_t        myXml;

      Port(bool prov=true): provider(prov),twoway(false), minBufferSize(DEFAULT_BUFFER_SIZE), minBufferCount(1), maxBufferSize(0), myXml(0){}

    };

    class Test {
      friend class Worker;
      unsigned int testId;
      unsigned int numInputs, numResults;
      unsigned int * inputValues;  // reference to property[n]
      unsigned int * resultValues;
    };

    // Dealing with worker meta as a bundle
    class Worker {
      Property *myProps;
      Port *myPorts;
      Test *myTests;
      unsigned nProps, nPorts, nTests, size;
      Test &findTest(unsigned int testId);
      bool decode(const char *props);
      bool decode(ezxml_t xml);
    public:
      inline Property *getProperties(unsigned &np) {
        np = nProps;
        return myProps;
      }
      inline Property &
        property(unsigned long which)
      {
        cpiAssert(which < nProps);
        return myProps[which];
      }
      Port *findPort(const char *id);
      inline Port & 
        port(unsigned long which) 
      {
        cpiAssert(which < nPorts);
        return myPorts[which];
      }
      enum ControlState {
        EXISTS,
        INITIALIZED,
        OPERATING,
        SUSPENDED,
        UNUSABLE,
        NONE
      };
#define CONTROL_OP(x, c, t, s1, s2, s3)  Op##c,
      enum Ops {
      CPI_CONTROL_OPS
      NoOp
      };
#undef CONTROL_OP
#define CONTROL_OP(x, c, t, s1, s2, s3) bool has##c;
CPI_CONTROL_OPS
#undef CONTROL_OP
      Property &findProperty(const char *id);
      unsigned whichProperty(const char *id);
      Worker(const char *workerMetadata);
      Worker(ezxml_t xml);
      ~Worker();
    };
  }
  namespace Container {
    class ApiError : public CPI::Util::EmbeddedException {
    public:
      ApiError(const char *err, ...);
    };
  }
}
#endif

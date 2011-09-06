
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

// The API header for container APIs
// We go to some lengths to make this file NOT reference any other internal non-API files.
#ifndef OCPI_CONTAINER_API_H
#define OCPI_CONTAINER_API_H
#include <stdarg.h>
#include <string>
#include "OcpiPValueApi.h"
#include "OcpiUtilPropertyApi.h"
#include "OcpiUtilExceptionApi.h"
namespace OCPI {
  namespace API {
    // The abstract class for exposed API functionality for containers
    //    class Application;
    class ExternalBuffer {
    protected:
      virtual ~ExternalBuffer();
    public:
      // For consumer buffers
      virtual void release() = 0;
      // For producer buffers
      virtual void put( uint32_t length, uint8_t opCode = 0, bool endOfData = false) = 0;
    };
    class ExternalPort {
    protected:
      virtual ~ExternalPort();
    public:
      // Return zero if there is no buffer ready
      // data pointer may be null if end-of-data is true.
      // This means NO MESSAGE, not a zero length message.
      // I.e. if "data" is null, length is not valid.
      virtual ExternalBuffer *
        getBuffer(uint8_t *&data, uint32_t &length, uint8_t &opCode, bool &endOfData) = 0;
      // Return zero when no buffers are available.
      virtual ExternalBuffer *getBuffer(uint8_t *&data, uint32_t &length) = 0;
      // Use this when end of data indication happens AFTER the last message.
      // Use the endOfData argument to put, when it is known at that time
      virtual void endOfData() = 0;
      // Return whether there are still buffers to send that can't be flushed now.
      virtual bool tryFlush() = 0;
    };
    class Port {
    protected:
      virtual ~Port();
    public:
      virtual void connect(Port &other, const PValue *myProps = NULL,
			   const PValue *otherProps = NULL) = 0;
      virtual void disconnect() = 0;
      // Connect directly to this port, which creates a UserPort object.
      virtual ExternalPort &connectExternal(const char *name = NULL, const PValue *props = NULL,
					    const PValue *uprops = NULL) = 0;
      virtual void loopback(Port &other) = 0;
    };
    class Property;
    class Worker {
      friend class Property;
      virtual void setupProperty(const char *name, Property &prop) = 0;
      //      virtual void setProperty(Property &prop, const PValue &val) = 0;
    protected:
      virtual ~Worker();
    public:
      virtual Port &getPort(const char *name, const PValue *props = NULL) = 0;
      // virtual void initialize() = 0; // NO THIS IS NOT ALLOWED BY THE API
      virtual void start() = 0;
      virtual void stop() = 0;
      virtual void release() = 0;
      virtual void beforeQuery() = 0;
      virtual void afterConfigure() = 0;
      virtual void test() = 0;
      virtual void setProperty(const char *name, const char *value) = 0;
      //      virtual void setProperty(const PValue &value) = 0;
      virtual void setProperties(const PValue *props) =  0;
      virtual void setProperties(const char *props[][2]) =  0;
      virtual bool getProperty(unsigned ordinal, std::string &name, std::string &value) = 0;
    protected:
      // These macros are used by the Property methods below when the
      // fast path using memory-mapped access cannot be used.
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)        \
      virtual void set##pretty##Property(Property &,const run) = 0;   \
      virtual void set##pretty##SequenceProperty(Property &,          \
						 const run *,	      \
						 unsigned length) = 0;

    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
      // generate the simple-type-specific getting methods
      // need a special item for strings
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
      virtual run get##pretty##Property(Property &) = 0;		\
      virtual unsigned get##pretty##SequenceProperty(Property&, run *,	\
						     unsigned length) = 0;

#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)     \
      virtual void get##pretty##Property(Property &, run,            \
					 unsigned length) = 0;	     \
      virtual unsigned get##pretty##SequenceProperty                 \
        (Property &, run *, unsigned length, char *buf,              \
	 unsigned space) = 0;
    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
    };


    // How to express how a worker will be connected in the application
    struct Connection {
      const char *port, *otherWorker, *otherPort;
    };

    // This class is used when the application is being constructed using
    // API calls placing specific workers on specific containers.
    // When the ContainerApplication is deleted, all the workers placed on it
    // are destroyed together.
    class ContainerApplication {
    public:
      virtual ~ContainerApplication();
      // Create an application from an explicit artifact url
      // specifying lots of details:
      // aProps - artifact loading properties
      // name - instance name within the application
      // impl - implementation name within the artifact (e.g. which worker)
      // inst - instance name within artifact that has fixed instances
      // wProps - initial values of worker properties
      // wParams - extensible parameters for worker creation
      virtual Worker &createWorker(const char *file, const PValue *aProps,
				   const char *name, const char *impl,
				   const char *inst = NULL,
				   const PValue *wProps = NULL,
				   const PValue *wParams = NULL,
				   const Connection *connections = NULL) = 0;
      // Simpler method to create a worker by name, with the artifact
      // found from looking at libraries in the library path, finding
      // what implementation will run on the container of this container-app.
      // Since some implementations might have connectivity contraints,
      // we also pass in a simple list of other workers destined for
      // the same container and how they are connected to this one.
      // The list is terminated with the "port" member == NULL
      virtual Worker &createWorker(const char *name, const char *impl,
				   const PValue *wProps = NULL,
				   const PValue *wParams = NULL,
				   const Connection *connections = NULL) = 0;
      virtual void start() = 0;
    };
    class Container {
    public:
      virtual ~Container();
      virtual ContainerApplication *createApplication(const char *name = NULL,
						      const PValue *props = NULL) = 0;
      // Do some work for this container
      // Return true if there is more to do.
      // Argument is yield time for blocking
      virtual bool run(uint32_t usecs = 0, bool verbose = false) = 0;
      // Perform the work of a separate thread, returning only when there is
      // nothing else to do.
      virtual void thread() = 0;
      virtual void stop() = 0;
    };
    class ContainerManager {
    public:
      static Container 
      *find(const char *model, const char *which = NULL, const PValue *props = NULL),
	*find(const PValue *list);
      static void shutdown();
    };
    // User interface for runtime property support for a worker.
    // Optimized for low-latency scalar and/or memory mapped access.
    // Not virtual.
    // Note that the API for this has the user typically constructing this structure
    // on their stack so that access to members (in inline methods) has no indirection.
    class Property {
      Worker &m_worker;               // which worker do I belong to
    public:
      void checkTypeAlways(ScalarType ctype, unsigned n, bool write);
      inline void checkType(ScalarType ctype, unsigned n, bool write) {
#if !defined(NDEBUG) || defined(OCPI_API_CHECK_PROPERTIES)
        checkTypeAlways(ctype, n, write);
#endif
      }
      volatile void 
        *m_writeVaddr, *m_readVaddr;  // pointers non-null if directly usable.
      ValueType m_type;               // cached info when not a struct
      PropertyInfo m_info;            // to info about me FIXME
      Property(Worker &, const char *);
      inline const char *name() { return m_info.m_name; }
      inline bool isWritable() { return m_info.m_isWritable; }
      inline bool isReadable() { return m_info.m_isReadable; }
      inline ScalarType type() { return m_type.scalar; }
      inline bool needsWriteSync() { return m_info.m_writeSync; }
      inline bool needsReadSync() { return m_info.m_readSync; }
      inline unsigned sequenceSize() { return m_type.length; }
      inline unsigned stringSize() { return m_type.stringLength; }
      inline unsigned isSequence() { return m_type.length != 0; }
      // We don't use scalar-type-based templates (sigh) so we can control which
      // types are supported explicitly.
      // The "m_writeVaddr" member is only non-zero if the implementation does
      // not produce errors and it is atomic at this data size
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)              \
      inline void set##pretty##Value(run val) {                             \
        checkType(OCPI_##pretty, 1, true);				    \
        if (m_writeVaddr)                                                   \
          *(store *)m_writeVaddr= *(store*)((void*)&(val));                 \
        else                                                                \
          m_worker.set##pretty##Property(*this, val);			    \
      }                                                                     \
      inline void set##pretty##SequenceValue(const run *vals, unsigned n) { \
        checkType(OCPI_##pretty, n, true);				    \
        m_worker.set##pretty##SequenceProperty(*this, vals, n);             \
      }                                                                     \
      inline run get##pretty##Value() {                                     \
        checkType(OCPI_##pretty, 1, false);				    \
        if (m_readVaddr) {                                                  \
          union { store s; run r; }u;                                       \
          u.s = *(store *)m_readVaddr;                                      \
          return u.r;                                                       \
        } else                                                              \
          return m_worker.get##pretty##Property(*this);                     \
      }                                                                     \
      inline unsigned get##pretty##SequenceValue(run *vals, unsigned n) {   \
        checkType(OCPI_##pretty, n, false);				    \
        return m_worker.get##pretty##SequenceProperty(*this, vals, n);      \
      }
#undef OCPI_DATA_TYPE_S
      // for a string we will take a function call
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)            \
      inline void set##pretty##Value(const run val) {                       \
        checkType(OCPI_##pretty, 1, true);				    \
        m_worker.set##pretty##Property(*this, val);                         \
      }                                                                     \
      inline void set##pretty##SequenceValue(const run *vals, unsigned n) { \
        checkType(OCPI_##pretty, n, true);				    \
        m_worker.set##pretty##SequenceProperty(*this, vals, n);             \
      }                                                                     \
      inline void get##pretty##Value(char *val, unsigned length) {          \
        checkType(OCPI_##pretty, 1, false);				    \
        m_worker.get##pretty##Property(*this, val, length);                 \
      }                                                                     \
      inline unsigned get##pretty##SequenceValue                            \
        (run *vals, unsigned n, char *buf, unsigned space) {                \
        checkType(OCPI_##pretty, n, false);				    \
        return m_worker.get##pretty##SequenceProperty                       \
          (*this, vals, n, buf, space);                                     \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE
    };
  }
}
#endif


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
// Dummy container template
// Each class is defined top down the hierarchy with all methods defined in the class definition
// except those constructing children, which are *declared* in the class definition, but
// *defined* outside, after the child's class definition.

#include "OcpiContainerManager.h"

namespace OCPI {
  namespace OCL {
    namespace OA = OCPI::API;
    namespace OC = OCPI::Container;
    namespace OS = OCPI::OS;
    namespace OM = OCPI::Metadata;
    namespace OU = OCPI::Util;
    namespace OP = OCPI::Util::Prop;

    class ExternalPort;
    class Container;
    const char *ocl = "ocl";
    class Driver : public OC::DriverBase<Driver, Container, ocl> {

    public:
      // This driver method is called when container-discovery happens, to see if there
      // are any container devices supported by this driver
      unsigned search(const OA::PValue*, const char **exclude);
      // This methid specifically creates a container if not already existing
      OC::Container *probeContainer(const char *which, const OA::PValue *props);
    };
    // We register this driver into the system
    OC::RegisterContainerDriver<Driver> driver;

    class Artifact;
    class Port;
    class Application;

    class Container : public OC::ContainerBase<Driver, Container, Application, Artifact> {
      friend class Driver;
      friend class Port;
      unsigned m_example;
    protected:
      Container(const char *name, unsigned example_specific_param)
	: OC::ContainerBase<Driver,Container,Application,Artifact>(name),
	  m_example(example_specific_param)
      {}
    public:
      ~Container() {
      }
      // do some work in a background thread, return whether to keep going
      bool dispatch() { return false; }
      // Create the object that represents a LOADED artifact.
      // (the OCPI::Library::Artifact object represents the artifact by
      // itself, whether loaded or not).
      OC::Artifact &
      createArtifact(OCPI::Library::Artifact &lart, const OA::PValue *artifactParams);
      // Create the container-specific local application class
      OA::ContainerApplication *
      createApplication(const char *name, const OCPI::Util::PValue *props)
	throw ( OCPI::Util::EmbeddedException );
      // Do I need extra thread processing to function?
      bool needThread() { return false; }
    };
    
    // If I depend on other things like hardware, I need to look for them here
    // and discover which ones exist, and create containers for all of them
    // except those in the "exclude" list
    unsigned Driver::search(const OA::PValue*, const char **exclude)
    {
      new Container("name", 456); // registration with driver is automatic
      return 0;
    }
    // Look for and create the specific one - we know there is no "which" container
    // when this is called.
    OC::Container *Driver::probeContainer(const char *which, const OA::PValue *props)
    {
      if (!strcmp(which, "i like this"))
	return new Container(which, 123456); // registration with driver is automatic
      return NULL;
    }

    class Artifact : public OC::ArtifactBase<Container,Artifact> {
      friend class Container;
      Artifact(Container &c, OCPI::Library::Artifact &lart, const OA::PValue *artifactParams) :
        OC::ArtifactBase<Container,Artifact>(c, lart, artifactParams) {
	// make this artifact loaded in this container
      }
    public:
      ~Artifact() {}
    };

    OC::Artifact & Container::
    createArtifact(OCPI::Library::Artifact &lart, const OA::PValue *artifactParams)
    {
      return *new Artifact(*this, lart, artifactParams);
    }
    class Worker;
    class Application : public OC::ApplicationBase<Container, Application, Worker> {
      friend class Container;
      // Not much here unless you want to maintain some resource accounting
      // across all the workers in an application
      Application(Container &con, const char *name, const OA::PValue *props) 
	: OC::ApplicationBase<Container, Application, Worker>(con, name, props)
      {}
      // Note that this cannot be a "default" implementation in the OC::Application
      // base class since we must pass the correct/derived type of application
      // to the artifact (to at least avoid casting...)
      OC::Worker & createWorker(OC::Artifact *art, const char *appInstName,
				ezxml_t impl, ezxml_t inst,
				const OCPI::Util::PValue *wParams);
    };
    OA::ContainerApplication *Container::
    createApplication(const char *name, const OCPI::Util::PValue *props)
      throw ( OCPI::Util::EmbeddedException ) {
      return new Application(*this, name, props);
    };
    class Worker : public OC::WorkerBase<Application, Worker, Port> {
      friend class Application;
      friend class Port;
      Container &m_container; // example convenient keeping track of the container
      volatile uint8_t *myProperties;
      Worker(Application &app, OC::Artifact *art, const char *name,
             ezxml_t implXml, ezxml_t instXml, const OA::PValue* execProps) :
        OC::WorkerBase<Application, Worker, Port>(app, art, name, implXml, instXml, execProps),
        m_container(app.parent())
      {
	(void)execProps;
      }
    public:
      ~Worker()
      {
      }
      // Define the control operations
      void controlOperation(OM::Worker::ControlOperation) {}
      // These are only for legacy testing.
      void read(uint32_t, uint32_t, void*){}
      void write(uint32_t, uint32_t, const void*){}

      OC::Port & createPort(OM::Port &metaport, const OA::PValue *props);

      virtual void prepareProperty(OP::Property &mp, OA::Property &cp) {
	// fill out the API property structure for fastest access
      }

      // For legacy testing
      OC::Port &
      createOutputPort(OM::PortOrdinal portId,
                       OS::uint32_t bufferCount,
                       OS::uint32_t bufferSize,
                       const OA::PValue* props) throw() {
	return *(OC::Port *)0;
      }
      OC::Port &
      createInputPort(OM::PortOrdinal portId,
                      OS::uint32_t bufferCount,
                      OS::uint32_t bufferSize,
                      const OA::PValue* props) throw() {
	return *(OC::Port *)0;
      }

      // These property access methods are called when the fast path
      // is not enabled, either due to no MMIO or that the property can
      // return errors.  OCCP has MMIO, so it must be the latter
#undef OCPI_DATA_TYPE_S
      // Set a scalar property value
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                \
      void set##pretty##Property(OA::Property &p, const run val) {                \
        if (p.m_info.m_writeError)					\
          throw; /*"worker has errors before write */                        \
        volatile store *pp = (volatile store *)(myProperties + p.m_info.m_offset);                        \
        if (bits > 32) {                                                \
          assert(bits == 64);                                                \
          volatile uint32_t *p32 = (volatile uint32_t *)pp;                                \
          p32[1] = ((const uint32_t *)&val)[1];                                \
          p32[0] = ((const uint32_t *)&val)[0];                                \
        } else                                                                \
          *pp = *(const store *)&val;                                                \
        if (p.m_info.m_writeError) \
          throw; /*"worker has errors after write */                        \
      }                                                                        \
      void set##pretty##SequenceProperty(OA::Property &p,const run *vals, unsigned length) { \
        if (p.m_info.m_writeError)					\
          throw; /*"worker has errors before write */                        \
        memcpy((void *)(myProperties + p.m_info.m_offset + p.m_info.m_maxAlign), vals, length * sizeof(run)); \
        *(volatile uint32_t *)(myProperties + p.m_info.m_offset) = length;                \
        if (p.m_info.m_writeError)					\
          throw; /*"worker has errors after write */                        \
      }
      // Set a string property value
      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                \
      virtual void set##pretty##Property(OA::Property &p, const run val) {        \
        unsigned ocpi_length;                                                \
        if (!val || (ocpi_length = strlen(val)) > p.m_type.stringLength)                \
          throw; /*"string property too long"*/;                        \
        if (p.m_info.m_writeError)					\
          throw; /*"worker has errors before write */                        \
        uint32_t *p32 = (uint32_t *)(myProperties + p.m_info.m_offset);                \
        /* if length to be written is more than 32 bits */                \
        if (++ocpi_length > 32/CHAR_BIT)                                        \
          memcpy(p32 + 1, val + 32/CHAR_BIT, ocpi_length - 32/CHAR_BIT); \
        uint32_t i;                                                        \
        memcpy(&i, val, 32/CHAR_BIT);                                        \
        p32[0] = i;                                                        \
        if (p.m_info.m_writeError )					\
          throw; /*"worker has errors after write */                        \
      }                                                                        \
      void set##pretty##SequenceProperty(OA::Property &p,const run *vals, unsigned length) { \
        if (length > p.m_type.length)                                        \
          throw;                                                        \
        if (p.m_info.m_writeError)					\
          throw; /*"worker has errors before write */                        \
        char *cp = (char *)(myProperties + p.m_info.m_offset + 32/CHAR_BIT);        \
        for (unsigned i = 0; i < length; i++) {                                \
          unsigned len = strlen(vals[i]);                                \
          if (len > p.m_type.length)                                        \
            throw; /* "string in sequence too long" */                        \
          memcpy(cp, vals[i], len+1);                                        \
        }                                                                \
        *(uint32_t *)(myProperties + p.m_info.m_offset) = length;                \
        if (p.m_info.m_writeError)					\
          throw; /*"worker has errors after write */                        \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
      // Get Scalar Property
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                \
      virtual run get##pretty##Property(OA::Property &p) {                        \
        if (p.m_info.m_readError)					\
          throw; /*"worker has errors before read "*/                        \
        uint32_t *pp = (uint32_t *)(myProperties + p.m_info.m_offset);                \
        union {                                                                \
                run r;                                                        \
                uint32_t u32[bits/32];                                        \
        } u;                                                                \
        if (bits > 32)                                                        \
          u.u32[1] = pp[1];                                                \
        u.u32[0] = pp[0];                                                \
        if (p.m_info.m_readError) \
          throw; /*"worker has errors after read */                        \
        return u.r;                                                        \
      }                                                                        \
      unsigned get##pretty##SequenceProperty(OA::Property &p, run *vals, unsigned length) { \
        if (p.m_info.m_readError)					\
          throw; /*"worker has errors before read "*/                        \
        uint32_t n = *(uint32_t *)(myProperties + p.m_info.m_offset);                \
        if (n > length)                                                        \
          throw; /* sequence longer than provided buffer */                \
        memcpy(vals, (void*)(myProperties + p.m_info.m_offset + p.m_info.m_maxAlign),        \
               n * sizeof(run));                                        \
        if (p.m_info.m_readError)					\
          throw; /*"worker has errors after read */                        \
        return n;                                                        \
      }

      // ASSUMPTION:  strings always occupy at least 4 bytes, and
      // are aligned on 4 byte boundaries.  The offset calculations
      // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                \
      virtual void get##pretty##Property(OA::Property &p, char *cp, unsigned length) { \
        unsigned stringLength = p.m_type.stringLength;		\
        if (length < stringLength + 1)                                        \
          throw; /*"string buffer smaller than property"*/;                \
        if (p.m_info.m_readError)					\
          throw; /*"worker has errors before write */                        \
        uint32_t i32, *p32 = (uint32_t *)(myProperties + p.m_info.m_offset);        \
        memcpy(cp + 32/CHAR_BIT, p32 + 1, stringLength + 1 - 32/CHAR_BIT); \
        i32 = *p32;                                                        \
        memcpy(cp, &i32, 32/CHAR_BIT);                                        \
        if (p.m_info.m_readError)					\
          throw; /*"worker has errors after write */                        \
      }                                                                        \
      unsigned get##pretty##SequenceProperty                                \
      (OA::Property &p, run *vals, unsigned length, char *buf, unsigned space) { \
        if (p.m_info.m_readError)					\
          throw; /*"worker has errors before read */                        \
        uint32_t                                                        \
          n = *(uint32_t *)(myProperties + p.m_info.m_offset),                        \
          wlen = p.m_type.stringLength + 1;                            \
        if (n > length)                                                        \
          throw; /* sequence longer than provided buffer */                \
        char *cp = (char *)(myProperties + p.m_info.m_offset + 32/CHAR_BIT);        \
        for (unsigned i = 0; i < n; i++) {                                \
          if (space < wlen)                                                \
            throw;                                                        \
          memcpy(buf, cp, wlen);                                        \
          cp += wlen;                                                        \
          vals[i] = buf;                                                \
          unsigned slen = strlen(buf) + 1;                                \
          buf += slen;                                                        \
          space -= slen;                                                \
        }                                                                \
        if (p.m_info.m_readError)					\
          throw; /*"worker has errors after read */                        \
        return n;                                                        \
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
    };
    OC::Worker & Application::createWorker(OC::Artifact *art, const char *appInstName,
					   ezxml_t impl, ezxml_t inst,
					   const OCPI::Util::PValue *wParams) {
      return *new Worker(*this, art, appInstName, impl, inst, wParams);
    }
    class Port : public OC::PortBase<Worker,Port,ExternalPort> {
      friend class Worker;
      friend class ExternalPort;
      ezxml_t m_connection;

      void disconnect()
        throw ( OCPI::Util::EmbeddedException )
      {
        throw OCPI::Util::EmbeddedException("disconnect not yet implemented !!");
      }

      // Called after connection PValues have been set, which is after our constructor
      // userDataBaseAddr, dataBufferBaseAddr are assumed set
      // Also error-check for bad combinations or values of parameters
      // FIXME:  we are relying on dataBufferBaseAddr being set before we know
      // buffer sizes etc.  If we are sharing a memory pool, this will not be the case,
      // and we would probably allocate the whole thing here.
      void checkConnectParams() {
        if (m_canBeExternal) {} // if the port is connectable externally...
      }
      // I am an input port, the other guy is an output port.
      // My job is to emulate a bitstream that consumes from me, and produces at otherPort
      // This is for testing
      void loopback(const OA::PValue *pProps,
		    OA::Port &apiUserPort,
		    const OA::PValue *uProps) {}

      Port(Worker &w,
	   const OA::PValue *props,
           const OM::Port &mPort, // the parsed port metadata
           ezxml_t connXml) // the xml connection for this port if any?
        : OC::PortBase<Worker,Port,ExternalPort>(w, props, mPort),
	  m_connection(connXml)
      {
        m_canBeExternal = true;
	if (isProvider()) {} // is this an input?
      }
      // All the info is in.  Do final work to (locally) establish the connection
      void finishConnection(OCPI::RDT::Descriptors &other) {
      }
      // Connection between two ports inside this container
      // We know they must be in the same artifact, and have a metadata-defined connection
      void connectInside(OC::Port &provider, const OA::PValue *myProps,
			 const OA::PValue *otherProps) {
        // We're both in the same runtime artifact object, so we know the port class
        Port &pport = static_cast<Port&>(provider);
        if (m_connection != pport.m_connection)
          throw "Ports are both local in bitstream/artifact, but are not connected";
        pport.applyConnectParams(otherProps);
        applyConnectParams(myProps);
        // For now we assume there is nothing to actually adjust in the bitstream.
      }
      // Connect to a port in a like container (same driver)
      bool connectLike(const OA::PValue *uProps, OC::Port &provider, const OA::PValue *pProps) {
        // We're both in the same runtime artifact object, so we know the port class
        Port &pport = static_cast<Port&>(provider);
        ocpiAssert(m_canBeExternal && pport.m_canBeExternal);
        pport.applyConnectParams(pProps);
        applyConnectParams(uProps);
        establishRoles(provider.connectionData.data);
        finishConnection(provider.connectionData.data);
        pport.finishConnection(connectionData.data);
        return true;
      }
      // Directly connect to this port
      // which creates a dummy user port
      OC::ExternalPort &connectExternal(const char *name, const OA::PValue *userProps,
					const OA::PValue *props);
    };
    // OCPI API
    OC::Port &Worker::
    createPort(OM::Port &metaPort, const OA::PValue *props) {
      bool isProvider = metaPort.provider;
      const char *name = metaPort.name;
      // Find connections attached to this port
      ezxml_t conn, ic = 0, icw = 0;
      for (conn = ezxml_child(myXml()->parent, "connection"); conn; conn = ezxml_next(conn)) {
        const char
          *from = ezxml_attr(conn,"from"), // instance with user port
          *to = ezxml_attr(conn,"to"),     // instance with provider port
          *out = ezxml_attr(conn, "out"),  // user port name
          *in = ezxml_attr(conn, "in");    // provider port name
        if (from && to && out && in &&
            (isProvider && !strcmp(instTag().c_str(), to) && !strcmp(in, name) ||
             !isProvider && !strcmp(instTag().c_str(), from) && !strcmp(out, name))) {
          // We have a connection.  See if it is to an external interconnect.  FIXME i/o later
          for (ic = ezxml_child(myXml()->parent, "interconnect"); ic; ic = ezxml_next(ic)) {
            const char *icName = ezxml_attr(ic, "name");
            if (icName &&
                (isProvider && !strcmp(icName, from) ||
                 !isProvider && !strcmp(icName, to))) {
              // We have a connection on this port to an interconnect worker!
              // Find its details
              const char *icwName = ezxml_attr(ic, "worker");
              if (icwName)
                for (icw = ezxml_child(myXml()->parent, "worker"); icw; icw = ezxml_next(icw)) {
                  const char *nameAttr = ezxml_attr(icw, "name");
                  if (nameAttr && !strcmp(nameAttr, icwName))
                    break;
                }
              if (!icw)
                throw "interconnect worker missing for connection";
              break; // we found an external connection
            }
          }
          break; // we found a connection
        }
      }
      return *new Port(*this, props, metaPort, conn);
    }

    // Buffers directly used by the "user" (non-container/component) API
    class ExternalBuffer : OC::ExternalBuffer {
      friend class ExternalPort;
      ExternalPort *myExternalPort;
      uint8_t *data;            // where is the data buffer
      uint32_t length;          // length of the buffer (not message)
      volatile uint32_t *readyForLocal;  // where is the flag set by remote on data movement
      volatile uint32_t *readyForRemote; // where is ready flag for remote data movement
      bool busy;                // in use by local processing (for error checking)
      bool last;                // last buffer in the set
      void release();
      void put(uint32_t dataLength, uint8_t opCode, bool endOfData) {
	(void)endOfData;
        ocpiAssert(dataLength <= length);
        release();
      }
    };

    // Producer or consumer
    class ExternalPort : public OC::ExternalPortBase<Port,ExternalPort> {
      friend class ExternalBuffer;
      // What we know about a far buffer
      struct FarBuffer {
        // When we are active, we use these far data pointers
        volatile uint8_t *data;
        // We use this all the time.
        volatile uint32_t *ready;
        bool last;
      };
      //      uint32_t nBuffers, *ready, next;
      uint32_t *flags;
      ExternalBuffer *localBuffers, *nextLocal, *nextRemote;
      FarBuffer *farBuffers, *nextFar;
      uint8_t *localData;
      friend class Port;

      ExternalPort(Port &port, const char *name, const OA::PValue *props) :
        OC::ExternalPortBase<Port,ExternalPort>(port, name, props, port.metaPort())
      {
        applyConnectParams(props);
        unsigned nFar = parent().connectionData.data.desc.nBuffers;
        unsigned nLocal = myDesc.nBuffers;
        myDesc.dataBufferPitch = parent().connectionData.data.desc.dataBufferPitch;
        myDesc.metaDataPitch = parent().connectionData.data.desc.metaDataPitch;
        myDesc.fullFlagPitch = sizeof(uint32_t);
        myDesc.emptyFlagPitch = sizeof(uint32_t);
        myDesc.emptyFlagValue = 1;
        myDesc.fullFlagValue = 1;
        snprintf(myDesc.oob.oep, sizeof(myDesc.oob.oep),
                 "ocpi-pci-pio://%s.%lld:%lld.3.10", "0", (unsigned long long)0,
                 (unsigned long long)0);

#ifdef NEEDED
        memset((void *)allocation, 0, nAlloc);
#endif


        // Initialize our structures that keep track of LOCAL buffer status
        ExternalBuffer *lb = nextLocal = nextRemote = localBuffers = new ExternalBuffer[nLocal];
        for (unsigned i = 0; i < nLocal; i++, lb++) {
          lb->myExternalPort = this;
          lb->data = localData + i * myDesc.dataBufferPitch;
          lb->length = myDesc.dataBufferPitch;
          lb->last = false;
          lb->busy = false;
          //lb->readyForLocal = localFlags + i;
          *lb->readyForLocal = parent().isProvider();
          //lb->readyForRemote = remoteFlags + i; //&parent().myOcdpRegisters->nRemoteDone;
          *lb->readyForRemote = !parent().isProvider();
        }
        (lb-1)->last = true;
      }
    public:
      ~ExternalPort() {
	delete [] localBuffers;
      }
      // The input method = get a buffer that has data in it.
      OA::ExternalBuffer *
      getBuffer(uint8_t *&bdata, uint32_t &length, uint8_t &opCode, bool &end) {
        ocpiAssert(!parent().isProvider());
	return 0;
      }
      OC::ExternalBuffer *getBuffer(uint8_t *&bdata, uint32_t &length) {
        ocpiAssert(parent().isProvider());
        return 0;
      }
      void endOfData() {
        ocpiAssert(parent().isProvider());
      }
      bool tryFlush() {
        ocpiAssert(parent().isProvider());
	return false;
      }
    };

    // FIXME make readyForRemote zero when active flow control
    void ExternalBuffer::release() {
    }
    OC::ExternalPort &Port::connectExternal(const char *name, const OA::PValue *userProps,
					    const OA::PValue *props) {
      if (!m_canBeExternal)
        throw "Port is locally connected in the bitstream.";
      applyConnectParams(props);
      // UserPort constructor must know the roles.
      ExternalPort *myExternalPort = new ExternalPort(*this, name, userProps);
      finishConnection(myExternalPort->connectionData.data);
      return *myExternalPort;
    }
  }
}


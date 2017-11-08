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

#include <unistd.h>
#include "OcpiOsAssert.h"
#include "OcpiUtilMisc.h"
#include "ContainerManager.h"
#include "HdlOCDP.h"
#include "HdlSdp.h"
#include "HdlDriver.h"
#include "HdlContainer.h"
#include "HdlDummyWorker.h"

#define wmb()        asm volatile("sfence" ::: "memory"); usleep(0)
#define clflush(p) asm volatile("clflush %0" : "+m" (*(char *)(p))) //(*(volatile char __force *)p))

namespace OH = OCPI::HDL;
namespace OA = OCPI::API;
namespace OC = OCPI::Container;
namespace OS = OCPI::OS;
namespace OU = OCPI::Util;
namespace OE = OCPI::Util::EzXml;
namespace OD = OCPI::DataTransport;
namespace OT = OCPI::Time;
namespace OL = OCPI::Library;
namespace OR = OCPI::RDT;

namespace OCPI {
  namespace Container {}
  namespace HDL {

    //    static inline unsigned max(unsigned a,unsigned b) { return a > b ? a : b;}
    // This is the alignment constraint of DMA buffers in the processor's memory.
    // It could be a cache line or a malloc granule...
    // It should come from somewhere else.  FIXME
    //    static const unsigned LOCAL_BUFFER_ALIGN = 32;
    

    static OT::Emit::Time getTicksFunc(OT::Emit::TimeSource *ts) {
      return static_cast<Container *>(ts)->getMyTicks();
    }
    Container::
    Container(OCPI::HDL::Device &device, ezxml_t config, const OU::PValue *params) 
      : OC::ContainerBase<Driver,Container,Application,Artifact>
	(*this, device.name().c_str(), config, params),
	Access(device.cAccess()), OT::Emit::TimeSource(getTicksFunc),
	m_device(device), m_hwEvents(this, *this, "FPGA Events")
    {
      // Note that the device has retrieved all the info from the admin space already
      static OT::Emit::RegisterEvent te("testevent");
      m_hwEvents.emit(te);
      m_hwEvents.emitT(te, getMyTicks());
      m_model = "hdl";
      m_os = "";
      m_osVersion = "";
      m_platform = m_device.platform();
      m_arch = m_device.arch();
      addTransport(m_device.protocol(),
		   // FIXME delegate this better to the device
		   strstr(m_device.protocol(), "socket") ? "" : OU::getSystemId().c_str(),
		   OR::ActiveMessage, OR::ActiveMessage,
		   m_device.dmaOptions(NULL, NULL, true),
		   m_device.dmaOptions(NULL, NULL, false));		   
      ocpiDebug("HDL Container for device %s constructed.  ESN: '%s' Platform/part is %s/%s.",
		name().c_str(), m_device.esn().c_str(), m_device.platform().c_str(),
		m_device.part().c_str());
    }
    Container::
    ~Container() {
      ocpiDebug("Entering ~HDL::Container()");
      // Shut this base class down while we're still here
      // Ignore any errors since it is not critical.
      try {
	OT::Emit::shutdown();
      } catch (...) {
	static const char msg[] = "***Exception during container shutdown\n";
	(void)write(2, msg, strlen(msg));
      }
      this->lock();
      shutdown();
      delete &m_device;
      ocpiDebug("Leaving ~HDL::Container()");
    }
#if 0
    void Container::
    start() {
      if (m_device.needThread())
	OC::Container::start();
    }
    void Container::
    stop() {}
#endif
    bool Container::
    needThread() { return m_device.needThread(); }

    Container::DispatchRetCode Container::
    dispatch(DataTransfer::EventManager*) {
      return m_device.run() ? Container::DispatchNoMore : Container::MoreWorkNeeded;
    }

    // This simply insulates the driver code from needing the container class implementation decl.
    OC::Container *Driver::
    createContainer(OCPI::HDL::Device &dev, ezxml_t config, const OU::PValue *params)  {
      return new Container(dev, config, params);
    }
    class Worker;
    class Artifact : public OC::ArtifactBase<Container,Artifact> {
      friend class Container;

      Artifact(Container &c, OCPI::Library::Artifact &lart, const OA::PValue *artifactParams) :
	OC::ArtifactBase<Container,Artifact>(c, *this, lart, artifactParams) {
#if 1	
	ensureLoaded();
#else
	if (!lart.uuid().empty() && c.hdlDevice().isLoadedUUID(lart.uuid())) {
	  ocpiInfo("For HDL container %s, when loading bitstream %s, uuid matches what is "
		   "already loaded\n", c.name().c_str(), name().c_str());
	  c.hdlDevice().connect();
	} else {
	  ocpiInfo("Loading bitstream %s on HDL container %s\n",
		   name().c_str(), c.name().c_str());
	  // If the device needs a container background thread, make sure its started.
	  c.start();
	  std::string error;
	  if (c.hdlDevice().load(name().c_str(), error))
	    throw OU::Error("loading %s on HDL device %s: %s",
			    name().c_str(), c.name().c_str(), error.c_str());
	  if (!c.hdlDevice().isLoadedUUID(lart.uuid()))
	    throw OU::Error("After loading %s on HDL device %s, uuid is wrong.  Wanted: %s",
			    name().c_str(), c.name().c_str(),
			    lart.uuid().c_str());
	  // make sure inserted adapters are started.
	  OL::Implementation *i;
	  for (unsigned n = 0; (i = lart.getImplementation(n)); n++)
	    if (i->m_inserted) {
	      WciControl wci(parent().hdlDevice(), i->m_metadataImpl.m_xml, i->m_staticInstance,
			     NULL);
	      wci.controlOperation(OU::Worker::OpInitialize);
	      wci.controlOperation(OU::Worker::OpStart);
	    }
	}
#endif
      }
      ~Artifact() {}
      void ensureLoaded() {
	Container &c = parent();
	const OL::Artifact &lart = libArtifact();
	if (!lart.uuid().empty() && c.hdlDevice().isLoadedUUID(lart.uuid())) {
	  ocpiInfo("For HDL container %s, when loading bitstream %s, uuid matches what is "
		   "already loaded\n", c.name().c_str(), name().c_str());
	  c.hdlDevice().connect();
	} else {
	  ocpiInfo("Loading bitstream %s on HDL container %s\n",
		   name().c_str(), c.name().c_str());
	  // If the device needs a container background thread, make sure its started.
	  c.start();
	  std::string error;
	  if (c.hdlDevice().load(name().c_str(), error))
	    throw OU::Error("loading %s on HDL device %s: %s",
			    name().c_str(), c.name().c_str(), error.c_str());
	  if (!c.hdlDevice().isLoadedUUID(lart.uuid()))
	    throw OU::Error("After loading %s on HDL device %s, uuid is wrong.  Wanted: %s",
			    name().c_str(), c.name().c_str(),
			    lart.uuid().c_str());
	  // make sure inserted adapters are started.
	  const OL::Implementation *i;
	  for (unsigned n = 0; (i = lart.getImplementation(n)); n++)
	    if (i->m_inserted) {
	      WciControl wci(parent().hdlDevice(), i->m_metadataImpl.m_xml, i->m_staticInstance,
			     NULL);
	      wci.controlOperation(OU::Worker::OpInitialize);
	      wci.controlOperation(OU::Worker::OpStart);
	    }
	}
      }
    };

    static void doWorkers(Device &device, ezxml_t top, char letter, bool hex) {
      for (ezxml_t x = OE::ezxml_firstChild(top); x; x = OE::ezxml_nextChild(x)) {
	if ((!strcasecmp(x->name, "instance") ||
	     !strcasecmp(x->name, "adapter") ||
	     !strcasecmp(x->name, "io") ||
	     !strcasecmp(x->name, "interconnect"))) {
	  size_t iindex;
	  bool ifound = false;
	  const char *err;
	  if ((err = OE::getNumber(x, "occpIndex", &iindex, &ifound)))
	    throw OU::Error("Unexpected occpIndex error: %s", err);
	  const char *iname = ezxml_cattr(x, "name");
	  if (letter && iname && iname[0] == letter) {
	    const char
	      *name = ezxml_attr(x, "name"),
	      *idx = ezxml_attr(x, "occpIndex"),
	      *wkr = ezxml_attr(x, "worker");
	    ezxml_t impl = OE::findChildWithAttr(top, "worker", "name", wkr);
	    if (!impl)
	      throw OU::Error("Can't find worker '%s' for instance '%s'", name, wkr);
	    fprintf(stderr, "  Instance %s of %s worker %s (spec %s)",
		    name, strcasecmp(x->name, "instance") ? x->name :
		    idx && !strcmp(idx, "0") ? "platform" : "normal",
		    wkr, wkr ? ezxml_cattr(impl, "specname") : "<none>");
	    if (idx) {
	      fprintf(stderr, " with index %s", idx);
	      DummyWorker w(device, impl, x, idx);
	      fprintf(stderr, " status %s\n", w.status());
	      std::string pname, value;
	      bool unreadable;
	      for (unsigned i = 0; w.getProperty(i, pname, value, &unreadable, hex); i++)
		fprintf(stderr, "%3u %20s: %s\n", i, pname.c_str(),
			unreadable ? "<unreadable>" : value.c_str());
	    } else
	      fprintf(stderr, " with no control interface\n");
	  }
	}
      }
    }
    void Container::
    dump(bool before, bool hex) {
      if (before)
	fprintf(stderr,
		"HDL Container \"%s\" on Device: \"%s\" is platform '%s' part '%s'.\n",
		name().c_str(), m_device.name().c_str(), m_device.platform().c_str(),
		m_device.part().c_str());
      Artifact *a = OU::Parent<Artifact>::firstChild();
      if (a) {
	const OL::Artifact &lart = a->libArtifact();
	fprintf(stderr, "Loaded artifact is: %s, with UUID %s\n", lart.name().c_str(), lart.uuid().c_str());
	fprintf(stderr, "Platform configuration workers are:\n");
	doWorkers(m_device, lart.xml(), 'p', hex);
	fprintf(stderr, "Container/device workers are:\n");
	doWorkers(m_device, lart.xml(), 'c', hex);
	fprintf(stderr, "Application workers are:\n");
	doWorkers(m_device, lart.xml(), 'a', hex);
      } else
	fprintf(stderr, "  No artifact loaded.\n");
    }

    // We know we have not already loaded it, but it still might be loaded on the device.
    OC::Artifact & Container::
    createArtifact(OCPI::Library::Artifact &lart, const OA::PValue *artifactParams)
    {
      return *new Artifact(*this, lart, artifactParams);
    }

    class Application : public OC::ApplicationBase<Container, Application, Worker> {
      friend class Container;
      Application(Container &con, const char *a_name, const OA::PValue *props) 
	: OC::ApplicationBase<Container, Application, Worker>(con, *this, a_name, props)
      {}
      OC::Worker & createWorker(OC::Artifact *art, const char *appInstName, ezxml_t impl,
				ezxml_t inst, OC::Worker *slave, bool hasMaster,
			        size_t member, size_t crewSize, const OU::PValue *wParams);
    };

    OA::ContainerApplication *Container::
    createApplication(const char *a_name, const OCPI::Util::PValue *props)
      throw ( OCPI::Util::EmbeddedException ) {
      return new Application(*this, a_name, props);
    };
    class Port;
    class Worker : public OC::WorkerBase<Application, Worker, Port>, public WciControl {
      friend class Application;
      friend class Port;
      friend class ExternalPort;
      Container &m_container;
      Worker(Application &app, OC::Artifact *art, const char *a_name, ezxml_t implXml,
	     ezxml_t instXml, OC::Worker *a_slave, bool a_hasMaster, size_t a_member,
	     size_t a_crewSize, const OA::PValue* execParams) :
        OC::WorkerBase<Application, Worker, Port>(app, *this, art, a_name, implXml, instXml,
						  a_slave, a_hasMaster, a_member, a_crewSize,
						  execParams),
        WciControl(app.parent().hdlDevice(), implXml, instXml, properties()),
        m_container(app.parent())
      {
      }
    public:
      ~Worker()
      {
	deleteChildren(); // delete before out base class is gone.
      }
      inline void controlOperation(OU::Worker::ControlOperation op) {
	WciControl::controlOperation(op);
      }

      OC::Port & createPort(const OU::Port &metaport, const OA::PValue *props);

      virtual void prepareProperty(OU::Property &mp,
				   volatile uint8_t *&writeVaddr,
				   const volatile uint8_t *&readVaddr) {
        return WciControl::prepareProperty(mp, writeVaddr, readVaddr);
      }
#undef OCPI_DATA_TYPE_S
      // Set a scalar property value

#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)     	    \
      void								    \
      set##pretty##Property(const OCPI::API::PropertyInfo &info, const Util::Member *m, \
			    size_t off, const run val, unsigned idx) const { \
        WciControl::set##pretty##Property(info, m, off, val, idx);	\
      }									    \
      void								    \
      set##pretty##SequenceProperty(const OA::Property &p, const run *vals, \
				    size_t length) const {		    \
	WciControl::set##pretty##SequenceProperty(p, vals, length);	    \
      }									    \
      run								    \
      get##pretty##Property(const OCPI::API::PropertyInfo &info, const Util::Member *m, \
			    size_t off, unsigned idx) const {		\
	return WciControl::get##pretty##Property(info, m, off, idx); \
      }									    \
      unsigned								    \
      get##pretty##SequenceProperty(const OA::Property &p, run *vals,	    \
				    size_t length) const {		    \
	return WciControl::get##pretty##SequenceProperty(p, vals, length);  \
      }
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
OCPI_DATA_TYPES
      void
      setStringProperty(const OCPI::API::PropertyInfo &info, const Util::Member *m, size_t off,
			const char* val, unsigned idx) const {
        WciControl::setStringProperty(info, m, off, val, idx);
      }
      void
      setStringSequenceProperty(const OA::Property &p, const char * const *val,
				size_t n) const {
	WciControl::setStringSequenceProperty(p, val, n);
      }
      void
      getStringProperty(const OCPI::API::PropertyInfo &info, const Util::Member *m, size_t off,
			char *val, size_t length, unsigned idx) const {
	WciControl::getStringProperty(info, m, off, val, length, idx);
      }
      unsigned
      getStringSequenceProperty(const OA::Property &p, char * *cp,
				size_t n ,char*pp, size_t nn) const {
	return WciControl::getStringSequenceProperty(p, cp, n, pp, nn);
      }
#define PUT_GET_PROPERTY(n)						         \
      void setProperty##n(const OA::PropertyInfo &info, size_t off, uint##n##_t val, unsigned idx) const { \
        WciControl::setProperty##n(info, off, val, idx);			\
      }									         \
      inline uint##n##_t getProperty##n(const OA::PropertyInfo &info, size_t off, unsigned idx) const { \
	return WciControl::getProperty##n(info, off, idx);		\
      }									         
      PUT_GET_PROPERTY(8)
      PUT_GET_PROPERTY(16)
      PUT_GET_PROPERTY(32)
      PUT_GET_PROPERTY(64)
      void setPropertyBytes(const OA::PropertyInfo &info, size_t offset,
			    const uint8_t *data, size_t nBytes, unsigned idx) const {
	WciControl::setPropertyBytes(info, offset, data, nBytes, idx);
      }
      inline void
      getPropertyBytes(const OA::PropertyInfo &info, size_t offset, uint8_t *buf,
	  size_t nBytes, unsigned idx, bool string) const {
	WciControl::getPropertyBytes(info, offset, buf, nBytes, idx, string);
      }
    };
    OC::Worker & Application::
    createWorker(OC::Artifact *art, const char *appInstName, ezxml_t impl, ezxml_t inst,
		 OC::Worker *slave, bool hasMaster, size_t member, size_t crewSize,
		 const OCPI::Util::PValue *wParams) {
      assert(!slave);
      return *new Worker(*this, art, appInstName, impl, inst, slave, hasMaster, member, crewSize,
			 wParams);
    }
    // This port class really has two cases: externally connected ports and
    // internally connected ports.
    // Also ports are either user or provider.
    // So this class takes care of all 4 cases, since the differences are so
    // minor as to not be worth (re)factoring (currently).
    // The inheritance of WciControl is for the external case
    class ExternalPort;
    class Port : public OC::PortBase<OH::Worker,OH::Port,OH::ExternalPort>, WciControl {
      friend class Container;
      friend class Worker;
      friend class ExternalPort;
      ezxml_t m_connection;
      // These are for external-to-FPGA ports
      // Which would be in a different class if we separate them
      bool m_sdp;
      uint32_t m_memorySize;
      //      bool m_userConnected;
      WciControl *m_adapter; // if there is an adapter
      bool m_hasAdapterConfig;
      size_t m_adapterConfig;
      static int dumpFd;
      DataTransfer::EndPoint *m_endPoint; // the data plane endpoint if externally connected
      Port(OCPI::HDL::Worker &w,
	   const OA::PValue *params,
           const OU::Port &mPort, // the parsed port metadata
           ezxml_t connXml, // the xml connection for this port
           ezxml_t icwXml,  // the xml interconnect/infrastructure worker attached to this port if any
           ezxml_t icXml,   // the xml interconnect instance attached to this port if any
           ezxml_t adwXml,  // the xml adapter/infrastructure worker attached to this port if any
           ezxml_t adXml) :   // the xml adapter instance attached to this port if any
        OC::PortBase<OH::Worker,OH::Port,OH::ExternalPort>
	(w, *this, mPort, params),
	// The WCI will control the interconnect worker.
	// If there is no such worker, usable will fail.
        WciControl(w.m_container.hdlDevice(), icwXml, icXml, NULL),
        m_connection(connXml), m_sdp(false), m_memorySize(0),
	m_adapter(adwXml ? new WciControl(w.m_container.hdlDevice(), adwXml, adXml, NULL) : 0),
	m_hasAdapterConfig(false),
	m_adapterConfig(0),
	m_endPoint(NULL)
      {
        if (!icwXml || !usable()) {
          m_canBeExternal = false;
          return;
        }
	const char *err;
	if (m_adapter && adXml &&
	    (err = OE::getNumber(adXml, "configure", &m_adapterConfig, &m_hasAdapterConfig, 0)))
	  throw OU::Error("Invalid configuration value for adapter: %s", err);
        m_canBeExternal = true;
	Device &device = w.parent().parent().hdlDevice();
	// Create an endpoint for the externalized worker port.
	// We have an endpoint-per-externalized-worker port to allow the FPGA implementation of
	// each port to share no state with other ports.  Someday this might not be the right
	// choice, especially when different DPs share an external memory port.
	// But it is possible to have both PCI and Ether ports and this will require it.
	// If the endpoint into is just a protocol, we just create one locally
	m_endPoint = &device.getEndPoint();
	m_endPoint->addRef();
	OD::Transport::fillDescriptorFromEndPoint(*m_endPoint, getData().data);
        // These will be determined at connection time
        myDesc.dataBufferPitch   = 0;
        myDesc.metaDataBaseAddr  = 0;
        // Fixed values not set later in startConnect
        myDesc.fullFlagSize      = 4;
        myDesc.fullFlagPitch     = 0;
        myDesc.fullFlagValue     = 1; // will be a count of buffers made full someday?
        myDesc.emptyFlagSize     = 4;
        myDesc.emptyFlagPitch    = 0;
        myDesc.emptyFlagValue    = 1; // Will be a count of buffers made empty

	const char *wname = ezxml_cattr(icwXml, "name");
	size_t myDataOffset;
	if (wname && !strncasecmp(wname, "sdp", 3)) {
	  m_sdp = true;
	  // almost none of these apply when the SDP is active.
	  m_memorySize =
	    m_properties.get32Register(memory_bytes, SDP::Properties);
	  myDataOffset =
	    (m_properties.get8Register(sdp_id, SDP::Properties) - 1) *
	    (1 << m_properties.get8Register(window_log2, SDP::Properties));
	  myDesc.metaDataPitch      = 0;
	  myDesc.metaDataBaseAddr   = 0; //m_properties.physOffset(offsetof(SDP::Properties,
	                                 //       metadata));
	  if (isProvider()) {
	    myDesc.fullFlagBaseAddr =
	      m_properties.physOffset(offsetof(SDP::Properties, remote_doorbell));
	    ocpiInfo("Input remote doorbell is %zu", offsetof(SDP::Properties, remote_doorbell));
	    //	      device.dAccess().physOffset(myDataOffset) + SDP::Propertiesc_flag_offset;
	    myDesc.metaDataBaseAddr =
	      m_properties.physOffset(offsetof(SDP::Properties, remote_doorbell));
	    //	      device.dAccess().physOffset(myDataOffset) + SDP::c_metadata_offset;
	    myDesc.emptyFlagBaseAddr = 0;
	    //	      m_properties.physOffset(offsetof(SDP::Properties, available_count));
	  } else {
	    // sdp sender.
	    // ActiveMessage:     empty flag from other side says other side's buffer is empty
	    // ActiveFlowControl: empty flag from other side says local buffer has been read
	    // ActiveOnly:        not implemented
	    // Passive:           empty flag from other side says local buffer has been read
	    //                    full flag (READ) says there is a buffer to read
	    myDesc.emptyFlagBaseAddr =
	      m_properties.physOffset(offsetof(SDP::Properties, remote_doorbell));
	    // full indications
	    myDesc.fullFlagBaseAddr = 0;
	    //	      m_properties.physOffset(offsetof(SDP::Properties, available_count));
	  }
	} else {
	  if (m_properties.usable())
	    m_memorySize = m_properties.get32Register(memoryBytes, OcdpProperties);
	  // This should be the region address from admin, using the 0x1c region register
	  // The region addresses are offsets in BAR1 at this point
	  if ((err = OE::getNumber(icXml, "ocdpOffset", &myDataOffset, NULL, 0, false, true)))
	    throw OU::Error("Invalid or missing ocdpOffset attribute: %s", err);
	  myDesc.metaDataPitch = sizeof(OcdpMetadata);
	  if (isProvider()) {
	    // CONSUMER
	    // BasicPort does this: getData().data.type = OCPI::RDT::ConsumerDescT;
	    // The flag is in the OCDP's register space.
	    // "full" is the flag telling me (the consumer) a buffer has become full
	    // Mode dependent usage:
	    // *Passive/ActiveFlowCtl: producer hits this after writing/filling local buffer
	    //  (thus it is a "local buffer is full")
	    // *Active Message: producer hits this when remote buffer is ready to pull
	    //  (thus it is a "remote buffer is full")
	    // This register is for WRITING.
	    myDesc.fullFlagBaseAddr =
	      m_properties.physOffset(offsetof(OcdpProperties, nRemoteDone));
	    // The nReady register is the empty flag, which tells the producer how many
	    // empty buffers there are to fill when consumer is in PASSIVE MODE
	    // Other modes it is not used.
	    // This register is for READING (in passive mode)
	    myDesc.emptyFlagBaseAddr =
	      m_properties.physOffset(offsetof(OcdpProperties, nReady));
	  } else {
	    // BasicPort does this: getData().data.type = OCPI::RDT::ProducerDescT;
	    // The flag is in the OCDP's register space.
	    // "empty" is the flag telling me (the producer) a buffer has become empty
	    // Mode dependent usage:
	    // *Passive/ActiveFlowCtl: consumer hits this after making a local buffer empty
	    //  (thus it is a "local buffer is empty")
	    // *ActiveMessage: consumer hits this after making a remote buffer empty
	    // (thus it is a "remote buffer is empty")
	    // This register is for writing.
	    myDesc.emptyFlagBaseAddr = 
	      m_properties.physOffset(offsetof(OcdpProperties, nRemoteDone));
	    // The nReady register is the full flag, which tells the consumer how many
	    // full buffers there are to read/take when producer is PASSIVE
	    // This register is for READING (in passive mode)
	    myDesc.fullFlagBaseAddr = 
	      m_properties.physOffset(offsetof(OcdpProperties, nReady));
	  }
	}
	// This is the offset within the overall endpoint of our data buffers
	myDesc.dataBufferBaseAddr = device.dAccess().physOffset(myDataOffset);
        const char *df = getenv("OCPI_DUMP_PORTS");
        if (df) {
          if (dumpFd < 0)
            ocpiCheck((dumpFd = creat(df, 0666)) >= 0);
          OC::PortData *pd = this;
          ocpiCheck(::write(dumpFd, (void *)pd, sizeof(*pd)) == sizeof(*pd));
        }
#if 0
        if (getenv("OCPI_OCFRP_DUMMY"))
          *(uint32_t*)&m_ocdpRegisters->foodFace = 0xf00dface;
#endif
	// Allow default connect params on port construction prior to connect
	// FIXME: isnt this redundant with the generic code?
	//applyConnectParams(NULL, params);
      }
    public: // for parent/child...
      ~Port() {
	if (m_endPoint)
	  m_endPoint->release();
      }
    private:
      const char *getPreferredProtocol() {
	return parent().m_container.hdlDevice().protocol();
      }
      void disconnect()
        throw ( OCPI::Util::EmbeddedException )
      {
        throw OCPI::Util::EmbeddedException("disconnect not yet implemented !!");
      }

      bool isLocal() const { return false; }
      bool isInProcess(OC::LocalPort */*other*/) const { return false; }

      // Called after connection PValues have been set, which is after our constructor
      // Also error-check for bad combinations or values of parameters
      // FIXME:  we are relying on dataBufferBaseAddr being set before we know
      // buffer sizes etc.  If we are sharing a memory pool, this will not be the case,
      // and we would probably allocate the whole thing here.
      const OCPI::RDT::Descriptors *
      startConnect(const OR::Descriptors *other, OR::Descriptors &feedback, bool &done) {
	assert(m_canBeExternal);
	uint32_t required;
	if (m_sdp) {
	  myDesc.dataBufferPitch =
	    OCPI_UTRUNCATE(uint32_t,
			   OU::roundUp(myDesc.dataBufferSize,
				       m_properties.get8Register(sdp_width, SDP::Properties) *
				       SDP::BYTES_PER_DWORD));
	  required = myDesc.nBuffers * myDesc.dataBufferPitch;
	} else {
	  myDesc.dataBufferPitch =
	    OCPI_UTRUNCATE(uint32_t,
			   OU::roundUp(myDesc.dataBufferSize, OCDP_LOCAL_BUFFER_ALIGN));
	  myDesc.metaDataBaseAddr =
	    myDesc.dataBufferBaseAddr + m_memorySize - myDesc.nBuffers * OCDP_METADATA_SIZE;
	  required = myDesc.nBuffers * (myDesc.dataBufferPitch + OCDP_METADATA_SIZE);
	}
	if (required > m_memorySize)
	  throw OU::Error("Requested buffer count/size (%u/%u) on port '%s' of worker '%s' "
			  "won't fit in the %s's memory (%u)",
			  myDesc.nBuffers, myDesc.dataBufferSize, name().c_str(),
			  parent().name().c_str(), m_sdp ? "SDP-DMA" : "OCDP-DMA",
			  m_memorySize);
	if (other)
	  return finishConnect(other, feedback, done);
	done = false;
	return &getData().data;
      }
    private:
      // All the info is in.  Do final work to (locally) establish the connection
      // If we're output, we must return the flow control descriptor
      const OCPI::RDT::Descriptors *
      finishConnect(const OR::Descriptors *argOther,
		    OR::Descriptors &/*feedback*/, bool &done) {
	assert(argOther);
	const OCPI::RDT::Descriptors &other = *argOther;
	if (m_sdp) {
	  m_properties.set8Register(buffer_count, SDP::Properties,
				    OCPI_UTRUNCATE(uint8_t, myDesc.nBuffers));
	  m_properties.set32Register(buffer_size, SDP::Properties, myDesc.dataBufferPitch);
	  m_properties.set8Register(readsAllowed, SDP::Properties,
				    getData().data.options & (1<<OCPI::RDT::FlagIsMeta) ? 1 : 0);
	} else {
	  // Here is where we can setup the OCDP producer/user
	  ocpiAssert(m_properties.get32Register(foodFace, OcdpProperties) == 0xf00dface);
	  m_properties.set32Register(nLocalBuffers, OcdpProperties, myDesc.nBuffers);
	  m_properties.set32Register(localBufferSize, OcdpProperties, myDesc.dataBufferPitch);
	  m_properties.set32Register(localBufferBase, OcdpProperties, 0);
	  m_properties.set32Register(localMetadataBase, OcdpProperties,
				     m_memorySize - myDesc.nBuffers * OCDP_METADATA_SIZE);
	}
        OcdpRole myOcdpRole;
        OCPI::RDT::PortRole myRole = (OCPI::RDT::PortRole)getData().data.role;
	
        ocpiDebug("finishConnection: other = %" PRIx64 ", offset = %" DTOSDATATYPES_OFFSET_PRIx
		  ", RFB = %" PRIx64 "",
		  other.desc.oob.address,
		  isProvider() ? other.desc.emptyFlagBaseAddr : other.desc.fullFlagBaseAddr,
		  other.desc.oob.address +
		  (isProvider() ? other.desc.emptyFlagBaseAddr : other.desc.fullFlagBaseAddr));
	ocpiDebug("Other ep = %s, myrole is %u\n", other.desc.oob.oep, myRole);
        switch (myRole) {
	  uint64_t addr;
	  uint32_t pitch;
        case OCPI::RDT::ActiveFlowControl:
          myOcdpRole = OCDP_ACTIVE_FLOWCONTROL;
	  addr = other.desc.oob.address +
	    (isProvider() ? other.desc.emptyFlagBaseAddr : other.desc.fullFlagBaseAddr);
	  pitch = isProvider() ? other.desc.emptyFlagPitch : other.desc.fullFlagPitch;
	  if (m_sdp) {
	    m_properties.set32Register(role, SDP::Properties, OCDP_ACTIVE_FLOWCONTROL);
	    m_properties.set64Register(remote_flag_addr, SDP::Properties, addr);
	    m_properties.set32Register(remote_flag_pitch, SDP::Properties, pitch);
	  } else {
	    m_properties.set32Register(remoteFlagBase, OcdpProperties, (uint32_t)addr);
	    m_properties.set32Register(remoteFlagHi, OcdpProperties, (uint32_t)(addr >> 32));
	    m_properties.set32Register(remoteFlagPitch, OcdpProperties, pitch);
	  }
	  ocpiDebug("HDL Port is %s, AFC, flag is 0x%" PRIx64 "pitch %u", 
		    isProvider() ? "consumer" : "producer", addr, pitch);
          break;
        case OCPI::RDT::ActiveMessage:
          myOcdpRole = OCDP_ACTIVE_MESSAGE;
          if (isProvider()) {
            if (other.desc.dataBufferSize > myDesc.dataBufferSize)
              throw OU::Error("At consumer, remote buffer size is larger than mine");
          } else if (other.desc.dataBufferSize < myDesc.dataBufferSize)
            throw OU::Error("At producer, remote buffer size smaller than mine");
	  if (m_sdp) {
	    m_properties.set32Register(role, SDP::Properties, OCDP_ACTIVE_MESSAGE);
	    m_properties.set64Register(remote_data_addr, SDP::Properties,
				       other.desc.oob.address + other.desc.dataBufferBaseAddr);
	    m_properties.set64Register(remote_meta_addr, SDP::Properties,
				       other.desc.oob.address + other.desc.metaDataBaseAddr);
	    m_properties.set64Register(remote_flag_addr, SDP::Properties,
				       other.desc.oob.address +
 				       (isProvider() ?
					other.desc.emptyFlagBaseAddr :
					other.desc.fullFlagBaseAddr));
	    m_properties.set32Register(remote_data_pitch, SDP::Properties,
				       other.desc.dataBufferPitch);
	    m_properties.set32Register(remote_meta_pitch, SDP::Properties,
				       other.desc.metaDataPitch);
	    m_properties.set32Register(remote_flag_pitch, SDP::Properties, 
				       isProvider() ?
				       other.desc.emptyFlagPitch : other.desc.fullFlagPitch);
	    m_properties.set32Register(remote_flag_value, SDP::Properties, 
				       isProvider() ?
				       other.desc.emptyFlagValue : other.desc.fullFlagValue);
	    m_properties.set8Register(remote_buffer_count, SDP::Properties,
				      OCPI_UTRUNCATE(uint8_t, other.desc.nBuffers));

	  } else {
	    addr = other.desc.oob.address + other.desc.dataBufferBaseAddr;
	    m_properties.set32Register(remoteBufferBase, OcdpProperties, (uint32_t)addr);
	    m_properties.set32Register(remoteBufferHi, OcdpProperties, (uint32_t)(addr >> 32));
	    addr = other.desc.oob.address + other.desc.metaDataBaseAddr;
	    m_properties.set32Register(remoteMetadataBase, OcdpProperties, (uint32_t)addr);
	    m_properties.set32Register(remoteMetadataHi, OcdpProperties, (uint32_t)(addr >> 32));
	    m_properties.set32Register(nRemoteBuffers, OcdpProperties, other.desc.nBuffers);
	    m_properties.set32Register(remoteBufferSize, OcdpProperties,
				       other.desc.dataBufferPitch);
	    m_properties.set32Register(remoteMetadataSize, OcdpProperties,
				       other.desc.metaDataPitch);
	    addr = other.desc.oob.address + (isProvider() ? other.desc.emptyFlagBaseAddr :
					     other.desc.fullFlagBaseAddr);
	    m_properties.set32Register(remoteFlagBase, OcdpProperties, (uint32_t)addr);
	    m_properties.set32Register(remoteFlagHi, OcdpProperties, (uint32_t)(addr >> 32));
	    m_properties.set32Register(remoteFlagPitch, OcdpProperties, 
				       isProvider() ?
				       other.desc.emptyFlagPitch : other.desc.fullFlagPitch);
	  }
          break;
        case OCPI::RDT::Passive:
          myOcdpRole = OCDP_PASSIVE;
	  // We don't need to know anything about the other side
          break;
        default:
          myOcdpRole = OCDP_PASSIVE; // quiet compiler warning
          ocpiAssert(0);
        }
        ocpiDebug("finishConnection: me = %" PRIx64 ", offset = %" DTOSDATATYPES_OFFSET_PRIx ", RFB = %" PRIx64 "",
		  myDesc.oob.address,
		  isProvider() ? myDesc.fullFlagBaseAddr : myDesc.emptyFlagBaseAddr,
		  myDesc.oob.address +
		  (isProvider() ? myDesc.fullFlagBaseAddr : myDesc.emptyFlagBaseAddr));
	ocpiDebug("My ep = %s\n", myDesc.oob.oep );
	if (m_sdp) {
#define DUMP(x,n,nn)		\
  ocpiDebug("SDP %s %" PRIx##n, \
	    #x, m_properties.get##n##RegisterOffset(offsetof(SDP::Properties,x)+nn*(n/8)))
	  DUMP(memory_bytes,32,0);
	  DUMP(sdp_width,8,0);
	  DUMP(sdp_id,8,0);
	  DUMP(buffer_size,32,0);
	  DUMP(buffer_count,8,0);
	  DUMP(segment_size,16,0);
	  DUMP(overflow,8,0);
	  for (unsigned n = 0; n < SDP::NREMOTES; n++)
	    DUMP(remote_data_addr,64,n);
	} else
	  m_properties.set32Register(control, OcdpProperties,
				     OCDP_CONTROL(isProvider() ? OCDP_CONTROL_CONSUMER :
						  OCDP_CONTROL_PRODUCER, myOcdpRole));
	// We aren't a worker so someone needs to start us.
	controlOperation(OU::Worker::OpInitialize);
	if (m_adapter) {
	  m_adapter->controlOperation(OU::Worker::OpInitialize);
	  if (m_hasAdapterConfig)
	    m_adapter->m_properties.set32RegisterOffset(0, (uint32_t)m_adapterConfig);
	  m_adapter->controlOperation(OU::Worker::OpStart);
	}
	// We need to tell the device object about this remote endpoint connection,
	// at least for the case of simulators
	m_device.connect(*m_endPoint, getData().data, other);
	controlOperation(OU::Worker::OpStart);
	done = true;
	portIsConnected();
	return isProvider() ? NULL : &getData().data;
      }
      OC::ExternalPort &createExternal(const char *extName, bool isProvider,
				       const OU::PValue *extParams, const OU::PValue *connParams);
    };
    // Connection between two ports inside this container
    // We know they must be in the same artifact, and have a metadata-defined connection
    bool Container::
    connectInside(OC::BasicPort &in, OC::BasicPort &out) {
      

      Port 
	&pport = static_cast<Port&>(in.isProvider() ? in : out),
	&uport = static_cast<Port&>(out.isProvider() ? in : out);
      assert(!pport.m_canBeExternal && !uport.m_canBeExternal);
      return true;
#if 0
      bool done;
      OCPI::RDT::Descriptors dummy;
      pport.startConnect(NULL, dummy, done);
      // We're both in the same runtime artifact object, so we know the port class
      if (uport.m_connection != pport.m_connection)
	throw OU::Error("Ports %s (instance %s) and %s (instance %s) are both local in "
			"bitstream/artifact %s, but are not connected (%p %p)",
			uport.name().c_str(), uport.parent().name().c_str(),
			pport.name().c_str(), pport.parent().name().c_str(),
			pport.parent().artifact() ?
			pport.parent().artifact()->name().c_str() : "<none>",
			uport.m_connection, pport.m_connection);
      return true;
#endif
    }
    int Port::dumpFd = -1;

    // The port may be bidirectional.  If so we need to defer its direction.
    // FIXME: share all this parsing with the OU::Implementation code etc.
    OC::Port &Worker::
    createPort(const OU::Port &mPort, const OA::PValue *props) {
      const char *myName = mPort.m_name.c_str();
      // Find connections attached to this port
      ezxml_t conn, ic = 0, icw = 0, ad = 0, adw = 0;
      for (conn = ezxml_cchild(myXml()->parent, "connection"); conn; conn = ezxml_next(conn)) {
        const char
          *from = ezxml_cattr(conn, "from"), // instance with user port
          *to = ezxml_cattr(conn, "to"),     // instance with provider port
          *out = ezxml_cattr(conn, "out"),   // user port name
          *in = ezxml_cattr(conn, "in");     // provider port name
        if (from && to && out && in) {
	  bool iAmTo;
	  if (!strcasecmp(instTag().c_str(), to) && !strcasecmp(in, myName))
	    iAmTo = true;
	  else if (!strcasecmp(instTag().c_str(), from) && !strcasecmp(out, myName))
	    iAmTo = false;
	  else
	    continue;
	  // We have a connection.  Check to see if it is an inserted adapter which we skip over.
          for (ezxml_t ix = ezxml_cchild(myXml()->parent, "instance"); ix; ix = ezxml_next(ix)) {
            const char *ixName = ezxml_cattr(ix, "name");
            if (ixName &&
                ((iAmTo && !strcasecmp(ixName, from)) ||
                 (!iAmTo && !strcasecmp(ixName, to))) ) {
	      bool inserted = false;
	      OE::getBoolean(ix, "inserted", &inserted);
	      if (!inserted)
		continue;
	      // find the connection on the other side of the inserted adapter
	      for (ezxml_t c = ezxml_cchild(myXml()->parent, "connection"); 
		   ixName && c; c = ezxml_next(c)) {
		const char
		  *iFrom = ezxml_cattr(c, "from"), // instance with user port
		  *iTo = ezxml_cattr(c, "to");     // instance with provider port
		if (iAmTo && !strcasecmp(iTo, ixName)) {
		  from = iFrom;
		  to = ixName;
		  conn = c; // alias our connection to the real to-adapter connection
		  ixName = NULL;
		} else if (!iAmTo && !strcasecmp(iFrom, ixName)) {
		  from = ixName;
		  to = iTo;
		  ixName = NULL;
		}
	      }
	      if (ixName)
		throw OU::Error("For port \"%s\": inserted adapter has no other connection",
				myName);
	      break;
	    }
	  }
          // We have a connection.  See if it is to a container adapter, which in turn would be
	  // connected to an interconnect.  No other adapters are expected yet.
          for (ad = ezxml_cchild(myXml()->parent, "adapter"); ad; ad = ezxml_next(ad)) {
            const char *adName = ezxml_cattr(ad, "name");
            if (adName &&
                ((iAmTo && !strcasecmp(adName, from)) ||
                 (!iAmTo && !strcasecmp(adName, to))) ) {
              // We have a connection on this port to an adapter instance.  Find the worker
	      const char *adwName = ezxml_cattr(ad, "worker");
	      if (adwName)
		for (adw = ezxml_cchild(myXml()->parent, "worker"); adw; adw = ezxml_next(adw)) {
		  const char *nameAttr = ezxml_cattr(adw, "name");
		  if (nameAttr && !strcasecmp(nameAttr, adwName))
		    break;
		}
	      if (!adw)
		throw OU::Error("For port \"%s\": adapter worker missing for connection", myName);
	      // Find the attached interconnect instance
	      const char *attach = ezxml_cattr(ad, "attachment");
	      for (ic = ezxml_cchild(myXml()->parent, "interconnect"); ic; ic = ezxml_next(ic)) {
		const char *icName = ezxml_cattr(ic, "attachment");
		if (icName && !strcasecmp(icName, attach)) {
		  // See if this interconnect worker is connected to this adapter
		  const char *iciName = ezxml_cattr(ic, "name");
		  ezxml_t c;
		  for (c = ezxml_cchild(myXml()->parent, "connection"); c; c = ezxml_next(c)) {
		    const char
		      *cfrom = ezxml_cattr(c,"from"), // instance with user port
		      *cto = ezxml_cattr(c,"to");     // instance with provider port
		    if ((!strcasecmp(cfrom, iciName) && !strcasecmp(cto, adName)) ||
			(!strcasecmp(cto, iciName) && !strcasecmp(cfrom, adName)))
		      break;
		  }
		  if (c)
		    break;
		}
	      }
	      if (!ic)
		throw OU::Error("For port \"%s\": adapter instance has no interconnect \"%s\"",
				myName, attach);
	      break; // with ad set to indicate we have an adapter, and ic set for its interconnect
	    }
	  }
          // We have a connection.  See if it is to an external interconnect.  FIXME i/o later
	  if (!ic)
	    for (ic = ezxml_child(myXml()->parent, "interconnect"); ic; ic = ezxml_next(ic)) {
	      const char *icName = ezxml_attr(ic, "name");
	      if (icName &&
		  ((iAmTo && !strcmp(icName, from)) ||
		   (!iAmTo && !strcmp(icName, to))))
		break;
	    }
	  if (ic) {
	    const char *icwName = ezxml_attr(ic, "worker");
	    if (icwName)
	      for (icw = ezxml_child(myXml()->parent, "worker"); icw; icw = ezxml_next(icw)) {
		const char *nameAttr = ezxml_attr(icw, "name");
		if (nameAttr && !strcmp(nameAttr, icwName))
		  break;
	      }
	    if (!icw)
	      throw OU::Error("For port \"%s\": interconnect worker missing for connection", myName);
	    break; // we found an external connection
	  }
	  break; // we found a connection
	}
      } // loop over all connections
      return *new Port(*this, props, mPort, conn, icw, ic, adw, ad);
    }
  }
}

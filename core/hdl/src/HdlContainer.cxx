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




/**
  @brief
  This file contains the implementation for the HDL container for FPGA
  Platformms
  It implements the OCPI::HDL::Container class, which implements the
  OCPI::Container::Interface class.

  Revision History:

    5/6/2009 - Jim Kulp
    Initial version.

************************************************************************** */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <unistd.h>
#include "OcpiOsAssert.h"
#include "OcpiUtilMisc.h"
#include "DtTransferInternal.h"
#include "ContainerManager.h"
#include "HdlOCDP.h"
#include "HdlDriver.h"
#include "HdlContainer.h"

#define wmb()        asm volatile("sfence" ::: "memory"); usleep(0)
#define clflush(p) asm volatile("clflush %0" : "+m" (*(char *)(p))) //(*(volatile char __force *)p))

namespace OA = OCPI::API;
namespace OC = OCPI::Container;
namespace OS = OCPI::OS;
namespace OU = OCPI::Util;
namespace OE = OCPI::Util::EzXml;
namespace OD = OCPI::DataTransport;
namespace OT = OCPI::Time;
namespace OR = OCPI::RDT;
namespace OH = OCPI::HDL;

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
      m_transports.resize(1);
      m_transports[0].transport = "ocpi-dma-pio";
      m_transports[0].id = ""; // someday this might be root node MAC address
      m_transports[0].roleIn = OR::ActiveMessage;
      m_transports[0].roleOut = OR::ActiveMessage;
      m_transports[0].optionsIn =
	(1 << OR::ActiveFlowControl) | (1 << OR::ActiveMessage) | (1 << OR::Passive);
      m_transports[0].optionsOut =
	(1 << OR::ActiveFlowControl) | (1 << OR::ActiveMessage) | (1 << OR::Passive);
      ocpiDebug("HDL Container for device %s constructed.  ESN: '%s' Platform/part is %s/%s.",
		name().c_str(), m_device.esn().c_str(), m_device.platform().c_str(),
		m_device.part().c_str());
    }
    Container::
    ~Container() {
      ocpiDebug("Entering ~HDL::Container()");
      OT::Emit::shutdown();
      this->lock();
      delete &m_device;
      ocpiDebug("Leaving ~HDL::Container()");
    }
    void Container::
    start() {}
    void Container::
    stop() {}
    bool Container::
    needThread() { return false; }
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
	if (!lart.uuid().empty() && c.hdlDevice().isLoadedUUID(lart.uuid()))
	  ocpiInfo("For HDL container %s, when loading bitstream %s, uuid matches what is already loaded\n",
		   c.name().c_str(), name().c_str());
	else {
	  ocpiInfo("Loading bitstream %s on HDL container %s\n",
		   name().c_str(), c.name().c_str());
	  c.hdlDevice().load(name().c_str());
	  // Now the bitstream is loaded, but that doesn't mean we have touched the device
	  std::string err;
	  if (c.hdlDevice().init(err) ||
	      c.hdlDevice().configure(NULL, err))
	    throw OU::Error("After loading %s on HDL device %s: %s",
			    name().c_str(), c.name().c_str(), err.c_str());
	  if (!c.hdlDevice().isLoadedUUID(lart.uuid()))
	    throw OU::Error("After loading %s on HDL device %s, uuid is wrong.  Wanted: %s",
			    name().c_str(), c.name().c_str(),
			    lart.uuid().c_str());
	}
      }
      ~Artifact() {}
    };
    // We know we have not already loaded it, but it still might be loaded on the device.
    OC::Artifact & Container::
    createArtifact(OCPI::Library::Artifact &lart, const OA::PValue *artifactParams)
    {
      return *new Artifact(*this, lart, artifactParams);
    }

    class Application : public OC::ApplicationBase<Container, Application, Worker> {
      friend class Container;
      Application(Container &con, const char *name, const OA::PValue *props) 
	: OC::ApplicationBase<Container, Application, Worker>(con, *this, name, props)
      {}
      OC::Worker &createWorker(OC::Artifact *art, const char *appInstName, ezxml_t impl,
			       ezxml_t inst, OC::Worker *slave, size_t member, size_t crewSize,
			       const OU::PValue *wParams);
    };

    OA::ContainerApplication *Container::
    createApplication(const char *name, const OCPI::Util::PValue *props)
      throw ( OCPI::Util::EmbeddedException ) {
      return new Application(*this, name, props);
    };
    class Port;
    class Worker : public OC::WorkerBase<Application, Worker, Port>, public WciControl {
      friend class Application;
      friend class Port;
      friend class ExternalPort;
      friend class Container;
      Container &m_container;
      Worker(Application &app, OC::Artifact *art, const char *name, ezxml_t implXml,
	     ezxml_t instXml, size_t member, size_t crewSize, const OA::PValue* execParams)
        : OC::WorkerBase<Application, Worker, Port>(app, *this, art, name, implXml,
						    instXml, member, crewSize, execParams),
	  WciControl(app.parent().hdlDevice(), implXml, instXml, properties()),
	  m_container(app.parent()) {
      }
    public:
      ~Worker()
      {
      }
      inline void controlOperation(OU::Worker::ControlOperation op) {
	WciControl::controlOperation(op);
      }

      // FIXME: These (and sequence/string stuff above) need to be sensitive to
      // addresing windows in OCCP.
      void read(size_t, size_t, void*) {
      }
      void write(size_t, size_t, const void*) {
      }

      OC::Port & createPort(const OU::Port &metaport, const OA::PValue *props);

      virtual void prepareProperty(OU::Property &mp,
				   volatile void *&writeVaddr,
				   const volatile void *&readVaddr) {
        return WciControl::prepareProperty(mp, writeVaddr, readVaddr);
      }

      OC::Port &
      createOutputPort(OU::PortOrdinal portId,
                       size_t bufferCount,
                       size_t bufferSize,
                       const OA::PValue* props) throw();
      OC::Port &
      createInputPort(OU::PortOrdinal portId,
                      size_t bufferCount,
                      size_t bufferSize,
                      const OA::PValue* props) throw();


#undef OCPI_DATA_TYPE_S
      // Set a scalar property value

#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)     	    \
      void								    \
      set##pretty##Property(unsigned ordinal, const run val) const {        \
        WciControl::set##pretty##Property(ordinal, val);        	    \
      }									    \
      void								    \
      set##pretty##SequenceProperty(const OA::Property &p, const run *vals, \
				    size_t length) const {		    \
	WciControl::set##pretty##SequenceProperty(p, vals, length);	    \
      }									    \
      run								    \
      get##pretty##Property(unsigned ordinal) const {		            \
	return WciControl::get##pretty##Property(ordinal);		    \
      }									    \
      unsigned								    \
      get##pretty##SequenceProperty(const OA::Property &p, run *vals,	    \
				    size_t length) const {		    \
	return								    \
      get##pretty##SequenceProperty(p, vals, length);		            \
      }
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
OCPI_DATA_TYPES
      void
      setStringProperty(unsigned ordinal, const char* val) const {
        WciControl::setStringProperty(ordinal, val);
      }
      void
      setStringSequenceProperty(const OA::Property &p, const char * const *val,
				size_t n) const {
	WciControl::setStringSequenceProperty(p, val, n);
      }
      void
      getStringProperty(unsigned ordinal, char *val, size_t length) const {
	WciControl::getStringProperty(ordinal, val, length);
      }
      unsigned
      getStringSequenceProperty(const OA::Property &p, char * *cp,
				size_t n ,char*pp, size_t nn) const {
	return WciControl::getStringSequenceProperty(p, cp, n, pp, nn);
      }
#define PUT_GET_PROPERTY(n)						         \
      void setProperty##n(const OA::PropertyInfo &info, uint##n##_t val) const { \
        WciControl::setProperty##n(info, val);				         \
      }									         \
      inline uint##n##_t getProperty##n(const OA::PropertyInfo &info) const {    \
	return WciControl::getProperty##n(info);			         \
      }									         
      PUT_GET_PROPERTY(8)
      PUT_GET_PROPERTY(16)
      PUT_GET_PROPERTY(32)
      PUT_GET_PROPERTY(64)
      void setPropertyBytes(const OA::PropertyInfo &info, size_t offset,
			    const uint8_t *data, size_t nBytes) const {
	WciControl::setPropertyBytes(info, offset, data, nBytes);
      }
      inline void
      getPropertyBytes(const OA::PropertyInfo &info, size_t offset, uint8_t *buf,
		       size_t nBytes) const {
	WciControl::getPropertyBytes(info, offset, buf, nBytes);
      }
    };
    OC::Worker & Application::
    createWorker(OC::Artifact *art, const char *appInstName, ezxml_t impl, ezxml_t inst,
		 OC::Worker *slave, size_t member, size_t crewSize,
		 const OCPI::Util::PValue *wParams) {
      assert(!slave);
      return *new Worker(*this, art, appInstName, impl, inst, member, crewSize, wParams);
    }
    // This port class really has two cases: externally connected ports and
    // internally connected ports.
    // Also ports are either user or provider.
    // So this class takes care of all 4 cases, since the differences are so
    // minor as to not be worth (re)factoring (currently).
    // The inheritance of WciControl is for the external case
    class ExternalPort;
    class Port : public OC::PortBase<OH::Worker,OH::Port,OH::ExternalPort>, WciControl {
      friend class Worker;
      friend class ExternalPort;
      friend class Container;
      ezxml_t m_connection;
      // These are for external-to-FPGA ports
      // Which would be in a different class if we separate them
      uint32_t m_ocdpSize;
      // For direct user access to ports
      volatile uint8_t *userDataBaseAddr;
      volatile OcdpMetadata *userMetadataBaseAddr;
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
        OC::PortBase<OH::Worker,OH::Port,OH::ExternalPort>(w, *this, mPort, params),
	// The WCI will control the interconnect worker.
	// If there is no such worker, usable will fail.
        WciControl(w.m_container.hdlDevice(), icwXml, icXml, NULL),
        m_connection(connXml), 
	// Note this can be from the region descriptors
	m_ocdpSize(m_properties.usable() ?
		   m_properties.get32RegisterOffset(offsetof(OcdpProperties, memoryBytes)) :
		   0),
      //        m_userConnected(false),
	m_adapter(adwXml ? new WciControl(w.m_container.hdlDevice(), adwXml, adXml, NULL) : 0),
	m_hasAdapterConfig(false),
	m_adapterConfig(0),
	m_endPoint(NULL)
      {
	const char *err;
	if (m_adapter && adXml &&
	    (err = OE::getNumber(adXml, "configure", &m_adapterConfig, &m_hasAdapterConfig, 0)))
	  throw OU::Error("Invalid configuration value for adapter: %s", err);
        if (!icwXml || !usable()) {
          m_canBeExternal = false;
          return;
        }
        m_canBeExternal = true;
	Device &device = w.parent().parent().hdlDevice();
	// Create an endpoint for the external port.
	// We have an endpoint-per-external-port to allow the FPGA implementation of each port
	// to share no state with other ports.  Someday this might not be the right choice,
	// especially when different DPs share an external memory port.
	// But it is possible to have both PCI and Ether ports and this requires it.
	m_endPoint =
	  &DataTransfer::getManager().allocateProxyEndPoint(device.endpointSpecific().c_str(),
							    OCPI_UTRUNCATE(size_t, device.endpointSize()));
	OD::Transport::fillDescriptorFromEndPoint(*m_endPoint, getData().data);
	// This should be the region address from admin, using the 0x1c region register
	// The region addresses are offsets in BAR1 at this point
	size_t myOcdpOffset;
	if ((err = OE::getNumber(icXml, "ocdpOffset", &myOcdpOffset, NULL, 0, false, true)))
	  throw OU::Error("Invalid or missing ocdpOffset attribute: %s", err);
	//        uint32_t myOcdpOffset = (uint32_t)OC::getAttrNum(icXml, "ocdpOffset"); // FIXME
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
        myDesc.metaDataPitch     = sizeof(OcdpMetadata);
	// This is the offset within the overall endpoint of our data buffers
        myDesc.dataBufferBaseAddr = device.dAccess().physOffset(myOcdpOffset);

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
        userDataBaseAddr = device.dAccess().registers() + myOcdpOffset;
        const char *df = getenv("OCPI_DUMP_PORTS");
        if (df) {
          if (dumpFd < 0)
            ocpiCheck((dumpFd = creat(df, 0666)) >= 0);
          OC::PortData *pd = this;
          ocpiCheck(::write(dumpFd, (void *)pd, sizeof(*pd)) == sizeof(*pd));
        }
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
      void disconnect()
        throw ( OCPI::Util::EmbeddedException )
      {
        throw OCPI::Util::EmbeddedException("disconnect not yet implemented !!");
      }

      bool isInProcess() const { return false; }

    protected:
      // Called after connection PValues have been set, which is after our constructor
      // userDataBaseAddr, dataBufferBaseAddr are assumed set
      // Also error-check for bad combinations or values of parameters
      // FIXME:  we are relying on dataBufferBaseAddr being set before we know
      // buffer sizes etc.  If we are sharing a memory pool, this will not be the case,
      // and we would probably allocate the whole thing here.
      const OCPI::RDT::Descriptors *
      startConnect(const OCPI::RDT::Descriptors */*other*/, bool &done) {
        if (!m_canBeExternal)
	  throw OU::Error("Port %s of this HDL worker is connected internally", name().c_str());

        if (myDesc.nBuffers *
            (OU::roundUp(myDesc.dataBufferSize, OCDP_LOCAL_BUFFER_ALIGN) + OCDP_METADATA_SIZE) > m_ocdpSize)
          throw OU::Error("Requested buffer count/size (%u/%u) on port '%s' of worker '%s' won't fit in the OCDP's memory (%u)",
			  myDesc.nBuffers, myDesc.dataBufferSize, name().c_str(),
			  parent().name().c_str(), m_ocdpSize);
        myDesc.dataBufferPitch = OCPI_UTRUNCATE(uint32_t, OU::roundUp(myDesc.dataBufferSize, OCDP_LOCAL_BUFFER_ALIGN));
        myDesc.metaDataBaseAddr =
          myDesc.dataBufferBaseAddr +
          m_ocdpSize - myDesc.nBuffers * OCDP_METADATA_SIZE;
        userMetadataBaseAddr = (OcdpMetadata *)(userDataBaseAddr +
                                                (myDesc.metaDataBaseAddr -
                                                 myDesc.dataBufferBaseAddr));
	done = false;
	return &getData().data;
      }

      // All the info is in.  Do final work to (locally) establish the connection
      // If we're output, we must return the flow control descriptor
      const OCPI::RDT::Descriptors *
      finishConnect(const OCPI::RDT::Descriptors *other,
		    OCPI::RDT::Descriptors &/*feedback*/, bool &done) {
        // Here is where we can setup the OCDP producer/user
        ocpiAssert(m_properties.get32Register(foodFace, OcdpProperties) == 0xf00dface);
	m_properties.set32Register(nLocalBuffers, OcdpProperties, myDesc.nBuffers);
	m_properties.set32Register(localBufferSize, OcdpProperties, myDesc.dataBufferPitch);
	m_properties.set32Register(localBufferBase, OcdpProperties, 0);
	m_properties.set32Register(localMetadataBase, OcdpProperties,
				    m_ocdpSize - myDesc.nBuffers * OCDP_METADATA_SIZE);
        OcdpRole myOcdpRole;
        OCPI::RDT::PortRole myRole = (OCPI::RDT::PortRole)getData().data.role;
	
	if (other) {
	  ocpiDebug("finishConnection: other = %" PRIx64 ", offset = %" DTOSDATATYPES_OFFSET_PRIx
		    ", RFB = %" PRIx64 "",
		    other->desc.oob.address,
		    isProvider() ? other->desc.emptyFlagBaseAddr : other->desc.fullFlagBaseAddr,
		    other->desc.oob.address +
		    (isProvider() ? other->desc.emptyFlagBaseAddr : other->desc.fullFlagBaseAddr));
	  ocpiDebug("Other ep = %s\n", other->desc.oob.oep );
	} else
	  ocpiDebug("Hdl finishConnect with no other");
        switch (myRole) {
	  uint64_t addr;
	  uint32_t pitch;
        case OCPI::RDT::ActiveFlowControl:
	  assert(other);
          myOcdpRole = OCDP_ACTIVE_FLOWCONTROL;
	  addr = other->desc.oob.address +
	    (isProvider() ? other->desc.emptyFlagBaseAddr : other->desc.fullFlagBaseAddr);
	  pitch = isProvider() ? other->desc.emptyFlagPitch : other->desc.fullFlagPitch;
          m_properties.set32Register(remoteFlagBase, OcdpProperties, (uint32_t)addr);
	  m_properties.set32Register(remoteFlagHi, OcdpProperties, (uint32_t)(addr >> 32));
          m_properties.set32Register(remoteFlagPitch, OcdpProperties, pitch);
	  ocpiDebug("HDL Port is %s, AFC, flag is 0x%" PRIx64 "pitch %u", 
		    isProvider() ? "consumer" : "producer", addr, pitch);
          break;
        case OCPI::RDT::ActiveMessage:
          myOcdpRole = OCDP_ACTIVE_MESSAGE;
	  addr = other->desc.oob.address + other->desc.dataBufferBaseAddr;
          m_properties.set32Register(remoteBufferBase, OcdpProperties, (uint32_t)addr);
	  m_properties.set32Register(remoteBufferHi, OcdpProperties, (uint32_t)(addr >> 32));
	  addr = other->desc.oob.address + other->desc.metaDataBaseAddr;
	  m_properties.set32Register(remoteMetadataBase, OcdpProperties, (uint32_t)addr);
	  m_properties.set32Register(remoteMetadataHi, OcdpProperties, (uint32_t)(addr >> 32));
          if (isProvider()) {
            if (other->desc.dataBufferSize > myDesc.dataBufferSize)
              throw OU::ApiError("At consumer, remote buffer size is larger than mine", NULL);
          } else if (other->desc.dataBufferSize < myDesc.dataBufferSize) {
            throw OU::ApiError("At producer, remote buffer size smaller than mine", NULL);
          }
          m_properties.set32Register(nRemoteBuffers, OcdpProperties, other->desc.nBuffers);
	  m_properties.set32Register(remoteBufferSize, OcdpProperties, other->desc.dataBufferPitch);
#ifdef WAS
          m_properties.set32Register(remoteMetadataSize, OcdpProperties, OCDP_METADATA_SIZE);
#else
          m_properties.set32Register(remoteMetadataSize, OcdpProperties, other->desc.metaDataPitch);
#endif
          addr = other->desc.oob.address + (isProvider() ? other->desc.emptyFlagBaseAddr : other->desc.fullFlagBaseAddr);
          m_properties.set32Register(remoteFlagBase, OcdpProperties, (uint32_t)addr);
          m_properties.set32Register(remoteFlagHi, OcdpProperties, (uint32_t)(addr >> 32));
          m_properties.set32Register(remoteFlagPitch, OcdpProperties, 
				      isProvider() ?
				      other->desc.emptyFlagPitch : other->desc.fullFlagPitch);
          break;
        case OCPI::RDT::Passive:
          myOcdpRole = OCDP_PASSIVE;
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
	controlOperation(OU::Worker::OpStart);
	done = true;
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
      bool done;
      pport.startConnect(NULL, done);
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
    }
    int Port::dumpFd = -1;

    // The port may be bidirectional.  If so we need to defer its direction.
    // FIXME: share all this parsing with the OU::Implementation code etc.
    OC::Port &Worker::
    createPort(const OU::Port &metaPort, const OA::PValue *props) {
      const char *myName = metaPort.m_name.c_str();
      // Find connections attached to this port
      ezxml_t conn, ic = 0, icw = 0, ad = 0, adw = 0;
      for (conn = ezxml_cchild(myXml()->parent, "connection"); conn; conn = ezxml_next(conn)) {
        const char
          *from = ezxml_cattr(conn,"from"), // instance with user port
          *to = ezxml_cattr(conn,"to"),     // instance with provider port
          *out = ezxml_cattr(conn, "out"),  // user port name
          *in = ezxml_cattr(conn, "in");    // provider port name
        if (from && to && out && in) {
	  bool iAmTo;
	  if (!strcasecmp(instTag().c_str(), to) && !strcasecmp(in, myName))
	    iAmTo = true;
	  else if (!strcasecmp(instTag().c_str(), from) && !strcasecmp(out, myName))
	    iAmTo = false;
	  else
	    continue;
          // We have a connection.  See if it is to a container adapter, which in turn would be
	  // connected to an interconnect.  No other adapters are expected yet.
          for (ad = ezxml_cchild(myXml()->parent, "adapter"); ad; ad = ezxml_next(ad)) {
            const char *adName = ezxml_cattr(ad, "name");
            if (adName &&
                (iAmTo && !strcasecmp(adName, from) ||
                 !iAmTo && !strcasecmp(adName, to))) {
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
		      *from = ezxml_cattr(c,"from"), // instance with user port
		      *to = ezxml_cattr(c,"to");     // instance with provider port
		    if (!strcasecmp(from, iciName) && !strcasecmp(to, adName) ||
			!strcasecmp(to, iciName) && !strcasecmp(from, adName))
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
		  (iAmTo && !strcmp(icName, from) ||
		   !iAmTo && !strcmp(icName, to)))
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
      return *new Port(*this, props, metaPort, conn, icw, ic, adw, ad);
    }
    // Here because these depend on Port
    OC::Port &Worker::
    createOutputPort(OU::PortOrdinal portId,
                     size_t bufferCount,
                     size_t bufferSize,
                     const OA::PValue* props) throw() {
      (void)portId; (void)bufferCount; (void)bufferSize;(void)props;
      return *(Port *)0;//return *new Port(*this);
    }
    OC::Port &Worker::
    createInputPort(OU::PortOrdinal portId,
                    size_t bufferCount,
                    size_t bufferSize,
                    const OA::PValue* props) throw() {
      (void)portId; (void)bufferCount; (void)bufferSize;(void)props;
      return *(Port *)0;//      return *new Port(*this);
    }
  }
}


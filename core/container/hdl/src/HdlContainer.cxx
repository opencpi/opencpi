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
#include <errno.h>
#include <uuid/uuid.h>
// FIXME: integrate this into our UUID utility properly
#ifndef _UUID_STRING_T
#define _UUID_STRING_T
typedef char uuid_string_t[50]; // darwin has 37 - lousy unsafe interface
#endif
#include "OcpiOsAssert.h"
#include "OcpiUtilMisc.h"
#include "OcpiPValue.h"
#include "DtTransferInternal.h"

#include "OcpiWorker.h"
#include "OcpiContainerMisc.h"

#include "HdlOCCP.h"
#include "HdlOCDP.h"
#include "HdlDriver.h"
#include "HdlContainer.h"

#define wmb()        asm volatile("sfence" ::: "memory"); usleep(0)
#define clflush(p) asm volatile("clflush %0" : "+m" (*(char *)(p))) //(*(volatile char __force *)p))

namespace OCPI {
  namespace Container {}
  namespace HDL {
    namespace OA = OCPI::API;
    namespace OC = OCPI::Container;
    namespace OS = OCPI::OS;
    namespace OM = OCPI::Metadata;
    namespace OU = OCPI::Util;
    namespace OE = OCPI::Util::EzXml;
    namespace OD = OCPI::DataTransport;
    namespace OT = OCPI::Time;

    static inline unsigned max(unsigned a,unsigned b) { return a > b ? a : b;}
    // This is the alignment constraint of DMA buffers in the processor's memory.
    // It could be a cache line or a malloc granule...
    // It should come from somewhere else.  FIXME
    static const unsigned LOCAL_BUFFER_ALIGN = 32;
    

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
      m_platform = m_device.platform();
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
    // friends
    void Container::
    getWorkerAccess(unsigned index,
		    Access &worker,
		    Access &properties) {
      if (index >= OCCP_MAX_WORKERS)
	throw OU::Error("Invalid occpIndex property");
      // FIXME:  check runtime for connected worker
      // This is ugly because offsetof is stupid
      offsetRegisters(worker, (intptr_t)(&((OccpSpace*)0)->worker[index]));
      offsetRegisters(properties,(intptr_t)(&((OccpSpace*)0)->config[index]));
    }
    void Container::
    releaseWorkerAccess(unsigned /* index */,
			Access & /* worker */,
			Access & /* properties */) {
    }
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
	  c.hdlDevice().getUUID();
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
    // The class that knows about WCI interfaces and the OCCP.
    class WciControl : public Access, virtual public OC::Controllable {
      friend class Port;
      const char *implName, *instName;
      mutable uint32_t m_window; // perfect use-case for mutable..
      bool m_hasControl;
    protected:
      Access m_properties;
      Container &m_wciContainer;
      // myRegisters is zero when this WCI does not really exist.
      // (since we inherit this in some cases were it is not needed).
      unsigned myOccpIndex;
      WciControl(Container &container, ezxml_t implXml, ezxml_t instXml)
        : implName(0), instName(0), m_window(0), m_hasControl(false), m_wciContainer(container)
      {
	setControlOperations(ezxml_cattr(implXml, "controlOperations"));
        if (!implXml)
          return;
        implName = ezxml_attr(implXml, "name");
        instName = ezxml_attr(instXml, "name");
        myOccpIndex = OC::getAttrNum(instXml, "occpIndex", true, &m_hasControl);
	if (m_hasControl) {
	  setControlMask(getControlMask() | 1 << OM::Worker::OpStart);
	  uint32_t timeout = OC::getAttrNum(implXml, "timeout", true);
	  if (!timeout)
	    timeout = 16;
	  unsigned logTimeout = 31;
	  for (uint32_t u = 1 << logTimeout; !(u & timeout);
	       u >>= 1, logTimeout--)
	    ;
	  ocpiDebug("Timeout for $%s is %d\n", implName, logTimeout);
	  // Get access to registers and properties
	  container.getWorkerAccess(myOccpIndex, *this, m_properties);
	  // Assert Reset
	  // myRegisters->control =  logTimeout;
	  set32Register(control,  OccpWorkerRegisters, logTimeout);
#ifndef SHEP_FIXME_THE_RESET
	  struct timespec spec;
	  spec.tv_sec = 0;
	  spec.tv_nsec = 10000;
	  int bad = nanosleep(&spec, 0);
	  ocpiCheck(bad == 0);
#endif
	  // Take out of reset
	  // myRegisters->control = OCCP_CONTROL_ENABLE | logTimeout ;
	  set32Register(control,  OccpWorkerRegisters, OCCP_WORKER_CONTROL_ENABLE | logTimeout);
	  ocpiDebug("Deasserted reset on worker %s.%s", implName, instName);
#if 0 // do this by changing the accessor
	  if (getenv("OCPI_OCFRP_DUMMY")) {
	    *(uint32_t *)&myRegisters->initialize = OCCP_SUCCESS_RESULT; //fakeout
	    *(uint32_t *)&myRegisters->start = OCCP_SUCCESS_RESULT; //fakeout
	  }
#endif
	} else {
	  setControlMask(0);
	  ocpiDebug("HDL container found impl %s inst %s with no control",
		    implName ? implName : "<none>", 
		    instName ? instName : "<none>");
	}
	m_window = 0;
      }
      virtual ~WciControl() {
	m_wciContainer.releaseWorkerAccess(myOccpIndex, *this, m_properties);
      }
      // Add the hardware considerations to the property object that supports
      // fast memory-mapped property access directly to users
      // the key members are "readVaddr" and "writeVaddr"
      virtual void prepareProperty(OU::Property &md, 
				   volatile void *&writeVaddr,
				   const volatile void *&readVaddr) {
	ocpiAssert(m_hasControl);
	(void)readVaddr;
        if (m_properties.registers() &&
	    md.m_baseType != OA::OCPI_Struct && !md.m_isSequence &&
	    md.m_baseType != OA::OCPI_String &&
	    OU::baseTypeSizes[md.m_baseType] <= 32 &&
	    md.m_offset < OCCP_WORKER_CONFIG_SIZE &&
	    !md.m_writeError &&
	    !md.m_isIndirect)
	  writeVaddr = m_properties.registers() + md.m_offset;
      }
      // Map the control op numbers to structure members
      static const unsigned controlOffsets[];
    protected:
      void controlOperation(OCPI::Metadata::Worker::ControlOperation op) {
	if (getControlMask() & (1 << op)) {
	  uint32_t result =
	    // *((volatile uint32_t *)myRegisters + controlOffsets[op]);
	    get32RegisterOffset(controlOffsets[op]);
	  if (result != OCCP_SUCCESS_RESULT
	      && result != 0 && result != 0x01020304) { // TEMP HACK UNTIL SHEP FIXES WCI MASTER
	    const char *oops;
	    switch (result) {
	    case OCCP_TIMEOUT_RESULT:
	      oops = "timed out performing control operation (no OCP response on the WCI)";
	      break;
	    case OCCP_ERROR_RESULT:
	      oops = "indicated an error from control operation";
	      break;
	    case OCCP_RESET_RESULT:
	      oops = "was in a reset state when control operation was requested";
	      break;
	    case OCCP_FATAL_RESULT:
	      oops = "indicated a fatal error from control operation";
	      break;
	    default:
	      oops = "returned unknown result value from control operation";
	    }
	    throw OU::Error("worker %s:%s op %u %s (%0x" PRIx32 ")", implName, instName, op, oops, result);
	  }
	}
      }
      inline uint32_t
      checkWindow(uint32_t offset, unsigned nBytes) const {
	ocpiAssert(m_hasControl);
	unsigned window = offset & ~(OCCP_WORKER_CONFIG_SIZE-1);
        ocpiAssert(window == ((offset+nBytes)&~(OCCP_WORKER_CONFIG_SIZE-1)));
	if (window != m_window) {
	  set32Register(window, OccpWorkerRegisters,
			window >> OCCP_WORKER_CONFIG_WINDOW_BITS);
	  m_window = window;
	}
	return offset & (OCCP_WORKER_CONFIG_SIZE - 1);
      }

#define PUT_GET_PROPERTY(n)						          \
      void setProperty##n(const OA::PropertyInfo &info, uint##n##_t val) const {  \
        uint32_t offset = checkWindow(info.m_offset, n/8);			  \
	uint32_t status = 0;						          \
	if (m_properties.m_registers) {					          \
	  if (!info.m_writeError ||					          \
	      !(status =						          \
		get32Register(status, OccpWorkerRegisters) &                      \
		OCCP_STATUS_WRITE_ERRORS))                			  \
	    m_properties.set##n##RegisterOffset(offset, val);                     \
	  if (info.m_writeError && !status)				          \
	    status =							          \
	      get32Register(status, OccpWorkerRegisters) &		          \
	      OCCP_STATUS_WRITE_ERRORS;					          \
	} else								          \
	  m_properties.m_accessor->set##n(m_properties.m_base + offset, val,      \
					  &status);			          \
	if (status)							          \
	  throwPropertyWriteError(status);				          \
      }									          \
      inline uint##n##_t getProperty##n(const OA::PropertyInfo &info) const {     \
        uint32_t offset = checkWindow(info.m_offset, n/8);			  \
	uint32_t status = 0;						          \
	uint##n##_t val;						          \
	if (m_properties.m_registers) {					          \
	  if (!info.m_readError ||					          \
	      !(status =						          \
		get32Register(status, OccpWorkerRegisters) &		          \
		OCCP_STATUS_READ_ERRORS))				          \
	    val = m_properties.get##n##RegisterOffset(offset);                    \
	  if (info.m_readError && !status)				          \
	    status =							          \
	      get32Register(status, OccpWorkerRegisters) &		          \
	      OCCP_STATUS_READ_ERRORS;					          \
	} else								          \
	  val = m_properties.m_accessor->get##n(m_properties.m_base + offset,     \
						&status);                         \
	if (status)							          \
	  throwPropertyReadError(status);				          \
	return val;							          \
      }
      PUT_GET_PROPERTY(8)
      PUT_GET_PROPERTY(16)
      PUT_GET_PROPERTY(32)
      PUT_GET_PROPERTY(64)
#undef PUT_GET_PROPERTY
      void setPropertyBytes(const OA::PropertyInfo &info, uint32_t offset,
			    const uint8_t *data, unsigned nBytes) const {
        offset = checkWindow(offset, nBytes);
	uint32_t status = 0;
	if (m_properties.m_registers) {
	  if (!info.m_writeError ||
	      !(status = (get32Register(status, OccpWorkerRegisters) &
			  OCCP_STATUS_WRITE_ERRORS)))
	    m_properties.setBytesRegisterOffset(offset,	data, nBytes);
	  if (info.m_writeError && !status)
	    status = (get32Register(status, OccpWorkerRegisters) &
		      OCCP_STATUS_WRITE_ERRORS);
	} else
	  m_properties.m_accessor->setBytes(m_properties.m_base + offset, data, nBytes, &status);
	if (status)
	  throwPropertyWriteError(status);
      }

      inline void
      getPropertyBytes(const OA::PropertyInfo &info, uint32_t offset, uint8_t *buf, unsigned nBytes) const {
        offset = checkWindow(offset, nBytes);
	uint32_t status = 0;

	if (m_properties.m_registers) {
	  if (!info.m_readError ||
	      !(status = (get32Register(status, OccpWorkerRegisters) &
			  OCCP_STATUS_READ_ERRORS))) {
	    m_properties.getBytesRegisterOffset(offset, buf, nBytes);
	    if (info.m_readError)
	      status = get32Register(status, OccpWorkerRegisters) & OCCP_STATUS_READ_ERRORS;
	  }
	} else
	  m_properties.m_accessor->getBytes(m_properties.m_base + offset, buf, nBytes, &status);
	if (status)
	  throwPropertyReadError(status);
      }
      inline void setPropertySequence(const OA::Property &p,
				      const uint8_t *val,
				      uint32_t nItems, unsigned nBytes) const {
	uint32_t offset = checkWindow(p.m_info.m_offset, nBytes);
	uint32_t status = 0;
	if (m_properties.m_registers) {
	  if (!p.m_info.m_writeError ||
	      !(status =
		get32Register(status, OccpWorkerRegisters) &
		OCCP_STATUS_WRITE_ERRORS)) {
	    m_properties.setBytesRegisterOffset(offset + p.m_info.m_align,
						val, nBytes);
	    m_properties.set32RegisterOffset(offset, nItems);
	  }
	  if (p.m_info.m_writeError && !status)
	    status =
	      get32Register(status, OccpWorkerRegisters) &
	      OCCP_STATUS_WRITE_ERRORS;
	} else {
	  m_properties.m_accessor->setBytes(m_properties.m_base + offset + p.m_info.m_align,
					    val, nBytes, &status);
	  if (!status)
	    m_properties.m_accessor->set32(m_properties.m_base + offset, nItems, &status);
	}
	if (status)
	  throwPropertyWriteError(status);
      }
      inline unsigned
      getPropertySequence(const OA::Property &p, uint8_t *buf, unsigned n) const {
	uint32_t offset = checkWindow(p.m_info.m_offset, n);
	uint32_t status = 0, nItems;

	if (m_properties.m_registers) {
	  if (!p.m_info.m_readError ||
	      !(status =
		get32Register(status, OccpWorkerRegisters) &
		OCCP_STATUS_READ_ERRORS))
	    nItems = m_properties.get32RegisterOffset(offset);
	  if (!p.m_info.m_readError && 
	      !(status =
		get32Register(status, OccpWorkerRegisters) &
		OCCP_STATUS_READ_ERRORS)) {
	    if (nItems * (p.m_info.m_nBits/8) <= n)
	      throwPropertySequenceError();
	    m_properties.getBytesRegisterOffset(offset + p.m_info.m_align, 
						buf, nItems * (p.m_info.m_nBits/8));
	    if (p.m_info.m_readError && !status)
	      status =
		get32Register(status, OccpWorkerRegisters) &
		OCCP_STATUS_READ_ERRORS;
	  }
	} else {
	  nItems = m_properties.m_accessor->get32(m_properties.m_base + offset, &status);
	  if (!status) {
	    if (nItems * (p.m_info.m_nBits/8) <= n)
	      throwPropertySequenceError();
	    m_properties.m_accessor->getBytes(m_properties.m_base + offset + p.m_info.m_align, 
					      buf, nItems * (p.m_info.m_nBits/8),
					      &status);
	  }
	}
	if (status)
	  throwPropertyReadError(status);
	return nItems;
      }

      
#undef OCPI_DATA_TYPE_S
      // Set a scalar property value

#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)     	    \
      void								    \
      set##pretty##Property(const OA::Property &p, const run val) const {   \
	setProperty##bits(p.m_info, *(uint##bits##_t *)&val);		    \
      }									    \
      void								    \
      set##pretty##SequenceProperty(const OA::Property &p, const run *vals, \
				    unsigned length) const {		\
	setPropertySequence(p, (const uint8_t *)vals,			\
			    length, length * (bits/8));			\
      }									\
      run								\
      get##pretty##Property(const OA::Property &p) const {		\
	return (run)getProperty##bits(p.m_info);			\
      }									\
      unsigned								\
      get##pretty##SequenceProperty(const OA::Property &p, run *vals,	\
				    unsigned length) const {		\
	return								\
	  getPropertySequence(p, (uint8_t *)vals, length * (bits/8));	\
      }
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
OCPI_DATA_TYPES
#undef OCPI_DATA_TYPE
      void
      setStringProperty(const OA::Property &p, const char* val) const {
	uint32_t n = strlen(val) + 1;
	setPropertySequence(p, (const uint8_t *)val, n, n);
      }
      void
      setStringSequenceProperty(const OA::Property &, const char * const *,
				unsigned ) const {
	throw OU::Error("No support for properties that are sequences of strings");
      }
      void
      getStringProperty(const OA::Property &p, char *val, unsigned length) const {
	// ignore return value
	getPropertySequence(p, (uint8_t*)val, length);
      }
      unsigned
      getStringSequenceProperty(const OA::Property &, char * *,
				unsigned ,char*, unsigned) const {
	throw OU::Error("No support for properties that are sequences of strings");
	return 0;
      }
      // These property access methods are called when the fast path
      // is not enabled, either due to no MMIO or that the property can
      // return errors.
      static void throwPropertyReadError(uint32_t status) {
	throw OU::Error("property reading error: %s",
			status & OCCP_STATUS_READ_TIMEOUT ? "timeout" : "worker generated error response");
      }
      static void throwPropertyWriteError(uint32_t status) {
	throw OU::Error("property writing error: %s",
			status & OCCP_STATUS_WRITE_TIMEOUT ? "timeout" : "worker generated error response");
      }
      static void throwPropertySequenceError() {
	throw OU::Error("property sequence length overflow on read/get");
      }
    };
    // c++ doesn't allow static initializations in class definitions.
    const unsigned WciControl::controlOffsets[] = {
#define CONTROL_OP(x, c, t, s1, s2, s3) \
	offsetof(OccpWorkerRegisters,x),
	OCPI_CONTROL_OPS
#undef CONTROL_OP
	0};

    class Application : public OC::ApplicationBase<Container, Application, Worker> {
      friend class Container;
      Application(Container &con, const char *name, const OA::PValue *props) 
	: OC::ApplicationBase<Container, Application, Worker>(con, *this, name, props)
      {}
      OC::Worker & createWorker(OC::Artifact *art,
				const char *appInstName,
				ezxml_t impl, ezxml_t inst,
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
      Container &m_container;
      Worker(Application &app, OC::Artifact *art, const char *name,
             ezxml_t implXml, ezxml_t instXml, const OA::PValue* execProps) :
        OC::WorkerBase<Application, Worker, Port>(app, *this, art, name, implXml,
						  instXml, execProps),
        WciControl(app.parent(), implXml, instXml),
        m_container(app.parent())
      {
	(void)execProps;
      }
    public:
      ~Worker()
      {
      }
      inline void controlOperation(OM::Worker::ControlOperation op) {
	WciControl::controlOperation(op);
      }

      void checkControlState() {
	// Override, and eventually poll/check for FINISHED etc.
      }
      // FIXME: These (and sequence/string stuff above) need to be sensitive to
      // addresing windows in OCCP.
      void read(uint32_t, uint32_t, void*) {
      }
      void write(uint32_t, uint32_t, const void*) {
      }

      OC::Port & createPort(const OM::Port &metaport, const OA::PValue *props);

      virtual void prepareProperty(OU::Property &mp,
				   volatile void *&writeVaddr,
				   const volatile void *&readVaddr) {
        return WciControl::prepareProperty(mp, writeVaddr, readVaddr);
      }

      OC::Port &
      createOutputPort(OM::PortOrdinal portId,
                       OS::uint32_t bufferCount,
                       OS::uint32_t bufferSize,
                       const OA::PValue* props) throw();
      OC::Port &
      createInputPort(OM::PortOrdinal portId,
                      OS::uint32_t bufferCount,
                      OS::uint32_t bufferSize,
                      const OA::PValue* props) throw();


#undef OCPI_DATA_TYPE_S
      // Set a scalar property value

#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)     	    \
      void								    \
      set##pretty##Property(const OA::Property &p, const run val) const {   \
        WciControl::set##pretty##Property(p, val);			    \
      }									    \
      void								    \
      set##pretty##SequenceProperty(const OA::Property &p, const run *vals, \
				    unsigned length) const {		    \
	WciControl::set##pretty##SequenceProperty(p, vals, length);	    \
      }									    \
      run								    \
      get##pretty##Property(const OA::Property &p) const {		    \
	return WciControl::get##pretty##Property(p);			    \
      }									    \
      unsigned								    \
      get##pretty##SequenceProperty(const OA::Property &p, run *vals,	    \
				    unsigned length) const {		    \
	return								    \
      get##pretty##SequenceProperty(p, vals, length);		            \
      }
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
OCPI_DATA_TYPES
      void
      setStringProperty(const OA::Property &p, const char* val) const {
        WciControl::setStringProperty(p, val);
      }
      void
      setStringSequenceProperty(const OA::Property &p, const char * const *val,
				unsigned n) const {
	WciControl::setStringSequenceProperty(p, val, n);
      }
      void
      getStringProperty(const OA::Property &p, char *val, unsigned length) const {
	WciControl::getStringProperty(p, val, length);
      }
      unsigned
      getStringSequenceProperty(const OA::Property &p, char * *cp,
				unsigned n ,char*pp, unsigned nn) const {
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
      void setPropertyBytes(const OA::PropertyInfo &info, uint32_t offset,
			    const uint8_t *data, unsigned nBytes) const {
	WciControl::setPropertyBytes(info, offset, data, nBytes);
      }
      inline void
      getPropertyBytes(const OA::PropertyInfo &info, uint32_t offset, uint8_t *buf,
		       unsigned nBytes) const {
	WciControl::getPropertyBytes(info, offset, buf, nBytes);
      }
    };
    OC::Worker & Application::createWorker(OC::Artifact *art, const char *appInstName,
					   ezxml_t impl, ezxml_t inst,
					   const OCPI::Util::PValue *wParams) {
	return *new Worker(*this, art, appInstName, impl, inst, wParams);
      }
    // This port class really has two cases: externally connected ports and
    // internally connected ports.
    // Also ports are either user or provider.
    // So this class takes care of all 4 cases, since the differences are so
    // minor as to not be worth (re)factoring (currently).
    // The inheritance of WciControl is for the external case
    class ExternalPort;
    class Port : public OC::PortBase<Worker,Port,ExternalPort>, WciControl {
      friend class Worker;
      friend class ExternalPort;
      ezxml_t m_connection;
      // These are for external-to-FPGA ports
      // Which would be in a different class if we separate them
      unsigned m_ocdpSize;
      // For direct user access to ports
      volatile uint8_t *userDataBaseAddr;
      volatile OcdpMetadata *userMetadataBaseAddr;
      bool m_userConnected;
      WciControl *m_adapter; // if there is an adapter
      bool m_hasAdapterConfig;
      uint32_t m_adapterConfig;
      static int dumpFd;
      DataTransfer::EndPoint *m_endPoint; // the data plane endpoint if externally connected

      Port(Worker &w,
	   const OA::PValue *params,
           const OM::Port &mPort, // the parsed port metadata
           ezxml_t connXml, // the xml connection for this port
           ezxml_t icwXml,  // the xml interconnect/infrastructure worker attached to this port if any
           ezxml_t icXml,   // the xml interconnect instance attached to this port if any
           ezxml_t adwXml,  // the xml adapter/infrastructure worker attached to this port if any
           ezxml_t adXml,   // the xml adapter instance attached to this port if any
	   bool argIsProvider) :
        OC::PortBase<Worker,Port,ExternalPort>(w, *this, mPort, argIsProvider,
					       // (1 << OCPI::RDT::Passive) |
					       // (1 << OCPI::RDT::ActiveFlowControl) |
					       (1 << OCPI::RDT::ActiveMessage), params),
	// The WCI will control the interconnect worker.
	// If there is no such worker, usable will fail.
        WciControl(w.m_container, icwXml, icXml),
        m_connection(connXml), 
	// Note this can be from the region descriptors
	m_ocdpSize(m_properties.usable() ?
		   m_properties.get32RegisterOffset(offsetof(OcdpProperties, memoryBytes)) :
		   0),
        m_userConnected(false),
	m_adapter(adwXml ? new WciControl(w.m_container, adwXml, adXml) : 0),
	m_hasAdapterConfig(false),
	m_adapterConfig(0),
	m_endPoint(NULL)
      {
	const char *err;
	if (m_adapter && adXml &&
	    (err = OU::EzXml::getNumber(adXml, "configure", &m_adapterConfig, &m_hasAdapterConfig, 0)))
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
							    device.endpointSize());
	OD::Transport::fillDescriptorFromEndPoint(*m_endPoint, getData().data);
	// This should be the region address from admin, using the 0x1c region register
	// The region addresses are offsets in BAR1 at this point
        uint32_t myOcdpOffset = OC::getAttrNum(icXml, "ocdpOffset"); // FIXME
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
#if 0
        if (getenv("OCPI_OCFRP_DUMMY"))
          *(uint32_t*)&m_ocdpRegisters->foodFace = 0xf00dface;
#endif
	// Allow default connect params on port construction prior to connect
	applyConnectParams(NULL, params);
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
      void setMode( ConnectionMode ){};
      void disconnect()
        throw ( OCPI::Util::EmbeddedException )
      {
        throw OCPI::Util::EmbeddedException("disconnect not yet implemented !!");
      }

      bool isLocal() const { return false; }

      // Called after connection PValues have been set, which is after our constructor
      // userDataBaseAddr, dataBufferBaseAddr are assumed set
      // Also error-check for bad combinations or values of parameters
      // FIXME:  we are relying on dataBufferBaseAddr being set before we know
      // buffer sizes etc.  If we are sharing a memory pool, this will not be the case,
      // and we would probably allocate the whole thing here.
      void startConnect(const OCPI::RDT::Descriptors */*other*/, const OCPI::Util::PValue */*params*/) {
        if (!m_canBeExternal)
          return;
        if (myDesc.nBuffers *
            (OC::roundup(myDesc.dataBufferSize, OCDP_LOCAL_BUFFER_ALIGN) + OCDP_METADATA_SIZE) > m_ocdpSize)
          throw OC::ApiError("Requested buffer count and size won't fit in the OCDP's memory", 0);
        myDesc.dataBufferPitch = OC::roundup(myDesc.dataBufferSize, OCDP_LOCAL_BUFFER_ALIGN);
        myDesc.metaDataBaseAddr =
          myDesc.dataBufferBaseAddr +
          m_ocdpSize - myDesc.nBuffers * OCDP_METADATA_SIZE;
        userMetadataBaseAddr = (OcdpMetadata *)(userDataBaseAddr +
                                                (myDesc.metaDataBaseAddr -
                                                 myDesc.dataBufferBaseAddr));
      }

      // All the info is in.  Do final work to (locally) establish the connection
      // If we're output, we must return the flow control descriptor
      const OCPI::RDT::Descriptors *
      finishConnect(const OCPI::RDT::Descriptors &other,
		    OCPI::RDT::Descriptors &/*feedback*/) {
        // Here is where we can setup the OCDP producer/user
        ocpiAssert(m_properties.get32Register(foodFace, OcdpProperties) == 0xf00dface);
	m_properties.set32Register(nLocalBuffers, OcdpProperties, myDesc.nBuffers);
	m_properties.set32Register(localBufferSize, OcdpProperties, myDesc.dataBufferPitch);
	m_properties.set32Register(localBufferBase, OcdpProperties, 0);
	m_properties.set32Register(localMetadataBase, OcdpProperties,
				    m_ocdpSize - myDesc.nBuffers * OCDP_METADATA_SIZE);
        OcdpRole myOcdpRole;
        OCPI::RDT::PortRole myRole = (OCPI::RDT::PortRole)getData().data.role;
	
        ocpiDebug("finishConnection: other = %" PRIx64 ", offset = %" PRIx64 ", RFB = %" PRIx64 "",
		  other.desc.oob.address,
		  isProvider() ? other.desc.emptyFlagBaseAddr : other.desc.fullFlagBaseAddr,
		  other.desc.oob.address +
		  (isProvider() ? other.desc.emptyFlagBaseAddr : other.desc.fullFlagBaseAddr));
	ocpiDebug("Other ep = %s\n", other.desc.oob.oep );
        switch (myRole) {
	  uint64_t addr;
	  uint32_t pitch;
        case OCPI::RDT::ActiveFlowControl:
          myOcdpRole = OCDP_ACTIVE_FLOWCONTROL;
	  addr = other.desc.oob.address +
	    (isProvider() ? other.desc.emptyFlagBaseAddr : other.desc.fullFlagBaseAddr);
	  pitch = isProvider() ? other.desc.emptyFlagPitch : other.desc.fullFlagPitch;
          m_properties.set32Register(remoteFlagBase, OcdpProperties, addr);
	  m_properties.set32Register(remoteFlagHi, OcdpProperties, addr > 32);
          m_properties.set32Register(remoteFlagPitch, OcdpProperties, pitch);
	  ocpiDebug("HDL Port is %s, AFC, flag is 0x%" PRIx64 "pitch %u", 
		    isProvider() ? "consumer" : "producer", addr, pitch);
          break;
        case OCPI::RDT::ActiveMessage:
          myOcdpRole = OCDP_ACTIVE_MESSAGE;
	  addr = other.desc.oob.address + other.desc.dataBufferBaseAddr;
          m_properties.set32Register(remoteBufferBase, OcdpProperties, addr);
	  m_properties.set32Register(remoteBufferHi, OcdpProperties, addr >> 32);
	  addr = other.desc.oob.address + other.desc.metaDataBaseAddr;
	  m_properties.set32Register(remoteMetadataBase, OcdpProperties, addr);
	  m_properties.set32Register(remoteMetadataHi, OcdpProperties, addr >> 32);
          if ( isProvider()) {
            if (other.desc.dataBufferSize > myDesc.dataBufferSize)
              throw OC::ApiError("At consumer, remote buffer size is larger than mine", NULL);
          } else if (other.desc.dataBufferSize < myDesc.dataBufferSize) {
            throw OC::ApiError("At producer, remote buffer size smaller than mine", NULL);
          }
          m_properties.set32Register(nRemoteBuffers, OcdpProperties, other.desc.nBuffers);
	  m_properties.set32Register(remoteBufferSize, OcdpProperties, other.desc.dataBufferPitch);
#ifdef WAS
          m_properties.set32Register(remoteMetadataSize, OcdpProperties, OCDP_METADATA_SIZE);
#else
          m_properties.set32Register(remoteMetadataSize, OcdpProperties, other.desc.metaDataPitch);
#endif
          addr = other.desc.oob.address + (isProvider() ? other.desc.emptyFlagBaseAddr : other.desc.fullFlagBaseAddr);
          m_properties.set32Register(remoteFlagBase, OcdpProperties, addr);
          m_properties.set32Register(remoteFlagHi, OcdpProperties, addr >> 32);
          m_properties.set32Register(remoteFlagPitch, OcdpProperties, 
				      isProvider() ?
				      other.desc.emptyFlagPitch : other.desc.fullFlagPitch);
          break;
        case OCPI::RDT::Passive:
          myOcdpRole = OCDP_PASSIVE;
          break;
        default:
          myOcdpRole = OCDP_PASSIVE; // quiet compiler warning
          ocpiAssert(0);
        }
        ocpiDebug("finishConnection: me = %" PRIx64 ", offset = %" PRIx64 ", RFB = %" PRIx64 "",
		  myDesc.oob.address,
		  isProvider() ? myDesc.fullFlagBaseAddr : myDesc.emptyFlagBaseAddr,
		  myDesc.oob.address +
		  (isProvider() ? myDesc.fullFlagBaseAddr : myDesc.emptyFlagBaseAddr));
	ocpiDebug("My ep = %s\n", myDesc.oob.oep );
	m_properties.set32Register(control, OcdpProperties,
				    OCDP_CONTROL(isProvider() ? OCDP_CONTROL_CONSUMER :
						 OCDP_CONTROL_PRODUCER, myOcdpRole));
	// We aren't a worker so someone needs to start us.
	controlOperation(OM::Worker::OpInitialize);
	if (m_adapter) {
	  m_adapter->controlOperation(OM::Worker::OpInitialize);
	  if (m_hasAdapterConfig)
	    m_adapter->m_properties.set32RegisterOffset(0, m_adapterConfig);
	  m_adapter->controlOperation(OM::Worker::OpStart);
	}
	controlOperation(OM::Worker::OpStart);
	return isProvider() ? NULL : &getData().data;
      }
      // Connection between two ports inside this container
      // We know they must be in the same artifact, and have a metadata-defined connection
      void connectInside(OC::Port &provider, const OA::PValue *) {
        // We're both in the same runtime artifact object, so we know the port class
        Port &pport = static_cast<Port&>(provider);
        if (m_connection != pport.m_connection)
          throw OC::ApiError("Ports are both local in bitstream/artifact, but are not connected", 0);
      }
#if 0
      // Connect to a port in a like container (same driver)
      bool connectLike(const OA::PValue *uProps, OC::Port &provider, const OA::PValue *pProps) {
        // We're both in the same runtime artifact object, so we know the port class
        Port &pport = static_cast<Port&>(provider);
        ocpiAssert(m_canBeExternal && pport.m_canBeExternal);
        pport.applyConnectParams(pProps);
        applyConnectParams(uProps);
        determineRoles(provider.getData().data);
        finishConnect(provider.getData().data);
        pport.finishConnection(getData().data);
        return true;
      }
#endif
      OC::ExternalPort &createExternal(const char *extName, bool isProvider,
				       const OU::PValue *extParams, const OU::PValue *connParams);
#if 0
      // Directly connect to this port
      // which creates a dummy user port
      OC::ExternalPort &connectExternal(const char *name, const OA::PValue *userProps,
					const OA::PValue *props);
#endif
    };
    int Port::dumpFd = -1;

    // The port may be bidirectional.  If so we need to defer its direction.
    OC::Port &Worker::
    createPort(const OM::Port &metaPort, const OA::PValue *props) {
      const char *myName = metaPort.name;
      bool isProvider = metaPort.provider;
#if 0
      // Process direction overrides for the instance
      if (myInstXml()) {
	if (OU::EzXml::inList(name, ezxml_cattr(myInstXml(), "inputs")))
	  isProvider = true;
	if (OU::EzXml::inList(name, ezxml_cattr(myInstXml(), "outputs")))
	  isProvider = false;
      }
#endif
      // Find connections attached to this port
      ezxml_t conn, ic = 0, icw = 0, ad = 0, adw = 0;
      for (conn = ezxml_child(myXml()->parent, "connection"); conn; conn = ezxml_next(conn)) {
        const char
          *from = ezxml_attr(conn,"from"), // instance with user port
          *to = ezxml_attr(conn,"to"),     // instance with provider port
          *out = ezxml_attr(conn, "out"),  // user port name
          *in = ezxml_attr(conn, "in");    // provider port name
        if (from && to && out && in) {
	  bool iAmTo;
	  if (!strcmp(instTag().c_str(), to) && !strcmp(in, myName))
	    iAmTo = true;
	  else if (!strcmp(instTag().c_str(), from) && !strcmp(out, myName))
	    iAmTo = false;
	  else
	    continue;
          // We have a connection.  See if it is to a container adapter, which in turn would be
	  // connected to an interconnect.  No other adapters are expected yet.
          for (ad = ezxml_child(myXml()->parent, "adapter"); ad; ad = ezxml_next(ad)) {
            const char *adName = ezxml_attr(ad, "name");
            if (adName &&
                (iAmTo && !strcmp(adName, from) ||
                 !iAmTo && !strcmp(adName, to))) {
              // We have a connection on this port to an adapter instance.  Find the worker
	      const char *adwName = ezxml_attr(ad, "worker");
	      if (adwName)
		for (adw = ezxml_child(myXml()->parent, "worker"); adw; adw = ezxml_next(adw)) {
		  const char *nameAttr = ezxml_attr(adw, "name");
		  if (nameAttr && !strcmp(nameAttr, adwName))
		    break;
		}
	      if (!adw)
		throw OU::Error("For port \"%s\": adapter worker missing for connection", myName);
	      // Find the attached interconnect instance
	      const char *attach = ezxml_attr(ad, "attachment");
	      for (ic = ezxml_child(myXml()->parent, "interconnect"); ic; ic = ezxml_next(ic)) {
		const char *icName = ezxml_attr(ic, "name");
		if (icName && !strcmp(icName, attach))
		  break; // We have the IC connected through an adapter.
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
      return *new Port(*this, props, metaPort, conn, icw, ic, adw, ad, isProvider);
    }
    // Here because these depend on Port
    OC::Port &Worker::
    createOutputPort(OM::PortOrdinal portId,
                     OS::uint32_t bufferCount,
                     OS::uint32_t bufferSize,
                     const OA::PValue* props) throw() {
      (void)portId; (void)bufferCount; (void)bufferSize;(void)props;
      return *(Port *)0;//return *new Port(*this);
    }
    OC::Port &Worker::
    createInputPort(OM::PortOrdinal portId,
                    OS::uint32_t bufferCount,
                    OS::uint32_t bufferSize,
                    const OA::PValue* props) throw() {
      (void)portId; (void)bufferCount; (void)bufferSize;(void)props;
      return *(Port *)0;//      return *new Port(*this);
    }

#if 1 // SHARED_EXTERNAL
    class ExternalPort : public OC::ExternalPortBase<Port,ExternalPort> {
      friend class Port;
    protected:
      ExternalPort(Port &port, const char *name, bool isProvider,
		   const OA::PValue *extParams, const OA::PValue *connParams) :
        OC::ExternalPortBase<Port,ExternalPort>(port, *this, name, extParams, connParams, isProvider) {
      }
    public:
      virtual ~ExternalPort() {}
    };
#endif
#ifdef GENERIC_EXTERNAL
    class ExternalBuffer : OC::ExternalBuffer {
      friend class ExternalPort;
      OD::BufferUserFacet *m_dtBuffer;
      OD::Port *m_dtPort;
      ExternalBuffer() :
	m_dtBuffer(NULL), m_dtPort(NULL)
      {}
      void release() {
	if (m_dtBuffer) {
	  m_dtPort->releaseInputBuffer(m_dtBuffer);
	  m_dtBuffer = NULL;
	}
      }
      void put( uint32_t length, uint8_t opCode, bool /*endOfData*/) {
	m_dtPort->sendOutputBuffer(m_dtBuffer, length, opCode);
      }
    };
    // Producer or consumer
    class ExternalPort : public OC::ExternalPortBase<Port,ExternalPort> {
      OD::Port *m_dtPort;
      ExternalBuffer m_lastBuffer;
    public:
      ExternalPort(Port &port, const char *name, bool isProvider, const OA::PValue *params) :
        OC::ExternalPortBase<Port,ExternalPort>(port, name, params, port.metaPort(), isProvider),
	m_dtPort(NULL)
      {
	if (isProvider) {
	  m_dtPort = port.parent().m_container.getTransport().createInputPort(getData().data, params);
	  // Since our worker port is finalized at this point...
	  m_dtPort->finalize(parent().getData().data, getData().data);
	  m_lastBuffer.m_dtPort = m_dtPort;
	} else {
	  m_dtPort = port.parent().m_container.getTransport().
	    createOutputPort(getData().data, port.getData().data);
	  m_dtPort->finalize(port.getData().data, getData().data);
	}
      }
      ~ExternalPort() {
	delete m_dtPort;
      }
      OA::ExternalBuffer *
      getBuffer(uint8_t *&data, uint32_t &length, uint8_t &opCode, bool &end) {
	end = false;
	ocpiAssert(m_lastBuffer.m_dtBuffer == NULL);
	void *vdata;
	if ((m_lastBuffer.m_dtBuffer = m_dtPort->getNextFullInputBuffer(vdata, length, opCode))) {
	  data = (uint8_t*)vdata; // fix all the buffer data types to match the API: uint8_t*
	  return &m_lastBuffer;
	}
	return NULL;
      }
      OC::ExternalBuffer *
      getBuffer(uint8_t *&data, uint32_t &length) {
	ocpiAssert(m_lastBuffer.m_dtBuffer == NULL);
	void *vdata;
	if ((m_lastBuffer.m_dtBuffer = m_dtPort->getNextEmptyOutputBuffer(vdata, length))) {
	  data = (uint8_t*)vdata; // fix all the buffer data types to match the API: uint8_t*
	  return &m_lastBuffer;
	}
	return NULL;
      }
      void endOfData() {
	ocpiAssert(!"No EndOfData support for external ports");
      }
      bool tryFlush() {
	return false;
      }
    };    
#endif
#if OLD_EXTERNAL
    // Buffers directly used by the "user" (non-container/component) API
    class ExternalBuffer : OC::ExternalBuffer {
      friend class ExternalPort;
      ExternalPort *myExternalPort;
      OcdpMetadata *metadata;   // where is the metadata buffer
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
        metadata->opCode = opCode;
        metadata->length = dataLength;
        release();
      }
    };

    // Producer or consumer
    class ExternalPort : public OC::ExternalPortBase<Port,ExternalPort> {
      friend class ExternalBuffer;
      // What we know about a far buffer
      struct FarBuffer {
        // When we are active, we use these far data pointers
        volatile OcdpMetadata *metadata;
        volatile uint8_t *data;
        // We use this all the time.
        volatile uint32_t *ready;
        bool last;
      };
      //      uint32_t nBuffers, *ready, next;
      OcdpMetadata *metadata;
      uint32_t *flags;
      ExternalBuffer *localBuffers, *nextLocal, *nextRemote;
      FarBuffer *farBuffers, *nextFar;
      uint8_t *localData;
      friend class Port;

      ExternalPort(Port &port, const char *name, bool isProvider, const OA::PValue *props) :
        OC::ExternalPortBase<Port,ExternalPort>(port, name, props, port.metaPort(), isProvider)
      {
        // Default is active only (host is master, never slave)
        getData().data.options =
          (1 << OCPI::RDT::ActiveFlowControl) |
          (1 << OCPI::RDT::ActiveMessage) |
          (1 << OCPI::RDT::ActiveOnly);
        applyConnectParams(props);
        port.determineRoles(getData().data);
        unsigned nFar = parent().getData().data.desc.nBuffers;
        unsigned nLocal = myDesc.nBuffers;
        myDesc.dataBufferPitch = parent().getData().data.desc.dataBufferPitch;
        myDesc.metaDataPitch = parent().getData().data.desc.metaDataPitch;
        myDesc.fullFlagPitch = sizeof(uint32_t);
        myDesc.emptyFlagPitch = sizeof(uint32_t);
        myDesc.emptyFlagValue = 1;
        myDesc.fullFlagValue = 1;
        // Allocate my local memory, making everything on a nice boundary.
        // (assume empty flag pitch same as full flag pitch)
        unsigned nAlloc =
          OC::roundup(myDesc.dataBufferPitch * nLocal, LOCAL_DMA_ALIGN) +
          OC::roundup(myDesc.metaDataPitch * nLocal, LOCAL_DMA_ALIGN) +
          OC::roundup(sizeof(uint32_t) * nLocal, LOCAL_DMA_ALIGN) + // local flags
          // These might actually be remote
          OC::roundup(sizeof(uint32_t) * nLocal, LOCAL_DMA_ALIGN) + // remote flags
          // These might not be needed if we are ActiveFlowControl
          OC::roundup(sizeof(uint32_t) * nFar, LOCAL_DMA_ALIGN);
        // Now we allocate all the (local) endpoint memory
        uint64_t phys;
        // If we are ActiveOnly we need no DMAable memory at all, so get it from the heap.
        if (getData().data.role == OCPI::RDT::ActiveOnly) {
          localData = new uint8_t[nAlloc];
	  phys = 0;
	} else {
	  std::string error;
	  if (!(localData = (uint8_t *)Driver::getSingleton().map(nAlloc, phys, error)))
	    throw error;
	}
        snprintf(myDesc.oob.oep, sizeof(myDesc.oob.oep),
                 "ocpi-pci-pio:%s.%lld:%lld.3.10", "0", (unsigned long long)phys,
                 (unsigned long long)nAlloc);
#if 0
	myEndpoint = OCPI::RDT::GetEndpoint("ocpi-pci//bus-id");
	if (!myEndpoint)
	  OC::ApiError("No local (CPU) endpoint support for pci bus %s", NULL);
	localData = myEndpoint->alloc(nAlloc);
#endif
        memset((void *)localData, 0, nAlloc);
        myDesc.dataBufferBaseAddr  = 0;
	uint8_t *allocation = localData;
        allocation += OC::roundup(myDesc.dataBufferPitch * nLocal, LOCAL_BUFFER_ALIGN);
        metadata = (OcdpMetadata *)allocation;
        myDesc.metaDataBaseAddr = allocation - localData;
        allocation += OC::roundup(myDesc.metaDataPitch * nLocal, LOCAL_BUFFER_ALIGN);
        uint32_t *localFlags = (uint32_t*)allocation;
        allocation += OC::roundup(sizeof(uint32_t) * nLocal, LOCAL_BUFFER_ALIGN);
        uint32_t *remoteFlags = (uint32_t*)allocation;
        allocation += OC::roundup(sizeof(uint32_t) * nLocal, LOCAL_BUFFER_ALIGN);
        uint32_t *farFlags = (uint32_t*)allocation;
        switch (getData().data.role) {
        case OCPI::RDT::ActiveMessage:
          // my exposed addresses are the flags in my memory that indicate far buffer state
          myDesc.emptyFlagBaseAddr =
            myDesc.fullFlagBaseAddr = allocation - localData;
          // FALL THROUGH
        case OCPI::RDT::ActiveOnly:
          {
            FarBuffer *fb = nextFar = farBuffers = new FarBuffer[nFar];
            for (unsigned i = 0; i < nFar; i++, fb++) {
              fb->metadata = parent().userMetadataBaseAddr + i;
              fb->data = parent().userDataBaseAddr + i * parent().myDesc.dataBufferPitch;
              fb->ready = farFlags + i; // not used for ActiveOnly
              *fb->ready = parent().isProvider();
              fb->last = false;
            }
            (fb-1)->last = true;
          }
          break;
        case OCPI::RDT::ActiveFlowControl:
          // here the far side needs to know about the remote flags;
          myDesc.emptyFlagBaseAddr =
            myDesc.fullFlagBaseAddr = (uint8_t*)localFlags - localData;
        }

        // Initialize our structures that keep track of LOCAL buffer status
        ExternalBuffer *lb = nextLocal = nextRemote = localBuffers = new ExternalBuffer[nLocal];
        for (unsigned i = 0; i < nLocal; i++, lb++) {
          lb->myExternalPort = this;
          lb->metadata = metadata + i;
          lb->data = localData + i * myDesc.dataBufferPitch;
          lb->length = myDesc.dataBufferPitch;
          lb->last = false;
          lb->busy = false;
          lb->readyForLocal = localFlags + i;
          *lb->readyForLocal = parent().isProvider();
          lb->readyForRemote = remoteFlags + i; //&parent().m_ocdpRegisters->nRemoteDone;
          *lb->readyForRemote = !parent().isProvider();
        }
        (lb-1)->last = true;
      }
    public:
      ~ExternalPort() {
	delete [] localBuffers;
      }
void memcpy64(uint64_t *to, uint64_t *from, unsigned nbytes)
{
  while (nbytes > 128) {
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
    nbytes -= 128;
  }
  while (nbytes > 8) {
    *to++ = *from++;
    nbytes -= 8;
  }
  if (nbytes)
    memcpy(to, from, nbytes);
}
      // We know a move can be done.  Do it.
      // We are either ActiveOnly or ActiveMessage
      void moveData() {
        if (parent().isProvider()) {
          // Here to far
          memcpy64((uint64_t *)nextFar->metadata, (uint64_t *)nextRemote->metadata, sizeof(OcdpMetadata));
          memcpy64((uint64_t *)nextFar->data, (uint64_t *)nextRemote->data, nextRemote->metadata->length);
        } else {
          // Far to here
          memcpy64((uint64_t *)nextRemote->metadata, (uint64_t *)nextFar->metadata, sizeof(OcdpMetadata));
          memcpy64((uint64_t *)nextRemote->data, (uint64_t *)nextFar->data, nextRemote->metadata->length);
        }
        // Set the local indication of readiness of the far buffer to false.
        // Far side will make it true (or us when we are ActiveOnly).
        *nextFar->ready = false;
        //        __asm__ __volatile__  ("lock; addl $0,0(%%esp)": : :"memory");
        // FIXME:  memory barrier to be sure?
        // Tell the far side that a its buffer has been used (filled or emptied)
        //wmb();
        if (parent().m_properties.get32Register(foodFace, OcdpProperties) != 0xf00dface)
          abort();
        parent().m_properties.set32Register(nRemoteDone, OcdpProperties, 1);
        // Advance our far side status
        if (nextFar->last)
          nextFar = farBuffers;
        else
          nextFar++;
        // Advance remote state (we are moving data)
        // The local buffer state has become ready for local access
        *nextRemote->readyForLocal = true;
        *nextRemote->readyForRemote = false;
        if (nextRemote->last)
          nextRemote = localBuffers;
        else
          nextRemote++;
      }
      // Try to move some data, return if there is data that can't be moved
      void tryMove() {
        // Try to advance my remote side
        switch (getData().data.role) {
        case OCPI::RDT::ActiveOnly:
          // Use far side "ready" register to determine whether far buffers are ready
          // Thus we need to do a remote PCIe read to know far size status
          if (*nextRemote->readyForRemote) { // avoid remote read if local is not ready
            for (uint32_t nReady = parent().m_properties.get32RegisterOffset(offsetof(OcdpProperties, nReady));
                 nReady && *nextRemote->readyForRemote; nReady--)
	      moveData();
          }
          break;
        case OCPI::RDT::ActiveMessage:
          // Use local version of far-is-ready flag for the far buffer,
          // which will be written by the ActiveFlowControl far side.
          while (*nextRemote->readyForRemote && *nextFar->ready)
            moveData();
          break;
        case OCPI::RDT::ActiveFlowControl:
          // Nothing to do here.  We don't move data.
          // When the other side moves data it will set our far-is-ready flag
          break;
        case OCPI::RDT::Passive:
        case OCPI::RDT::NoRole:
          ocpiAssert(0);
        }
      }
      bool getLocal() {
        tryMove();
        if (!*nextLocal->readyForLocal)
          return false;
        ocpiAssert(getData().data.role == OCPI::RDT::ActiveFlowControl ||
                  getData().data.role == OCPI::RDT::Passive ||
                  !*nextLocal->readyForRemote);
        ocpiAssert(!nextLocal->busy);
        nextLocal->busy = true; // to ensure callers use the API correctly
        *nextLocal->readyForLocal = 0;
        return true;
      }
      // The input method = get a buffer that has data in it.
      OA::ExternalBuffer *
      getBuffer(uint8_t *&bdata, uint32_t &length, uint8_t &opCode, bool &end) {
        ocpiAssert(!parent().isProvider());
        if (!getLocal())
          return 0;
        bdata = nextLocal->data;
        length = nextLocal->metadata->length;
        opCode = nextLocal->metadata->opCode;
        end = false; // someday bit in metadata
        //FIXME cast unnecessary
        return static_cast<OC::ExternalBuffer *>(nextLocal);
      }
      OC::ExternalBuffer *getBuffer(uint8_t *&bdata, uint32_t &length) {
        ocpiAssert(parent().isProvider());
        if (!getLocal())
          return 0;
        bdata = nextLocal->data;
        length = nextLocal->length;
        return static_cast<OC::ExternalBuffer *>(nextLocal);
      }
      void endOfData() {
        ocpiAssert(parent().isProvider());
      }
      bool tryFlush() {
        ocpiAssert(parent().isProvider());
        tryMove();
        switch (getData().data.role) {
        case OCPI::RDT::ActiveOnly:
        case OCPI::RDT::ActiveMessage:
	  return *nextRemote->readyForRemote != 0;
        case OCPI::RDT::ActiveFlowControl:
	  {
	    ExternalBuffer *local = nextLocal; 
	    do {
	      if (local == localBuffers)
		local = localBuffers + myDesc.nBuffers - 1;
	      else
		local--;
	      if (!*local->readyForLocal)
		return true;
	    } while (local != nextLocal);
	  }
	  break;
        case OCPI::RDT::Passive:
        case OCPI::RDT::NoRole:
          ocpiAssert(0);
	}
	return false;
      }
      void advanceLocal() {
        if (getData().data.role == OCPI::RDT::ActiveFlowControl) {
          //          if (parent().m_ocdpRegisters->foodFace != 0xf00dface)
          //            abort();
          //          wmb();
          parent().m_properties.set32Register(nRemoteDone, OcdpProperties, 1);
          //usleep(0);
        }
        if (nextLocal->last)
          nextLocal = localBuffers;
        else
          nextLocal++;
        tryMove();
      }
    };

    // FIXME make readyForRemote zero when active flow control
    void ExternalBuffer::release() {
      ocpiAssert(myExternalPort->getData().data.role == OCPI::RDT::ActiveFlowControl ||
                myExternalPort->getData().data.role == OCPI::RDT::Passive ||
                !*readyForRemote);
      // The buffer is not ready for local processing
      //      *readyForLocal = false;
      //      clflush(readyForLocal);
      // The remote process can use this local buffer now
      *readyForRemote = true;
      // The local buffer is not being used locally
      busy = false;
      // Tell the other side that the buffer has become available
      myExternalPort->advanceLocal();
    }
#endif
#if 0
    OC::ExternalPort &Port::connectExternal(const char *extName, const OA::PValue *userProps,
					    const OA::PValue *props) {
      if (!m_canBeExternal)
        throw OC::ApiError ("For external port \"", extName, "\", port \"",
			    name().c_str(), "\" of worker \"",
			    parent().implTag().c_str(), "/", parent().instTag().c_str(), "/",
			    parent().name().c_str(),
			    "\" is locally connected in the HDL bitstream. ", NULL);
      applyConnectParams(props);
      // UserPort constructor must know the roles.
      ExternalPort *myExternalPort = new ExternalPort(*this, extName, !isProvider(), userProps);
      finishConnect(myExternalPort->getData().data);
      return *myExternalPort;
    }
#else
    OC::ExternalPort &Port::createExternal(const char *extName, bool isProvider,
					       const OU::PValue *extParams, const OU::PValue *connParams) {
      return *new ExternalPort(*this, extName, isProvider, extParams, connParams);
    }
#endif
  }
}


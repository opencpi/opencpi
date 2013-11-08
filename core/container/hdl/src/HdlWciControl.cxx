#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "OcpiUtilException.h"
#include "OcpiUtilImplementation.h"
#include "OcpiContainerMisc.h"
#include "HdlWciControl.h"
#include "HdlDevice.h"

namespace OCPI {
  namespace HDL {
    namespace OA = OCPI::API;
    namespace OC = OCPI::Container;
    namespace OS = OCPI::OS;
    namespace OM = OCPI::Metadata;
    namespace OU = OCPI::Util;
    namespace OE = OCPI::Util::EzXml;

    static const unsigned DEFAULT_TIMEOUT = 16;

    WciControl::
    WciControl(Device &device, const char *impl, const char *inst, unsigned index, bool hasControl)
      : m_implName(impl), m_instName(inst), m_hasControl(hasControl), m_timeout(DEFAULT_TIMEOUT),
	m_device(device), m_occpIndex(index)
    {
      init(false);
    }

    WciControl::
    WciControl(Device &device, ezxml_t implXml, ezxml_t instXml)
      : m_implName("<none>"), m_instName("<none>"), m_hasControl(false), m_timeout(DEFAULT_TIMEOUT),
	m_device(device), m_occpIndex(0)
    {
      if (implXml) {
	const char *name, *err;
	setControlOperations(ezxml_cattr(implXml, "controlOperations"));
	if ((name = ezxml_attr(implXml, "name")))
	  m_implName = name;
	if (instXml) {
	  if ((name = ezxml_attr(instXml, "name")))
	    m_instName = name;
	  if ((err = OE::getNumber(instXml, "occpIndex", &m_occpIndex, &m_hasControl, 0, false)))
	    throw OU::Error("XML error for worker %s:%s: %s", m_implName, m_instName, err);
	}
	if ((err = OE::getNumber(instXml, "timeout", &m_timeout, NULL, 0, false)))
	  throw OU::Error("XML error for worker %s:%s: %s", m_implName, m_instName, err);
      }
      init(false);
    }

    // Common to both constructors: assumes has_control, occpIndex and timeout are set up
    // ALSO is used after loading a new bitstream
    void WciControl::
    init(bool redo) {
      m_window = 0;
      if (m_hasControl) {
	setControlMask(getControlMask() | 1 << OM::Worker::OpStart);
	unsigned logTimeout = 31;
	for (size_t u = 1 << logTimeout; !(u & m_timeout);
	     u >>= 1, logTimeout--)
	  ;
	ocpiDebug("Timeout for %s is %zu log is %u\n", m_implName, m_timeout, logTimeout);
	// Get access to registers and properties.  The "this" is for our Access
	if (!redo)
	  m_device.getWorkerAccess(m_occpIndex, *this, m_properties);
	// Assert Reset
	// myRegisters->control =  logTimeout;
	logTimeout = 0xf; // per shep
	set32Register(control, OccpWorkerRegisters, logTimeout);
	//#ifndef SHEP_FIXME_THE_RESET
	struct timespec spec;
	spec.tv_sec = 0;
	spec.tv_nsec = 10000;
	int bad = nanosleep(&spec, 0);
	ocpiCheck(bad == 0);
	//#endif
	// Take out of reset
	// myRegisters->control = OCCP_CONTROL_ENABLE | logTimeout ;
	set32Register(control,  OccpWorkerRegisters, OCCP_WORKER_CONTROL_ENABLE | logTimeout);
	ocpiInfo("Deasserted reset on worker %s.%s", m_implName, m_instName);
#if 0 // do this by changing the accessor
	if (getenv("OCPI_OCFRP_DUMMY")) {
	  *(uint32_t *)&myRegisters->initialize = OCCP_SUCCESS_RESULT; //fakeout
	  *(uint32_t *)&myRegisters->start = OCCP_SUCCESS_RESULT; //fakeout
	}
#endif
      } else {
	setControlMask(0);
	ocpiDebug("HDL container found impl %s inst %s with no control", m_implName, m_instName);
      }
    }

    WciControl::
    ~WciControl() {
      m_device.releaseWorkerAccess(m_occpIndex, *this, m_properties);
    }

    // Add the hardware considerations to the property object that supports
    // fast memory-mapped property access directly to users
    // the key members are "readVaddr" and "writeVaddr"
    void WciControl::
    prepareProperty(OU::Property &md, 
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
    void WciControl::
    controlOperation(OCPI::Metadata::Worker::ControlOperation op) {
      std::string err;
      if (controlOperation(op, err))
	throw OU::Error(err);
    }

    bool WciControl::
    controlOperation(OCPI::Metadata::Worker::ControlOperation op, std::string &err) {
      if (getControlMask() & (1 << op)) {
	uint32_t result =
	  // *((volatile uint32_t *)myRegisters + controlOffsets[op]);
	  get32RegisterOffset(controlOffsets[op]);
	if (result != OCCP_SUCCESS_RESULT) {
	  //	    && result != 0 && result != 0x01020304) { // TEMP HACK UNTIL SHEP FIXES WCI MASTER
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
	  ocpiInfo("HDL Control Op Failed: worker %s:%s(%zu) op %s(%u) %s (%0x" PRIx32 ")",
		     m_implName, m_instName, m_occpIndex, OU::controlOpNames[op], op, oops, result);
	  OU::format(err, "HDL Control Op Failed: worker %s:%s(%zu) op %s(%u) %s (%0x" PRIx32 ")",
		     m_implName, m_instName, m_occpIndex, OU::controlOpNames[op], op, oops, result);
	  return true;
	}
	ocpiInfo("HDL Control Op Succeeded: worker %s:%s(%zu) op %s(%u)",
		  m_implName, m_instName, m_occpIndex, OU::controlOpNames[op], op);
      } else
	ocpiInfo("HDL Control Op Avoided worker %s:%s(%zu) op %s(%u)",
		  m_implName, m_instName, m_occpIndex, OU::controlOpNames[op], op);
      return false;
    }

#define PUT_GET_PROPERTY(n)						     \
    void WciControl::                                                        \
    setProperty##n(const OA::PropertyInfo &info, uint##n##_t val) const {    \
        uint32_t offset = checkWindow(info.m_offset, n/8);		     \
	uint32_t status = 0;						     \
	if (m_properties.m_registers) {					     \
	  if (!info.m_writeError ||					     \
	      !(status =						     \
		get32Register(status, OccpWorkerRegisters) &                 \
		OCCP_STATUS_WRITE_ERRORS))                		     \
	    m_properties.set##n##RegisterOffset(offset, val);                \
	  if (info.m_writeError && !status)				     \
	    status =							     \
	      get32Register(status, OccpWorkerRegisters) &		     \
	      OCCP_STATUS_WRITE_ERRORS;					     \
	} else								     \
	  m_properties.m_accessor->set##n(m_properties.m_base + offset, val, \
					  &status);			     \
	if (status)							     \
	  throwPropertyWriteError(status);				     \
      }
      PUT_GET_PROPERTY(8)
      PUT_GET_PROPERTY(16)
      PUT_GET_PROPERTY(32)
      PUT_GET_PROPERTY(64)
#undef PUT_GET_PROPERTY
    void WciControl::
    setPropertyBytes(const OA::PropertyInfo &info, size_t offset,
		     const uint8_t *data, size_t nBytes) const {
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

    void WciControl::
    getPropertyBytes(const OA::PropertyInfo &info, size_t offset, uint8_t *buf,
		     size_t nBytes) const {
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
    void WciControl::
    setPropertySequence(const OA::Property &p,
			const uint8_t *val,
			size_t nItems, size_t nBytes) const {
      uint32_t offset = checkWindow(p.m_info.m_offset, nBytes);
      uint32_t status = 0;
      if (m_properties.m_registers) {
	if (!p.m_info.m_writeError ||
	    !(status =
	      get32Register(status, OccpWorkerRegisters) &
	      OCCP_STATUS_WRITE_ERRORS))
	  if (p.m_info.m_isSequence) {				\
	    m_properties.setBytesRegisterOffset(offset + p.m_info.m_align,
						val, nBytes);
	    m_properties.set32RegisterOffset(offset, (uint32_t)nItems);
	  } else
	    m_properties.setBytesRegisterOffset(offset, val, nBytes);
	if (p.m_info.m_writeError && !status)
	  status =
	    get32Register(status, OccpWorkerRegisters) &
	    OCCP_STATUS_WRITE_ERRORS;
      } else if (p.m_info.m_isSequence) {					\
	m_properties.m_accessor->setBytes(m_properties.m_base + offset + p.m_info.m_align,
					  val, nBytes, &status);
	if (!status)
	  m_properties.m_accessor->set32(m_properties.m_base + offset, (uint32_t)nItems, &status);
      } else
	m_properties.m_accessor->setBytes(m_properties.m_base + offset, val, nBytes, &status);
      if (status)
	throwPropertyWriteError(status);
    }
    unsigned WciControl::
    getPropertySequence(const OA::Property &p, uint8_t *buf, size_t n) const {
      uint32_t offset = checkWindow(p.m_info.m_offset, n);
      uint32_t status = 0, nItems;

      if (m_properties.m_registers) {
	if (!p.m_info.m_readError ||
	    !(status =
	      get32Register(status, OccpWorkerRegisters) &
	      OCCP_STATUS_READ_ERRORS))
	  nItems = m_properties.get32RegisterOffset(offset);
	else
	  nItems = 0; // warning
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

    void WciControl::
    setStringProperty(const OA::Property &p, const char* val) const {
      size_t n = strlen(val) + 1;
      setPropertySequence(p, (const uint8_t *)val, n, n);
    }
    void WciControl::
    setStringSequenceProperty(const OA::Property &, const char * const *,
			      size_t ) const {
      throw OU::Error("No support for properties that are sequences of strings");
    }
    void WciControl::
    getStringProperty(const OA::Property &p, char *val, size_t length) const {
      // ignore return value
      getPropertySequence(p, (uint8_t*)val, length);
    }
    unsigned WciControl::
    getStringSequenceProperty(const OA::Property &, char * *,
			      size_t ,char*, size_t) const {
      throw OU::Error("No support for properties that are sequences of strings");
      return 0;
    }

    void WciControl::
    throwPropertyReadError(uint32_t status) {
      throw OU::Error("property reading error: %s",
		      status & OCCP_STATUS_READ_TIMEOUT ?
		      "timeout" : "worker generated error response");
    }
    void WciControl::
    throwPropertyWriteError(uint32_t status) {
      throw OU::Error("property writing error: %s",
		      status & OCCP_STATUS_WRITE_TIMEOUT ?
		      "timeout" : "worker generated error response");
    }
    void WciControl::
    throwPropertySequenceError() {
      throw OU::Error("property sequence length overflow on read/get");
    }
    // c++ doesn't allow static initializations in class definitions.
    const unsigned WciControl::
    controlOffsets[] = {
#define CONTROL_OP(x, c, t, s1, s2, s3) \
	offsetof(OccpWorkerRegisters,x),
	OCPI_CONTROL_OPS
#undef CONTROL_OP
	0};
  }
}

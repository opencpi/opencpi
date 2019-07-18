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

#include <inttypes.h>
#include "HdlDevice.h"
#include "HdlDriver.h"
#include "HdlWciControl.h"

namespace OCPI {
  namespace HDL {
    namespace OA = OCPI::API;
    namespace OC = OCPI::Container;
    namespace OS = OCPI::OS;
    namespace OU = OCPI::Util;
    namespace OE = OCPI::Util::EzXml;

    static const unsigned DEFAULT_TIMEOUT = 16;

    WciControl::
    WciControl(Device &device, const char *impl, const char *inst, unsigned a_index,
	       bool hasControl)
      : m_implName(impl), m_instName(inst), m_hasControl(hasControl), m_timeout(DEFAULT_TIMEOUT),
	m_device(device), m_occpIndex(a_index)
    {
      init(false, false);
    }

    WciControl::
    WciControl(Device &device, ezxml_t implXml, ezxml_t instXml, OU::Property *props, bool doInit)
      : m_implName("<none>"), m_instName("<none>"), m_hasControl(false), m_timeout(DEFAULT_TIMEOUT),
	m_device(device), m_occpIndex(0), m_propInfo(props)
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
	if ((err = OE::getNumber(implXml, "timeout", &m_timeout, NULL, 0, false)))
	  throw OU::Error("XML error for worker %s:%s: %s", m_implName, m_instName, err);
      }
      init(false, doInit);
    }

    // Common to both constructors: assumes has_control, occpIndex and timeout are set up
    // ALSO is used after loading a new bitstream
    void WciControl::
    init(bool redo, bool doInit) {
      m_window = 0;
      //      m_wName = m_instName ? m_instName : "<unknown>";
      if (m_hasControl) {
	setControlMask(getControlMask() | 1 << OU::Worker::OpStart);
	// Find the log base 2 of the m_timeout value since that is what is specified in
	// the hardware control register.
	// FIXME: maybe use logTimeout = OU::ceilLog2(m_timeout).
	unsigned logTimeout = 31;
	for (size_t u = 1 << logTimeout; !(u & m_timeout); u >>= 1, logTimeout--)
	  ;
	ocpiDebug("Timeout for %s is %zu log is %u\n", m_implName, m_timeout, logTimeout);
	// Get access to registers and properties.  The "this" is for our Access
	if (!redo)
	  m_device.getWorkerAccess(m_occpIndex, *this, m_properties);
	if (doInit) {
	  // Assert Reset
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
	}
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

    bool WciControl::
    isReset() const {
      return (get32Register(control, OccpWorkerRegisters) & OCCP_WORKER_CONTROL_ENABLE) == 0;
    }

    // Add the hardware considerations to the property object that supports
    // fast memory-mapped property access directly to users
    // the key members are "readVaddr" and "writeVaddr"
    void WciControl::
    prepareProperty(OU::Property &md, 
		    volatile uint8_t *&writeVaddr,
		    const volatile uint8_t *&readVaddr) const {
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
    checkControlState() const {
      if (!m_hasControl)
	return;
      if (!m_device.isAlive() || m_device.isFailed()) {
	setControlState(OU::Worker::UNUSABLE);
	return;
      }
      // This polling call is to detect the current control state when
      // it might be changed by the worker or container autonomously,
      // in a way that does not update the state here.
      // FIXME: when the new control plane is fixed to provide
      // "finished" and "fatal" status, there is where we check it, and do
      // setControlState(OC::FINISHED);
      // setControlState(OC::UNUSABLE);
      uint32_t status = get32Register(status, OccpWorkerRegisters);
      ocpiDebug("Checking control state for worker: status: 0x%x", status);
      if (status & OCCP_STATUS_ALL_ERRORS)
	ocpiDebug("Worker %s has errors: %" PRIx32, m_instName, status);
      if ((status & OCCP_STATUS_FINISHED) &&
	  (getState() == OU::Worker::OPERATING || getState() == OU::Worker::SUSPENDED))
	setControlState(OU::Worker::FINISHED);
    }
    void WciControl::
    controlOperation(OU::Worker::ControlOperation op) {
      std::string err;
      if (controlOperation(op, err))
	throw OU::Error(err);
    }

    bool WciControl::
    controlOperation(OU::Worker::ControlOperation op, std::string &err) {
      if (getControlMask() & (1 << op)) {
	uint32_t result =
	  // *((volatile uint32_t *)myRegisters + controlOffsets[op]);
	  get32RegisterOffset(controlOffsets[op]);
	if (result != OCCP_SUCCESS_RESULT) {
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
	  case OCCP_BUSY_RESULT:
	    oops = "indicated the worker was (still) busy from a previous operation";
	    break;
	  default:
	    oops = "returned unknown result value from control operation";
	  }
	  ocpiInfo("HDL Control Op Failed: worker %s:%s(%zu) op %s(%u) %s (%0x" PRIx32 ")",
		   m_implName, m_instName, m_occpIndex, OU::Worker::s_controlOpNames[op],
		   op, oops, result);
	  OU::format(err, "HDL Control Op Failed: worker %s:%s(%zu) op %s(%u) %s (0x%" PRIx32 ")",
		     m_implName, m_instName, m_occpIndex, OU::Worker::s_controlOpNames[op],
		     op, oops, result);
	  return true;
	}
	ocpiInfo("HDL Control Op Succeeded: worker %s:%s(%zu) op %s(%u)",
		 m_implName, m_instName, m_occpIndex, OU::Worker::s_controlOpNames[op], op);
      } else
	ocpiInfo("HDL Control Op Avoided worker %s:%s(%zu) op %s(%u)",
		 m_implName, m_instName, m_occpIndex, OU::Worker::s_controlOpNames[op], op);
      return false;
    }

    void WciControl::propertyWritten(unsigned /*ordinal*/) const {};
    void WciControl::propertyRead(unsigned /*ordinal*/) const {};

#define PUT_GET_PROPERTY(n)						     \
    void WciControl::                                                        \
    setProperty##n(const OA::PropertyInfo &info, size_t off, uint##n##_t val, unsigned idx) const { \
      uint32_t offset = checkWindow(info.m_offset + off + idx * (n/8), n/8);	\
	uint32_t status = 0;						     \
	if (m_properties.registers()) {					     \
	  if (!info.m_writeError ||					     \
	      !(status =						     \
		get32Register(status, OccpWorkerRegisters) &                 \
		OCCP_STATUS_WRITE_ERRORS))                		     \
	    m_properties.set##n##RegisterOffset(offset, val);                \
	  if (info.m_writeError && !status)				     \
	    status =							     \
	      get32Register(status, OccpWorkerRegisters) &		     \
	      OCCP_STATUS_WRITE_ERRORS;					     \
	} else if (n == 64)						\
	  m_properties.accessor()->set64(m_properties.base() + offset, val, &status); \
	else								\
	  m_properties.accessor()->set(m_properties.base() + offset, sizeof(uint##n##_t), val, \
                                       &status);			     \
	if (status)							     \
	  throwPropertyWriteError(status);				     \
      }
    //                                       (uint32_t)(val << ((offset &3) * 8)), &status);
      PUT_GET_PROPERTY(8)
      PUT_GET_PROPERTY(16)
      PUT_GET_PROPERTY(32)
      PUT_GET_PROPERTY(64)
#undef PUT_GET_PROPERTY
    void WciControl::
    setPropertyBytes(const OA::PropertyInfo &info, size_t offset,
		     const uint8_t *data, size_t nBytes, unsigned idx) const {
      offset = checkWindow(info.m_offset + offset + idx * info.m_elementBytes, nBytes);
      uint32_t status = 0;
      if (m_properties.registers()) {
	if (!info.m_writeError ||
	    !(status = (get32Register(status, OccpWorkerRegisters) &
			OCCP_STATUS_WRITE_ERRORS)))
	  m_properties.setBytesRegisterOffset(offset, data, nBytes, info.m_elementBytes);
	if (info.m_writeError && !status)
	  status = (get32Register(status, OccpWorkerRegisters) &
		    OCCP_STATUS_WRITE_ERRORS);
      } else
	m_properties.accessor()->setBytes(m_properties.base() + offset, data, nBytes,
					  info.m_elementBytes, &status);
      if (status)
	throwPropertyWriteError(status);
    }

    void WciControl::
    getPropertyBytes(const OA::PropertyInfo &info, size_t offset, uint8_t *buf,
		     size_t nBytes, unsigned idx, bool string) const {
      offset = checkWindow(info.m_offset + offset + idx * info.m_elementBytes, nBytes);
      uint32_t status = 0;

      if (m_properties.registers()) {
	if (!info.m_readError ||
	    !(status = (get32Register(status, OccpWorkerRegisters) &
			OCCP_STATUS_READ_ERRORS))) {
	  m_properties.getBytesRegisterOffset(offset, buf, nBytes, info.m_elementBytes, string);
	  if (info.m_readError)
	    status = get32Register(status, OccpWorkerRegisters) & OCCP_STATUS_READ_ERRORS;
	}
      } else
	m_properties.accessor()->getBytes(m_properties.base() + offset, buf, nBytes,
					  info.m_elementBytes, &status, string);
      if (status)
	throwPropertyReadError(status, (uint32_t)offset, nBytes, 0);
    }
    void WciControl::
    setPropertySequence(const OA::PropertyInfo &p,
			const uint8_t *val,
			size_t nItems, size_t nBytes) const {
      uint32_t offset = checkWindow(p.m_offset, nBytes);
      uint32_t status = 0;
      if (m_properties.registers()) {
	if (!p.m_writeError ||
	    !(status = get32Register(status, OccpWorkerRegisters) & OCCP_STATUS_WRITE_ERRORS)) {
	  if (p.m_isSequence) {
	    m_properties.setBytesRegisterOffset(offset + p.m_align,
						val, nBytes, p.m_elementBytes);
	    m_properties.set32RegisterOffset(offset, (uint32_t)nItems);
	  } else
	    m_properties.setBytesRegisterOffset(offset, val, nBytes, p.m_elementBytes);
        }
	if (p.m_writeError && !status)
	  status = get32Register(status, OccpWorkerRegisters) & OCCP_STATUS_WRITE_ERRORS;
      } else if (p.m_isSequence) {
	m_properties.accessor()->setBytes(m_properties.base() + offset + p.m_align,
					  val, nBytes, p.m_elementBytes, &status);
	if (!status)
	  m_properties.accessor()->set(m_properties.base() + offset, sizeof(uint32_t),
				       (uint32_t)nItems, &status);
      } else
	m_properties.accessor()->setBytes(m_properties.base() + offset, val, nBytes,
					  p.m_elementBytes, &status);
      if (status)
	throwPropertyWriteError(status);
    }
    unsigned WciControl::
    getPropertySequence(const OA::PropertyInfo &p, uint8_t *buf, size_t n) const {
      uint32_t offset = checkWindow(p.m_offset, n);
      uint32_t status = 0, nItems;
      size_t nBytes;

      if (m_properties.registers()) {
	if (!p.m_readError ||
	    !(status =
	      get32Register(status, OccpWorkerRegisters) &
	      OCCP_STATUS_READ_ERRORS))
	  nItems = m_properties.get32RegisterOffset(offset);
	else
	  nItems = 0; // warning
	nBytes = nItems * p.m_elementBytes;
	if (!p.m_readError && 
	    !(status =
	      get32Register(status, OccpWorkerRegisters) &
	      OCCP_STATUS_READ_ERRORS)) {
	  if (nBytes > n)
	    throwPropertySequenceError();
	  m_properties.getBytesRegisterOffset(offset + p.m_align, buf, nBytes, p.m_elementBytes);
	  if (p.m_readError && !status)
	    status =
	      get32Register(status, OccpWorkerRegisters) &
	      OCCP_STATUS_READ_ERRORS;
	}
      } else {
	nItems = m_properties.accessor()->get(m_properties.base() + offset, sizeof(uint32_t),
					      &status);
	nBytes = nItems * p.m_elementBytes;
	if (!status) {
	  if (nBytes > n)
	    throwPropertySequenceError();
	  m_properties.accessor()->getBytes(m_properties.base() + offset + p.m_align,
					    buf, nBytes, p.m_elementBytes, &status);
	}
      }
      if (status)
	throwPropertyReadError(status, offset, nBytes, 0);
      return nItems;
    }

    void WciControl::
    setStringProperty(const OCPI::API::PropertyInfo &info, const Util::Member &, size_t offset,
		      const char* val, unsigned idx) const {
      size_t n = strlen(val) + 1;
      setPropertyBytes(info, info.m_offset + offset, (const uint8_t*)val, n, idx);
    }
    void WciControl::
    setStringSequenceProperty(const OA::PropertyInfo &, const char * const *,
			      size_t ) const {
      throw OU::Error("No support for properties that are sequences of strings");
    }
    void WciControl::
    getStringProperty(const OCPI::API::PropertyInfo &info, const Util::Member &, size_t offset,
		      char *val, size_t length, unsigned idx) const {
      getPropertyBytes(info, info.m_offset + offset, (uint8_t*)val, length, idx, true);
    }
    unsigned WciControl::
    getStringSequenceProperty(const OA::PropertyInfo &, char * *,
			      size_t ,char*, size_t) const {
      throw OU::Error("No support for properties that are sequences of strings");
      return 0;
    }

    void WciControl::
    throwPropertyReadError(uint32_t status, uint32_t offset, size_t n, uint64_t val) const {
      throw OU::Error("property read error on worker %s instance %s status %x offset %x bytes %zd val %" PRIx64 ": %s",
		      m_implName, m_instName, status, offset, n, val,
		      status & OCCP_STATUS_READ_TIMEOUT ? "timeout" :
		      (status & OCCP_STATUS_READ_FAIL ?
		       "busy" : "worker generated error response"));
    }
    void WciControl::
    throwPropertyWriteError(uint32_t status) const {
      throw OU::Error("property writing error on worker %s instance %s status %x: %s",
		      m_implName, m_instName, status,
		      status & OCCP_STATUS_WRITE_TIMEOUT ? "timeout" :
		      (status & OCCP_STATUS_WRITE_FAIL ?
		       "busy" : "worker generated error response"));
    }
    void WciControl::
    throwPropertySequenceError() const {
      throw OU::Error("property sequence length overflow on worker %s instance %s",
		      m_implName, m_instName);
    }
    // c++ doesn't allow static initializations in class definitions.
    const unsigned WciControl::
    controlOffsets[] = {
#define CONTROL_OP(x, c, t, s1, s2, s3, s4)	\
	offsetof(OccpWorkerRegisters,x),
	OCPI_CONTROL_OPS
#undef CONTROL_OP
	0};
    DirectWorker::DirectWorker(Device &dev, const Access &cAccess, Access &wAccess, ezxml_t impl,
			       ezxml_t inst, const char *idx, unsigned timeout) 
      : OC::Worker(NULL, impl, inst, OC::NoWorkers, false, 0, 1, NULL),
      WciControl(dev, impl, inst, properties(), false),
      m_name(ezxml_cattr(inst, "name")),
      m_wName(ezxml_cattr(impl, "name")),
      m_wAccess(wAccess), m_timeout(timeout)
    {
      // We need to initialize the status of the worker since the OC::Worker class
      // object is being created without knowledge of previous state.
      // The worker's status register tells us the last control operation
      // that was performed.  It also has a sticky indication of
      // errors from the worker itself, but it doesn't remember whether the
      // previous control operation failed for other reasons (FIXME: the OCCP should
      // capture this information).  We do our best here by first bypassing the software.

      // check the presumed-correct XML just to avoid bad accesses to the control plane.
      if ((m_occpIndex = strtoul(idx, NULL, 10)) > OCCP_MAX_WORKERS)
	throw OU::Error("Internal artifact XML problem: worker number `%s' invalid", idx);
      cAccess.offsetRegisters(m_wAccess, (intptr_t)(&((OccpSpace*)0)->worker[m_occpIndex]));
      uint32_t
	l_control = m_wAccess.get32Register(control, OccpWorkerRegisters),
	l_status =  m_wAccess.get32Register(status, OccpWorkerRegisters);
      OU::Worker::ControlState cs;
      OU::Worker::ControlOperation lastOp =
	(OU::Worker::ControlOperation)OCCP_STATUS_LAST_OP(l_status);
      if (!(l_control & OCCP_WORKER_CONTROL_ENABLE))
	cs = OU::Worker::EXISTS; // there is no specific reset state since it isn't hetero
      else if (!(l_status & OCCP_STATUS_CONFIG_OP_VALID) || lastOp == 4)
	cs = OU::Worker::EXISTS; // no control op since reset
      else if (l_status & OCCP_STATUS_CONTROL_ERRORS)
	cs = OU::Worker::UNUSABLE;
      else if (lastOp == OU::Worker::OpRelease)
	cs = OU::Worker::UNUSABLE;
      else if (l_status & OCCP_STATUS_FINISHED)
	cs = OU::Worker::FINISHED;
      else
	switch(lastOp) {
	case OU::Worker::OpInitialize: cs = OU::Worker::INITIALIZED; break;
	case OU::Worker::OpStart: cs = OU::Worker::OPERATING; break;
	case OU::Worker::OpStop: cs = OU::Worker::SUSPENDED; break;
	default:
	  cs = OU::Worker::OPERATING;
	  // FIXME:  the beforeQuery, and AfterConfig and test ops screw us up here.
	}
      setControlState(cs);
    }
    void DirectWorker::
    control(const char *op) {
      bool ignored = false;
      uint32_t c =  m_wAccess.get32Register(control, OccpWorkerRegisters);
      if (!strcasecmp(op, "reset")) {
	if (OCCP_WORKER_CONTROL_ENABLE & c) {
	  // Force the last control op to be bad
	  c &= ~OCCP_WORKER_CONTROL_ENABLE;
	  m_wAccess.set32Register(control, OccpWorkerRegisters,
				  c | OCCP_CONTROL_CLEAR_ATTENTION | OCCP_CONTROL_CLEAR_ERRORS);
	} else
	  ignored = true;
      } else if (!strcasecmp(op, "unreset")) {
	if (OCCP_WORKER_CONTROL_ENABLE & c && !m_timeout)
	  ignored = true;
	else {
	  if (m_timeout) {
	    c &= ~0x1f;
	    unsigned logTimeout = 31;
	    for (size_t u = 1 << logTimeout; !(u & m_timeout);
		 u >>= 1, logTimeout--)
	      ;
	    c |= logTimeout;
	  }
	  m_wAccess.set32Register(control, OccpWorkerRegisters, c |= OCCP_WORKER_CONTROL_ENABLE);
	  m_wAccess.get32Register(test, OccpWorkerRegisters);
	  m_wAccess.set32Register(control, OccpWorkerRegisters,
				  c | OCCP_CONTROL_CLEAR_ATTENTION | OCCP_CONTROL_CLEAR_ERRORS);
	}
      } else {
	unsigned i;
	for (i = 0; OU::Worker::s_controlOpNames[i]; i++) 
	  if (!strcasecmp(OU::Worker::s_controlOpNames[i], op)) {
	    ignored = controlOp((OU::Worker::ControlOperation)i);
	    break;
	  }
	if (!OU::Worker::s_controlOpNames[i])
	  throw OU::Error("Unknown control operation: %s", op);
      }
      if (ignored)
	printf("The '%s' operation was IGNORED on instance '%s' of worker '%s'.\n",
	       op, name().c_str(), implTag().c_str());
      else
	printf("The '%s' operation was performed on instance '%s' of worker '%s'.\n",
	       op, name().c_str(), implTag().c_str());
    }
    void DirectWorker::
    status() {
      printf("Status of instance '%s' of worker '%s' is '%s'\n",
	     name().c_str(), implTag().c_str(),
	     m_wAccess.get32Register(control,
				     OccpWorkerRegisters) & OCCP_WORKER_CONTROL_ENABLE ?
	                             OU::Worker::s_controlStateNames[getState()] : "RESET");
    }
    OC::Port *DirectWorker::findPort(const char *) { return NULL; }
    const std::string &DirectWorker::name() const { return m_name; }
    void DirectWorker::
    prepareProperty(OU::Property &, volatile uint8_t *&, const volatile uint8_t *&) const {}
    OC::Port &DirectWorker::createPort(const OU::Port &, const OU::PValue *) {
      ocpiAssert("This method is not expected to ever be called" == 0);
      return *(OC::Port*)this;
    }
    OC::Port &DirectWorker::
    createOutputPort(OU::PortOrdinal, size_t, size_t, const OU::PValue*)
      throw (OU::EmbeddedException) {
      ocpiAssert("This method is not expected to ever be called" == 0);
      return *(OC::Port*)this;
    }
    OC::Port &DirectWorker::
    createInputPort(OU::PortOrdinal, size_t, size_t, const OU::PValue*)
      throw (OU::EmbeddedException) {
      ocpiAssert("This method is not expected to ever be called" == 0);
      return *(OC::Port*)this;
    }
    OC::Application *DirectWorker::application() { return NULL;}
    OC::Worker *DirectWorker::nextWorker() { return NULL; }
    void DirectWorker::read(size_t, size_t, void *) {}
    void DirectWorker::write(size_t, size_t, const void *) {}
  }
}

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

#include "OcpiOsMisc.h"
#include "OcpiUtilValue.h"
#include "ValueReader.h"
#include "ValueWriter.h"
#include "Container.h"
#include "ContainerPort.h"
#include "ContainerApplication.h"
#include "ContainerArtifact.h"
#include "ContainerWorker.h"

namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OCPI {
  namespace Container {
    Controllable::Controllable()
      : m_state(OU::Worker::EXISTS), m_controlMask(0) {
    }
    void Controllable::setControlOperations(const char *ops) {
      if (ops) {
#define CONTROL_OP(x, c, t, s1, s2, s3, s4)		\
	if (strstr(ops, #x))				\
	  m_controlMask |= 1 << OU::Worker::Op##c;
	OCPI_CONTROL_OPS
#undef CONTROL_OP
      }
    }

    Worker::
    Worker(Artifact *art, ezxml_t impl, ezxml_t inst, Worker *a_slave, bool a_hasMaster,
	   size_t a_member, size_t a_crewSize, const OA::PValue *) 
      : OU::Worker::Worker(),
	m_artifact(art), m_xml(impl), m_instXml(inst), m_workerMutex(true),
	m_controlOpPending(false), m_slave(a_slave), m_hasMaster(a_hasMaster),
        m_member(a_member), m_crewSize(a_crewSize), m_connectedPorts(0), m_optionalPorts(0) {
      if (impl) {
	const char *err = parse(impl);
	if (err)
	  throw OU::Error("Error parsing worker metadata: %s", err);
	setControlOperations(ezxml_cattr(impl, "controlOperations"));
	m_implTag = ezxml_cattr(impl, "name");
      }
      if (inst)
	m_instTag = ezxml_cattr(inst, "name");
      for (unsigned n = 0; n < m_nPorts; n++)
	if (metaPort(n).m_isOptional)
	  m_optionalPorts |= 1 << n;
    }

    Worker::~Worker()
    {
      ocpiDebug("In  Container::Worker::~Worker()");
      if (m_artifact)
	m_artifact->removeWorker(*this);
      for (unsigned i = 0; i < m_cache.size(); i++)
	delete [] m_cache[i];
    }

    void Worker::
    connectPort(OU::PortOrdinal ordinal) {
      m_connectedPorts |= 1 << ordinal;
      portIsConnected(ordinal); // the virtual notification
    }

    Port &Worker::
    getPort(const char *a_name, size_t nOthers, const OA::PValue *params ) {
      Port *p = findPort(a_name);
      if (p) {
	assert(p->nOthers() == nOthers);
        return *p;
      }
      OU::Port *mPort = findMetaPort(a_name);
      if (!mPort)
        throw OU::Error("no port found with name \"%s\"", a_name);
      Port &newP = createPort(*mPort, params);
      // This setting of the "other" is separate from construction so that the derived
      // port classes in particular containers stay unaware of the scaling.  If that changes
      // then this will become a constructor argument and an argument to worker::createPort.
      newP.prepareOthers(nOthers, m_crewSize);
      return newP;
    }
    OA::Port &Worker::
    getPort(const char *a_name, const OA::PValue *params ) {
      return getPort(a_name, 1, params);
    }
    OA::PropertyInfo & Worker::setupProperty(const char *pname, 
					     volatile uint8_t *&m_writeVaddr,
					     const volatile uint8_t *&m_readVaddr) {
      OU::Property &prop = findProperty(pname);
      if (!prop.m_isReadable && !prop.m_isWritable && !prop.m_isParameter)
	throw OU::Error("Property '%s' of worker '%s' is neither readable nor writable",
			pname, name().c_str());
      if (!prop.m_isParameter)
	prepareProperty(prop, m_writeVaddr, m_readVaddr);
      return prop;
    }
    OA::PropertyInfo & Worker::setupProperty(unsigned n, 
					     volatile uint8_t *&m_writeVaddr,
					     const volatile uint8_t *&m_readVaddr) {
      OU::Property &prop = property(n);
      if (!prop.m_isParameter)
	prepareProperty(prop, m_writeVaddr, m_readVaddr);
      return prop;
    }
    // Internal used by others.
    void Worker::setPropertyValue(const OU::Property &prop, const std::string &v) {
      OU::Value val(prop);
      const char *err = val.parse(v.c_str());
      if (err)
	throw OU::Error("For value \"%s\" for property \"%s\": %s",
			v.c_str(), prop.m_name.c_str(), err);
      setPropertyValue(prop, val);
    }

    // Internal used by others.
    // FIXME:  would a copy-constructor of OU::Value be better for caching?
    //         most callers are construcint an OU::Value already
    //         m_cache would be a sparse OU::Value pointer vector?
    void Worker::setPropertyValue(const OU::Property &info, const OU::Value &v) {
      if (!info.m_isWritable)
	throw OU::Error("The '%s' property of worker '%s' is not writable",
			info.m_name.c_str(), name().c_str());
      if (info.m_isInitial && !beforeStart())
	throw OU::Error("The '%s' property of worker '%s' is initial, and cannot be written after start",
			info.m_name.c_str(), name().c_str());
      if (info.m_baseType == OA::OCPI_Type)
	throw OU::Error("Typedef properties are not settable");
      uint8_t *cache = NULL;
      if (!info.m_isVolatile) {
	if (m_cache.size() <= info.m_ordinal)
	  m_cache.resize(info.m_ordinal + 10, NULL);
	if (!m_cache[info.m_ordinal])
	  m_cache[info.m_ordinal] = new uint8_t[info.m_nBytes];
	cache = m_cache[info.m_ordinal];
      }
      if (info.m_baseType == OA::OCPI_Struct || info.m_isSequence || info.m_arrayRank > 0) {
	// FIXME should we use m_dataOffset here?
	size_t offset = info.m_offset + (info.m_isSequence ? info.m_align : 0);
	if (info.m_baseType == OA::OCPI_String) {
	  const char **sp = v.m_pString;
	  for (unsigned n = 0; n < v.m_nTotal; n++) {
	    size_t l = (sp[n] ? strlen(sp[n]) : 0) + 1;
	    setPropertyBytes(info, offset, (uint8_t *)(sp[n] ? sp[n] : ""), l);
	    if (cache)
	      memcpy(cache + (offset - info.m_offset), sp[n] ? sp [n] : "", l);
	    offset += OU::roundUp(info.m_stringLength + 1, 4);
	  }
	} else {
	  uint8_t *data;
	  uint64_t *alloc = NULL;
	  size_t nBytes = v.m_nTotal * info.m_elementBytes;
	  if (info.m_baseType == OA::OCPI_Struct) {
	    // We need to create a temporary linear value - explicitly align it
	    // This value is fully serialized, including sequence length
	    if (info.m_isSequence)
              nBytes += std::max(sizeof(uint32_t), info.m_dataAlign);
	    size_t length = (nBytes + 7)/8;
	    alloc = new uint64_t[length];
	    length = nBytes;
	    data = (uint8_t*)alloc;
	    const OU::Value *vp = &v;
	    OU::ValueReader reader(&vp);
	    info.read(reader, data, length, false, true); // not fake, top level
	    assert(length < info.m_dataAlign); // padding at end may not be written
	    data = (uint8_t*)alloc;
	    if (info.m_isSequence) { // because we set the sequence length last below
	      assert(*(uint32_t *)data == v.m_nElements);
	      data += sizeof(uint32_t);
	      nBytes -= sizeof(uint32_t);
	    }
	  } else
	    data = v.m_pUChar;
	  if (nBytes) {
	    setPropertyBytes(info, offset, data, nBytes);
	    if (cache)
	      memcpy(cache + (offset - info.m_offset), data, nBytes);
	  }
	  delete [] alloc; // may be null if not serialized (not struct)
	}
        if (info.m_isSequence) {
	  setProperty32(info, 0, OCPI_UTRUNCATE(uint32_t, v.m_nElements));
	  if (cache)
	    *(uint32_t*)(cache) = OCPI_UTRUNCATE(uint32_t, v.m_nElements);
	}
      } else if (info.m_baseType == OA::OCPI_String) {
	size_t l = strlen(v.m_String) + 1; // amount to actually copy
	if (l > 4)
	  setPropertyBytes(info, info.m_offset + 4,
			   (uint8_t *)(v.m_String + 4), l - 4);
	setProperty32(info, 0, *(uint32_t *)v.m_String);
	if (cache)
	  memcpy(cache, (uint8_t*)v.m_String, l);
      } else {
	switch (info.m_nBits) {
	case 8:
	  setProperty8(info, 0, v.m_UChar); break;
	case 16:
	  setProperty16(info, 0, v.m_UShort); break;
	case 32:
	  setProperty32(info, 0, v.m_ULong); break;
	case 64:
	  setProperty64(info, 0, v.m_ULongLong); break;
	default:;
	}
	if (cache)
	  memcpy(cache, &v.m_UChar, info.m_nBytes);
      }
      if (info.m_writeSync)
	propertyWritten(info.m_ordinal);
    }
    void Worker::setProperty(const char *pname, const char *value) {
      OU::Property &prop = findProperty(pname);
      OU::ValueType &vt = prop;
      OU::Value v(vt); // FIXME storage when not scalar
      const char *err = v.parse(value);
      if (err)
        throw OU::ApiError("Error parsing property value:\"", value, "\"", NULL);
      setPropertyValue(prop, v);
    }
    void Worker::
    getPropertyValue(const OU::Property &p, std::string &value, bool hex, bool add, bool uncached) {
      OU::Value v(p);
      OA::Property a(*this, p.m_name.c_str()); // FIXME clumsy because get methods take API props
      OA::PropertyInfo &info = a.m_info;
      if (p.m_baseType == OA::OCPI_Type)
	throw OU::Error("Typedef properties are unsupported");
      if (info.m_readSync)
	propertyRead(info.m_ordinal);
      uint8_t *cache = // use cache unless we are told not to and its readable
	(!uncached || !p.m_isReadable) && m_cache.size() > p.m_ordinal ?
	m_cache[p.m_ordinal] : NULL;
      if (info.m_baseType == OA::OCPI_Struct || info.m_isSequence || info.m_arrayRank > 0) {
	v.m_nTotal = info.m_nItems;
	if (info.m_isSequence) {
	  v.m_nElements = cache ? *(uint32_t*)cache : getProperty32(info, 0);
	  if (v.m_nElements > info.m_sequenceLength)
	    throw OU::Error("Worker's %s property has invalid sequence length: %zu",
			    info.m_name.c_str(), v.m_nElements);
	  v.m_nTotal *= v.m_nElements;
	}
	size_t nBytes = v.m_nTotal * info.m_elementBytes;
	if (info.m_baseType == OA::OCPI_String) {
	  size_t offset = info.m_offset + (info.m_isSequence ? info.m_align : 0);
	  size_t length = OU::roundUp(info.m_stringLength + 1, 4);
	  v.m_stringSpaceLength = v.m_nTotal * length;
	  v.m_stringNext = v.m_stringSpace = new char[v.m_stringSpaceLength];
	  char **sp = new char *[v.m_nTotal];
	  v.m_pString = (const char **)sp;
	  for (unsigned n = 0; n < v.m_nTotal; n++) {
	    sp[n] = v.m_stringNext;
	    if (cache)
	      memcpy(v.m_stringNext, cache + (offset - info.m_offset), length);
	    else
	      getPropertyBytes(info, offset, (uint8_t *)v.m_stringNext, length);
	    v.m_stringNext += length;
	    offset += length;
	  }	  
	} else if (nBytes) {
          if (info.m_isSequence)
            nBytes += std::max(sizeof(uint32_t), info.m_dataAlign);
          uint8_t *data = (uint8_t*) new uint64_t[(nBytes + 7)/8];
	  if (cache)
	    memcpy(data, cache, nBytes);
	  else
	    getPropertyBytes(info, info.m_offset, data, nBytes); // include length field
	  //	  if (info.m_baseType == OA::OCPI_Struct)
	  {
	    const uint8_t *tmp = data;
	    size_t length = nBytes;
	    // The writer creates its own value objects...
	    // FIXME: use more ValueWriter functionality for this whole method
	    OU::Value *vp = NULL;
	    OU::ValueWriter writer(&vp, 1);
	    info.write(writer, tmp, length, true); // is top-level
	    assert(length < info.m_dataAlign); // padding at end may not be taken
	    delete [] data;
	    vp->unparse(value, NULL, add, hex);
	    delete vp;
	    return; // FIXME - see above
	  }
	  // else
	  //v.m_pUChar = data + (info.m_isSequence ? info.m_dataAlign : 0);
	}
      } else if (info.m_baseType == OA::OCPI_String) {
	// FIXME: modularity violation
	v.m_stringSpace = new char[info.m_stringLength + 1];
	v.m_String = v.m_stringSpace;
	if (cache)
	  strcpy((char*)v.m_String, (char *)cache);
	else
	  getPropertyBytes(info, info.m_offset, (uint8_t*)v.m_String, info.m_stringLength + 1);
      } else if (cache)
	memcpy(&v.m_UChar, cache, info.m_nBytes);
      else
	switch (info.m_nBits) {
	case 8:
	  v.m_UChar = getProperty8(info, 0); break;
	case 16:
	  v.m_UShort = getProperty16(info, 0); break;
	case 32:
	  v.m_ULong = getProperty32(info, 0); break;
	case 64:
	  v.m_ULongLong = getProperty64(info, 0); break;
	default:;
	}
      v.unparse(value, NULL, add, hex);
    }
    bool Worker::getProperty(unsigned ordinal, std::string &a_name, std::string &value,
			     bool *unreadablep, bool hex, bool *cachedp, bool uncached) {
      unsigned nProps;
      OU::Property *props = properties(nProps);
      if (ordinal >= nProps)
	return false;
      OU::Property &p = props[ordinal];
      a_name = p.m_name;
      if (p.m_isReadable || p.m_isParameter ||
	  (p.m_isWritable && !p.m_isVolatile && m_cache.size() > p.m_ordinal &&
	   m_cache[p.m_ordinal])) {
	if (unreadablep)
	  *unreadablep = false;
	if (cachedp)
	  *cachedp =
	    m_cache.size() > p.m_ordinal && m_cache[p.m_ordinal] &&
	    (!uncached || !p.m_isReadable);
      } else if (unreadablep) {
	*unreadablep = true;
	if (cachedp)
	  *cachedp = false;
	return true;
      } else
	throw OU::Error("Property number %u '%s' is unreadable", ordinal, p.m_name.c_str());
      if (p.m_isParameter) {
	p.m_default->unparse(value, NULL, false, hex);
	return true;
      }
      getPropertyValue(p, value, hex, false, uncached);
      return true;
    }
    void Worker::setProperty(unsigned ordinal, OU::Value &value) {
      OU::Property &prop(property(ordinal));
      setPropertyValue(prop, value);
    }
    // batch setting with lots of error checking - all or nothing
    void Worker::setProperties(const OA::PValue *props) {
      if (props)
	for (const OA::PValue *p = props; p->name; p++) {
	  OA::Property prop(*this, p->name); // exception goes out
	  prop.checkTypeAlways(&prop.m_info, p->type, 1, true);
	  switch (prop.m_info.m_baseType) {
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
	    case OA::OCPI_##pretty:				\
	      prop.set##pretty##Value(p->v##pretty);			\
	      break;
            OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
            default:
	      ocpiAssert("unknown data type"==0);
	  }
	}
    }
    // batch setting with lots of error checking - all or nothing
    void Worker::setProperties(const char *props[][2]) {
      for (const char *(*p)[2] = props; (*p)[0]; p++)
	setProperty((*p)[0], (*p)[1]);
    }
    // Common top level implementation for control operations
    // Note that the m_controlMask does not apply at this level
    // since the container might want to know anyway, even if the
    // worker doesn't have an implementation
#define CONTROL_OP(x, c, t, s1, s2, s3, s4)	\
    void Worker::x() { controlOp(OU::Worker::Op##c); }
    OCPI_CONTROL_OPS
#undef CONTROL_OP

    struct ControlTransition {
      OU::Worker::ControlState valid[4];
      OU::Worker::ControlState next;
    } controlTransitions[] = {
#define CONTROL_OP(x, c, t, s1, s2, s3, s4)	\
      {{OU::Worker::s1, OU::Worker::s2, OU::Worker::s3, OU::Worker::s4}, OU::Worker::t},
	OCPI_CONTROL_OPS
#undef CONTROL_OP
    };

    // sched_yield does not work with the default
    // Linux scheduler: SCHED_OTHER, and it requires special permission/capability to use
    // the other "realtime" schedulers that actually implement sched_yield.  For many
    // other reasons enabling realtime scheduling is worse that this for now.
    // (e.g. special permissions and capabilities are required).
    void Worker::checkControl() {
      if (m_controlOpPending) {
	m_controlMutex.lock();
	m_controlMutex.unlock();
      }
    }	

    bool Worker::controlOp(OU::Worker::ControlOperation op) {
      // sched_yield does not work with the default
      // Linux scheduler: SCHED_OTHER, and it requires special permission/capability to use
      // the other "realtime" schedulers that actually implement sched_yield.
      OU::AutoMutex ctl (m_controlMutex);
      struct autoflag {
	bool &m_flag;
	autoflag(bool &flag) : m_flag(flag) { m_flag = true; }
	~autoflag() { m_flag = false; }
      } flag(m_controlOpPending);
      OU::AutoMutex guard (m_workerMutex, true);
      ControlState cs = getControlState();
      ControlTransition ct = controlTransitions[op];
      // Special case starting and stopping after finished
      if (cs == OU::Worker::FINISHED && (op == OU::Worker::OpStop || op == OU::Worker::OpStart))
	return true;
      // If we are already in the desired state, just ignore it so that
      // Neither workers not containers need to deal with this
      if (ct.next != OU::Worker::NONE && cs == ct.next)
	return true;
      if (cs == ct.valid[0] ||
	  (ct.valid[1] != NONE && cs == ct.valid[1]) ||
	  (ct.valid[2] != NONE && cs == ct.valid[2]) ||
	  (ct.valid[3] != NONE && cs == ct.valid[3])) {
	if (op == OU::Worker::OpStart) {
	  // If a worker gets started before all of its required ports are created: error
	  PortMask mandatory = ~(-1 << m_nPorts) & ~optionalPorts();
	  PortMask bad = mandatory & ~connectedPorts();
	  for (unsigned n = 0; n < m_nPorts; n++)
	    if (bad & (1 << n)) {
	      const char *inst = instTag().c_str();
	      throw OU::Error("Port \"%s\" of%s%s%s worker \"%s\" is not connected",
			      m_ports ? metaPort(n).cname() : "unnamed",
			      inst[0] ? " instance '" : "", inst, inst[0] ? "'" : "",
			      implTag().c_str());
	    }
	}
	controlOperation(op);
	if (ct.next != NONE)
	  setControlState(ct.next);
      } else
	throw
	  OU::Error("Control operation '%s' failed on worker '%s%s%s' in state: '%s'",
		    OU::Worker::s_controlOpNames[op], implTag().c_str(),
		    instTag().empty() ? "" : "/", instTag().c_str(),
		    OU::Worker::s_controlStateNames[cs]);
      Application *a = application();
      if (a && op == OU::Worker::OpStart)
	a->container().start();
      return false;
    }
    bool Worker::beforeStart() {
      return getControlState() == INITIALIZED;
    }
    bool Worker::isDone() {
      ControlState cs = getControlState();
      return cs == UNUSABLE || cs == FINISHED;
    }
    bool Worker::wait(OCPI::OS::Timer *timer) {
      while (!isDone()) {
	OS::sleep(10);
	if (timer && timer->expired())
	  return true;
      }
      return false;
    }

#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
    run Worker::							\
    get##pretty##PropertyOrd(unsigned ordinal, unsigned idx) const { \
    const OA::PropertyInfo &pi = m_properties[ordinal];		  \
    const OU::Member *m = &m_properties[ordinal]; \
    return get##pretty##Property(pi, m, (size_t)0, idx);		\
    }									\
    void Worker:: \
    set##pretty##PropertyOrd(unsigned ordinal, run val, unsigned idx) const { \
      set##pretty##Property(m_properties[ordinal], &m_properties[ordinal], 0, val, idx); \
    }\
    run Worker::							\
    get##pretty##Parameter(unsigned ordinal, unsigned idx) const { \
      OU::Property &p = m_properties[ordinal]; \
      assert(p.m_default); \
      OU::Value &v = *p.m_default; \
      return p.m_isSequence || p.m_arrayRank ? v.m_p##pretty[idx] : v.m_##pretty; \
    }
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE

    void Worker::
    setStringPropertyOrd(unsigned ordinal, const char *s, unsigned idx) const {
      setStringProperty(m_properties[ordinal], &m_properties[ordinal], 0, s, idx);
    }
    void Worker::
    getStringPropertyOrd(unsigned ord, char *str, size_t length, unsigned idx) const {
      getStringProperty(m_properties[ord], &m_properties[ord], 0, str, length, idx);
    }
    void Worker::
    getStringParameter(unsigned ordinal, char *out, size_t length, unsigned idx) const {
      OU::Property &p = m_properties[ordinal];
      assert(p.m_default);
      OU::Value &v = *p.m_default;
      strncpy(out, p.m_isSequence || p.m_arrayRank ? v.m_pString[idx] : v.m_String, length);
    }
    Port &Worker::
    createOutputPort(OU::PortOrdinal /*portId*/, size_t /*bufferCount*/, size_t /*bufferSize*/,
		     const OU::PValue */*params*/) {
      ocpiAssert("This method is not expected to ever be called" == 0);
      return *(Port*)this;
    }
    Port &Worker::
    createInputPort(OU::PortOrdinal /*portId*/, size_t /*bufferCount*/, size_t /*bufferSize*/, 
		    const OU::PValue */*params*/) {
      ocpiAssert("This method is not expected to ever be called" == 0);
      return *(Port*)this;
    }
    Port &Worker::
    createTestPort(OU::PortOrdinal /*portId*/, size_t /*bufferCount*/, size_t /*bufferSize*/,
		   bool /*isProvider*/, const OU::PValue */*params*/) {
      ocpiAssert("This method is not expected to ever be called" == 0);
      return *(Port*)this;
    }
    void Worker::
    read(size_t /*offset*/, size_t /*size*/, void */*data*/) {
      ocpiAssert("This method is not expected to ever be called" == 0);
    }
    void Worker::
    write(size_t /*offset*/, size_t /*size*/, const void */*data*/) {
      ocpiAssert("This method is not expected to ever be called" == 0);
    }
    WorkerControl::~WorkerControl(){}
  }
  namespace API {
    Worker::~Worker(){}
  }
}

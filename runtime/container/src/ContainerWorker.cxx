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

#include <climits> // CHAR_BIT
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

    // A very 8-bit-byte-oriented cache
    class Cache {
      typedef uint8_t Word; // no templates here, just to allow this to change from 8 to 64
      static unsigned c_bpw, c_shift;
      uint8_t *m_data;
      Word    *m_dirty;
    public:
      Cache(size_t nBytes) : m_data(new uint8_t[nBytes]) {
	size_t nWords = OU::roundUp(nBytes, sizeof(Word))/sizeof(Word);
	m_dirty = new Word[nWords];
	memset(m_dirty, 0, nWords * sizeof(Word));
      }
      ~Cache() { delete [] m_data; delete [] m_dirty; }
#if 0
      bool anySet(size_t start, size_t n) {
	(void)start;(void)n;
	return false;
      }
#endif
      bool allSet(size_t start, size_t n) {
	assert(m_dirty);
	Word *p = m_dirty + (start >> c_shift);
	size_t offset = start & (c_bpw - 1);
	if (offset) {
	  size_t i = c_bpw - offset;
	  Word mask = (Word)(~0u << offset);
	  if (n < i)
	    mask &= (Word)~(~0u << (n + offset));
	  if ((*p++ & mask) != mask)
	    return false;
	  if (n <= i)
	    return true;
	  n -= i;
	}
	for (; n >= c_bpw; n-= c_bpw)
	  if (*p++ != (uint8_t)~0u)
	    return false;
	if (n) {
	  Word mask = (uint8_t)~(~0u << n);
	  if ((*p & mask) != mask)
	    return false;
	}
	return true;
      }
      void copyFromCache(size_t start, size_t n, uint8_t *data) {
	memcpy(data, m_data + start, n);
      }
      uint8_t *dataAt(size_t start) {
	return m_data + start;
      }
      void fill(size_t start, size_t n, const uint8_t *data) {
	memcpy(m_data + start, data, n);
	setDirty(start, n);
      }
      void setDirty(size_t start, size_t n) {
	Word *p = m_dirty + (start >> c_shift);
	size_t offset = start & (c_bpw - 1);
	if (offset) {
	  size_t i = c_bpw - offset;
	  Word mask = (Word)(~0u << offset);
	  if (n < i)
	    mask &= (Word)~(~0u << (n + offset));
	  *p++ |= mask;
	  if (n <= i)
	    return;
	  n -= i;
	}
	#if 0
	size_t nWords = n >> c_shift;
	memset(p, ~0u, nWords << c_shift);
	n -= nWords << c_shift;
	#else
	for (; n >= c_bpw; n-= c_bpw)
	  *p++ = (uint8_t)~0u;
	#endif
	if (n)
	  *p |= (uint8_t)~(~0u << n);
      }
    };
    unsigned
      Cache::c_bpw = (unsigned)(sizeof(Cache::Word) * CHAR_BIT),
      Cache::c_shift =
	sizeof(Cache::Word) == 1 ? 3 : sizeof(Cache::Word) == 2 ? 4 : sizeof(Cache::Word) == 4 ? 5 : 6;

    const Workers NoWorkers;
    Worker::
    Worker(Artifact *art, ezxml_t impl, ezxml_t inst, const Workers &a_slaves, bool a_hasMaster,
	   size_t a_member, size_t a_crewSize, const OA::PValue *)
      : OU::Worker::Worker(),
	m_artifact(art), m_xml(impl), m_instXml(inst), m_workerMutex(true),
	m_controlOpPending(false), m_slaves(a_slaves), m_hasMaster(a_hasMaster),
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
	  m_optionalPorts |= 1u << n;
    }

    Worker::~Worker()
    {
      ocpiDebug("In  Container::Worker::~Worker()");
      if (m_artifact)
	m_artifact->removeWorker(*this);
      for (unsigned i = 0; i < m_cache.size(); i++)
	delete m_cache[i];
    }

    bool Worker::
    isOperating() const{
      return this->getControlState() == OU::Worker::OPERATING;
    }
    void Worker::
    connectPort(OU::PortOrdinal ordinal) {
      m_connectedPorts |= 1u << ordinal;
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
      return getPort(a_name, 0, params);
    }
    OA::PropertyInfo & Worker::setupProperty(const char *pname,
					     volatile uint8_t *&m_writeVaddr,
					     const volatile uint8_t *&m_readVaddr) const {
      OU::Property &prop = findProperty(pname);
      if (prop.m_isDebug && !isDebug())
	throw OU::Error("For setting debug property \"%s\": worker \"%s\" is not in debug mode",
			prop.cname(), cname());

#if 0 // not readable is not possible given caching
      if (!prop.m_isReadable && !prop.m_isWritable && !prop.m_isParameter)
	throw OU::Error("Property '%s' of worker '%s' is neither readable nor writable",
			pname, name().c_str());
#endif
      if (!prop.m_isParameter)
	prepareProperty(prop, m_writeVaddr, m_readVaddr);
      return prop;
    }
    OA::PropertyInfo & Worker::setupProperty(unsigned n,
					     volatile uint8_t *&m_writeVaddr,
					     const volatile uint8_t *&m_readVaddr) const {
      OU::Property &prop = property(n);
      if (!prop.m_isParameter)
	prepareProperty(prop, m_writeVaddr, m_readVaddr);
      return prop;
    }
    // ========================== Property Setting: 5 levels ===========================

    // Level 1 of 5, converting the property name into a reference to the property info object
    // There is a higher level API in the header that simply converts string refs to const char *
    // This is used in the ACI to use text names and values.
    void Worker::
    setProperty(const char *pname, const char *value, OA::AccessList &list) const {
      setProperty(findProperty(pname), value, list);
    }
    const char *Worker::
    getProperty(const char *pname, std::string &value, OA::AccessList &list,
		OA::PropertyOptionList &options,
		OA::PropertyAttributes *a_attributes) const {
      return getProperty(findProperty(pname), value, list, options, a_attributes);
    }
    // Level 1.5 - use ordinals rather than names
    void Worker::
    setProperty(unsigned ordinal, const char *value, OA::AccessList &list) const {
      unsigned nProps;
      OU::Property *props = properties(nProps);
      assert(ordinal < nProps); // internal
      setProperty(props[ordinal], value, list);
    }
    const char *Worker::
    getProperty(unsigned ordinal, std::string &value, OA::AccessList &list,
		OA::PropertyOptionList &options, OA::PropertyAttributes *a_attributes) const {
      unsigned nProps;
      OU::Property *props = properties(nProps);
      return ordinal >= nProps ? NULL :
	getProperty(props[ordinal], value, list, options, a_attributes);
    }
    // Level 2 of 5, parsing the access list against the data type
    // Used for OA::Property methods that provide text valuers, that implicitly have the PropertyInfo
    // This is where navigation of property values occurs
    void Worker::
    setProperty(const OA::PropertyInfo &prop, const char *v, OA::AccessList &list) const {
      size_t offset, dimension;
      const OU::Member *m;
      const char *err = prop.descend(list, m, offset, &dimension);
      if (err)
	throw OU::Error("For setting value \"%s\" for property \"%s\": %s", v, prop.cname(), err);
      setProperty(prop, v, *m, offset, dimension);
    }
    const char *Worker::
    getProperty(const OCPI::API::PropertyInfo &prop, std::string &v, OA::AccessList &list,
		OA::PropertyOptionList &options, OA::PropertyAttributes *a_attributes) const {
      size_t offset, dimension;
      const OU::Member *m;
      const char *err = prop.descend(list, m, offset, &dimension);
      if (err)
	throw OU::Error("For getting value \"%s\" for property \"%s\": %s", v.c_str(), prop.cname(), err);
      if (a_attributes) {
	// This can be a substructure to property info someway and then we could just copy
	OA::PropertyAttributes &a = *a_attributes;
	a.isParameter = prop.m_isParameter;
	a.isHidden = prop.m_isHidden;
	a.isInitial = prop.m_isInitial;
	a.isVolatile = prop.m_isVolatile;
	a.isWritable = prop.m_isWritable;
	a.isDebug = prop.m_isDebug;
	a.isWorker = prop.m_isImpl;
	a.isCached = false;     // set by lower levels if cached
	a.isUnreadable = false; // set by lower levels if not readable at all
	a.name = prop.m_name;
      }
      getProperty(prop, v, *m, offset, dimension, options, a_attributes);
      return v.c_str();
    }
    // Level 3 of 5, post, navigation, adjusting the data type based on dimension/slicing
    // This is the level that is intercepted for remote containers to avoid parse/unparse etc.
    void Worker::
    setProperty(const OA::PropertyInfo &prop, const char *val, const OU::Member &a_member, size_t offset,
		size_t dimension) const {
      if (prop.m_isDebug && !isDebug())
	throw OU::Error("For setting debug property \"%s\": worker is not in debug mode",
			prop.cname());
      if (dimension) {
	// Clone the data type, and then trim the dimensions.
	OU::Member m(a_member);
	if (m.m_isSequence) {
	  m.m_isSequence = false;
	  dimension--;
	}
	if (dimension) {
	  assert(m.m_arrayRank && dimension <= m.m_arrayRank);
	  for (size_t n = dimension; n < m.m_arrayRank; ++n)
	    m.m_arrayDimensions[n - dimension] = m.m_arrayDimensions[n];
	  m.m_arrayRank -= dimension;
	}
	setProperty(prop, val, m, offset);
      } else
	setProperty(prop, val, a_member, offset);
    }
    void Worker::
    getProperty(const OCPI::API::PropertyInfo &prop, std::string &val,
		const OCPI::Util::Member &a_member, size_t offset, size_t dimension,
		OA::PropertyOptionList &options, OA::PropertyAttributes *a_attributes) const {
      if (prop.m_isDebug && !isDebug()) {
	if (hasOption(options, OA::PropertyOption::UNREADABLE_OK)) {
	  if (a_attributes)
	    a_attributes->isUnreadable = true;
	  if (!hasOption(options, OA::PropertyOption::APPEND))
	    val.clear();
	  return;
	} else
	  throw OU::Error("For getting debug property \"%s\": worker is not in debug mode",
			  prop.cname());
      }
      if (dimension) {
	// Clone the data type, and then trim the dimensions.
	OU::Member m(a_member);
	if (m.m_isSequence) {
	  m.m_isSequence = false;
	  dimension--;
	}
	if (dimension) {
	  assert(m.m_arrayRank && dimension <= m.m_arrayRank);
	  for (size_t n = dimension; n < m.m_arrayRank; ++n)
	    m.m_arrayDimensions[n - dimension] = m.m_arrayDimensions[n];
	  m.m_arrayRank -= dimension;
	}
	getProperty(prop, val, m, offset, options, a_attributes);
      } else
	getProperty(prop, val, a_member, offset, options, a_attributes);
    }
    // Level 4 of 5, parsing the value string into a OU::Value object
    // Used to avoid heap allocation when there is slicing (above).
    void Worker::
    setProperty(const OA::PropertyInfo &prop, const char *v, const OU::Member &m,
		size_t offset) const {
      OU::Value val(m);
      const char *err = val.parse(v);
      if (err)
	throw OU::Error("For value \"%s\" for property \"%s\": %s", v, prop.m_name.c_str(), err);
      setProperty(prop, val, m, offset);
    }
    bool Worker::
    hasOption(OA::PropertyOptionList &options, OA::PropertyOption o) {
      for (auto it = options.begin(); it != options.end(); ++it)
	if (*it == o)
	  return true;
      return false;
    }
    void Worker::
    getProperty(const OCPI::API::PropertyInfo &info, std::string &val, const OCPI::Util::Member &m,
		size_t offset, OA::PropertyOptionList &options,
		OA::PropertyAttributes *a_attributes) const {
      OU::Value v(m);
      getProperty(info, v, m, offset, options, a_attributes);
      if (a_attributes && a_attributes->isUnreadable) {
	if (!hasOption(options, OA::PropertyOption::APPEND))
	  val.clear();
      } else
	v.unparse(val, NULL, hasOption(options, OA::PropertyOption::APPEND),
		  hasOption(options, OA::PropertyOption::HEX));
    }
    // Level 5 of 5: doing the real work based on a value object and an offset, dealing with caching
    // This level is used directly for delayed settings from the application level
    // Internal used by others.
    // FIXME:  would a copy-constructor of OU::Value be better for caching?
    //         most callers are constructing an OU::Value already
    //         m_cache could be a sparse OU::Value pointer vector?
    // The optionally non-NULL memberp arg identifies the member within this property
    // which might a struct member, and moffset is an offset into the overall property
    // which might be into the middle of an array or sequence.
    void Worker::
    setProperty(unsigned ordinal, const OCPI::Util::Value &v) const {
      OU::Property &p = property(ordinal);
      setProperty(p, v, p, 0);
    }
    // dirty != NULL means for reading rather than writing
    Cache *Worker::
    getCache(const OA::PropertyInfo &info, size_t offset, const OU::Member &m, bool *dirty,
	     OA::PropertyOptionList &options, OA::PropertyAttributes *a_attributes) const {
      if (dirty) {
	*dirty = false;
	if (info.m_readSync)
	  propertyRead(info.m_ordinal);
      }
      if (info.m_isVolatile || (hasOption(options, OA::PropertyOption::UNCACHED) && info.m_isReadable))
	return NULL;
      if (m_cache.size() <= info.m_ordinal)
	m_cache.resize(info.m_ordinal + 10, NULL);
      Cache *cache = m_cache[info.m_ordinal];
      if (a_attributes)
	a_attributes->isCached = false;
      if (!cache)
	cache = m_cache[info.m_ordinal] = new Cache(info.m_nBytes);
      else if (dirty) {
	if (cache->allSet(offset, m.m_isSequence || m.m_baseType == OA::OCPI_String ?
			  sizeof(uint32_t) : m.m_nBytes)) {
	  if (a_attributes)
	    a_attributes->isCached = true;
	  *dirty = true;
	} else if (!info.m_isReadable) {
	  if (hasOption(options, OA::PropertyOption::UNREADABLE_OK)) {
	    if (a_attributes)
	      a_attributes->isUnreadable = true;
	  } else
	    throw OU::Error("When reading \"%s\" property, "
			    "there is no value since it has never been written", info.cname());
	}
      }
      return cache;
    }
    // Basic setting without any data type information
    void Worker::
    setData(const OA::PropertyInfo &info, Cache *cache, size_t offset, const uint8_t *data,
	    size_t nBytes, size_t nBits, size_t sequenceOffset, size_t sequenceLength, bool last) const {
      switch (nBits) {
      case 8:
	setProperty8(info, offset, *data); break;
      case 16:
	setProperty16(info, offset, *(uint16_t *)data); break;
      case 32:
	setProperty32(info, offset, *(uint32_t *)data); break;
      case 64:
	setProperty64(info, offset, *(uint64_t *)data); break;
      default:
	if (nBytes)
	  setPropertyBytes(info, offset + sequenceOffset, data, nBytes);
	if (sequenceOffset) {
	  setProperty32(info, offset, OCPI_UTRUNCATE(uint32_t, sequenceLength));
	  if (cache)
	    cache->fill(offset, sizeof(uint32_t), (const uint8_t *)&sequenceLength);
	}
      }
      if (cache)
	cache->fill(offset + sequenceOffset, nBits ? nBits/8 : nBytes, data);
      if (last && info.m_writeSync)
	propertyWritten(info.m_ordinal); // Some has been written
    }
    // Basic getting of possibly-cached data without any data type information
    // There are basically two different modes as seen by the caller
    // 1. copy the value into the callers buffer, maybe from cache
    //    this mode is indicated by the data pointer arg (by reference) being NOT NULL
    // 2. return a pointer to the data, maybe allocating a buffer if there is no cache
    //    this mode is indicated by the data pointer arg being NULL
    const uint8_t *Worker::
    getData(const OA::PropertyInfo &info, Cache *cache, bool dirty, size_t offset, uint8_t *&data,
	    size_t nBytes, size_t nBits) const {
      uint8_t *cacheData, *dest;
      if (!nBytes)
	nBytes = nBits/CHAR_BIT;
      if (cache) {
	cacheData = cache->dataAt(offset);
	if (dirty)
	  return data ? (const uint8_t *)memcpy(data, cacheData, nBytes) : cacheData;
	dest = cacheData; // read into cache in either mode
      } else {
        cacheData = NULL;
	if (!data)
	  data = (uint8_t *)new uint64_t[(nBytes + sizeof(uint64_t)-1)/sizeof(uint64_t)];
	dest = data;      // read directory into user buffer
      }
      switch (nBits) {
      case 8:
	*dest = getProperty8(info, offset); break;
      case 16:
	*(uint16_t *)dest = getProperty16(info, offset); break;
      case 32:
	*(uint32_t *)dest = getProperty32(info, offset); break;
      case 64:
	*(uint64_t *)dest = getProperty64(info, offset); break;
      default:
	getPropertyBytes(info, offset, dest, nBytes);
      }
      if (cache) {
	cache->setDirty(offset, nBytes);
	return data ? (const uint8_t *)memcpy(data, cacheData, nBytes) : cacheData;
      }
      return data;
    }
    void Worker::
    setProperty(const OA::PropertyInfo &info, const OU::Value &v, const OU::Member &m,
		size_t mOffset) const {
      if (!info.m_isWritable)
	throw OU::Error("The '%s' property of worker '%s' is not writable",
			info.m_name.c_str(), name().c_str());
      if (info.m_isInitial && !beforeStart())
	throw OU::Error("The '%s' property of worker '%s' is initial, and cannot be written after start",
			info.m_name.c_str(), name().c_str());
      // FIXME: this is certainly possible, with added code.
      if (info.m_baseType == OA::OCPI_Type || m.m_baseType == OA::OCPI_Type)
	throw OU::Error("Typedef properties are not settable yet");
      Cache *cache = getCache(info, mOffset, m);
      if (m.m_baseType == OA::OCPI_Struct || m.m_isSequence || m.m_arrayRank > 0) {
	// FIXME should we use m_dataOffset here?
	size_t offset = mOffset + (m.m_isSequence ? m.m_align : 0);
	if (m.m_baseType == OA::OCPI_String) { // not a struct
	  const char **sp = v.m_pString;
	  if (cache)
	    cache->setDirty(offset, m.m_nBytes); // make it all dirty so dirty is not sparse
	  for (unsigned n = 0; n < v.m_nTotal; n++) {
	    size_t l = (sp[n] ? strlen(sp[n]) : 0) + 1;
	    setData(info, cache, offset, (uint8_t *)(sp[n] ? sp[n] : ""), l, 0, 0, 0, !m.m_isSequence);
	    offset += OU::roundUp(m.m_stringLength + 1, 4);
	  }
	} else {
	  uint8_t *data;
	  uint64_t *alloc = NULL;
	  size_t nBytes = v.m_nTotal * m.m_elementBytes;
	  if (m.m_baseType == OA::OCPI_Struct) {
	    // We need to create a temporary linear value - explicitly align it
	    // This value is fully serialized, including sequence length
	    if (m.m_isSequence)
              nBytes += std::max(sizeof(uint32_t), m.m_dataAlign);
	    alloc = new uint64_t[(nBytes + 7)/8]; // round nBytes up to next 64bit chunk
	    size_t length = nBytes;
	    data = (uint8_t*)alloc;
	    const OU::Value *vp = &v;
	    OU::ValueReader reader(&vp);
	    // put data from the value object into the data buffer, not fake top level
	    m.read(reader, data, length, false, true);
	    if (m.m_isSequence) {
	      size_t unset = (v.m_nTotal - v.m_nElements) * m.m_elementBytes;
	      assert(length >= unset);
	      length -= unset;
	    }
	    assert(length < m.m_dataAlign); // padding at end may not be written
	    data = (uint8_t*)alloc;
	    if (m.m_isSequence) { // because we set the sequence length last below
	      assert(*(uint32_t *)data == v.m_nElements);
	      data += sizeof(uint32_t);
	      nBytes -= sizeof(uint32_t);
	    }
	  } else
	    data = v.m_pUChar;
	  setData(info, cache, offset, data, nBytes, 0, 0, 0, !m.m_isSequence);
	  delete [] alloc; // may be null if not serialized (not struct)
	}
        if (m.m_isSequence)
	  setData(info, cache, mOffset, 0, 0, 0, m.m_dataAlign, v.m_nElements);
      } else if (m.m_baseType == OA::OCPI_String) {
	size_t l = strlen(v.m_String) + 1; // amount to actually copy
	if (l > 4)
	  setData(info, cache, mOffset, (uint8_t *)(v.m_String + 4), l - 4, 0, 4, 
		  *(uint32_t *)v.m_String);
	else
	  setData(info, cache, mOffset, (uint8_t *)v.m_String, l);
	if (cache)
	  cache->setDirty(mOffset, m.m_nBytes); // make it all dirty so dirty is not sparse
      } else
	setData(info, cache, mOffset, &v.m_UChar, 0, m.m_nBits);
    }
    void Worker::
    getProperty(const OA::PropertyInfo &info, OU::Value &v, const OU::Member &m,
		size_t mOffset, OA::PropertyOptionList &options,
		OA::PropertyAttributes *a_attributes) const {
      if (info.m_baseType == OA::OCPI_Type || m.m_baseType == OA::OCPI_Type)
	throw OU::Error("Typedef properties are not supported yet");
      if (info.m_isParameter) {
	v = *m.m_default;
	return;
      }
      bool dirty;
      Cache *cache = getCache(info, mOffset, m, &dirty, options, a_attributes);
      if (m.m_baseType == OA::OCPI_Struct || m.m_isSequence || m.m_arrayRank > 0) {
	v.m_nTotal = m.m_nItems;
	// Even though the "writer" below deals with this length, we need to precalculate
	// The length of the marshalling buffer
	size_t offset = mOffset;
	if (m.m_isSequence) {
	  uint32_t nElements;
	  uint8_t *data = (uint8_t *)&nElements;
	  getData(info, cache, dirty, mOffset, data, 0, 32);
	  if ((v.m_nElements = nElements) > m.m_sequenceLength)
	    throw OU::Error("Worker's %s property has invalid sequence length: %zu",
			    info.cname(), v.m_nElements);
	  v.m_nTotal *= v.m_nElements;
	  offset += m.m_align;
	}
	size_t nBytes = v.m_nTotal * m.m_elementBytes; // allow seq of zero items
	if (nBytes) { // if its a zero-length sequence, we do nothing at all.
          if (m.m_isSequence)
            nBytes += std::max(sizeof(uint32_t), m.m_dataAlign);
	  uint8_t *alloc = NULL; // if allocation is needed due to uncached.
	  const uint8_t *data = getData(info, cache, dirty, mOffset, alloc, nBytes);
	  size_t length = nBytes;
	  // The writer creates its own value objects...
	  // FIXME: use more ValueWriter functionality for this whole method
	  OU::Value *vp = NULL;
	  OU::ValueWriter writer(&vp, 1);
	  m.write(writer, data, length, true); // is top-level
	  assert(length < m.m_dataAlign); // padding at end may not be taken
	  delete [] alloc;  // deallocate if not null
	  v = *vp; // this object copy is unfortunate - the way the writer works for now
	  delete vp;
	}
      } else if (m.m_baseType == OA::OCPI_String) {
	// FIXME: modularity violation
	size_t length = m.m_stringLength + 1;
	v.m_stringSpace = new char[length];
	v.m_String = v.m_stringSpace;
	uint8_t *data = (uint8_t *)v.m_stringSpace;
	getData(info, cache, dirty, mOffset, data, length);
      } else {
	uint8_t *data = &v.m_UChar;
	getData(info, cache, dirty, mOffset, data, 0, m.m_nBits);
      }
    }
    bool Worker::
    getProperty(unsigned ordinal, std::string &a_name, std::string &value, bool *unreadablep,
		bool hex, bool *cachedp, bool uncached, bool *hiddenp) {
      unsigned nProps;
      OU::Property *props = properties(nProps);
      if (ordinal >= nProps)
	return false;
      const OA::PropertyInfo &p = props[ordinal];
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
      if (hiddenp)
	*hiddenp = p.m_isHidden;
      if (p.m_isParameter) {
	p.m_default->unparse(value, NULL, false, hex);
	return true;
      }
      OA::PropertyAttributes attrs;
      getProperty(p, value, p, 0, 0,
		  OA::PropertyOptionList({ hex ? OA::HEX : OA::NONE,
			uncached ? OA::UNCACHED : OA::NONE,
			unreadablep ? OA::UNREADABLE_OK : OA::NONE}),
		  &attrs);
      if (attrs.isUnreadable && unreadablep)
	*unreadablep = true;
      return true;
    }
    // batch setting with lots of error checking - all or nothing
    void Worker::setProperties(const OA::PValue *props) {
      if (props)
	for (const OA::PValue *p = props; p->name; p++) {
	  OA::Property prop(*this, p->name); // exception goes out
	  prop.checkTypeAlways(prop.m_info, p->type, 1, true);
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
	  PortMask mandatory = ~(~0u << m_nPorts) & ~optionalPorts();
	  PortMask bad = mandatory & ~connectedPorts();
	  for (unsigned n = 0; n < m_nPorts; n++)
	    if (bad & (1u << n)) {
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
    bool Worker::beforeStart() const {
      return getControlState() == INITIALIZED;
    }
    bool Worker::isDone() {
      switch(getControlState()) {
      case FINISHED:
	return true;
      case UNUSABLE:
	throw OU::Error("Worker \"%s\" is now unusable", name().c_str());
      default:
	return false;
      }
    }
    bool Worker::wait(OCPI::OS::Timer *timer) {
      Container &c = application()->container();
      ocpiDebug("Waiting1 for \"done\" worker! %p %u %u %u", &c, c.enabled(), isDone(), getControlState());
      while (!isDone()) {
	ocpiDebug("Waiting for \"done\" worker! %p %u", &c, c.enabled());
	if (!c.enabled())
	  throw OU::Error("Container \"%s\" for worker \"%s\" was shutdown",
			  c.name().c_str(), cname());
	OS::sleep(10);
	if (timer && timer->expired())
	  return true;
      }
      return false;
    }

    const OA::PropertyInfo &
    Worker::checkInfo(unsigned ordinal) const {
      const OA::PropertyInfo &pi = m_properties[ordinal];
      if (pi.m_isDebug && !isDebug())
	throw OU::Error("Cannot access debug property: \"%s\" in worker \"%s\", which is not in "
			"debug mode", pi.cname(), cname());
      return pi;
    }

#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
    run Worker::							\
    get##pretty##PropertyOrd(unsigned ordinal, unsigned idx) const {	\
      auto pi = checkInfo(ordinal);					\
      return get##pretty##Property(pi, pi, (size_t)0, idx);		\
    }									\
    void Worker::							\
    set##pretty##PropertyOrd(unsigned ordinal, run val, unsigned idx) const { \
      auto pi = checkInfo(ordinal);					\
      set##pretty##Property(pi, pi, 0, val, idx);			\
    }									\
    run Worker::							\
    get##pretty##Parameter(unsigned ordinal, unsigned idx) const {	\
      OU::Property &p = m_properties[ordinal];				\
      assert(p.m_default);						\
      OU::Value &v = *p.m_default;					\
      return p.m_isSequence || p.m_arrayRank ? v.m_p##pretty[idx] : v.m_##pretty; \
    } \
    void Worker::							\
    set##pretty##Cached(const OCPI::API::PropertyInfo &info, const OCPI::Util::Member &m, \
			size_t offset, run val, unsigned idx) const {	\
      Cache *cache = getCache(info, offset, m);				\
      setData(info, cache, offset + idx * m.m_elementBytes, (uint8_t*)&val, 0, m.m_nBits); \
    }									\
    run Worker::							\
    get##pretty##Cached(const OCPI::API::PropertyInfo &info, const OCPI::Util::Member &m, \
			size_t offset, unsigned idx) const {;		\
      bool dirty;							\
      Cache *cache = getCache(info, offset, m, &dirty);			\
      run val;								\
      uint8_t *data = (uint8_t *)&val;					\
      getData(info, cache, dirty, offset + idx * m.m_elementBytes, data, 0, m.m_nBits); \
      return val;							\
    }									\
    void Worker::							\
    set##pretty##SequenceCached(const OCPI::API::PropertyInfo &info, const run *vals, size_t n) const { \
      Cache *cache = getCache(info, 0, info);				\
      setData(info, cache, 0, (uint8_t*)vals, n * info.m_elementBytes, 0, info.m_dataAlign, n); \
    }									\
    unsigned Worker::							\
    get##pretty##SequenceCached(const OCPI::API::PropertyInfo &info, run *vals, size_t n) const { \
      bool dirty;							\
      Cache *cache = getCache(info, 0, info, &dirty);			\
      uint32_t nElements = OCPI_UTRUNCATE(uint32_t, n);			\
      uint8_t *data = (uint8_t *)&nElements;				\
      if (info.m_isSequence) {						\
	getData(info, cache, dirty, 0, data, 0, 32);			\
	if (nElements > n)						\
	  throw OU::Error("Buffer size (%zu items) for getting the %s sequence property is too small " \
			  "(need %zu)", n, info.cname(), (size_t)nElements); \
      }									\
      data = (uint8_t *)vals;						\
      getData(info, cache, dirty, 0, data, nElements * info.m_elementBytes); \
      return OCPI_UTRUNCATE(unsigned, nElements);			\
    }

#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE

    void Worker::
    setStringPropertyOrd(unsigned ordinal, const char *s, unsigned idx) const {
      checkInfo(ordinal);
      setStringProperty(m_properties[ordinal], m_properties[ordinal], 0, s, idx);
    }
    void Worker::
    getStringPropertyOrd(unsigned ordinal, char *str, size_t length, unsigned idx) const {
      checkInfo(ordinal);
      getStringProperty(m_properties[ordinal], m_properties[ordinal], 0, str, length, idx);
    }
    void Worker::
    setStringCached(const OCPI::API::PropertyInfo &info, const OCPI::Util::Member &m,
		    size_t offset, const char *val, unsigned idx) const {
      size_t len = strlen(val);
      if (len > m.m_stringLength)
	throw OU::Error("String value for %s property too long (%zu vs. %zu)",
			m.cname(), len, m.m_stringLength);
      Cache *cache = getCache(info, offset, m);
      setData(info, cache, offset + idx * m.m_elementBytes, (uint8_t*)val, len + 1);
    }
    const char *Worker::
    getStringCached(const OCPI::API::PropertyInfo &info, const OCPI::Util::Member &m,
		    size_t offset, char *buf, size_t a_len, unsigned idx) const {
      if (a_len < m.m_stringLength + 1)
	throw OU::Error("String value for %s property too long for buffer (%zu vs. %zu)",
			m.cname(), a_len, m.m_stringLength + 1);
      bool dirty;
      Cache *cache = getCache(info, offset, m, &dirty);
      uint8_t *data = (uint8_t *)buf;
      // FIXME: like elsewhere, getData could have a string flag to only copy until null.
      getData(info, cache, dirty, offset + idx * m.m_elementBytes, data, m.m_stringLength + 1);
      return buf;
    }
    void Worker::
    setStringSequenceCached(const OCPI::API::PropertyInfo &info, const char * const *vals, size_t n) const {
      Cache *cache = getCache(info, 0, info);				\
      if (info.m_isSequence && n > info.m_sequenceLength)
	throw OU::Error("String sequence value for %s property has too many items (%zu vs. %zu)",
			info.cname(), n, info.m_sequenceLength);
      uint32_t nElements = OCPI_UTRUNCATE(uint32_t, n);
      for (size_t offset = info.m_dataAlign; n; offset += info.m_elementBytes, n--, vals++) {
	size_t len = strlen(*vals);
	if (len > info.m_stringLength)
	  throw OU::Error("String value in sequence for %s property too long (%zu vs. %zu)",
			  info.cname(), len, info.m_stringLength);
	setData(info, cache, offset, (uint8_t *)*vals, len+1);
      }
      if (info.m_isSequence)
	setData(info, cache, 0, (uint8_t*)&nElements, 0, sizeof(uint32_t) * 8);
    }
    unsigned Worker::
    getStringSequenceCached(const OCPI::API::PropertyInfo &info, char **vals, size_t n,
				char *buf, size_t space) const {
      bool dirty;
      Cache *cache = getCache(info, 0, info, &dirty);
      uint32_t nElements = OCPI_UTRUNCATE(uint32_t, n);
      uint8_t *data = (uint8_t *)&nElements;
      if (info.m_isSequence) {
	getData(info, cache, dirty, 0, data, 0, 32);
	if (nElements > n)
	  throw OU::Error("Buffer size (%zu items) for getting the %s sequence property is too small "
			  "(need %zu)", n, info.cname(), (size_t)nElements);
      } else if (n < info.m_arrayDimensions[0])
	  throw OU::Error("Buffer size (%zu items) for getting the %s array property is too small "
			  "(need %zu)", n, info.cname(), info.m_arrayDimensions[0]);
      data = (uint8_t *)buf;
      for (size_t offset = info.m_dataAlign; nElements; offset += info.m_elementBytes, nElements--, vals++) {
	if (space < info.m_elementBytes)
	  throw OU::Error("Buffer string space too small for array of strings in the %s array property",
			  info.cname());
	getData(info, cache, dirty, offset, data, info.m_elementBytes);
	size_t len = strlen(buf) + 1;
	space -= len;
	data += len;
      }
      return nElements;
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

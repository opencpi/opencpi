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

#include <stdint.h>
#include <pthread.h>
// This is obviously temporary
#ifdef __APPLE__
#include "../../../util/pwq/src/platform.c"
#endif
#include "OcpiOsAssert.h"
#include "OcpiUtilCDR.h"
#include "Container.h"
#include "ContainerPort.h"

#define CAS __sync_bool_compare_and_swap
namespace OCPI {
  namespace Container {
    namespace OA = OCPI::API;
    namespace OU = OCPI::Util;
    namespace OD = OCPI::DataTransport;
    namespace OR = OCPI::RDT;

    PortData::
    PortData(const OU::Port &mPort, bool a_isProvider, PortConnectionDesc *desc)
      : m_ordinal(mPort.m_ordinal), m_isProvider(a_isProvider), m_connectionData(desc)
    {
      OR::Descriptors &d = getData().data;
      d.type = m_isProvider ? OR::ConsumerDescT : OR::ProducerDescT;
      d.role = OR::NoRole;
      d.options = 0;
      bzero((void *)&d.desc, sizeof(d.desc));
      size_t l_nBuffers =
	mPort.m_defaultBufferCount == SIZE_MAX ? DEFAULT_NBUFFERS : mPort.m_defaultBufferCount;
      m_nBuffers = l_nBuffers > mPort.m_minBufferCount ? l_nBuffers : mPort.m_minBufferCount;
      // FIXME: this is really set at connection time and should be removed here and left
      // == SIZE_MAX
      if (mPort.m_bufferSize == SIZE_MAX) {
	m_bufferSize = OU::DEFAULT_BUFFER_SIZE;
	ocpiDebug("PortData %s(%p): setting buffer size from default: %zu",
		  mPort.m_name.c_str(), this, OU::DEFAULT_BUFFER_SIZE);
      } else {
	m_bufferSize = mPort.m_bufferSize;
	ocpiDebug("PortData %s(%p): setting buffer size from metadata: %zu",
		  mPort.m_name.c_str(), this, mPort.m_bufferSize);
      }
      d.desc.nBuffers = OCPI_UTRUNCATE(uint32_t, m_nBuffers);
      d.desc.dataBufferSize = OCPI_UTRUNCATE(uint32_t, m_bufferSize);
      ocpiDebug("PortData final nbuffers %zu bufsize %zu", m_nBuffers, m_bufferSize);
    }

    static bool findRole(const OU::PValue *params, const char *&s, OR::PortRole &role) {
      if (OU::findString(params, "transferRole", s)) {
	if (!strcasecmp(s, "passive"))
	  role = OR::Passive;
	else if (!strcasecmp(s, "active") ||
		 !strcasecmp(s, "activemessage"))
	  role = OR::ActiveMessage;
	else if (!strcasecmp(s, "flowcontrol") ||
		 !strcasecmp(s, "activeflowcontrol"))
	  role = OR::ActiveFlowControl;
	else if (!strcasecmp(s, "activeonly"))
	  role = OR::ActiveOnly;
	else
	  throw OU::Error("transferRole property must be passive|active|flowcontrol|activeonly");
	return true;
      }
      return false;
    }

    ExternalBuffer::
    ExternalBuffer(BasicPort &a_port, ExternalBuffer *a_next, unsigned n)
      : m_port(a_port), m_full(false), m_busy(false), m_position(n), m_next(a_next),
	m_zcHead(NULL), m_zcTail(NULL), m_zcNext(NULL), m_zcHost(NULL), m_dtBuffer(NULL),
	m_dtData(NULL) {
      memset(&m_hdr, 0, sizeof(m_hdr));
      pthread_spin_init(&m_zcLock, PTHREAD_PROCESS_PRIVATE);
    }

    size_t ExternalBuffer::offset() {
      return m_position * m_port.m_bufferStride;
    }

    // ZC put
    void ExternalBuffer::
    put(ExternalPort &port) {
      ocpiDebug("putting buffer %p zc to port %p", this, &port);
      if (&port == &m_port)
	put();
      else
	port.put(*this);
    }

    // ZC put
    void ExternalBuffer::
    put(ExternalPort &port, size_t a_length, uint8_t a_opCode, bool a_end, size_t a_direct) {
      m_hdr.m_length = OCPI_UTRUNCATE(uint32_t, a_length);
      m_hdr.m_opCode = a_opCode;
      m_hdr.m_eof    = a_end ? 1 : 0;
      m_hdr.m_direct = OCPI_UTRUNCATE(uint8_t, a_direct);
      put(port);
    }

    BasicPort::
    BasicPort(Container &c, const OU::Port &mPort, bool a_isProvider, const OU::PValue *params)
      : PortData(mPort, a_isProvider, NULL), m_lastInBuffer(NULL), m_lastOutBuffer(NULL),
	m_dtLastBuffer(NULL), m_dtPort(NULL), m_allocation(NULL), m_bufferStride(0),
	m_next2write(NULL), m_next2put(NULL), m_next2read(NULL), m_next2release(NULL),
	m_forward(NULL), m_backward(NULL), m_nRead(0), m_nWritten(0),
	myDesc(getData().data.desc), m_metaPort(mPort), m_container(c) {
      applyPortParams(params);
    }

    BasicPort::
    ~BasicPort() {
      // Note the mode derived class must lock this explicitly
      //OU::SelfAutoMutex guard(this);
      if (m_backward) {
	// If we are being forwarded-to, we need to break this chain, while
	// the other side is not in the middle of forwarding to us.
	assert(m_backward->m_forward == this);
	OU::SelfAutoMutex guard(m_backward);
	m_backward->m_forward = NULL;
	m_backward = NULL; // just to be clean
      } else if (m_forward) {
	assert(m_forward->m_backward == this);
	OU::SelfAutoMutex guard(m_forward);
	m_forward->m_backward = NULL;
	m_forward = NULL; // just to be clean
      }
      if (m_dtPort)
	m_dtPort->reset();
      if (m_allocation && m_allocator == this)
	freeBuffers(m_allocation);
      delete m_dtLastBuffer;
    }

    void BasicPort::
    applyPortParams(const OU::PValue *params) {
      OA::ULong ul;
      if (OU::findULong(params, "bufferCount", ul)) {
	if (ul < m_metaPort.m_minBufferCount)
	  throw OU::Error("bufferCount is below worker's minimum");
        else {
	  getData().data.desc.nBuffers = OCPI_UTRUNCATE(uint32_t, (m_nBuffers = ul));
	  ocpiDebug("Setting nbuffers from parameters on \"%s\" to %zu",
		    name().c_str(), m_nBuffers);
	}
      }
    }

    /*
     * ----------------------------------------------------------------------
     * A simple test.
     * ----------------------------------------------------------------------
     */
    /*
      static int
      pack_unpack_test (int argc, char *argv[])
      {
      OR::Descriptors d;
      std::string data;
      bool good;

      std::memset (&d, 0, sizeof (OR::Descriptors));
      d.mode = OR::ConsumerDescType;
      d.desc.c.fullFlagValue = 42;
      std::strcpy (d.desc.c.oob.oep, "Hello World");
      data = packDescriptor (d);
      std::memset (&d, 0, sizeof (OR::Descriptors));
      good = unpackDescriptor (data, d);
      ocpiAssert (good);
      ocpiAssert (d.mode == OR::ConsumerDescType);
      ocpiAssert (d.desc.c.fullFlagValue == 42);
      ocpiAssert (std::strcmp (d.desc.c.oob.oep, "Hello World") == 0);

      std::memset (&d, 0, sizeof (OR::Descriptors));
      d.mode = OR::ProducerDescType;
      d.desc.p.emptyFlagValue = 42;
      std::strcpy (d.desc.p.oob.oep, "Hello World");
      data = packDescriptor (d);
      std::memset (&d, 0, sizeof (OR::Descriptors));
      good = unpackDescriptor (data, d);
      ocpiAssert (good);
      ocpiAssert (d.mode == OR::ProducerDescType);
      ocpiAssert (d.desc.p.emptyFlagValue == 42);
      ocpiAssert (std::strcmp (d.desc.p.oob.oep, "Hello World") == 0);

      data[0] = ((data[0] == '\0') ? '\1' : '\0'); // Hack: flip byteorder
      good = unpackDescriptor (data, d);
      ocpiAssert (!good);

      return 0;
      }
    */
    static void putOffset(OU::CDR::Encoder &packer, DtOsDataTypes::Offset val) {
      packer.
#if OCPI_EP_SIZE_BITS == 64
      putULongLong(val);
#else
      putULong(val);
#endif
    }
    static void putFlag(OU::CDR::Encoder &packer, DtOsDataTypes::Flag val) {
      packer.
#if OCPI_EP_FLAG_BITS == 64
      putULongLong(val);
#else
      putULong(val);
#endif
    }
    static void getOffset(OU::CDR::Decoder &unpacker, DtOsDataTypes::Offset &val) {
      unpacker.
#if OCPI_EP_SIZE_BITS == 64
      getULongLong(val);
#else
      getULong(val);
#endif
    }
    static void getFlag(OU::CDR::Decoder &unpacker, DtOsDataTypes::Flag &val) {
      unpacker.
#if OCPI_EP_FLAG_BITS == 64
      getULongLong(val);
#else
      getULong(val);
#endif
    }

    void BasicPort::
    packPortDesc(const OR::Descriptors & desc, std::string &out) throw() {
      ocpiDebug("Packing desc %p into %p %p %zu %zu",
		&desc, &out, out.data(), out.length(), out.capacity());
      OU::CDR::Encoder packer;
      packer.putBoolean (OU::CDR::nativeByteorder());
      packer.putULong     (desc.type);
      packer.putULong     (desc.role);
      packer.putULong     (desc.options);
      const OR::Desc_t & d = desc.desc;
      packer.putULong     (d.nBuffers);
      putOffset(packer, d.dataBufferBaseAddr);
      packer.putULong     (d.dataBufferPitch);
      packer.putULong     (d.dataBufferSize);
      putOffset(packer, d.metaDataBaseAddr);
      packer.putULong     (d.metaDataPitch);
      putOffset(packer, d.fullFlagBaseAddr);
      packer.putULong     (d.fullFlagSize);
      packer.putULong     (d.fullFlagPitch);
      putFlag(packer, d.fullFlagValue);
      putOffset(packer, d.emptyFlagBaseAddr);
      packer.putULong     (d.emptyFlagSize);
      packer.putULong     (d.emptyFlagPitch);
      putFlag(packer, d.emptyFlagValue);
      packer.putULongLong (d.oob.port_id);
      packer.putString    (d.oob.oep);
      packer.putULongLong (d.oob.cookie);
      packer.putULongLong (d.oob.address);
      out = packer.data();
      ocpiDebug("Packed desc %p into %p %p %zu %zu",
		&desc, &out, out.data(), out.length(), out.capacity());
    }

    bool BasicPort::
    unpackPortDesc(const std::string &data, OR::Descriptors &desc) throw () {
      OU::CDR::Decoder unpacker (data);

      try {
	bool bo;
	unpacker.getBoolean (bo);
	unpacker.byteorder (bo);
        unpacker.getULong (desc.type);
        unpacker.getLong (desc.role);
        unpacker.getULong (desc.options);
	OR::Desc_t & d = desc.desc;
	unpacker.getULong     (d.nBuffers);
	getOffset(unpacker, d.dataBufferBaseAddr);
	unpacker.getULong     (d.dataBufferPitch);
	unpacker.getULong     (d.dataBufferSize);
	getOffset(unpacker, d.metaDataBaseAddr);
	unpacker.getULong     (d.metaDataPitch);
	getOffset(unpacker, d.fullFlagBaseAddr);
	unpacker.getULong     (d.fullFlagSize);
	unpacker.getULong     (d.fullFlagPitch);
	getFlag(unpacker, d.fullFlagValue);
	getOffset(unpacker, d.emptyFlagBaseAddr);
	unpacker.getULong     (d.emptyFlagSize);
	unpacker.getULong     (d.emptyFlagPitch);
	getFlag(unpacker, d.emptyFlagValue);
	unpacker.getULongLong (d.oob.port_id);
        std::string oep;
	unpacker.getString (oep);
        if (oep.length()+1 > sizeof(d.oob.oep))
          return false;
	unpacker.getULongLong (d.oob.cookie);
	unpacker.getULongLong (d.oob.address);
        std::strcpy (d.oob.oep, oep.c_str());
      }
      catch (const OU::CDR::Decoder::InvalidData &) {
	return false;
      }
      return true;
    }

    static void defaultRole(OR::PortRole &role, unsigned options) {
      if (role == OR::NoRole) {
	for (unsigned n = 0; n < OR::MaxRole; n++)
	  if (options & (1 << n)) {
	    role = (OR::PortRole)n;
	    return;
	  }
	throw OU::Error("Container port has no transfer roles");
      }
    }

    // coming in, specified roles are preferences or explicit instructions.
    // The existing settings are either NoRole, a preference, or a mandate
    // static method
    const char *BasicPort::
    chooseRoles(OR::PortRole &uRole, unsigned &uOptions, OR::PortRole &pRole, unsigned &pOptions)
    {
      // FIXME this relies on knowledge of the values of the enum constants
      static OR::PortRole otherRoles[] = { OCPI_RDT_OTHER_ROLES };
      defaultRole(uRole, uOptions);
      defaultRole(pRole, pOptions);
      OR::PortRole
        pOther = otherRoles[pRole],
        uOther = otherRoles[uRole];
      if (uOptions & (1 << OCPI::RDT::FlagIsMeta)) {
	if (!(pOptions & ((1 << OCPI::RDT::FlagIsMeta) | (1 << OCPI::RDT::FlagIsMetaOptional))))
	  return "Incompatible Metadata mode: input side cannot do flag-is-meta, output must";
	pOptions |= (1 << OCPI::RDT::FlagIsMeta);
      } else if (uOptions & (1 << OCPI::RDT::FlagIsMetaOptional)) {
	if (pOptions & ((1 << OCPI::RDT::FlagIsMeta) | (1 << OCPI::RDT::FlagIsMetaOptional))) {
	  pOptions |= (1 << OCPI::RDT::FlagIsMeta);
	  uOptions |= (1 << OCPI::RDT::FlagIsMeta);
	} else
	  uOptions &= ~(1 << OCPI::RDT::FlagIsMeta);
      } else if (pOptions & (1 << OCPI::RDT::FlagIsMeta))
	return "Incompatible Metadata mode: output side cannot do flag-is-meta, input must";
      if (pOptions & (1 << OR::MandatedRole)) {
        // provider has a mandate
        ocpiAssert(pRole != OR::NoRole);
        if (uRole == pOther)
          return NULL;
        if (uOptions & (1 << OR::MandatedRole))
          return "Incompatible mandated transfer roles";
        if (uOptions & (1 << pOther)) {
          uRole = pOther;
          return NULL;
        }
        return "No compatible role available against mandated role";
      } else if (pRole != OR::NoRole) {
        // provider has a preference
        if (uOptions & (1 << OR::MandatedRole)) {
          // user has a mandate
          ocpiAssert(uRole != OR::NoRole);
          if (pRole == uOther)
            return NULL;
          if (pOptions & (1 << uOther)) {
            pRole = uOther;
            return NULL;
          }
          throw OU::Error("No compatible role available against mandated role");
        } else if (uRole != OR::NoRole) {
          // We have preferences on both sides, but no mandate
          // If preferences match, all is well
          if (pRole == uOther)
            return NULL;
          // If one preference is against push, we better listen to it.
          if (uRole == OR::ActiveFlowControl &&
              pOptions & (1 << OR::ActiveMessage)) {
            pRole = OR::ActiveMessage;
            return NULL;
          }
          // Let's try active push if we can
          if (uRole == OR::ActiveMessage &&
              pOptions & (1 << OR::ActiveFlowControl)) {
            pRole = OR::ActiveFlowControl;
            return NULL;
          }
          if (pRole == OR::ActiveFlowControl &&
              uOptions & (1 << OR::ActiveMessage)) {
            uRole = OR::ActiveFlowControl;
            return NULL;
          }
          // Let's try activeonly push if we can
          if (uRole == OR::ActiveOnly &&
              pOptions & (1 << OR::Passive)) {
            pRole = OR::Passive;
            return NULL;
          }
          if (pRole == OR::Passive &&
              pOptions & (1 << OR::ActiveOnly)) {
            pRole = OR::ActiveOnly;
            return NULL;
          }
          // Let's give priority to the "better" role.
          if (uRole < pRole &&
              pOptions & (1 << uOther)) {
            pRole = uOther;
            return NULL;
          }
          // Give priority to the provider
          if (uOptions & (1 << pOther)) {
            uRole = pOther;
            return NULL;
          }
          if (pOptions & (1 << uOther)) {
            pRole = uOther;
            return NULL;
          }
          // Can't use either preference.  Fall throught to no mandates, no preferences
        } else {
          // User role unspecified, but provider has a preference
          if (uOptions & (1 << pOther)) {
            uRole = pOther;
            return NULL;
          }
          // Can't use provider preference, Fall through to no mandates, no preferences
        }
      } else if (uOptions & (1 << OR::MandatedRole)) {
        // Provider has no mandate or preference, but user has a mandate
        if (pOptions & (1 << uOther)) {
          pRole = uOther;
          return NULL;
        }
        return "No compatible role available against mandated role";
      } else if (uRole != OR::NoRole) {
        // Provider has no mandate or preference, but user has a preference
        if (pOptions & (1 << uOther)) {
          pRole = uOther;
          return NULL;
        }
        // Fall through to no mandates, no preferences.
      }
      // Neither has useful mandates or preferences.  Find anything, biasing to push
      for (unsigned i = 0; i < OR::MaxRole; i++)
        // Provider has no mandate or preference
        if (uOptions & (1 << i) &&
            pOptions & (1 << otherRoles[i])) {
          uRole = (OR::PortRole)i;
          pRole = otherRoles[i];
          return NULL;
        }
      return "No compatible combination of roles exist";
    }
    // Figure out the transport, the interconnect instance id, and roles for a connection.
    // Port parameters override connection parameters that act as a default for all
    // ports on the connection.
    // static method
    void BasicPort::
    determineTransport(const Transports &in, const Transports &out,
		       const OU::PValue *paramsIn, const OU::PValue *paramsOut,
		       const OU::PValue *paramsConn, Transport &transport) {
      // Giving priority to the input side in a tie, find the first transport that
      // both sides support that have compatible roles.
      static const char *roleNames[] = { OCPI_RDT_ROLE_NAMES };
      OR::PortRole roleIn = OR::NoRole, roleOut = OR::NoRole;
      const char *s;
      findRole(paramsIn, s, roleIn); // overrides per port for xfer role
      findRole(paramsOut, s, roleOut);
      const char *err = NULL, *tIn, *tOut, *tConn;
      std::string sIn, sOut, sConn;
      if (OU::findString(paramsConn, "transport", tConn)) {
	if (!strchr(tConn, '-'))
	  OU::format(sConn, "ocpi-%s-rdma", tConn);
	else
	  sConn = tConn;
      }
      if (OU::findString(paramsIn, "transport", tIn)) {
	if (!strchr(tIn, '-'))
	  OU::format(sIn, "ocpi-%s-rdma", tIn);
	else
	  sIn = tIn;
      }
      if (OU::findString(paramsOut, "transport", tOut)) {
	if (!strchr(tOut, '-'))
	  OU::format(sOut, "ocpi-%s-rdma", tOut);
	else
	  sOut = tOut;
      }
      if (sConn.length()) {
	if ((sIn.length() && strcasecmp(sIn.c_str(), sConn.c_str())) ||
	    (sOut.length() && strcasecmp(sOut.c_str(), sConn.c_str())))
	  throw OU::Error("Inconsistent transports: connection \"%s\" out \"%s\" in \"%s\"",
			  tConn, tIn, tOut);
	sIn = sOut = sConn;
      } else if (sIn.length()) {
	if (sOut.length() && strcasecmp(sIn.c_str(), sOut.c_str()))
	  throw OU::Error("Inconsistent transports: out \"%s\" in \"%s\"",
			  tIn, tOut);
	sConn = sIn;
      } else
	sConn = sOut;
      for (unsigned ni = 0; ni < in.size(); ni++) {
	const Transport &it = in[ni];
	if (sConn.length() && strcasecmp(sConn.c_str(), it.transport.c_str()))
	  ocpiInfo("Rejecting input transport %s since %s was specified for the connection",
		   it.transport.c_str(), sConn.c_str());
	else if (roleIn != OR::NoRole && !((1 << roleIn) & it.optionsIn))
	  ocpiInfo("Rejecting input role %s for transport %s: container doesn't support it",
		   roleNames[roleIn], it.transport.c_str());
	else
	  // fall through - an acceptable possible input
	  for (unsigned no = 0; no < out.size(); no++) {
	    const Transport &ot = out[no];
	    if (strcasecmp(it.transport.c_str(), ot.transport.c_str()))
	      ;
	    else if (strcasecmp(it.id.c_str(), ot.id.c_str()))
	      ocpiInfo("Rejecting output transport %s since input id is %s but output id is %s",
		       ot.transport.c_str(), it.id.c_str(), ot.id.c_str());
	    else if (sConn.length() && strcasecmp(sConn.c_str(), ot.transport.c_str()))
	      ocpiInfo("Rejecting out transport %s since %s was specified for the connection",
		       ot.transport.c_str(), sConn.c_str());
	    else if (roleOut != OR::NoRole && !((1 << roleOut) & ot.optionsOut))
	      ocpiInfo("Rejecting input role %s for transport %s: container doesn't support it",
		       roleNames[roleOut], ot.transport.c_str());
	    else {
	      // Everything matches, negotiate roles
	      transport.optionsIn = it.optionsIn;
	      transport.optionsOut = ot.optionsOut;
	      transport.roleIn = it.roleIn;
	      transport.roleOut = ot.roleOut;
	      if (roleIn != OR::NoRole) {
		transport.roleIn = roleIn;
		transport.optionsIn |= 1 << OR::MandatedRole;
	      }
	      if (roleOut != OR::NoRole) {
		transport.roleOut = roleOut;
		transport.optionsOut |= 1 << OR::MandatedRole;
	      }
	      if ((err = chooseRoles(transport.roleOut, transport.optionsOut, transport.roleIn,
				     transport.optionsIn)))
		ocpiInfo("Rejecting transport %s since role support is incompatible: %s",
			 it.transport.c_str(), err);
	      else {
		transport.transport = it.transport;
		transport.id = it.id;
		transport.optionsIn |= (1 << OR::MandatedRole);
		transport.optionsOut |= (1 << OR::MandatedRole);
		ocpiInfo("Choosing transport %s/%s for connection with roles %s(0x%x)->%s(0x%x)",
			 it.transport.c_str(), it.id.c_str(), roleNames[transport.roleOut],
			 transport.optionsOut, roleNames[transport.roleIn], transport.optionsIn);
		return;
	      }
	    }
	  }
      }
      if (err)
	throw OU::Error("Error choosing transfer roles: %s", err);
      else
	throw OU::Error("No compatible transports for connection");
    }

    // Step 1: get an empty buffer to fill.
    ExternalBuffer *BasicPort::
    getEmptyBuffer() {
      ocpiDebug("getEmptyBuffer: w %p p %p f %p", &metaPort().metaWorker(), this, m_forward);
      if (m_forward)
	return m_forward->getEmptyBuffer();
      if (m_next2write) { // shim mode
	ocpiDebug("getempty: %p %p %p %u", &metaPort().metaWorker(), this, m_next2write, m_next2write->m_full);
	ExternalBuffer *b = m_next2write;
	if (!b->m_full && !b->m_busy) {
	  b->m_hdr.m_data = OCPI_UTRUNCATE(uint8_t,
					   sizeof(ExternalBuffer) -
					   OCPI_OFFSETOF(size_t, ExternalBuffer, m_hdr));
	  b->m_hdr.m_length = OCPI_UTRUNCATE(uint32_t, m_bufferSize);
	  m_next2write = b->m_next;
	  ocpiDebug("GetEmpty on %p returns %p", this, b);
	  b->m_busy = true;
	  return b;
	}
	return NULL;
      }
      size_t length;
      if (!m_dtLastBuffer)
	m_dtLastBuffer = new ExternalBuffer(*this, NULL, 0);
      if (m_dtPort &&
	  (m_dtLastBuffer->m_dtBuffer =
	   m_dtPort->getNextEmptyOutputBuffer(m_dtLastBuffer->m_dtData, length))) {
	m_dtLastBuffer->m_hdr.m_length = OCPI_UTRUNCATE(uint32_t, length);
	return m_dtLastBuffer;
      }
      return NULL;
    }

    // Step 1: high level API
    OA::ExternalBuffer *BasicPort::
    getBuffer(uint8_t *&data, size_t &length) {
      if (isProvider())
	throw OU::Error("getBuffer for output port called on input port \"%s\"",
			name().c_str());
      if ((m_forward ? m_forward : this)->m_lastOutBuffer)
	throw OU::Error("getBuffer called on output port \"%s\" without putting previous buffer",
			name().c_str());
      ExternalBuffer *b = getEmptyBuffer();
      if (b) {
	data = b->data();
	length = b->m_hdr.m_length;
	(m_forward ? m_forward : this)->m_lastOutBuffer = b;
      }
      return b;
    }

    void ExternalBuffer::
    send(size_t a_length, uint8_t a_opCode, bool a_end, size_t a_direct) {
      ocpiDebug("Sending buffer %p: l %zu o %u %u %zu data %x %x", this, a_length, a_opCode,
		a_end, a_direct,
		((uint32_t*)((uint8_t *)this + m_hdr.m_data))[0],
		((uint32_t*)((uint8_t *)this + m_hdr.m_data))[1]);
      m_hdr.m_length = OCPI_UTRUNCATE(uint32_t, a_length);
      m_hdr.m_opCode = a_opCode;
      m_hdr.m_eof    = a_end ? 1 : 0;
      m_hdr.m_direct = OCPI_UTRUNCATE(uint8_t, a_direct);
      put();
    }
    void BasicPort::
    putInternal(size_t length, uint8_t opCode, bool end, size_t direct) {
      if (!m_lastOutBuffer)
	throw OU::Error("put called on output port %s without a previous buffer",
			name().c_str());
      m_lastOutBuffer->send(length, opCode, end, direct);
      ocpiDebug("Putting (internal) on %p(f %p) buffer %p length %zu", this, m_forward,
		m_lastOutBuffer, length);
      m_lastOutBuffer = NULL;
    }
    // Step 2: (API/high level) put the next2put buffer, it is full
    void BasicPort::
    put(size_t length, uint8_t opCode, bool end, size_t direct) {
      ocpiDebug("Putting on port %p: l %zu o %u %u %zu", this, length, opCode, end, direct);
      if (isProvider())
	throw OU::Error("put of output port called on input port %s",
			name().c_str());
      (m_forward ? m_forward : this)->putInternal(length, opCode, end, direct);
    }
    // Step 2: API level put last buffer method on buffer object
    void ExternalBuffer::
    put(size_t a_length, uint8_t a_opCode, bool a_end, size_t a_direct) {
      m_port.putInternal(a_length, a_opCode, a_end, a_direct);
    }
    // Step 2: low level API on port
    void BasicPort::
    put() {
      ocpiDebug("Putting on port %p()", this);
      if (m_forward)
	m_forward->put();
      else
	m_next2put->put();
    }
    // Step 2: low level API on buffer
    void ExternalBuffer::
    put() {
      ocpiDebug("Putting %p port %p buffer %p (fwd %p) length %u",
		&m_port.metaPort().metaWorker(), &m_port, this, m_port.m_forward,
		m_hdr.m_length);
      ocpiDebug("Put words %x %x %x %x %x %x",
		((uint32_t*)this)[0],
		((uint32_t*)this)[1],
		((uint32_t*)this)[2],
		((uint32_t*)this)[3],
		((uint32_t*)this)[4],
		((uint32_t*)this)[5]);
      ocpiDebug("Put op %zu eof %zu data %zu direct %zu len %zu full %zu",
		&m_hdr.m_opCode - (uint8_t*)this,
		&m_hdr.m_eof - (uint8_t*)this,
		&m_hdr.m_data - (uint8_t*)this,
		&m_hdr.m_direct - (uint8_t*)this,
		(uint8_t*)&m_hdr.m_length - (uint8_t*)this,
		(uint8_t*)&m_full - (uint8_t*)this);

      m_full = true;
      m_busy = false;
      if (m_next) {
	m_port.m_nWritten++;
	assert(this == m_port.m_next2put);
	m_port.m_next2put = m_next;
      } else if (m_port.m_dtPort) {
	ocpiAssert(m_dtBuffer);
	m_port.m_dtPort->sendOutputBuffer(m_dtBuffer, m_hdr.m_length, m_hdr.m_opCode);
	m_dtBuffer = NULL;
      }
    }

    // Step 2: Standalone EOF.  Returns NULL if it can't go, just like getbuffer.
    bool BasicPort::
    endOfData() {
      if (isProvider())
	throw OU::Error("end of data for output port called on input port %s",
			name().c_str());
      if ((m_forward ? m_forward : this)->m_lastOutBuffer)
	throw OU::Error("end of data called on output port %s with a previous buffer",
			name().c_str());
      ExternalBuffer *b = getEmptyBuffer(); // might forward
      if (b) {
	if (m_dtPort) { // cannot be forwarded
	  ocpiAssert(m_dtLastBuffer && m_dtLastBuffer->m_dtBuffer);
	  m_dtPort->sendOutputBuffer(m_dtLastBuffer->m_dtBuffer, 0, 0, true, true);
	  m_dtLastBuffer->m_dtBuffer = NULL;
	} else {
	  b->m_hdr.m_length = 0;
	  b->m_hdr.m_opCode = 0;
	  b->m_hdr.m_eof = true;
	  b->m_hdr.m_data = 0; // standalone EOF
	  b->m_full = true;
	  m_nWritten++;
	}
	return true;
      }
      return false;
    }

    void BasicPort::
    put(OCPI::API::ExternalBuffer &buf, size_t len, uint8_t op, bool end, size_t direct) {
      ExternalBuffer &b = static_cast<ExternalBuffer&>(buf);
      b.m_hdr.m_length = OCPI_UTRUNCATE(uint32_t, len);
      b.m_hdr.m_opCode = op;
      b.m_hdr.m_eof = end ? 1 : 0;
      b.m_hdr.m_direct = OCPI_UTRUNCATE(uint8_t, direct);
      put(buf);
    }
    // The API level zero-copy put of another port's buffer to this port
    void BasicPort::
    put(OA::ExternalBuffer &buf) {
      ExternalBuffer &b = static_cast<ExternalBuffer&>(buf);
      ocpiDebug("port.put(buf %p) on %p forward %p", &buf, this, m_forward);
      if (m_forward)
	m_forward->put(buf);
      else if (&b.m_port == this)
	buf.put();
      else if (m_next2write) {
	ocpiDebug("Putting ZC buffer %p %p %p on host %p",
		  &metaPort().metaWorker(), this, &b, m_next2write);
	assert(&b.m_port != &m_next2write->m_port);
	b.m_zcNext = NULL;
	assert(b.m_zcHost == NULL);
	b.m_zcHost = m_next2write;
	b.m_full = true;
        pthread_spin_lock(&m_next2write->m_zcLock);
	if (m_next2write->m_zcTail)
	  m_next2write->m_zcTail->m_zcNext = &b;
	else
	  m_next2write->m_zcHead = &b;
	m_next2write->m_zcTail = &b;
	pthread_spin_unlock(&m_next2write->m_zcLock);
      } else if (m_dtPort && b.m_dtBuffer)
	m_dtPort->sendZcopyInputBuffer(*b.m_dtBuffer,
				       b.m_hdr.m_length, b.m_hdr.m_opCode, b.m_hdr.m_eof);
      else
	assert("No support yet for zery-copy send of shim buffer to external port"==0);
    }

    bool BasicPort::
    tryFlush() {
      if (isProvider())
	throw OU::Error("tryflush output port called on input port %s",
			name().c_str());
      if ((m_forward ? m_forward : this)->m_lastOutBuffer)
	throw OU::Error("tryFlush called on output port %s with a previous buffer",
			name().c_str());
      return
	(m_forward ? m_forward->m_nWritten - m_forward->m_nRead : m_nWritten - m_nRead) != 0;
    }

    // Step 3: high level
    OA::ExternalBuffer *BasicPort::
    getBuffer(uint8_t *&data, size_t &length, uint8_t &opCode, bool &end) {
      if (!isProvider())
	throw OU::Error("getBuffer for input port called on output port %s",
			name().c_str());
      if ((m_forward ? m_forward : this)->m_lastInBuffer)
	throw
	  OU::Error("getBuffer called on input port \"%s\" of worker \"%s\" without releasing "
		    "previous buffer", name().c_str(), metaPort().metaWorker().cname());
      ExternalBuffer *b = getFullBuffer();
      if (b) {
	data = b->data();
	length = b->m_hdr.m_length;
	opCode = b->m_hdr.m_opCode;
	end = b->m_hdr.m_eof;
	assert(!m_forward);
	m_lastInBuffer = b;
	//	(m_forward ? m_forward : this)->m_lastInBuffer = b;
      }
      return b;
    }

    ExternalBuffer *
    ExternalBuffer::zcPeek() {
      // A peek
      ocpiDebug("zcpeek buf %p head %p tail %p next %p", this, m_zcHead, m_zcTail, m_zcHead->m_zcNext);
      ExternalBuffer *l_next = NULL;
      if (m_zcHead) {
        pthread_spin_lock(&m_zcLock);
	if ((l_next = m_zcHead)) {
	  m_zcHead = l_next->m_zcNext;
	  if (l_next == m_zcTail)
	    m_zcTail = NULL;
	}
	pthread_spin_unlock(&m_zcLock);
      }
      return l_next;
    }
    // This buffer is the head->next, we are the single reader
    void ExternalBuffer::
    zcPop() {
      ocpiDebug("zcPop on %p host %p head %p headnext %p early next %p", this, m_zcHost,
		m_zcHost->m_zcHead,
		m_zcHost->m_zcHead ? m_zcHost->m_zcHead->m_zcNext : NULL, m_zcNext);
      //      m_zcHost->m_zcHead->m_zcNext = m_zcNext;
      m_zcHost = NULL;
    }

    // Step 3: low level
    ExternalBuffer *BasicPort::
    getFullBuffer() {
      assert(!m_forward);
      ExternalBuffer *b = m_next2read;
      ocpiDebug("getfull on %p early next %p dt %p", this, b, m_dtPort);
      if (b) { // if shim mode
	bool zc = false;
	do {
	  if (b->m_zcHead) {
	    ExternalBuffer *zcb = b->zcPeek();
	    if (zcb) {
	      zc = true;
	      zcb->zcPop();
	      b = zcb;
	      break;
	    }
	  }
	  if (b->m_full && !b->m_busy && !b->m_zcHost)
	    m_next2read = b->m_next;
	  else
	    return NULL;
	} while (0);
	ocpiDebug("getFull%s on %p %p returns %p  len %zu op %u", zc ? "ZC" : "",
		  &metaPort().metaWorker(), this, b, (size_t)(b->m_hdr.m_length),
		  b->m_hdr.m_opCode);
	b->m_busy = true;
	return b;
      }
      if (m_dtPort) {
	size_t length;
	if (!m_dtLastBuffer)
	  m_dtLastBuffer = new ExternalBuffer(*this, NULL, 0);
	if (!m_dtLastBuffer->m_dtBuffer &&
	    (m_dtLastBuffer->m_dtBuffer =
	     m_dtPort->getNextFullInputBuffer(m_dtLastBuffer->m_dtData, length,
					      m_dtLastBuffer->m_hdr.m_opCode)))
	  m_dtLastBuffer->m_hdr.m_length = OCPI_UTRUNCATE(uint32_t, length);
	if (m_dtLastBuffer->m_dtBuffer) {
	  m_dtLastBuffer->m_hdr.m_eof = false;
	  return m_dtLastBuffer;
	}
      }
      return NULL;
    }

    // Step 3: peek at next buffer to read
    bool BasicPort::
    peekOpCode(uint8_t &op) {
      if (m_forward)
	return m_forward->peekOpCode(op);
      if (m_next2read) { // if shim mode
	ExternalBuffer *b;
	if ((m_next2read->m_zcHead && (b = m_next2read->zcPeek())) ||
	    (b = m_next2read)->m_full) {
	  op = b->m_hdr.m_opCode;
	  return true;
	}
      } else if (m_dtPort) {
	size_t length;

	if (!m_dtLastBuffer)
	  m_dtLastBuffer = new ExternalBuffer(*this, NULL, 0);
	if (!m_dtLastBuffer->m_dtBuffer &&
	    (m_dtLastBuffer->m_dtBuffer =
	     m_dtPort->getNextFullInputBuffer(m_dtLastBuffer->m_dtData, length,
					      m_dtLastBuffer->m_hdr.m_opCode)))
	  m_dtLastBuffer->m_hdr.m_length = OCPI_UTRUNCATE(uint32_t, length);
	if (m_dtLastBuffer->m_dtBuffer) {
	  op = m_dtLastBuffer->m_hdr.m_opCode;
	  return true;
	}
      }
      return false;
    }

    // API level to take a buffer and allow for more, without releasing
    void ExternalBuffer::
    take() {
      m_port.takeBuffer(*this);
    }
    // This is the lower API
    void BasicPort::
    takeBuffer(ExternalBuffer &b) {
      ocpiDebug("takeBuffer: port %p last %p", this, m_lastInBuffer);
      assert(!m_forward);
      // if (m_forward)
      //   return m_forward->takeBuffer(b);
      if (!m_lastInBuffer)
	throw
	  OU::Error("take called on input port \"%s\" with no current buffer", name().c_str());
      if (&b != m_lastInBuffer)
	throw
	  OU::Error("release called on input port \"%s\" with the wrong buffer", name().c_str());
      if (!isProvider())
	throw OU::Error("take called on output port %s", name().c_str());
      assert(m_lastInBuffer);
      if (m_dtPort) {
	assert(m_lastInBuffer == m_dtLastBuffer);
	// our single dt buffer has been taken (held for release later)
	m_dtLastBuffer = NULL;
      }
      m_lastInBuffer = NULL;
    }

    // This is the lower API
    void BasicPort::
    releaseBuffer(ExternalBuffer &b) {
      assert(!m_forward);
      if (&b.m_port != this)               // buffer is zc queued buffer
	b.m_port.releaseBuffer(b);         // release from its true port
      else if (m_next2release) {
	assert(&b.m_port == this);
	ocpiAssert(&b == m_next2release); // want trace; having random problems on Jenkins
	assert(b.m_busy);
	b.m_full = false;
	b.m_busy = false;
	m_nRead++;
	m_next2release = b.m_next;
	ocpiDebug("Release on %p of %p head %p tail %p next %p", this, &b, b.m_zcHead, b.m_zcTail, b.m_zcNext);
	b.m_zcHead = b.m_zcTail = b.m_zcNext = b.m_zcHost = NULL;
      } else if (m_dtPort) {
	assert(&b.m_port == this);
	assert(b.m_dtBuffer);
	m_dtPort->releaseInputBuffer(b.m_dtBuffer);
	b.m_dtBuffer = NULL;
      }
      if (m_lastInBuffer == &b)
	m_lastInBuffer = NULL;
    }
    // Step 4: release input buffers, return them to empty state
    void BasicPort::
    release() {
      assert(!m_forward);
      //      if (m_forward)
      //	return m_forward->release();
      if (!isProvider())
	throw OU::Error("release called on output port %s", name().c_str());
      if (!m_lastInBuffer)
	throw OU::Error("release called on input port \"%s\" without a previous buffer",
			name().c_str());
      return releaseBuffer(*m_lastInBuffer);
    }

    // Step 4: release, API level on a particular buffer - perhaps not the current one
    void ExternalBuffer::
    release() {
#if 0
      if (!m_port.m_lastInBuffer)
	throw
	  OU::Error("release called on input port \"%s\" without releasing previous buffer",
		    m_port.name().c_str());
      if (this != m_port.m_lastInBuffer)
	throw
	  OU::Error("release called on input port \"%s\" with the wrong buffer",
		    m_port.name().c_str());
#endif
      m_port.releaseBuffer(*this);
    }

    void BasicPort::
    setBufferSize(size_t a_bufferSize) {
      m_bufferSize = a_bufferSize;
      getData().data.desc.dataBufferSize = OCPI_UTRUNCATE(uint32_t, m_bufferSize);
    }

    void BasicPort::
    applyConnection(const Transport &t, size_t a_bufferSize) {
      OR::Descriptors &d = getData().data;
      d.role = isProvider() ? t.roleIn : t.roleOut;
      d.options = isProvider() ? t.optionsIn : t.optionsOut;
      if (!d.desc.oob.oep[0])
	strcpy(d.desc.oob.oep, t.transport.c_str());
      assert(!strncmp(d.desc.oob.oep, t.transport.c_str(), strlen(t.transport.c_str())));
      setBufferSize(a_bufferSize);
    }

    uint8_t *BasicPort::
    allocateBuffers(size_t len) {
      return new uint8_t[len];
    }
    // This is virtual, but during destruction it gets called anyway, so we do the check.
    void BasicPort::
    freeBuffers(uint8_t *p) {
      if (m_allocator == this)
	delete [] p;
    }

    // Make this port into a SHIM - meaning there is no "connection",
    // just the front side (normal API for in-process worker ports)
    // and back side (external port API accessing the same buffers).
    // We do one allocation, where each ExternalBuffer is directly followed by its buffer
    // Alignment and allocation may be specialized
    void BasicPort::
    becomeShim(BasicPort *other) {
      size_t alignment = other ? other->bufferAlignment() : 0;
      alignment = std::max(alignment, bufferAlignment());
      // Align both the headers and the data.
      assert(m_bufferSize != SIZE_MAX);
      m_bufferStride = OU::roundUp(OU::roundUp(sizeof(ExternalBuffer), alignment) +
				   m_bufferSize, alignment);
      if (other && other->hasAllocator() && !hasAllocator()) {
	m_allocator = other;
	other->m_allocator = other;
      } else {
	assert(!other || !other->hasAllocator() ||
	       other->hasAllocator() == hasAllocator());
	m_allocator = this;
      }
      m_allocation = m_allocator->allocateBuffers(m_nBuffers * m_bufferStride);
      uint8_t *p = m_allocation;
      for (unsigned n = 0; n < m_nBuffers; n++, p += m_bufferStride) {
	ExternalBuffer *next =
	  (ExternalBuffer*)(n == m_nBuffers-1 ? m_allocation : p + m_bufferStride);
	new(p) ExternalBuffer(*this, next, n);
      }
      m_next2read = m_next2write = m_next2put = m_next2release = (ExternalBuffer*)m_allocation;
    }

    // Connect inside the same process
    void BasicPort::
    connectInProcess(Launcher::Connection &c, BasicPort &other) {
      if (&container() != &other.container() ||
	  !container().connectInside(*this, other)) {
	assert(m_bufferSize != SIZE_MAX);
	other.setBufferSize(m_bufferSize);
	becomeShim(&other);
	other.forward2shim(*this);
	portIsConnected();
	other.portIsConnected();
	c.m_in.m_done = c.m_out.m_done = true;
      }
    }

    size_t BasicPort::
    bufferAlignment() const { return OU::BUFFER_ALIGNMENT; }

    void BasicPort::
    forward2shim(BasicPort &shim) {
      assert(!m_forward);
      assert(!shim.m_backward);
      shim.m_backward = this;
      m_forward = &shim;
      m_bufferSize = shim.m_bufferSize;
    }

    // Connect inside the same process, but between containers.
    // The actual ports are not in this process (this cannot be shimmed).
    void BasicPort::
    connectLocal(BasicPort &other, Launcher::Connection *c) {
      BasicPort
	&in = isProvider() ? *this : other,
	&out = isProvider() ? other : *this;
      OR::Descriptors buf, buf1;
      bool inDone = false, outDone = false;
      const OR::Descriptors *result = in.startConnect(NULL, buf, inDone);
      assert(result);
      result = out.startConnect(result, buf1, outDone);
      assert((result && !inDone) || (!result && inDone));
      if (result) {
	result = in.finishConnect(result, buf, inDone);
	assert((result && !outDone) || (!result && outDone));
	if (result)
	  result = out.finishConnect(result, buf1, outDone);
      }
      assert(inDone && outDone && !result);
      if (c)
	c->m_in.m_done = c->m_out.m_done = true;
    }

    // return true if we need more info, false if we're done.
    // Even if we're done, we may have produced info for the other side,
    // and placed it in m_initial.
    bool BasicPort::
    startRemote(Launcher::Connection &c) {
      Launcher::Port
	&p = isProvider() ? c.m_in : c.m_out,
	&other = isProvider() ? c.m_out : c.m_in;
      assert(!p.m_done);
      OR::Descriptors buf, buf1, *otherInfo = NULL;
      ocpiDebug("StartRemote %p prov %u initial %p length %zu other %p %zu",
		this, isProvider(), &p.m_initial, p.m_initial.length(),
		&other.m_initial, other.m_initial.length());
      if (other.m_initial.length()) {
	ocpiCheck(unpackPortDesc(other.m_initial, buf));
	other.m_initial.clear();
	otherInfo = &buf;
      }
      const OR::Descriptors *result = startConnect(otherInfo, buf1, p.m_done);
      if (result) {
	p.m_started = true;
	ocpiDebug("pack0 %p prov %u initial %p length %zu data %p",
		  this, isProvider(), &p.m_initial, p.m_initial.length(), p.m_initial.data());
	packPortDesc(*result, p.m_initial);
	uint32_t *p32 = (uint32_t*)p.m_initial.data();
	ocpiDebug("pack1 %p prov %u initial %p length %zu data %p %x %x %x %x",
		  this, isProvider(), &p.m_initial, p.m_initial.length(),
		  p.m_initial.data(), p32[0], p32[1], p32[2], p32[3]);
      }
      return !p.m_done;
    }

    // This is "try to finish remote".
    // return true if more to do - we need more info
    bool BasicPort::
    finishRemote(Launcher::Connection &c) {
      OR::Descriptors buf, buf1;
      const OR::Descriptors *result;
      Launcher::Port
	&p = isProvider() ? c.m_in : c.m_out,
	&other = isProvider() ? c.m_out : c.m_in;
      assert(!p.m_done);
      ocpiDebug("finishRemote: %p %s i %zu f %zu", this, isProvider() ? "in" : "out",
		other.m_initial.length(), other.m_final.length());
      std::string &otherInfo = other.m_final.length() ? other.m_final : other.m_initial;
      ocpiCheck(unpackPortDesc(otherInfo, buf));
      otherInfo.clear();
      result = finishConnect(&buf, buf1, p.m_done);
      if (result) {
	packPortDesc(*result, p.m_started ? p.m_final : p.m_initial);
	p.m_started = true;
      }
      ocpiDebug("finishRemote: result %p done %u", result, p.m_done);
      return !p.m_done;
    }

    // Default local behavior for basic ports that need to behave like external or bridge ports
    const OCPI::RDT::Descriptors *BasicPort::
    startConnect(const OCPI::RDT::Descriptors *other, OCPI::RDT::Descriptors &feedback, bool &done) {
      if (isProvider())
	m_dtPort = container().getTransport().createInputPort(getData().data);
      else if (other)
	m_dtPort = container().getTransport().createOutputPort(getData().data, *other);
      if (m_dtPort) {
	// FIXME: put this in the constructor, and have better names
	m_dtPort->setInstanceName(m_metaPort.m_name.c_str());
	if (other)
	  return finishConnect(other, feedback, done);
	done = false;
	return &getData().data;
      }
      done = false;
      return NULL; // we got nuthin
    }

    const OCPI::RDT::Descriptors *BasicPort::
    finishConnect(const OCPI::RDT::Descriptors *other, OCPI::RDT::Descriptors &feedback,
		  bool &done) {
      ocpiDebug("finishConnect enter on '%s' other %p dtport %p",
		name().c_str(), other, m_dtPort);
      const OCPI::RDT::Descriptors *rv;
      if (!m_dtPort) {
	assert(!isProvider());
	assert(other);
	rv = startConnect(other, feedback, done);
      } else
	rv = m_dtPort->finalize(other, getData().data, &feedback, done);
      if (done)
	portIsConnected();
      ocpiDebug("finishConnect exit on '%s' rv %p done %u", name().c_str(), rv, done);
      return rv;
    }

    unsigned BasicPort::fullCount() {
      if (m_forward)
	return m_forward->fullCount();
      if (m_next2read->m_full) {
	unsigned r = m_next2read->m_position, p = m_next2put->m_position;
	return p + (p > r ? 0 : OCPI_UTRUNCATE(unsigned, m_nBuffers)) - r;
      }
      return 0;
    }
    unsigned BasicPort::emptyCount() {
      if (m_forward)
	return m_forward->emptyCount();
      if (!m_next2write->m_full) {
	unsigned w = m_next2write->m_position, r = m_next2release->m_position;
	return r + (r > w ? 0 : OCPI_UTRUNCATE(unsigned, m_nBuffers)) - w;
      }
      return 0;
    }
    OA::BaseType BasicPort::
    getOperationInfo(uint8_t opCode, size_t &nbytes) {
      OU::Operation *ops = m_metaPort.operations();
      if (ops && opCode < m_metaPort.nOperations() && ops[opCode].nArgs() == 1) {
	nbytes = ops[opCode].args()->m_elementBytes;
	return ops[opCode].args()->m_baseType;
      }
      return OA::OCPI_none;
    }
  } // end of namespace Container
} // end of namespace OCPI

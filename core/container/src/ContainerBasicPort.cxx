/*
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

#include "OcpiOsAssert.h"
#include "OcpiUtilCDR.h"
#include "Container.h"
//#include "ContainerWorker.h"
#include "ContainerPort.h"

namespace OCPI {
  namespace Container {
    namespace OA = OCPI::API;
    namespace OU = OCPI::Util;
    namespace OD = OCPI::DataTransport;
    namespace OR = OCPI::RDT;

    PortData::
    PortData(const OU::Port &mPort, PortConnectionDesc *desc)
      : m_ordinal(mPort.m_ordinal), m_isProvider(mPort.m_provider), m_connectionData(desc)
    {
      OR::Descriptors &d = getData().data;
      d.type = m_isProvider ? OR::ConsumerDescT : OR::ProducerDescT;
      d.role = OR::NoRole;
      d.options = 0;
      bzero((void *)&d.desc, sizeof(d.desc));
      size_t nBuffers =
	mPort.m_defaultBufferCount == SIZE_MAX ? DEFAULT_NBUFFERS : mPort.m_defaultBufferCount;
      d.desc.nBuffers = (uint32_t)(nBuffers > mPort.m_minBufferCount ?
				   nBuffers : mPort.m_minBufferCount);
      if (mPort.m_bufferSize == SIZE_MAX) {
	d.desc.dataBufferSize = OU::DEFAULT_BUFFER_SIZE;
	ocpiDebug("PortData %s(%p): setting buffer size from default: %zu",
		  mPort.m_name.c_str(), this, OU::DEFAULT_BUFFER_SIZE);
      } else {
	d.desc.dataBufferSize = (uint32_t)mPort.m_bufferSize;
	ocpiDebug("PortData %s(%p): setting buffer size from metadata: %zu",
		  mPort.m_name.c_str(), this, mPort.m_bufferSize);
      }
    }

    static bool findRole(const OU::PValue *params, const char *&s, OR::PortRole &role) {
      if (OU::findString(params, "xferRole", s)) {
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
	  throw OU::Error("xferRole property must be passive|active|flowcontrol|activeonly");
	return true;
      }
      return false;
    }

#if 0
    // Note that transports, roles, and buffer sizes are already settled.
    // Not much left, but we may extend this.
    // Set parameters for a port, whether at creation/construction time or later at connect time
    void PortData::setPortParams(const OU::Port &mPort, const OU::PValue *params) {
      OA::ULong ul;

      if (OU::findULong(params, "bufferCount", ul))
	if (ul < mPort.m_minBufferCount)
	  throw OU::Error("bufferCount is below worker's minimum");
        else
	  getData().data.desc.nBuffers = ul;
      if (OU::findULong(params, "bufferSize", ul))
	if (ul < mPort.m_minBufferSize)
	  throw OU::Error("bufferSize %u is below worker's minimum: %zu",
			  ul, mPort.m_minBufferSize);
        else {
	  getData().data.desc.dataBufferSize = ul;
	  ocpiDebug("Portdata %s(%p): setting buffer size from parameter: %zu",
		    mPort.m_name.c_str(), this, (size_t)ul);
	}
      
      OR::PortRole role;
      const char *s;
      if (findRole(params, s, role)) {
	if (!(getData().data.options & (1 << role)))
	  throw OU::Error("xferRole of \"%s\" not supported by port \"%s\"",
			  s, mPort.m_name.c_str());
	getData().data.role = role;
	getData().data.options |= (1 << OR::MandatedRole);
      }
      // deal with protocol and transport parameters
      if (OU::findString(params, "endpoint", s)) {
	if (!strchr(s, ':'))
	  throw OU::Error("endpoint parameter must have leading transport and colon");
	strncpy(getData().data.desc.oob.oep, s, sizeof(getData().data.desc.oob.oep));
      } else if ((s = getenv("OCPI_DEFAULT_TRANSPORT")) ||
		 OU::findString(params, "protocol", s) ||
		 OU::findString(params, "transport", s)) {
	ocpiDebug("Setting transport on port %s to %s", mPort.m_name.c_str(), s);
	if (strchr(s, ':'))
	  throw OU::Error("transport parameters cannot contain colons");
	strncpy(getData().data.desc.oob.oep, s, sizeof(getData().data.desc.oob.oep));
      }
    }
#endif

    ExternalBuffer::
    ExternalBuffer(BasicPort &port, ExternalBuffer *next)
      : m_port(port), m_full(false), m_eof(false), m_opCode(0), m_length(0), m_data(NULL),
	m_direct(0), m_next(next), m_zcBack(NULL), m_zcQueue(NULL), m_zcNext(NULL),
	m_dtBuffer(NULL) {
    }

    void ExternalBuffer::
    release() {
      if (m_next) {
	m_full = false;
	m_port.m_nRead++;
	if (this == m_port.m_lastRead)
	  m_port.m_lastRead = NULL;
      } else if (m_dtBuffer) {
	m_port.m_dtPort->releaseInputBuffer(m_dtBuffer);
	m_dtBuffer = NULL;
      }
    }
    void ExternalBuffer::
    put() {
      ocpiDebug("Putting buffer %p to port %p (fwd %p) length %zu", this, &m_port,
		m_port.m_forward, m_length);
      if (m_next) {
	m_data   = (uint8_t*)(this + 1);
	m_full   = true;
	m_port.m_nWritten++;
	if (this == m_port.m_lastWrite)
	  m_port.m_lastWrite = NULL;
      } else {
	ocpiAssert(m_dtBuffer);
	m_port.m_dtPort->sendOutputBuffer(m_dtBuffer, m_length, m_opCode);
	m_dtBuffer = NULL;
      }
    }
    void ExternalBuffer::
    put(size_t length, uint8_t opCode, bool end, size_t direct) {
      m_length = length;
      m_opCode = opCode;
      m_direct = direct;
      m_eof    = end;
      put();
    }
    // ZC put
    void ExternalBuffer::
    put(ExternalPort &port) {
      if (&port == &m_port)
	put();
      else
	port.put(*this);
    }

    // ZC put
    void ExternalBuffer::
    put(ExternalPort &port, size_t length, uint8_t opCode, bool end, size_t direct) {
      m_length = length;
      m_opCode = opCode;
      m_eof    = end;
      m_direct = direct;
      put(port);
    }

    BasicPort::
    BasicPort(Container &c, const OU::Port &metaData, const OU::PValue *params)
      : PortData(metaData, NULL), OU::SelfRefMutex(c), m_lastBuffer(*this, NULL),
	m_dtPort(NULL), m_allocation(NULL), m_bufferSize(SIZE_MAX), m_next2read(NULL),
	m_next2write(NULL), m_lastRead(NULL), m_lastWrite(NULL), m_forward(NULL),
	m_nRead(0), m_nWritten(0), myDesc(getData().data.desc), m_metaPort(metaData),
	m_container(c) {
      applyPortParams(params);
    }

    BasicPort::
    ~BasicPort(){
      if (m_dtPort)
	m_dtPort->reset();
      if (m_allocation)
	delete [] m_allocation;
    }

    void BasicPort::
    applyPortParams(const OU::PValue *params) {
      OA::ULong ul;
      if (OU::findULong(params, "bufferCount", ul))
	if (ul < m_metaPort.m_minBufferCount)
	  throw OU::Error("bufferCount is below worker's minimum");
        else
	  getData().data.desc.nBuffers = ul;
    }

#if 0
    void BasicPort::
    startConnect(const OR::Descriptors */*other*/, const OU::PValue *){} // default

    // Convert PValues into descriptor values, with metadata constraint checking
    void BasicPort::
    setConnectParams(const OU::PValue *params) {
      setPortParams(m_metaPort, params);
      // There are no connection parameters (yet) other than those that can be provided
      // to ports before connection.
    }

    // Do the work on this port when connection parameters are specified.
    // This is still prior to receiving info from the other side, thus this is not necessarily
    // the final info.
    // FIXME: we should error check against bitstream-fixed parameters
    void BasicPort::
    applyConnectParams(const OR::Descriptors *other, const OU::PValue *params) {
      setConnectParams(params);
      startConnect(other, params);
    }
    // Establish the roles, which might happen earlier than the finalization of the connection
    // Since roles can determine resource allocations
    // This could be table-driven...
    void BasicPort::
    determineRoles(const char *name, OR::Descriptors &other) {
      OR::Descriptors
        &pDesc = isProvider() ? getData().data : other,
        &uDesc = isProvider() ? other : getData().data;
      ocpiInfo("Port %s of %s, a %s, has options 0x%x, initial role %s, buffers %u size %u",
	       m_metaPort.m_name.c_str(), name,
	       isProvider() ? "provider/consumer" : "user/producer",
		getData().data.options, roleNames[getData().data.role],
		getData().data.desc.nBuffers, getData().data.desc.dataBufferSize);
      ocpiInfo("  other has options 0x%x, initial role %s, buffers %u size %u",
		other.options, roleNames[other.role], other.desc.nBuffers,
	       other.desc.dataBufferSize);
      OR::PortRole uRole = (OR::PortRole)uDesc.role, pRole = (OR::PortRole)pDesc.role;
      const char *err = chooseRoles(uRole, uDesc.options, pRole, pDesc.options);
      if (err)
	throw OU::Error("Error negotiating transfer roles: %s", err);
      uDesc.role = uRole;
      pDesc.role = pRole;
      ocpiInfo("  after negotiation, port %s, a %s, has role %s,"
	       "  other has role %s",
	       m_metaPort.m_name.c_str(), isProvider() ? "provider/consumer" : "user/producer",
		roleNames[getData().data.role], roleNames[other.role]);
      size_t maxSize =  pDesc.desc.dataBufferSize;
      if (uDesc.desc.dataBufferSize > pDesc.desc.dataBufferSize)
	maxSize = uDesc.desc.dataBufferSize;
      maxSize = OU::roundUp(maxSize, OU::BUFFER_ALIGNMENT);
      if (maxSize > pDesc.desc.dataBufferSize) {
	// Expanding the input size buffers
	pDesc.desc.dataBufferSize = OCPI_UTRUNCATE(uint32_t, maxSize);
      }
      if (maxSize > uDesc.desc.dataBufferSize) {
	// Expanding the output size buffer
	uDesc.desc.dataBufferSize = OCPI_UTRUNCATE(uint32_t, maxSize);
      }
      // FIXME: update bufferSizePort relationships to ports that are changing their size
      // But this depends on the order of the connections..
      // but this should only happen with runtime-parameter buffer sizes
      // perhaps make it an error
      ocpiInfo("  after negotiation, buffer size is %zu", maxSize);
      // We must make sure other side doesn't mess with roles anymore.
      uDesc.options |= 1 << OR::MandatedRole;
      pDesc.options |= 1 << OR::MandatedRole;
    }
#endif

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
      out = packer.data();
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
    chooseRoles(OR::PortRole &uRole, unsigned uOptions, OR::PortRole &pRole, unsigned pOptions)
    {
      // FIXME this relies on knowledge of the values of the enum constants
      static OR::PortRole otherRoles[] = { OCPI_RDT_OTHER_ROLES };
      defaultRole(uRole, uOptions);
      defaultRole(pRole, pOptions);
      OR::PortRole
        pOther = otherRoles[pRole],
        uOther = otherRoles[uRole];
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
      const char
	*err = NULL,
	*transportIn = NULL,
	*transportOut = NULL,
	*transportConn = NULL;
      OR::PortRole roleIn = OR::NoRole, roleOut = OR::NoRole;
      const char *s;
      findRole(paramsIn, s, roleIn); // overrides per port for xfer role
      findRole(paramsOut, s, roleOut);
      OU::findString(paramsIn, "transport", transportIn);
      OU::findString(paramsOut, "transport", transportOut);
      if (transportIn && transportOut && !strcasecmp(transportIn, transportOut))
      OU::findString(paramsConn, "transport", transportConn);
      for (unsigned ni = 0; ni < in.size(); ni++) {
	const Transport &it = in[ni];
	if (transportIn && strcasecmp(transportIn, it.transport.c_str()))
	  ocpiInfo("Rejecting input transport %s since %s was specified",
		   it.transport.c_str(), transportIn);
	else if (transportConn && strcasecmp(transportConn, it.transport.c_str()))
	  ocpiInfo("Rejecting input transport %s since %s was specified for the connection",
		   it.transport.c_str(), transportConn);
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
	    else if (transportOut && strcasecmp(transportOut, ot.transport.c_str()))
	      ocpiInfo("Rejecting output transport %s since %s was specified",
		       ot.transport.c_str(), transportOut);
	    else if (transportConn && strcasecmp(transportConn, ot.transport.c_str()))
	      ocpiInfo("Rejecting input transport %s since %s was specified for the connection",
		       ot.transport.c_str(), transportConn);
	    else if (roleOut != OR::NoRole && !((1 << roleOut) & ot.optionsOut))
	      ocpiInfo("Rejecting input role %s for transport %s: container doesn't support it",
		       roleNames[roleOut], ot.transport.c_str());
	    else {
	      // Everything matches
	      transport.roleIn = it.roleIn;
	      transport.roleOut = ot.roleOut;
	      if ((err = chooseRoles(transport.roleOut, ot.optionsOut, transport.roleIn,
				     it.optionsIn)))
		ocpiInfo("Rejecting transport %s since role support is incompatible",
			 it.transport.c_str());
	      else {
		transport.transport = it.transport;
		transport.id = it.id;
		transport.optionsIn = transport.optionsOut = OR::MandatedRole;
		ocpiInfo("Choosing transport %s/%s for connection",
			 it.transport.c_str(), it.id.c_str());
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

    // bi-modal - either for a shim or an external port
    OA::ExternalBuffer *BasicPort::
    getBuffer(uint8_t *&data, size_t &length, uint8_t &opCode, bool &end) {
      ExternalBuffer *b = getFullBuffer();
      if (b) {
	data = b->m_data;
	length = b->m_length;
	opCode = b->m_opCode;
	end = b->m_eof;
      }
      return b;
    }

    bool BasicPort::
    peekOpCode(uint8_t &op) {
      if (m_forward)
	return m_forward->peekOpCode(op);
      if (m_next2read) { // if shim mode
	assert(!m_lastRead);
	if (m_next2read->m_full) {
	  op = m_next2read->m_opCode;
	  return true;
	}
      }
      return false;
    }

    ExternalBuffer *BasicPort::
    getFullBuffer() {
      if (m_forward)
	return m_forward->getFullBuffer();
      if (m_next2read) { // if shim mode
	assert(!m_lastRead);
	if (m_next2read->m_full) {
	  m_lastRead  = m_next2read;
	  m_next2read = m_next2read->m_next;
	  return m_lastRead;
	}
	return NULL;
      }
      if (!isProvider())
	throw OU::Error("getBuffer on input port called on external output port %s",
			name().c_str());
      if (m_lastBuffer.m_dtBuffer != NULL)
	throw OU::Error("getBuffer called on input port %s without releasing previous buffer",
			name().c_str());
      m_lastBuffer.m_eof = false;
      return
	(m_lastBuffer.m_dtBuffer =
	 m_dtPort->getNextFullInputBuffer(m_lastBuffer.m_data, m_lastBuffer.m_length,
					  m_lastBuffer.m_opCode)) ?
	&m_lastBuffer : NULL;
    }
    OA::ExternalBuffer *BasicPort::
    getBuffer(uint8_t *&data, size_t &length) {
      ExternalBuffer *b = getEmptyBuffer();
      if (b) {
	data = b->m_data;
	length = b->m_length;
      }
      return b;
    }

    ExternalBuffer *BasicPort::
    getEmptyBuffer() {
      if (m_forward)
	return m_forward->getEmptyBuffer();
      if (m_next2write) { // shim mode
	ocpiAssert(!m_lastWrite);
	if (!m_next2write->m_full) {
	  m_next2write->m_data = (uint8_t*)(m_next2write + 1);
	  m_next2write->m_length = m_bufferSize;
	  m_lastWrite  = m_next2write;
	  m_next2write = m_next2write->m_next;
	  return m_lastWrite;
	}
	return NULL;
      }
      if (isProvider())
	throw OU::Error("getBuffer for output port called on input port %s",
			name().c_str());
      if (m_lastBuffer.m_dtBuffer != NULL)
	throw OU::Error("getBuffer called on output port %s without sending previous buffer",
			name().c_str());
      return
	(m_lastBuffer.m_dtBuffer =
	 m_dtPort->getNextEmptyOutputBuffer(m_lastBuffer.m_data, m_lastBuffer.m_length)) ?
	&m_lastBuffer : NULL;
    }
    // Standalone EOF.  Returns NULL if it can't go, just like getbuffer.
    bool BasicPort::
    endOfData() {
      if (m_forward)
	return m_forward->endOfData();
      uint8_t *data;
      size_t length;
      if (getBuffer(data, length)) {
	if (m_lastWrite) {
	  m_lastWrite->m_length = 0;
	  m_lastWrite->m_opCode = 0;
	  m_lastWrite->m_eof = true;
	  m_lastWrite->m_data = NULL;
	  m_lastWrite->m_full = true;
	  m_nWritten++;
	} else {
	  ocpiAssert(m_dtPort && m_lastBuffer.m_dtBuffer);
	  m_dtPort->sendOutputBuffer(m_lastBuffer.m_dtBuffer, 0, 0, true, true);
	  m_lastBuffer.m_dtBuffer = NULL;
	}
	return true;
      }
      return false;
    }
    bool BasicPort::
    tryFlush() {
      if (m_forward)
	return m_forward->tryFlush();
      return (m_nWritten - m_nRead) != 0;
    }
    void BasicPort::
    release() {
      if (m_forward)
	return m_forward->release();
      m_lastRead->release();
    }
    void BasicPort::
    put(size_t length, uint8_t opCode, bool end, size_t direct) {
      if (m_forward)
	return m_forward->put(length, opCode, end, direct);
      ocpiDebug("Putting buffer %p to port %p length %zu", m_lastWrite, this, length);
      ocpiAssert(m_lastWrite);
      m_lastWrite->put(length, opCode, end, direct);
    }

    // The zero-copy put of another port's buffer to this port
    void BasicPort::
    put(OA::ExternalBuffer &buf) {
      ExternalBuffer &b = static_cast<ExternalBuffer&>(buf);
      if (m_forward)
	m_forward->put(buf);
      else if (&b.m_port == this)
	buf.put();
      else if (m_next2write) {
	b.m_zcQueue = m_next2write;
	b.m_zcNext = NULL;
	m_next2write->m_zcBack->m_zcNext = &b; // might be writing "front" here
	m_next2write->m_zcBack = &b;
      } else if (b.m_dtBuffer)
	m_dtPort->sendZcopyInputBuffer(*b.m_dtBuffer, b.m_length, b.m_opCode, b.m_eof);
      else
	assert("No support yet for zery-copy send of shim buffer to external port"==0);
    }

    void BasicPort::
    setBufferSize(size_t bufferSize) {
      m_bufferSize = bufferSize;
      getData().data.desc.dataBufferSize = OCPI_UTRUNCATE(uint32_t, bufferSize);
    }

    void BasicPort::
    applyConnection(const Launcher::Connection &c) {
      OR::Descriptors &d = getData().data;
      d.role = isProvider() ? c.m_transport.roleIn : c.m_transport.roleOut;
      d.options = isProvider() ? c.m_transport.optionsIn : c.m_transport.optionsOut;
      strcpy(d.desc.oob.oep, c.m_transport.transport.c_str());
      setBufferSize(c.m_bufferSize);
    }

    // Make this port into a SHIM - meaning there is no "connection",
    // just the front side (normal API for in-process worker ports)
    // and back side (external port API accessing the same buffers).
    // We do one allocation, where each ExternalBuffer is directly followed by its buffer
    void BasicPort::
    becomeShim() {
      OR::Descriptors &d = getData().data;
      // We assume buffer size is already nicely rounded up and aligned etc.
      size_t each = sizeof(ExternalBuffer) + d.desc.dataBufferSize;
      m_allocation = new uint8_t[d.desc.nBuffers * each];
      uint8_t *p = m_allocation;
      for (unsigned n = 1; n <= d.desc.nBuffers; n++, p += each) {
	ExternalBuffer *next =
	  (ExternalBuffer*)(n == d.desc.nBuffers ? m_allocation : p + each);
	new(p) ExternalBuffer(*this, next);
      }
      m_next2read = m_next2write = (ExternalBuffer*)m_allocation;
      portIsConnected();
    }

    // Connect inside the same process
    void BasicPort::
    connectInProcess(BasicPort &other) {
      if (&container() != &other.container() ||
	  !container().connectInside(*this, other)) {
	becomeShim();
	// Poor man's "applyConnection" but without any transport
	assert(m_bufferSize != SIZE_MAX);
	other.setBufferSize(m_bufferSize);
	other.forward2shim(*this);
      }
    }

    void BasicPort::
    forward2shim(BasicPort &shim) {
      m_forward = &shim;
      m_bufferSize = shim.m_bufferSize;
      portIsConnected();
    }

    // Connect inside the same process, but beteen containers.
    // FIXME: optimization for local connections between ports in the same process,
    // but different containers.
    void BasicPort::
    connectLocal(Launcher::Connection &c) {
      BasicPort
	&in = isProvider() ? *this : *c.m_in.m_port,
	&out = isProvider() ? *c.m_out.m_port : *this;
      bool iDone, oDone;
      OR::Descriptors buf, buf1;
      const OR::Descriptors *result = in.startConnect(NULL, iDone);
      assert(result);
      result = out.startConnect(result, oDone);
      assert((result && !iDone) || (!result && iDone));
      if (result) {
	result = in.finishConnect(result, buf, iDone);
	assert((result && !oDone) || (!result && oDone));
	if (result)
	  result = out.finishConnect(result, buf1, oDone);
      }
      assert(iDone && oDone && !result);
      portIsConnected();
    }

    // return if a result was produced.
    bool BasicPort::
    startRemote(Launcher::Connection &c) {
      const OR::Descriptors *result;
      bool done;
      if (isProvider()) {
	result = startConnect(NULL, done);
	ocpiAssert(result && !done);
      } else {
	OR::Descriptors buf;
	ocpiCheck(unpackPortDesc(c.m_in.m_initial, buf));
	result = startConnect(&buf, done);
	ocpiAssert(result || done);
      }
      if (result)
	packPortDesc(*result, (isProvider() ? c.m_in : c.m_out).m_initial);
      return result != NULL; // could be true for output
    }

    bool BasicPort::
    finishRemote(Launcher::Connection &c) {
      OR::Descriptors buf, buf1;
      const OR::Descriptors *result;
      bool done;
      ocpiCheck(unpackPortDesc((isProvider() ? c.m_out : c.m_in).m_initial, buf));
      result = finishConnect(&buf, buf1, done);
      ocpiAssert(done);
      if (result)
	packPortDesc(*result, (isProvider() ? c.m_in : c.m_out).m_final);
      return result != NULL; // could be true for output
    }

    // Default local behavior for basic ports that need to behave like external or bridge ports
    const OCPI::RDT::Descriptors *BasicPort::
    startConnect(const OCPI::RDT::Descriptors *other, bool &done) {
      if (isProvider())
	m_dtPort = container().getTransport().createInputPort(getData().data);
      else if (other)
	m_dtPort = container().getTransport().createOutputPort(getData().data, *other);
      if (m_dtPort)
	// FIXME: put this in the constructor, and have better names
	m_dtPort->setInstanceName(m_metaPort.m_name.c_str());
      done = false;
      return &getData().data;
    }

    const OCPI::RDT::Descriptors *BasicPort::
    finishConnect(const OCPI::RDT::Descriptors *other, OCPI::RDT::Descriptors &feedback,
		  bool &done) {
      return m_dtPort->finalize(other, getData().data, &feedback, done);
    }

  } // end of namespace Container
} // end of namespace OCPI


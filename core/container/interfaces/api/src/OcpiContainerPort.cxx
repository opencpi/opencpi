
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

#include "OcpiContainerPort.h"
#include "OcpiWorker.h"
#include "OcpiContainerApplication.h"
#include "OcpiContainerMisc.h"
#include "OcpiPValue.h"

namespace OCPI {
  namespace Container {
    namespace OA = OCPI::API;
    namespace CM = OCPI::Metadata;
    namespace CP = OCPI::Util::Prop;
    BasicPort::BasicPort(const OCPI::Metadata::Port & metaData, bool isProvider ) :
      PortData(metaData, isProvider), myDesc(connectionData.data.desc)
    {
      // FIXME: put these in the default PortData constructor
      myDesc.nBuffers = DEFAULT_NBUFFERS;
      myDesc.dataBufferSize = DEFAULT_BUFFER_SIZE;
      OCPI::RDT::Descriptors &d = connectionData.data;
      d.type = m_metaPort.provider ? OCPI::RDT::ConsumerDescT : OCPI::RDT::ProducerDescT;
      d.role = OCPI::RDT::NoRole;
    }
    BasicPort::~BasicPort(){}

    // This base class constructor for generic initialization
    // FIXME: parse buffer count here at least? (check that others don't do it).
    Port::Port(Container &container, const OCPI::Metadata::Port &mPort, bool isProvider,
	       const OCPI::Util::PValue *) :
      BasicPort( mPort, isProvider ),
      m_container(container),
      m_canBeExternal(true)
    {
      uint32_t n;
      bool found;
      
      // Some of the tests use the default values of the meta-port so there may
      // not be xml associated with it.
      if ( mPort.myXml ) {
        n = getAttrNum(mPort.myXml, "minBufferSize", true, &found);
        if (found)
          m_metaPort.minBufferSize = n;
        n = getAttrNum(mPort.myXml, "maxBufferSize", true, &found);
        if (found)
          m_metaPort.maxBufferSize = n; // no max if not specified.
        n = getAttrNum(mPort.myXml, "minNumBuffers", true, &found);
        if (found)
          m_metaPort.minBufferCount = n;
      }
      connectionData.port = (intptr_t)this;
    }

    Container &Port::container() const { return m_container; }

    void Port::loopback(OA::Port &) {}

    bool Port::hasName(const char *name) {
      return (name == m_metaPort.name );
    }

    // The default behavior is that there is nothing special to do between
    // ports of like containers.
    bool Port::connectLike(Port &other, const OCPI::Util::PValue *myProps,
			   const OCPI::Util::PValue *otherProps) {
      (void)other;(void)myProps;(void)otherProps;
      return false;
    }

    // The general case of connecting ports that are in the same process.
    void Port::connect(OCPI::API::Port &apiOther, const OCPI::Util::PValue *myProps,
		       const OCPI::Util::PValue *otherProps) {
      Port &other = *static_cast<Port*>(&apiOther);
      if (isProvider())
        if (other.isProvider())
          throw ApiError("Cannot connect two provider ports", NULL);
        else
          other.connect( *this, otherProps, myProps);
      else if (!other.isProvider()) {
        throw ApiError("Cannot connect to user ports", NULL);
      }
      else {
        Container
          &otherContainer = other.container();
        // Containers know how to do internal connections
        if (&m_container == &otherContainer) {
          connectInside(other, myProps, otherProps);
        }
        // Container MAY know how to do intercontainer connections between like containers.
        else if (&container().driver() == &otherContainer.driver() &&
                 connectLike( other, myProps, otherProps)) {
	  return;
        }
        // We're in different containers managed locally.  Perform the connection protocol
        else {
          // FIXME:  a more "binary" way to do this locally?
          const std::string
            &ipi = other.getInitialProviderInfo(otherProps),
            &iui = setInitialProviderInfo(myProps, ipi);
          if (!iui.empty()) {
            const std::string &fpi = other.setInitialUserInfo(iui);
            if (!fpi.empty()) {
              const std::string &fui = setFinalProviderInfo(fpi);
              if (!fui.empty())
                other.setFinalUserInfo(fui);
            }
          }
        }
      }
    }

    // Start the remote/intercontainer connection process
    // FIXME: Need simpler protocol to connect between containers in same process
    // without all this pack/unpack overhead.
    // We do not set up the OCDP here since we don't know everything.
    const std::string &Port::getInitialProviderInfo(const OCPI::Util::PValue *props) {
      ocpiAssert(isProvider());
      if (!m_canBeExternal)
	throw ApiError("Port \", name, \" cannot be connected external to container", NULL);
      applyConnectParams(props);
      m_initialPortInfo = m_container.packPortDesc(*this);
      return m_initialPortInfo;
    }

    // User side initial method, that carries provider info and returns user info
    const std::string &Port::setInitialProviderInfo(const OCPI::Util::PValue *props,
						    const std::string &ipi) {
      // User side, producer side.
      ocpiAssert(!isProvider());
      if (!m_canBeExternal)
	throw ApiError("Port \", name, \" cannot be connected external to container", NULL);
      PortData otherPortData;
      m_container.unpackPortDesc(ipi, &otherPortData);
      // Adjust any parameters from connection metadata
      applyConnectParams(props);
      // We now know the role aspects of both sides.  Make the decision so we know what
      // resource allocations to make in finishConnection.
      establishRoles(otherPortData.connectionData.data);
      finishConnection(otherPortData.connectionData.data);
      // We're done but other size still needs info.
      // FIXME done here?
      m_initialPortInfo = m_container.packPortDesc(*this);
      return m_initialPortInfo;
    }

    const std::string Port::s_empty;    // Provider Only
    const std::string &Port::setInitialUserInfo(const std::string &iui) {
      ocpiAssert(isProvider());
      PortData otherPortData;
      m_container.unpackPortDesc(iui, &otherPortData);
      establishRoles(otherPortData.connectionData.data);
      // Adjust any parameters from connection metadata
      finishConnection(otherPortData.connectionData.data);
      return s_empty;
    }

    // User only
    const std::string &Port::setFinalProviderInfo(const std::string &) {
      return s_empty;
    }
    // Provider Only
    void Port::setFinalUserInfo(const std::string &) {
    }
    // Establish the roles, which might happen earlier than the finalization of the connection
    // Since roles can determine resource allocations
    // This could be table-driven...
    void Port::establishRoles(OCPI::RDT::Descriptors &other) {

      OCPI::RDT::Descriptors
        &pDesc = isProvider() ? connectionData.data : other,
        &uDesc = isProvider() ? other : connectionData.data;
      static const char *roleName[] =
        {"NoRole", "ActiveMessage", "ActiveFlowControl", "ActiveOnly", "Passive", "MaxRole"};
      printf("Port %s, a %s, has options 0x%x, initial role %s\n"
             "  other has options 0x%x, initial role %s\n",
             m_metaPort.name, isProvider() ? "provider/consumer" : "user/producer",
             connectionData.data.options, roleName[connectionData.data.role],
             other.options, roleName[other.role]);
      chooseRoles(uDesc.role, uDesc.options, pDesc.role, pDesc.options);
      printf("  after negotiation, port %s, a %s, has role %s\n"
             "  other has role %s\n",
             m_metaPort.name, isProvider() ? "provider/consumer" : "user/producer",
             roleName[connectionData.data.role], roleName[other.role]);
      // We must make sure other side doesn't mess with roles anymore.
      uDesc.options |= 1 << OCPI::RDT::MandatedRole;
      pDesc.options |= 1 << OCPI::RDT::MandatedRole;
    }
    // Convert PValues into descriptor values, with constraint checking
    // A static method to be able use in other contexts for objects that are not this class.
    void BasicPort::setConnectParams(const OCPI::Util::PValue *props) {
      if (!props)
        return;
      for (const OCPI::Util::PValue *p = props; p->name; p++) {
        if (strcasecmp(p->name, "bufferCount") == 0) {
          if (p->type != CP::Scalar::OCPI_ULong)
            throw ApiError("bufferCount property has wrong type, should be ULong", NULL);
          if (p->vULong < m_metaPort.minBufferCount)
            throw ApiError("bufferCount is below worker's minimum", NULL);
          myDesc.nBuffers = p->vULong;
        } else if (strcasecmp(p->name, "bufferSize") == 0) {
          if (p->type != CP::Scalar::OCPI_ULong)
            throw ApiError("bufferSize property has wrong type, should be ULong", NULL);
          if (p->vULong < m_metaPort.minBufferSize)
            throw ApiError("bufferSize is below worker's minimum", NULL);
          if (m_metaPort.maxBufferSize && p->vULong > m_metaPort.maxBufferSize)
            throw ApiError("bufferSize exceeds worker's maximum", NULL);
          myDesc.dataBufferSize = p->vULong;
        } else if (strcasecmp(p->name, "xferRole") == 0 && p->vString) {
          if (p->type != CP::Scalar::OCPI_String)
            throw ApiError("xferRole property has wrong type, should be string", NULL);
          OCPI::RDT::PortRole role;
          if (!strcasecmp(p->vString, "passive"))
            role = OCPI::RDT::Passive;
          else if (!strcasecmp(p->vString, "active") ||
		   !strcasecmp(p->vString, "activemessage"))
            role = OCPI::RDT::ActiveMessage;
          else if (!strcasecmp(p->vString, "flowcontrol") ||
		   !strcasecmp(p->vString, "activeflowcontrol"))
            role = OCPI::RDT::ActiveFlowControl;
          else if (!strcasecmp(p->vString, "activeonly"))
            role = OCPI::RDT::ActiveOnly;
          else
            throw ApiError("xferRole property must be passive|active|flowcontrol|activeonly", NULL);
          if (!(connectionData.data.options & (1 << role)))
            throw ApiError("xferRole of \"%s\" not supported by port \"%s\"", p->vString, m_metaPort.name);
          connectionData.data.role = role;
        }
      }
    }
    // Default = we don't check or do anything here as generic parameters are set.
    void BasicPort::checkConnectParams() {}
    // Do the work on this port when connection properties are specified.
    // This is still prior to receiving info from the other side, thus this is not necessarily
    // the final info.
    // FIXME: we should error check against bitstream-fixed parameters
    void BasicPort::applyConnectParams(const OCPI::Util::PValue *props) {
      setConnectParams(props);
      checkConnectParams();
    }
    // coming in, specified roles are preferences or explicit instructions.
    // The existing settings are either NoRole, a preference, or a mandate
    void BasicPort::chooseRoles(int32_t &uRole, uint32_t uOptions, int32_t &pRole, uint32_t pOptions) {
      // FIXME this relies on knowledge of the values of the enum constants
      static OCPI::RDT::PortRole otherRoles[] =
        {OCPI::RDT::NoRole, OCPI::RDT::ActiveFlowControl, OCPI::RDT::ActiveMessage,
         OCPI::RDT::Passive, OCPI::RDT::ActiveOnly};
      OCPI::RDT::PortRole
        pOther = otherRoles[pRole],
        uOther = otherRoles[uRole];
      if (pOptions & (1 << OCPI::RDT::MandatedRole)) {
        // provider has a mandate
        ocpiAssert(pRole != OCPI::RDT::NoRole);
        if (uRole == pOther)
          return;
        if (uOptions & (1 << OCPI::RDT::MandatedRole))
          throw ApiError("Incompatible mandated transfer roles", NULL);
        if (uOptions & (1 << pOther)) {
          uRole = pOther;
          return;
        }
        throw ApiError("No compatible role available against mandated role", NULL);
      } else if (pRole != OCPI::RDT::NoRole) {
        // provider has a preference
        if (uOptions & (1 << OCPI::RDT::MandatedRole)) {
          // user has a mandate
          ocpiAssert(uRole != OCPI::RDT::NoRole);
          if (pRole == uOther)
            return;
          if (pOptions & (1 << uOther)) {
            pRole = uOther;
            return;
          }
          throw ApiError("No compatible role available against mandated role", NULL);
        } else if (uRole != OCPI::RDT::NoRole) {
          // We have preferences on both sides, but no mandate
          // If preferences match, all is well
          if (pRole == uOther)
            return;
          // If one preference is against push, we better listen to it.
          if (uRole == OCPI::RDT::ActiveFlowControl &&
              pOptions & (1 << OCPI::RDT::ActiveMessage)) {
            pRole = OCPI::RDT::ActiveMessage;
            return;
          }
          // Let's try active push if we can
          if (uRole == OCPI::RDT::ActiveMessage &&
              pOptions & (1 << OCPI::RDT::ActiveFlowControl)) {
            pRole = OCPI::RDT::ActiveFlowControl;
            return;
          }
          if (pRole == OCPI::RDT::ActiveFlowControl &&
              uOptions & (1 << OCPI::RDT::ActiveMessage)) {
            uRole = OCPI::RDT::ActiveFlowControl;
            return;
          }
          // Let's try activeonly push if we can
          if (uRole == OCPI::RDT::ActiveOnly &&
              pOptions & (1 << OCPI::RDT::Passive)) {
            pRole = OCPI::RDT::Passive;
            return;
          }
          if (pRole == OCPI::RDT::Passive &&
              pOptions & (1 << OCPI::RDT::ActiveOnly)) {
            pRole = OCPI::RDT::ActiveOnly;
            return;
          }
          // Let's give priority to the "better" role.
          if (uRole < pRole &&
              pOptions & (1 << uOther)) {
            pRole = uOther;
            return;
          }
          // Give priority to the provider
          if (uOptions & (1 << pOther)) {
            uRole = pOther;
            return;
          }
          if (pOptions & (1 << uOther)) {
            pRole = uOther;
            return;
          }
          // Can't use either preference.  Fall throught to no mandates, no preferences
        } else {
          // User role unspecified, but provider has a preference
          if (uOptions & (1 << pOther)) {
            uRole = pOther;
            return;
          }
          // Can't use provider preference, Fall through to no mandates, no preferences
        }
      } else if (uOptions & (1 << OCPI::RDT::MandatedRole)) {
        // Provider has no mandate or preference, but user has a mandate
        if (pOptions & (1 << uOther)) {
          pRole = uOther;
          return;
        }
        throw ApiError("No compatible role available against mandated role", NULL);
      } else if (uRole != OCPI::RDT::NoRole) {
        // Provider has no mandate or preference, but user has a preference
        if (pOptions & (1 << uOther)) {
          pRole = uOther;
          return;
        }
        // Fall through to no mandates, no preferences.
      }
      // Neither has useful mandates or preferences.  Find anything, biasing to push
      for (int i = 0; i < OCPI::RDT::MaxRole; i++)
        // Provider has no mandate or preference
        if (uOptions & (1 << i) &&
            pOptions & (1 << otherRoles[i])) {
          uRole = i;
          pRole = otherRoles[i];
          return;
        }
      throw ApiError("No compatible combination of roles exist", NULL);
    }            
    ExternalPort::ExternalPort(const OCPI::Metadata::Port & metaPort,
			       bool isProvider, const OCPI::Util::PValue *)
      : BasicPort(metaPort, isProvider) {
    }
    void ExternalPort::checkConnectParams() {}
  }
  namespace API {
    ExternalBuffer::~ExternalBuffer(){}
    ExternalPort::~ExternalPort(){}
    Port::~Port(){}
  }
}

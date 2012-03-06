
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

/*
 * SCA CP289 Generic Proxy Port.
 *
 * Revision History:
 *
 *     06/29/2009 - Jim Kulp
 *                  Initial version
 */

#include <new>
#include <string>
#include <OcpiOsAssert.h>
#include <OcpiOsMutex.h>
#include <OcpiUtilCDR.h>
#include <OcpiUtilIOP.h>
#include <OcpiUtilMisc.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiLoggerLogger.h>
#include <OcpiLoggerNullOutput.h>
#include <OcpiLoggerDebugLogger.h>
#include <OcpiContainerInterface.h>
#include "OcpiWorker.h"
#include <OcpiStringifyCorbaException.h>
#include <OcpiCFUtilLegacyErrorNumbers.h>
#include <CF.h>
#include <Cp289ProviderPort.h>
#include "OcpiContainerPort.h"
#include "Cp289GenericProxy.h"

/*
 * Counteract the TAO/ORBexpress compatibility magic.
 */

#ifdef IOP
#undef IOP
#endif

namespace OCPI {
  namespace SCA {
    namespace CU = OCPI::Util;
    /*
     * ----------------------------------------------------------------------
     * Port and connection management
     * ----------------------------------------------------------------------
     */
    Cp289GenericProxyPort::
    Cp289GenericProxyPort(const std::string &portName, Cp289GenericProxy *proxy) :
      BaseProxyPort(portName, proxy),
      m_ocpiPort(*static_cast<CC::Port*>(&proxy->m_worker.getPort(portName.c_str())))
    {
      // Externalize us.
      m_scaPort = proxy->m_poa->id_to_reference(*proxy->m_poa->activate_object(this));
      /*
       * For provider ports, we add a special profile to the object
       * reference that holds the target port data.  That avoids a
       * roundtrip at connection time.
       */
      if (m_ocpiPort.isProvider()) {
        std::string providerInfo;
	m_ocpiPort.getInitialProviderInfo(0, providerInfo);
        /*
         * Unfortunately, the only portable way to get to the IOR is
         * by detour via IOR: string.
         */
        CORBA::String_var portIorString = proxy->m_orb->object_to_string (m_scaPort);
        CU::IOP::IOR portIor = CU::IOP::string_to_ior (portIorString.in());
        /*
         * Add a profile with the target port data.
         */
        portIor.addProfile (OCPI_PORT_INFO_TAG, providerInfo);
        /*
         * Now convert back to an object reference.
         */
        std::string newPortIorString = CU::IOP::ior_to_string (portIor);
        m_scaPort = proxy->m_orb->string_to_object (newPortIorString.c_str());
      }
      _remove_ref();
    }

    Cp289GenericProxyPort::
    ~Cp289GenericProxyPort() throw() {
      // FIXME?
    }

    void Cp289GenericProxyPort::
    getProfile(CORBA::Object_ptr o, CU::IOP::ProfileId tag, std::string &data) {
      CORBA::String_var iorString = m_proxy->m_orb->object_to_string (o);
      CU::IOP::IOR ior = CU::IOP::string_to_ior (iorString.in());
      if (ior.hasProfile (tag))
        data = ior.profileData(tag);
      else
        data.clear();
    }
    // Some of this could be in the base class
    // We need to know several things about a port:
    // name
    //    uses info: repid, twoway?
    //    provides info: repid, twoway?
    void Cp289GenericProxyPort::
    connectPort (CORBA::Object_ptr connection, const char * connectionId)
      throw (CF::Port::InvalidPort,
             CF::Port::OccupiedPort,
             CORBA::SystemException) {

      CU::AutoMutex lock (m_proxy->m_mutex);
      OCPI::Logger::DebugLogger debug (*m_proxy->m_logger);

      if (m_proxy->m_disabled) {
        throw CORBA::BAD_INV_ORDER ();
      }

      debug << m_proxy->m_logProducerName
            << OCPI::Logger::Verbosity (2)
            << "connectPort (\""
            << m_portName
            << "\", \""
            << connectionId
            << "\")"
            << std::flush;

      // LIMITATION:  we only support one connection (no fan-out, fan-in, user+provider)
      if (m_connections.find(connectionId) != m_connections.end()) {
        std::string msg = "Port is already connected: \"";
        msg += m_portName;
        msg += "\": ";
        msg += " (connection id \"";
        msg += connectionId;
        msg += ")";

        *m_proxy->m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
                  << m_proxy->m_logProducerName
                  << msg << "."
                  << std::flush;

        CF::Port::OccupiedPort ip;
        //        ip.errorCode = CF::CF_EINVAL;
        //        ip.msg = msg.c_str ();
        throw ip;
      }
      const char *oops = 0;
      try { // break on oops errors
        // LIMITATION: we don't support user and provider with same name
        if (m_ocpiPort.isProvider()) {
          // Legacy support.  We are receiving "connectInitial" via a connectPort in the other direction
          std::string shadow;
          getProfile(connection, OCPI_SOURCE_SHADOW_PORT_DATA_TAG, shadow);
          if (shadow.empty())
            throw std::string ("OCPI source shadow port profile missing from object reference");
          Cp289ProviderPort::Octets os(shadow.length(), shadow.length(), (CORBA::Octet*)shadow.data());
          connectInitial(os);
        } else {
          // So we are a user port with no existing connections
          // Several cases here:
	  // 1. The provider is a normal SCA component and does not support our CORBA transport, we have some choices:
	  //    1a. We can provide a bridge between the local worker and the remote IIOP
	  //        endpoint/socket, synthesizing GIOP: inserting GIOP headers and possibly
	  //        transcoding messages when necessary.
	  //    1b. We can use the local ORB with a code-generated loadable proxy bridge.
	  //    1c. A DII-based generic proxy port bridge
	  //    LIMITATION:  We don't support this case yet.
          //
          // 2. The provider is a normal SCA component and DOES support our CORBA transport with in-band connection
          //    Meaning that the IOR profile in the provider IOR has info for our transport, and we can make a
	  //    direct connection whereby our (remote) transport plugin synthesizes GIOP, using connectURL.
	  //
          // 3. The provider is "one of ours" and we can negotiate a direct connection using our transport.
	  //
          std::string ipi;
          getProfile(connection, OCPI_PORT_INFO_TAG, ipi);

          if (ipi.empty())
            // LIMITATION: no local GIOP adapter
            oops = "Unsupported connection to provider";
          else {
            // Tell our local port about the provider's initial information
            std::string initialUserInfo;
	    m_ocpiPort.setInitialProviderInfo(0, ipi, initialUserInfo);
            if (!initialUserInfo.empty()) {
#if 1
              // We need to exchange information with the remote port using our private IDL.
              Cp289ProviderPort_var remoteProvider = Cp289ProviderPort::_narrow (connection);
              if (CORBA::is_nil(remoteProvider))
                oops = "Incompatible OCPI transport with remote port";
              else {
                // Now give our local user-side information to the remote provider
                // Since we have info from both sides this will include all the provider
                // side needs to know about us.  But since this info might tell the provider
                // some final information about the connection, we may still need some final
                // information from the provider (like RDMA flow control for the
                // provider-to-user data flow).
                const Cp289ProviderPort::Octets iui(initialUserInfo.length(), initialUserInfo.length(),
						    (CORBA::Octet*)initialUserInfo.data());
                Cp289ProviderPort::Octets_var finalProviderInfo = remoteProvider->connectInitial(iui);
                // Yes this copies it...
                std::string fpi((const char*)finalProviderInfo->get_buffer(),
				(size_t)finalProviderInfo->length());
                if (!fpi.empty()) {
                  // The connection protocol has given us more/final information about the provider.
                  std::string finalUserInfo;
		  m_ocpiPort.setFinalProviderInfo(fpi, finalUserInfo);
                  if (!finalUserInfo.empty()) {
                    const Cp289ProviderPort::Octets fui(finalUserInfo.length(), finalUserInfo.length(),
							(CORBA::Octet*)finalUserInfo.data());
                    remoteProvider->connectFinal(fui);
                  }
                }
              }
#else
              // "legacy"
              /*
               * Tell the other end our "shadow port data" so that it can complete
               * the circuit.  We do that by packing the shadow port's data into an
               * IOR and calling the provider port's connect() operation.
               */

              CF::Port_var remotePort = CF::Port::_narrow (connection);

              if (CORBA::is_nil (remotePort)) {
                throw std::string ("Failed to narrow remote object to CF::Port");
              }
              OCPI::Util::IOP::IOR shadowPortIor;
              shadowPortIor.addProfile (OCPI_SOURCE_SHADOW_PORT_DATA_TAG, initialUserInfo.data(), initialUserInfo.length());
              std::string shadowPortIorString = OCPI::Util::IOP::ior_to_string (shadowPortIor);
              CORBA::Object_var spObj = m_proxy->m_orb->string_to_object (shadowPortIorString.c_str());
              
              try {
                remotePort->connectPort (spObj, connectionId); 
              }
              catch (const CORBA::Exception & oops) {
                try {
                  //m_container->disconnectPorts (m_appContext, connectionCookie);
                }
                catch (...) {
                }
                
                std::string msg = "Failed to connect remote port to local shadow port: ";
                msg += OCPI::CORBAUtil::Misc::stringifyCorbaException (oops);
                throw msg;
              }

#endif
            }
          }
        }
      } catch (const char*e) {
        oops = e;
      }
      if (oops) {
        std::string msg = oops;
        msg += ": \"";
        msg += m_portName;
        msg += "\": ";
        msg += oops;
        msg += " (connection id \"";
        msg += connectionId;
        msg += ")";
          
        *m_proxy->m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
                  << m_proxy->m_logProducerName
                  << msg << "."
                  << std::flush;
          
        CF::Port::InvalidPort ip;
        ip.errorCode = CF::CF_EINVAL;
        ip.msg = msg.c_str ();
        throw ip;
      }
    }
    // Private (to Cp289 generic proxies) method
    Cp289ProviderPort::Octets* Cp289GenericProxyPort::
    connectInitial(const Cp289ProviderPort::Octets& initialUserInfo) {
      const char *oops;
      if (m_ocpiPort.isProvider())
        try {
          // A copy here from corba to our string
          const std::string iui((const char*)initialUserInfo.get_buffer(), (size_t)initialUserInfo.length());
          std::string finalProviderInfo;
	  m_ocpiPort.setInitialUserInfo(iui, finalProviderInfo);
          // String is not copied here
          Cp289ProviderPort::Octets* fpi =
            new Cp289ProviderPort::Octets(finalProviderInfo.length(), finalProviderInfo.length(),
                                          (CORBA::Octet*)finalProviderInfo.data());
          return fpi;
        } catch (const char* e) {
          oops = e;
        }
      else
        oops = "Connection from remote user port to this user port";
      std::string msg = oops;
      msg += ": \"";
      msg += m_portName;
      msg += "\"";
          
      *m_proxy->m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
                << m_proxy->m_logProducerName
                << msg << "."
                << std::flush;
      
      CF::Port::InvalidPort ip;
      ip.errorCode = CF::CF_EINVAL;
      ip.msg = msg.c_str ();
      throw ip;
    }
    // Private (to Cp289 generic proxies) method
    void Cp289GenericProxyPort::
    connectFinal(const Cp289ProviderPort::Octets &finalUserInfo) {
      const char *oops;
      if (m_ocpiPort.isProvider())
        try {
          // A copy here from corba to our string
          const std::string fui((const char*)finalUserInfo.get_buffer(), (size_t)finalUserInfo.length());
          m_ocpiPort.setFinalUserInfo(fui);
          return;
        } catch (const char* e) {
          oops = e;
        }
      else
        oops = "Connection from remote provider port to this provider port";
      std::string msg = oops;
      msg += ": \"";
      msg += m_portName;
      msg += "\"";
          
      *m_proxy->m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
                << m_proxy->m_logProducerName
                << msg << "."
                << std::flush;
      
      CF::Port::InvalidPort ip;
      ip.errorCode = CF::CF_EINVAL;
      ip.msg = msg.c_str ();
      throw ip;
    }
    // We defer the removal of connections until the proxy itself is released
    // since all the assets will be automatically released then.
    void Cp289GenericProxyPort::
    disconnectPort (const char * connectionId)
      throw (CF::Port::InvalidPort,
             CORBA::SystemException) {

      OCPI::Util::AutoMutex lock (m_proxy->m_mutex);
      OCPI::Logger::DebugLogger debug (*m_proxy->m_logger);

      if (m_proxy->m_disabled) {
        throw CORBA::BAD_INV_ORDER ();
      }
      debug << m_proxy->m_logProducerName
        << OCPI::Logger::Verbosity (2)
        << "disconnectPort (\""
        << m_portName
        << "\", \""
        << connectionId
        << "\")"
        << std::flush;

      ConnectionSet::iterator cmit = m_connections.find (connectionId);
      try {
        try {
          if (cmit == m_connections.end())
            throw std::string ("Connection id not in use");
        } catch (const std::string & oops) {
          std::string msg = "Failed to disconnect port \"";
          msg += m_portName;
          msg += "\": ";
          msg += oops;
          msg += " (connection id \"";
          msg += connectionId;
          msg += ")";

          *m_proxy->m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
                             << m_proxy->m_logProducerName
                             << msg << "."
                             << std::flush;

          CF::Port::InvalidPort ip;
          ip.errorCode = CF::CF_EINVAL;
          ip.msg = msg.c_str ();
          throw ip;
        }
        // FIXME: undo connection map.
      } catch(...) {
        m_connections.erase (cmit);
        throw;
      }
    }
  }
}

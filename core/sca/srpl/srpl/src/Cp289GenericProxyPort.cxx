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
#include <CpiOsAssert.h>
#include <CpiOsMutex.h>
#include <CpiUtilCDR.h>
#include <CpiUtilIOP.h>
#include <CpiUtilMisc.h>
#include <CpiUtilAutoMutex.h>
#include <CpiLoggerLogger.h>
#include <CpiLoggerNullOutput.h>
#include <CpiLoggerDebugLogger.h>
#include <CpiContainerInterface.h>
#include "CpiWorker.h"
#include <CpiStringifyCorbaException.h>
#include <CpiCFUtilLegacyErrorNumbers.h>
#include <CF.h>
#include <Cp289ProviderPort.h>
#include "CpiContainerPort.h"
#include "Cp289GenericProxy.h"

/*
 * Counteract the TAO/ORBexpress compatibility magic.
 */

#ifdef IOP
#undef IOP
#endif

namespace CPI {
  namespace SCA {
    namespace CU = CPI::Util;
    /*
     * ----------------------------------------------------------------------
     * Port and connection management
     * ----------------------------------------------------------------------
     */
    Cp289GenericProxyPort::
    Cp289GenericProxyPort(const std::string &portName, Cp289GenericProxy *proxy) :
      BaseProxyPort(portName, proxy),
      m_cpiPort(proxy->m_worker.getPort(portName.c_str()))
    {
      // Externalize us.
      m_scaPort = proxy->m_poa->id_to_reference(*proxy->m_poa->activate_object(this));
      /*
       * For provider ports, we add a special profile to the object
       * reference that holds the target port data.  That avoids a
       * roundtrip at connection time.
       */
      if (m_cpiPort.isProvider()) {
	const std::string &providerInfo = m_cpiPort.getInitialProviderInfo(0);
	/*
	 * Unfortunately, the only portable way to get to the IOR is
	 * by detour via IOR: string.
	 */
	CORBA::String_var portIorString = proxy->m_orb->object_to_string (m_scaPort);
	CU::IOP::IOR portIor = CU::IOP::string_to_ior (portIorString.in());
	/*
	 * Add a profile with the target port data.
	 */
	portIor.addProfile (CPI_PORT_INFO_TAG, providerInfo);
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
      CPI::Logger::DebugLogger debug (*m_proxy->m_logger);

      if (m_proxy->m_disabled) {
	throw CORBA::BAD_INV_ORDER ();
      }

      debug << m_proxy->m_logProducerName
	    << CPI::Logger::Verbosity (2)
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

	*m_proxy->m_logger << CPI::Logger::Level::EXCEPTION_ERROR
		  << m_proxy->m_logProducerName
		  << msg << "."
		  << std::flush;

	CF::Port::OccupiedPort ip;
	//	ip.errorCode = CF::CF_EINVAL;
	//	ip.msg = msg.c_str ();
	throw ip;
      }
      const char *oops = 0;
      try { // break on oops errors
	// LIMITATION: we don't support user and provider with same name
	if (m_cpiPort.isProvider()) {
	  // Legacy support.  We are receiving "connectInitial" via a connectPort in the other direction
	  std::string shadow;
	  getProfile(connection, CPI_SOURCE_SHADOW_PORT_DATA_TAG, shadow);
	  if (shadow.empty())
	    throw std::string ("CPI source shadow port profile missing from object reference");
	  Cp289ProviderPort::Octets os(shadow.length(), shadow.length(), (CORBA::Octet*)shadow.data());
	  connectInitial(os);
	} else {
	  // So we are a provider with no existing connections
	  // Several cases here:
	  // 1. Connection protocol provided by CORBA is deemed sufficient.
	  //    This implies, for a Cp289 component, that we need to do CORBA adaptation
	  //    here in the proxy.  LIMITATION: we don't support this yet: it would require
	  //    the proxy to establish a local GIOP bridge and a data connection to the proxy
	  // 2. The connection protocol does not require us to exchange information
	  //    Meaning that the IOR profile in the provider IOR is given to us, the user
	  //    side, and that is sufficient to establish the connection.  THis means we
	  //    do not need to use the "private IDL" for exchanging info between proxies.
	  // 3. The connection protocol DOES require extra exchanges of information, and we 
	  //    do it via private IDL
	  std::string ipi;
	  getProfile(connection, CPI_PORT_INFO_TAG, ipi);

	  if (ipi.empty())
	    // LIMITATION: no local GIOP adapter
	    oops = "Unsupported connection to provider";
	  else {
	    // Tell our local port about the provider's initial information
	    const std::string &initialUserInfo = m_cpiPort.setInitialProviderInfo(0, ipi);
	    if (!initialUserInfo.empty()) {
#if 1
	      // We need to exchange information with the remote port using our private IDL.
	      Cp289ProviderPort_var remoteProvider = Cp289ProviderPort::_narrow (connection);
	      if (CORBA::is_nil(remoteProvider))
		oops = "Incompatible CPI transport with remote port";
	      else {
		// Now give our local user-side information to the remote provider
		// Since we have info from both sides this will include all the provider
		// side needs to know about us.  But since this info might tell the provider
		// some final information about the connection, we may still need some final
		// information from the provider (like RDMA flow control for the
		// provider-to-user data flow).
		const Cp289ProviderPort::Octets iui(initialUserInfo.length(), initialUserInfo.length(), (CORBA::Octet*)initialUserInfo.data());
		Cp289ProviderPort::Octets_var finalProviderInfo = remoteProvider->connectInitial(iui);
		// Yes this copies it...
		std::string fpi((const char*)finalProviderInfo->get_buffer(), (size_t)finalProviderInfo->length());
		if (!fpi.empty()) {
		  // The connection protocol has given us more/final information about the provider.
		  const std::string &finalUserInfo = m_cpiPort.setFinalProviderInfo(fpi);
		  if (!finalUserInfo.empty()) {
		    const Cp289ProviderPort::Octets fui(finalUserInfo.length(), finalUserInfo.length(), (CORBA::Octet*)finalUserInfo.data());
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
	      CPI::Util::IOP::IOR shadowPortIor;
	      shadowPortIor.addProfile (CPI_SOURCE_SHADOW_PORT_DATA_TAG, initialUserInfo.data(), initialUserInfo.length());
	      std::string shadowPortIorString = CPI::Util::IOP::ior_to_string (shadowPortIor);
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
		msg += CPI::CORBAUtil::Misc::stringifyCorbaException (oops);
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
	  
	*m_proxy->m_logger << CPI::Logger::Level::EXCEPTION_ERROR
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
      if (m_cpiPort.isProvider())
	try {
	  // A copy here from corba to our string
	  const std::string iui((const char*)initialUserInfo.get_buffer(), (size_t)initialUserInfo.length());
	  const std::string &finalProviderInfo = m_cpiPort.setInitialUserInfo(iui);
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
	  
      *m_proxy->m_logger << CPI::Logger::Level::EXCEPTION_ERROR
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
      if (m_cpiPort.isProvider())
	try {
	  // A copy here from corba to our string
	  const std::string fui((const char*)finalUserInfo.get_buffer(), (size_t)finalUserInfo.length());
	  m_cpiPort.setFinalUserInfo(fui);
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
	  
      *m_proxy->m_logger << CPI::Logger::Level::EXCEPTION_ERROR
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

      CPI::Util::AutoMutex lock (m_proxy->m_mutex);
      CPI::Logger::DebugLogger debug (*m_proxy->m_logger);

      if (m_proxy->m_disabled) {
	throw CORBA::BAD_INV_ORDER ();
      }
      debug << m_proxy->m_logProducerName
	<< CPI::Logger::Verbosity (2)
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

	  *m_proxy->m_logger << CPI::Logger::Level::EXCEPTION_ERROR
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

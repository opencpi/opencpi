// -*- c++ -*-

#ifndef CPI_CFUTIL_RECEPTACLE_HELPER_
#define CPI_CFUTIL_RECEPTACLE_HELPER_

/**
 * \file
 * \brief Helper class for an SCA Device's or Resource's receptacle.
 *
 * Revision History:
 *
 *     10/13/2008 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <CpiOsAssert.h>
#include <CpiStringifyCorbaException.h>
#include <CF_s.h>

namespace CPI {
  namespace CFUtil {

    /*
     * Internal helper classes.
     */

    /** \cond */

    namespace {

      class ReceptacleHelperServantCallback {
      public:
	virtual void connectPort (CORBA::Object_ptr connection,
				  const char * connectionId)
	  throw (CF::Port::InvalidPort,
		 CF::Port::OccupiedPort,
		 CORBA::SystemException) = 0;

	virtual void disconnectPort (const char * connectionId)
	  throw (CF::Port::InvalidPort,
		 CORBA::SystemException) = 0;
      };

      class ReceptacleHelperPortServant : virtual public POA_CF::Port {
      public:
	ReceptacleHelperPortServant (ReceptacleHelperServantCallback * cb)
	  throw ();
	~ReceptacleHelperPortServant ()
	  throw ();

	void connectPort (CORBA::Object_ptr connection,
			  const char * connectionId)
	  throw (CF::Port::InvalidPort,
		 CF::Port::OccupiedPort,
		 CORBA::SystemException);

	void disconnectPort (const char * connectionId)
	  throw (CF::Port::InvalidPort,
		 CORBA::SystemException);

      private:
	ReceptacleHelperServantCallback * m_cb;
      };

      ReceptacleHelperPortServant::
      ReceptacleHelperPortServant (ReceptacleHelperServantCallback * cb)
	throw ()
	: m_cb (cb)
      {
      }

      ReceptacleHelperPortServant::
      ~ReceptacleHelperPortServant ()
	throw ()
      {
      }

      void
      ReceptacleHelperPortServant::
      connectPort (CORBA::Object_ptr connection,
		   const char * connectionId)
	throw (CF::Port::InvalidPort,
	       CF::Port::OccupiedPort,
	       CORBA::SystemException)
      {
	m_cb->connectPort (connection, connectionId);
      }

      void
      ReceptacleHelperPortServant::
      disconnectPort (const char * connectionId)
	throw (CF::Port::InvalidPort,
	       CORBA::SystemException)
      {
	m_cb->disconnectPort (connectionId);
      }

    }

    /** \endcond */

    /**
     * \brief Callback from the ReceptacleHelper.
     *
     * Classes (Resource or Device implementations) that make use of the
     * ReceptacleHelper shall inherit from this class to establish a
     * connection.
     */

    class ReceptacleHelperCallback {
    public:
      /**
       * Notify the Resource/Device that a connection was established.
       * The implementation can call the ReceptacleHelper's getConnection()
       * operation to retrieve the peer's object reference.
       *
       * \param[in] portName The name of the port.
       * \param[in] connectionId The connection identifier.
       *
       * \note The ReceptacleHelper performs the necessary validation for
       * the port name, so the implementation can assume that the port name
       * matches the name that was passed to the ReceptacleHelper's
       * constructor, and can also assume that the port is not already
       * connected.
       */

      virtual void connectPort (const std::string & portName,
				const std::string & connectionId)
	throw (CF::Port::InvalidPort,
	       CF::Port::OccupiedPort,
	       CORBA::SystemException) = 0;

      /**
       * Notify the Resource/Device that a connection was disconnected.  The
       * implementation shall release any resources that were associated with
       * the connection.
       *
       * \param[in] portName The name of the port.
       * \param[in] connectionId The connection identifier.
       *
       * \note The ReceptacleHelper performs the necessary validation.  The
       * implementation can assume that the portName is valid and that the
       * connection identifier matches the already-established connection.
       */

      virtual void disconnectPort (const std::string & portName,
				   const std::string & connectionId)
	throw (CF::Port::InvalidPort,
	       CORBA::SystemException) = 0;
    };

    /**
     * \brief Helper class for an SCA Device's or Resource's receptacle.
     *
     * The implementation of an SCA Device or Resource can use instances
     * of this class to ease management of its receptacles.  This class
     * provides an object of type CF::Port and implements the usual checks
     * that need to be performed upon connection and disconnection.
     *
     * The device or resource will usually aggregate member(s) of type
     * ReceptacleHelper.  In its getPort() operation, the device or resource
     * then returns the port object from this class' getPort() operation.
     * Finally, the device or resource needs to provide an instance of the
     * ReceptacleHelperCallback class, usually by inheriting from it, which
     * is used to notify the device or resource notified when a connection
     * is established or disconnected.
     *
     * The type of the connection is supplied as a template parameter, e.g.,
     * CosLwLog::LogProducer for the log service.
     *
     * Abbreviated example:
     *
     * \code
     * class MyResource : public POA_CF::Resource, ReceptacleHelperCallback {
     * private:
     *   ReceptacleHelper<CosLwLog::LogProducer> m_logPort;
     *   CosLwLog::LogProducer_var m_logService;
     * public:
     *   MyDevice (PortableServer::POA_ptr poa)
     *     : m_logPort (poa, "logOut", this)
     *   {}
     *   CORBA::Object_ptr getPort (const char * name)
     *   { if (strcmp(name,"logOut")==0) return m_logPort.getPort(); }
     *   void connectPort (const std::string & name, const std::string &)
     *   { m_logService = m_logPort.getConnection (); }
     *   void disconnectPort (const std::string & name, const std::string &)
     *   { m_logService = CosLwLog::LogProducer::_nil (); }
     * };
     * \endcode
     */

    template<class T>
    class ReceptacleHelper : virtual public ReceptacleHelperServantCallback {
    public:
      /**
       * Constructor.
       *
       * \param[in] poa      This POA is used for the servant that implements
       *                     the CF::Port object.
       * \param[in] portName The name of the receptacle port.
       * \param[in] callback Callback if the user of this class wants to be
       *                     notified when a connection is established or
       *                     disconnected.
       */

      ReceptacleHelper (PortableServer::POA_ptr poa,
			const std::string & portName,
			ReceptacleHelperCallback * callback = 0)
	throw (std::string);

      /**
       * Destructor.
       */

      virtual
      ~ReceptacleHelper ()
	throw ();

      /**
       * Whether a connection is established or not.
       *
       * \return true if a connection is established, false if not.
       */

      bool isConnected ()
	throw ();

      /**
       * Returns the port object that represents this receptacle.
       *
       * Usually the device or resource implementation will call this
       * method from its getPort() operation.
       *
       * \return A CF::Port object that an SCA client can use to connect
       * or disconnect to this receptacle.
       */

      CF::Port_ptr getPort ()
	throw ();

      /**
       * Returns a reference to the remote port, if a connection is
       * established.
       *
       * \return The remote port if a connection is established, nil
       * if not.
       */

      typename T::_ptr_type getConnection ()
	throw ();

    protected:
      void connectPort (CORBA::Object_ptr connection,
			const char * connectionId)
	throw (CF::Port::InvalidPort,
	       CF::Port::OccupiedPort,
	       CORBA::SystemException);

      void disconnectPort (const char * connectionId)
	throw (CF::Port::InvalidPort,
	       CORBA::SystemException);

    private:
      std::string m_portName;
      PortableServer::POA_var m_poa;
      PortableServer::ObjectId_var m_oid;
      CF::Port_var m_me;
      typename T::_var_type m_connection;
      std::string m_connectionId;
      ReceptacleHelperCallback * m_callback;
      ReceptacleHelperPortServant * m_servant;
    };

  }
}

template<class T>
inline
CPI::CFUtil::ReceptacleHelper<T>::
ReceptacleHelper (PortableServer::POA_ptr poa,
		  const std::string & portName,
		  ReceptacleHelperCallback * callback)
  throw (std::string)
  : m_portName (portName),
    m_callback (callback)
{
  m_poa = PortableServer::POA::_narrow (poa);
  m_servant = new ReceptacleHelperPortServant (this);

  try {
    m_oid = m_poa->activate_object (m_servant);

    try {
      CORBA::Object_var obj = m_poa->id_to_reference (m_oid.in());
      m_me = CF::Port::_narrow (obj);
    }
    catch (...) {
      m_poa->deactivate_object (m_oid);
      throw;
    }
  }
  catch (const CORBA::Exception & cex) {
    delete m_servant;
    throw CPI::CORBAUtil::Misc::stringifyCorbaException (cex);
  }
}

template<class T>
inline
CPI::CFUtil::ReceptacleHelper<T>::
~ReceptacleHelper ()
  throw ()
{
  try {
    m_poa->deactivate_object (m_oid);
  }
  catch (...) {
  }

  m_servant->_remove_ref ();
}

template<class T>
inline
CF::Port_ptr
CPI::CFUtil::ReceptacleHelper<T>::
getPort ()
  throw ()
{
  return CF::Port::_duplicate (m_me);
}

template<class T>
inline
typename T::_ptr_type
CPI::CFUtil::ReceptacleHelper<T>::
getConnection ()
  throw ()
{
  return T::_duplicate (m_connection);
}

template<class T>
inline
bool
CPI::CFUtil::ReceptacleHelper<T>::
isConnected ()
  throw ()
{
  return !CORBA::is_nil (m_connection);
}

template<class T>
inline
void
CPI::CFUtil::ReceptacleHelper<T>::
connectPort (CORBA::Object_ptr connection,
	     const char * connectionId)
  throw (CF::Port::InvalidPort,
	 CF::Port::OccupiedPort,
	 CORBA::SystemException)
{
  if (!CORBA::is_nil (m_connection)) {
    throw CF::Port::OccupiedPort ();
  }

  m_connectionId = connectionId;
  m_connection = T::_narrow (connection);


  if (CORBA::is_nil (m_connection)) {
    throw CF::Port::InvalidPort (1, "incompatible type");
  }

  if (m_callback) {
    try {
      m_callback->connectPort (m_portName,
			       m_connectionId);
    }
    catch (...) {
      m_connection = T::_nil ();
      throw;
    }
  }
}

template<class T>
inline
void
CPI::CFUtil::ReceptacleHelper<T>::
disconnectPort (const char * connectionId)
  throw (CF::Port::InvalidPort,
	 CORBA::SystemException)
{
  if (CORBA::is_nil (m_connection)) {
    throw CF::Port::InvalidPort (2, "not connected");
  }

  if (m_connectionId != connectionId) {
    throw CF::Port::InvalidPort (2, "wrong connection id");
  }

  if (m_callback) {
    m_callback->disconnectPort (m_portName, m_connectionId);
  }

  m_connection = T::_nil ();
}

#endif

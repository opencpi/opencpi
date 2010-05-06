/*
 * SCA CP289 Generic Proxy.
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
#include <CpiStringifyCorbaException.h>
#include <CpiCFUtilLegacyErrorNumbers.h>
#include <CF.h>
#include <Cp289ProviderPort.h>
#include "CpiApplication.h"
#include "CpiProperty.h"
#include "Cp289GenericProxy.h"

/*
 * Counteract the TAO/ORBexpress compatibility magic.
 */

#ifdef IOP
#undef IOP
#endif

namespace CPI {
  namespace SCA {
    namespace CC = CPI::Container;
    namespace CM = CPI::Metadata;
    namespace CU = CPI::Util;
    /*
     * ----------------------------------------------------------------------
     * Constructor and Destructor
     * ----------------------------------------------------------------------
     */

    Cp289GenericProxy::
    Cp289GenericProxy (// needed by base class
                       CORBA::ORB_ptr orb,
                       PortableServer::POA_ptr poa,
                       // Identity within the application/assembly
                       const std::string & identifier,
                       // spd:softpkg/implementation/code/localfile@name
                       const char *codeLocalFileName,
                       // spd:softpkg/implementation/code/entrypoint
                       const char *functionName,
                       const char *instanceName,
                       CC::Application &application,
                       const char *namingContextIor,
                       const char *nameBinding,
                       // Optional
                       CPI::Logger::Logger * logger,
                       bool adoptLogger,
                       bool shutdownOrbOnRelease)
      throw (std::string)
      : BaseProxy (orb, poa, identifier,
                      logger, adoptLogger,
                      shutdownOrbOnRelease),
        m_application (application),
        // ALl the hard work happens here.
        m_worker(application.createWorker(codeLocalFileName, 0, functionName, instanceName, 0))
    {
      // Externalize us.
      m_scaResource = poa->id_to_reference(*poa->activate_object(this));
      if (namingContextIor) {
        CORBA::Object_var ncref = orb->string_to_object (namingContextIor);
        CosNaming::NamingContext_var nc = CosNaming::NamingContext::_narrow (ncref);
        CosNaming::Name nn;
        nn.length (1);
        nn[0].id = nameBinding;
        nc->bind (nn, m_scaResource);
      }      
      static CF::ExecutableDevice::ProcessID_Type lastPid;
      m_scaPid = ++lastPid; // FIXME: interlocked for trhead safety.
    }
    Cp289GenericProxy::
    ~Cp289GenericProxy ()
      throw ()
    {
      releasePorts();
      // This is only because something like JTAP would not want to wait until
      // the app is torn down, when it would actually be a better time..
      delete &m_worker;
    }

    /*
     * ----------------------------------------------------------------------
     * CF::LifeCycle
     * ----------------------------------------------------------------------
     */
    
    void
    CPI::SCA::Cp289GenericProxy::
    releaseObject ()
      throw (CF::LifeCycle::ReleaseError,
             CORBA::SystemException)
    {
      /*
       * Disconnect and release all ports.
       */

      CU::AutoMutex lock (m_mutex);
      CPI::Logger::DebugLogger debug (*m_logger);
      
      if (m_disabled) {
        throw CORBA::BAD_INV_ORDER ();
      }
      releasePorts();
      // base class deactivates
      BaseProxy::releaseObject ();
    }

    /*
     * ----------------------------------------------------------------------
     * CF::PortSupplier
     * ----------------------------------------------------------------------
     */
    
    CORBA::Object_ptr
    CPI::SCA::Cp289GenericProxy::
    getPort (const char * name)
      throw (CF::PortSupplier::UnknownPort,
             CORBA::SystemException)
    {
      CU::AutoMutex lock (m_mutex);
      CPI::Logger::DebugLogger debug (*m_logger);

      if (m_disabled) {
        throw CORBA::BAD_INV_ORDER ();
      }

      debug << m_logProducerName
            << CPI::Logger::Verbosity (2)
            << "getPort (\""
            << name
            << "\")"
            << std::flush;

      Cp289GenericProxyPort *cggp;
      PortMap::iterator pit = m_portMap.find (name);
      if (pit != m_portMap.end())
        cggp = (*pit).second;
      else {
        try {
          cggp = new Cp289GenericProxyPort (name, this);
        }
        catch (const CORBA::Exception & oops) {
          *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
                    << m_logProducerName
                    << "Oops: "
                    << CPI::CORBAUtil::Misc::stringifyCorbaException (oops)
                    << std::flush;
          throw;
        }
        catch (const std::bad_alloc & oops) {
          *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
                    << m_logProducerName
                    << "Oops: " << oops.what ()
                    << std::flush;
          throw CORBA::NO_RESOURCES();
        }
        catch (...) {
          *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
                    << m_logProducerName
                    << "Oops."
                    << std::flush;
          throw;
        }
        m_portMap[name] = cggp;
      }
      return CORBA::Object::_duplicate (cggp->getScaPort());
    }

    void Cp289GenericProxy::
    releasePorts() {
      for (PortMap::iterator pit = m_portMap.begin ();
           pit != m_portMap.end (); pit++)
        (*pit).second->release();
      m_portMap.clear();
    }

    /*
     * ----------------------------------------------------------------------
     * Configure and query the worker.
     * ----------------------------------------------------------------------
     */

    // This macrology generates an inline case to a directly dispatched switch statement
    // (based on contiguous enumeration)
    // "_H" are the types that need "helpers" per the C++ CORBA language mapping.

#undef CPI_DATA_TYPE_H
#undef CPI_DATA_TYPE_S

#define CPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                \
    case CM::Property::CPI_##pretty:                                        \
    if (p.isSequence()) {                                                \
      const CORBA::StringSeq *seq;                                        \
      if (!(any >>= seq))                                                \
        oops = "property value is not correct string sequence type";        \
      else                                                                \
        p.setStringSequenceValue((const run *)seq->get_buffer(), seq->length()); \
    } else {                                                                \
      /* we simply have a string */                                        \
      const char *cp;                                                        \
      CORBA::ULong bound;                                                \
      if (!(any >>= CORBA::Any::to_string(cp, bound)))                        \
        oops = "property value not a string as is should be";                \
      else                                                                \
        p.setStringValue(cp);                                                \
    }                                                                        \
        break;                                                                \
        /**/

    // For simple types (not strings) and simple sequences (not strings)
#undef CPI_DATA_TYPE_H
#define CPI_DATA_TYPE_H(sca,corba,letter,bits,run,pretty,store)                \
    SCA_SIMPLE(sca,corba,letter,bits,CORBA::Any::to_##sca(typed_value), pretty, run)
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                \
    SCA_SIMPLE(sca,corba,letter, bits, typed_value, pretty, run)
#define SCA_SIMPLE(l,c,t,n,h,pt,run)                                        \
    case CM::Property::CPI_##pt:                                        \
    /* now we know its not MMIO */                                        \
    if (p.isSequence()) {                                                \
      const CORBA::c##Seq *seq;                                                \
      if (any >>= seq)                                                        \
        p.set##pt##SequenceValue((const run *)seq->get_buffer(), seq->length()); \
      else                                                                \
        oops = "property value not correct sequence type";                \
    } else {                                                                \
      /* It is a simple scalar type that is not MMIO */                        \
      CORBA::c typed_value;                                                \
      if ((any >>= h))                                                        \
        p.set##pt##Value(typed_value);                                        \
      else                                                                \
        oops =  "property value has incorrect type";                        \
    }                                                                        \
        break;                                                                \
        /**/  

    void
    CPI::SCA::Cp289GenericProxy::
    configureWorker (const char * name,
                     const CORBA::Any & any,
                     bool last,
                     bool & needSync)
      throw (std::string)
    {
      // There is a slight amount of caching optimization possible
      // if we had a (redundant) name mapping to pre-established CC:Property objects.
      CC::Property p(m_worker, name);
      needSync = p.needWriteSync();
      const char *oops = 0;
      switch (p.getType()) {
        CPI_PROPERTY_DATA_TYPES
      case CM::Property::CPI_none:
      case CM::Property::CPI_data_type_limit:
        ;
      }
      if (oops)
        throw std::string (oops);
      if (last && needSync)
        m_worker.afterConfigure();
    }

#undef CPI_DATA_TYPE_H
#define CPI_DATA_TYPE_H(sca,corba,letter,bits,run,pretty,store)                \
    SCA_SIMPLE(sca,corba,letter,bits,CORBA::Any::from_##sca(typed_value), pretty, run)
#undef CPI_DATA_TYPE_S
#undef SCA_SIMPLE
    // For simple types (not strings) and simple sequences (not strings)
    // We allocate storage - which we could in fact preallocate, but would that avoid an extra allocation?
#define SCA_SIMPLE(l,c,t,n,h,pt,run)                                        \
    case CM::Property::CPI_##pt:                                        \
    if (p.isSequence()) {                                                \
      unsigned size = p.sequenceSize();                                        \
      /* is a sequence FIXME trycatch for allocation unless cached*/        \
      /* cached is probaby better but must not consume into any */        \
      CORBA::c##Seq *seq = new CORBA::c##Seq(size);                        \
      seq->length(p.get##pt##SequenceValue((run *)seq->get_buffer(), size)); \
      any <<= seq; /* seq and its buffer are consumed */                \
    } else {                                                                \
      CORBA::c typed_value = p.get##pt##Value();                        \
      any <<= h;                                                        \
    }                                                                        \
        break;                                                                \
        /**/  

#define CPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)        \
    case CM::Property::CPI_##pretty: {                                \
      unsigned length = p.stringSize();                                \
      if (p.isSequence()) {                                        \
        uint32_t limit = p.sequenceSize();                        \
        CORBA::corba##Seq *seq = new CORBA::corba##Seq(limit);        \
        seq->length(limit);                                        \
        char **data = seq->get_buffer();                        \
        uint32_t nSpace = limit * (length + 1);                        \
        char *space = new char[nSpace];                                \
        p.getStringSequenceValue(data, limit, space, nSpace);        \
        for (unsigned i = 0; i < limit; i++)                        \
          if (!(data[i] = CORBA::string_dup(data[i]))) {        \
            oops =  "can't allocate string for property value";        \
            break;                                                \
          }                                                        \
        if (!oops)                                                \
          any <<= seq;                                                \
      } else {                                                        \
        /* we have a string, not a sequence */                        \
        char *cp = CORBA::string_alloc(length + 1);                \
        if (!cp)                                                \
          oops =  "can't allocate string for property value";        \
        else {                                                        \
          cp[length] = '\0';                                        \
          p.getStringValue(cp, length + 1);                        \
          any <<= cp;                                                \
        }                                                        \
      }                                                                \
    }                                                                \
        /**/

    void
    CPI::SCA::Cp289GenericProxy::
    queryWorker (const char * name,
                 CORBA::Any & any,
                 bool & haveSync)
      throw (std::string)
    {
      CC::Property p(m_worker, name);
      if (!haveSync &&
          ((haveSync = p.needReadSync())))
        m_worker.beforeQuery();
      const char * oops;
      switch (p.getType()) {
        CPI_PROPERTY_DATA_TYPES
      case CM::Property::CPI_none:
      case CM::Property::CPI_data_type_limit:
        ;
      }
      if (oops)
        throw std::string (oops);
    }

    const CPI::Metadata::Property *
    CPI::SCA::Cp289GenericProxy::
    getProperties (unsigned int & numProperties)
      throw ()
    {
      return m_worker.getProperties (numProperties);
    }

#define CONTROL_OP(x,c,t,s1,s2,s3) void CPI::SCA::Cp289GenericProxy::x##Worker() {m_worker.x();}
    CPI_CONTROL_OPS
#undef CONTROL_OP      

  }
}

#if 0

    const CPI::SCA::Test *
    CPI::SCA::Cp289GenericProxy::
    findTest (unsigned int testid)
      throw (CF::TestableObject::UnknownTest)
    {
      const CPI::SCA::Test * t = m_props.getTest (testid);

      if (!t) {
        throw CF::TestableObject::UnknownTest ();
      }

      return t;
    }

    /*
     * ----------------------------------------------------------------------
     * Connection management
     * ----------------------------------------------------------------------
     */

    void
    CPI::SCA::Cp289GenericProxy::
    connectPort (const std::string & portName,
                 const std::string & connectionId,
                 CORBA::Object_ptr connection)
      throw (CF::Port::InvalidPort,
             CF::Port::OccupiedPort,
             CORBA::SystemException)
    {
#if 0
      // FIXME:  base class wants this but it is obsolete now.
      // This method exists just for compatibility with the pre-OpenCPI connection
      // protocol where the proxy (not the proxy port) is contacted (mis) using the SCA protocol
      // (but nicely symmetrical)
      // We are being contacted from a pre-OpenCPI proxy
      // If we are a provider, our cggp should already exist
      Cp289GenericProxyPort *cggp;
      PortMap::iterator pit = m_portMap.find (name);
      if (pit != m_portMap.end()) {
        // Our port object exists, which means getPort was called before this,
        // which means we are a provider, 
        cggp = (*pit).second;
        cggp->preOpenCpiConnectToProvider(connectId, connection);
      } else
#endif
        // This shouldn't happen, since:
        // The CF will call the operation on the port, not the prxy.
        // The "other side" will only make this proxy call for provider ports.
        throw std::string ("Unexpected connectPort to CP289Generic Proxy");
    }

    void
    CPI::SCA::Cp289GenericProxy::
    disconnectPort (const std::string & portName,
                    const std::string & connectionId)
      throw (CF::Port::InvalidPort,
             CORBA::SystemException)
    {
#if 0
      // FIXME:  base class wants this but it is obsolete now.
      CU::AutoMutex lock (m_mutex);
      CPI::Logger::DebugLogger debug (*m_logger);

      if (m_disabled) {
        throw CORBA::BAD_INV_ORDER ();
      }

      debug << m_logProducerName
            << CPI::Logger::Verbosity (2)
            << "disconnectPort (\""
            << portName
            << "\", \""
            << connectionId
            << "\")"
            << std::flush;

      try {
        disconnectPortLocked (portName, connectionId);
      }
      catch (const std::string & oops) {
        std::string msg = "Failed to disconnect port \"";
        msg += portName;
        msg += "\": ";
        msg += oops;
        msg += " (connection id \"";
        msg += connectionId;
        msg += ")";

        *m_logger << CPI::Logger::Level::EXCEPTION_ERROR
                  << m_logProducerName
                  << msg << "."
                  << std::flush;

        CF::Port::InvalidPort ip;
        ip.errorCode = CF::CF_EINVAL;
        ip.msg = msg.c_str ();
        throw ip;
      }
#endif
    }

    void
    CPI::SCA::Cp289GenericProxy::
    disconnectPortLocked (const std::string & portName,
                          const std::string & connectionId)
      throw (std::string)
    {
      PortMap::iterator pit = m_portMap.find (portName);
      cpiAssert (pit != m_portMap.end());
      PortData & pd = (*pit).second;

      ConnectionMap::iterator cmit = m_connections.find (connectionId);

      if (cmit == m_connections.end()) {
        throw std::string ("Connection id not in use");
      }

      try {
        ConnectionData & cd = (*cmit).second;

        /*
         * Ask the container to disconnect.
         */

        cpiAssert (cd.connectionCookie);

        try {
          m_container->disconnectPorts (&m_application,
                                        cd.connectionCookie);
        }
        catch (const CU::EmbeddedException & oops) {
          const char * auxInfo = oops.getAuxInfo ();
          std::string msg = "Failed to break connection: error code ";
          msg += CU::Misc::unsignedToString (static_cast<unsigned int> (oops.getErrorCode()));

          if (auxInfo && *auxInfo) {
            msg += ": ";
            msg += auxInfo;
          }

          throw msg;
        }

        if (!pd.provider) {
          /*
           * Also tell the remote container to disconnect.
           */

          try {
            cd.remotePort->disconnectPort (connectionId.c_str());
          }
          catch (const CORBA::Exception & oops) {
            std::string msg = "Failed to disconnect remote port: ";
            msg += CPI::CORBAUtil::Misc::stringifyCorbaException (oops);
            throw msg;
          }
        }
      }
      catch (...) {
        m_connections.erase (cmit);
        throw;
      }

      m_connections.erase (cmit);
    }

    /*
     * ----------------------------------------------------------------------
     * Misc
     * ----------------------------------------------------------------------
     */

    unsigned int
    CPI::SCA::Cp289GenericProxy::
    computeMaximumBufferSize (unsigned int memorySize,
                              unsigned int bufferCount)
      throw ()
    {
      /*
       * The bitstream meta-data stores the total memory size for this port.
       * The memory holds data (bufferSize * bufferCount), plus there is a
       * per-buffer overhead of 256 bytes per local buffer times remote
       * buffer (N*M according to John, I don't know why it's just N+M)
       * and a per-port overhead of 1024 bytes.  (According to John, these
       * are worst-case numbers; the actual constants are hardware and
       * fabric specific.)
       *
       * We don't know the number of remote buffers, but with the current
       * RPL limitation to n:n buffering, the number of remote buffers must
       * match the number of local buffers.
       *
       * I.e., memorySize >= bufferCount * bufferSize + 256 * bufferCount^2 + 1024
       *
       * So, given the total memorySize and the bufferCount, we can compute
       * the maximum data buffer size:
       *
       *   bufferSize <= (memorySize - 256 * bufferCount^2 - 1024) / bufferCount
       * or
       *   bufferSize <= (memorySize - 1024) / bufferCount - 256 * bufferCount
       */

      if (memorySize < 1024) {
        return 0;
      }

      unsigned int bufferSize = (memorySize - 1024) / bufferCount;

      if (bufferSize < 256 * bufferCount) {
        return 0;
      }

      bufferSize -= 256 * bufferCount;

      /*
       * HACK: John tells me that he hasn't tested with values larger than 2048.
       */

      if (bufferSize > 2048) {
        bufferSize = 2048;
      }

      return bufferSize;
    }
#endif

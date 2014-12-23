
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
 * SCA CP289 Generic Proxy.
 *
 * Revision History:
 *
 *     06/29/2009 - Jim Kulp
 *                  Initial version
 */

#include "OcpiStringifyCorbaException.h"
#include "OcpiLoggerDebugLogger.h"
#include "ContainerWorker.h"
#include "Cp289GenericProxy.h"

/*
 * Counteract the TAO/ORBexpress compatibility magic.
 */

#ifdef IOP
#undef IOP
#endif

namespace OCPI {
  namespace SCA {
    namespace OC = OCPI::Container;
    namespace OU = OCPI::Util;
    namespace OA = OCPI::API;

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
                       const std::string & aIdentifier,
                       // spd:softpkg/implementation/code/localfile@name
                       const char *codeLocalFileName,
                       // spd:softpkg/implementation/code/entrypoint
                       const char *functionName,
                       const char *instanceName,
                       OA::ContainerApplication &application,
                       const char *namingContextIor,
                       const char *nameBinding,
                       // Optional
                       OCPI::Logger::Logger * logger,
                       bool adoptLogger,
                       bool shutdownOrbOnRelease)
      throw (std::string)
      : BaseProxy (orb, poa, aIdentifier,
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
      //delete &m_worker; FIXME: mark for deletion (release), and take down the app
      // when the last one is gone
    }

    /*
     * ----------------------------------------------------------------------
     * CF::LifeCycle
     * ----------------------------------------------------------------------
     */
    
    void
    OCPI::SCA::Cp289GenericProxy::
    releaseObject ()
      throw (CF::LifeCycle::ReleaseError,
             CORBA::SystemException)
    {
      /*
       * Disconnect and release all ports.
       */

      OU::AutoMutex lock (m_mutex);
      OCPI::Logger::DebugLogger debug (*m_logger);
      
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
    OCPI::SCA::Cp289GenericProxy::
    getPort (const char * name)
      throw (CF::PortSupplier::UnknownPort,
             CORBA::SystemException)
    {
      OU::AutoMutex lock (m_mutex);
      OCPI::Logger::DebugLogger debug (*m_logger);

      if (m_disabled) {
        throw CORBA::BAD_INV_ORDER ();
      }

      debug << m_logProducerName
            << OCPI::Logger::Verbosity (2)
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
          *m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
                    << m_logProducerName
                    << "Oops: "
                    << OCPI::CORBAUtil::Misc::stringifyCorbaException (oops)
                    << std::flush;
          throw;
        }
        catch (const std::bad_alloc & oops) {
          *m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
                    << m_logProducerName
                    << "Oops: " << oops.what ()
                    << std::flush;
          throw CORBA::NO_RESOURCES();
        }
        catch (...) {
          *m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
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

#undef OCPI_DATA_TYPE_H
#undef OCPI_DATA_TYPE_S

#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)                \
    case OA::OCPI_##pretty:					\
    if (p.m_info.m_isSequence) {                                                \
      const CORBA::StringSeq *seq;                                        \
      if (!(any >>= seq))                                                \
        oops = "property value is not correct string sequence type";        \
      else                                                                \
        p.setStringSequenceValue((const run *)seq->get_buffer(), seq->length()); \
    } else {                                                                \
      /* we simply have a string */                                        \
      const char *cp;                                                        \
      CORBA::ULong bound = 0;                                                \
      if (!(any >>= CORBA::Any::to_string(cp, bound)))                        \
        oops = "property value not a string as is should be";                \
      else                                                                \
        p.setStringValue(cp);                                                \
    }                                                                        \
        break;                                                                \
        /**/

    // For simple types (not strings) and simple sequences (not strings)
#undef OCPI_DATA_TYPE_H

#define OCPI_DATA_TYPE_H(sca,corba,letter,bits,run,pretty,store)                \
    SCA_SIMPLE(sca,corba,letter,bits,CORBA::Any::to_##sca(typed_value), pretty, run)

#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                \
    SCA_SIMPLE(sca,corba,letter, bits, typed_value, pretty, run)

#define SCA_SIMPLE(l,c,t,n,h,pt,run)                                        \
    case OA::OCPI_##pt:					\
    /* now we know its not MMIO */                                        \
    if (p.m_info.m_isSequence) {                                                \
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
    OCPI::SCA::Cp289GenericProxy::
    configureWorker (const char * name,
                     const CORBA::Any & any,
                     bool last,
                     bool & needSync)
      throw (std::string)
    {
      // There is a slight amount of caching optimization possible
      // if we had a (redundant) name mapping to pre-established CC:Property objects.
      OA::Property p(m_worker, name);
      needSync = p.writeSync();
      const char *oops = 0;
      switch (p.m_info.m_baseType) {
        OCPI_PROPERTY_DATA_TYPES
      case OA::OCPI_none:
      case OA::OCPI_Struct:
      case OA::OCPI_Type:
      case OA::OCPI_Enum:
      case OA::OCPI_scalar_type_limit:
        ;
      }
      if (oops)
        throw std::string (oops);
      if (last && needSync)
        m_worker.afterConfigure();
    }

#undef OCPI_DATA_TYPE_H
#define OCPI_DATA_TYPE_H(sca,corba,letter,bits,run,pretty,store)                \
    SCA_SIMPLE(sca,corba,letter,bits,CORBA::Any::from_##sca(typed_value), pretty, run)
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
#undef SCA_SIMPLE
    // For simple types (not strings) and simple sequences (not strings)
    // We allocate storage - which we could in fact preallocate, but would that avoid an extra allocation?
#define SCA_SIMPLE(l,c,t,n,h,pt,run)                                        \
    case OA::OCPI_##pt:					\
    if (p.m_info.m_isSequence) {						\
      size_t size = p.m_info.m_sequenceLength;                                        \
      /* is a sequence FIXME trycatch for allocation unless cached*/        \
      /* cached is probaby better but must not consume into any */        \
      CORBA::c##Seq *seq = new CORBA::c##Seq(OCPI_UTRUNCATE(unsigned,size)); \
      seq->length(p.get##pt##SequenceValue((run *)seq->get_buffer(), size)); \
      any <<= seq; /* seq and its buffer are consumed */                \
    } else {                                                                \
      CORBA::c typed_value = p.get##pt##Value();                        \
      any <<= h;                                                        \
    }                                                                        \
        break;                                                                \
        /**/  

#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)        \

    void
    OCPI::SCA::Cp289GenericProxy::
    queryWorker (const char * name,
                 CORBA::Any & any,
                 bool & haveSync)
      throw (std::string)
    {
      OA::Property p(m_worker, name);
      if (!haveSync &&
          ((haveSync = p.readSync())))
        m_worker.beforeQuery();
      const char * oops = NULL;
      switch (p.m_info.m_baseType) {
        OCPI_PROPERTY_DATA_TYPES
      case OA::OCPI_String: {
	  size_t length = p.m_info.m_stringLength;
	  if (p.m_info.m_isSequence) {
	    size_t limit = p.m_info.m_sequenceLength;
	    CORBA::StringSeq *seq = new CORBA::StringSeq(OCPI_UTRUNCATE(unsigned,limit));
	    seq->length(OCPI_UTRUNCATE(unsigned,limit));
	    char **data = seq->get_buffer();
	    size_t nSpace = limit * (length + 1);
	    char *space = new char[nSpace];
	    p.getStringSequenceValue(data, limit, space, nSpace);
	    for (unsigned i = 0; i < limit; i++)
	      if (!(data[i] = CORBA::string_dup(data[i]))) {
		oops =  "can't allocate string for property value";
		break;
	      }
	    if (!oops)
	      any <<= seq;
	  } else {
	    /* we have a string, not a sequence */
	    char *cp = CORBA::string_alloc(OCPI_UTRUNCATE(unsigned,length + 1));
	    if (!cp)
	      oops =  "can't allocate string for property value";
	    else {
	      cp[length] = '\0';
	      p.getStringValue(cp, length + 1);
	      any <<= cp;
	    }
	  }
	}
      case OA::OCPI_none:
      case OA::OCPI_Struct:
      case OA::OCPI_Type:
      case OA::OCPI_Enum:
      case OA::OCPI_scalar_type_limit:
        ;
      }
      if (oops)
        throw std::string (oops);
    }

    const OU::Property *
    OCPI::SCA::Cp289GenericProxy::
    getProperties (unsigned int & numProperties)
      throw ()
    {
      return static_cast<OC::Worker *>(&m_worker)->properties (numProperties);
    }

#undef CONTROL_OP_I
#define CONTROL_OP_I(x,c,t,s1,s2,s3,s4)
#define CONTROL_OP(x,c,t,s1,s2,s3,s4) void OCPI::SCA::Cp289GenericProxy::x##Worker() {m_worker.x();}
    OCPI_CONTROL_OPS
#undef CONTROL_OP      
#undef CONTROL_OP_I
#define CONTROL_OP_I CONTROL_OP
  }
}

#if 0

    const OCPI::SCA::Test *
    OCPI::SCA::Cp289GenericProxy::
    findTest (unsigned int testid)
      throw (CF::TestableObject::UnknownTest)
    {
      const OCPI::SCA::Test * t = m_props.getTest (testid);

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
    OCPI::SCA::Cp289GenericProxy::
    connectPort (const std::string & portName,
                 const std::string & connectionId,
                 CORBA::Object_ptr connection)
      throw (CF::Port::InvalidPort,
             CF::Port::OccupiedPort,
             CORBA::SystemException)
    {
#if 0
      // FIXME:  base class wants this but it is obsolete now.
      // This method exists just for compatibility with the pre-OpenOCPI connection
      // protocol where the proxy (not the proxy port) is contacted (mis) using the SCA protocol
      // (but nicely symmetrical)
      // We are being contacted from a pre-OpenOCPI proxy
      // If we are a provider, our cggp should already exist
      Cp289GenericProxyPort *cggp;
      PortMap::iterator pit = m_portMap.find (name);
      if (pit != m_portMap.end()) {
        // Our port object exists, which means getPort was called before this,
        // which means we are a provider, 
        cggp = (*pit).second;
        cggp->preOpenOcpiConnectToProvider(connectId, connection);
      } else
#endif
        // This shouldn't happen, since:
        // The CF will call the operation on the port, not the prxy.
        // The "other side" will only make this proxy call for provider ports.
        throw std::string ("Unexpected connectPort to CP289Generic Proxy");
    }

    void
    OCPI::SCA::Cp289GenericProxy::
    disconnectPort (const std::string & portName,
                    const std::string & connectionId)
      throw (CF::Port::InvalidPort,
             CORBA::SystemException)
    {
#if 0
      // FIXME:  base class wants this but it is obsolete now.
      CU::AutoMutex lock (m_mutex);
      OCPI::Logger::DebugLogger debug (*m_logger);

      if (m_disabled) {
        throw CORBA::BAD_INV_ORDER ();
      }

      debug << m_logProducerName
            << OCPI::Logger::Verbosity (2)
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

        *m_logger << OCPI::Logger::Level::EXCEPTION_ERROR
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
    OCPI::SCA::Cp289GenericProxy::
    disconnectPortLocked (const std::string & portName,
                          const std::string & connectionId)
      throw (std::string)
    {
      PortMap::iterator pit = m_portMap.find (portName);
      ocpiAssert (pit != m_portMap.end());
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

        ocpiAssert (cd.connectionCookie);

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
            msg += OCPI::CORBAUtil::Misc::stringifyCorbaException (oops);
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
    OCPI::SCA::Cp289GenericProxy::
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

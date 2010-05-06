/*
 * SCA CP289 Executable Device.
 *
 * Revision History:
 *
 *     07/09/2009 - Jim Kulp
 *                  Initial version, starting from old parts
 */

#include <new>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <CpiOsMisc.h>
#include <CpiOsAssert.h>
#include <CpiLoggerLogger.h>
#include <CpiLoggerDebugLogger.h>
#include <CpiUtilMisc.h>
#include <CpiUtilAutoMutex.h>
#include "CpiContainerInterface.h"
#include "CpiPValue.h"
#include "CpiApplication.h"
#include "CpiDriver.h"
#include <CpiCFUtilMisc.h>
#include <CpiCFUtilDeviceBase.h>
#include <CpiCFUtilLegacyErrorNumbers.h>
#include <CpiStringifyCorbaException.h>
#include <Cp289GenericProxy.h>
#include <CF_s.h>
#include "Cp289ExecutableDevice.h"


namespace CU = CPI::Util;

#if 0
/*
 * Dispatch thread helper.
 */

namespace {

  struct DispatchThreadData {
    CPI::Container::Interface * container;
    DataTransfer::EventManager * eventManager;
  };

  void
  dispatchThread (void * opaque)
  {
    DispatchThreadData * dtd = static_cast<DispatchThreadData *> (opaque);
    CPI::Container::Interface::DispatchRetCode rc;
    bool keepWorking = true;

    while (keepWorking) {
      rc = dtd->container->dispatch (dtd->eventManager);

      switch (rc) {
      case CPI::Container::Interface::DispatchNoMore:
        // All done, exit from dispatch thread.
        keepWorking = false;
        break;

      case CPI::Container::Interface::MoreWorkNeeded:
        // No-op. To prevent blocking the CPU, yield.
        CPI::OS::sleep (0);
        break;

      case CPI::Container::Interface::Stopped:
        // Go to sleep until started again.  In this device, means all done.
        keepWorking = false;
        break;

      case CPI::Container::Interface::Spin:
        /*
         * If we have an event manager, ask it to go to sleep and wait for
         * an event.  If we are not event driven, the event manager will
         * tell us that it is spinning.  In that case, yield to give other
         * threads a chance to run.
         */

        if (dtd->eventManager) {
          if (dtd->eventManager->waitForEvent (0) == DataTransfer::EventSpin) {
            CPI::OS::sleep (0);
          }
        }
        else {
          CPI::OS::sleep (0);
        }
        break;
      }
    }

    delete dtd;
  }

}
#endif

namespace CPI {
  namespace SCA {
    Cp289ExecutableDevice::
    Cp289ExecutableDevice (CORBA::ORB_ptr orb,
                           PortableServer::POA_ptr poa,
                           CF::DeviceManager_ptr devMgr,
                           const std::string &profileFileName,
                           const std::string &identifier,
                           const std::string &label,
                           const std::string &tempDir,
                           unsigned int cpiDeviceId,
                           const std::string & cpiDeviceType,
                           const std::string & cpiDeviceUnit,
                           const std::string & endpoint,
                           bool polled,
                           const std::string & osName,
                           const std::string & processorName,
                           CPI::Logger::Logger * logger,
                           bool adoptLogger,
                           bool shutdownOrbOnRelease)
      throw (std::string)
      : CPI::CFUtil::DeviceBase (orb,
                                 poa,
                                 devMgr,
                                 profileFileName,
                                 identifier,
                                 label,
                                 logger,
                                 adoptLogger,
                                 shutdownOrbOnRelease),
        m_driverManager( cpiDeviceType.c_str() ),
        m_osName(osName), m_processorName(processorName),
        m_cpiDeviceId(cpiDeviceId),
        m_cpiDeviceType(cpiDeviceType),
        m_container(0), m_application(0), m_eventManager(0), m_tempDir(tempDir)
    {
      CPI::Logger::DebugLogger debug (m_out);
      debug << m_logProducerName
            << "CPI SCA CP289 Executable Device for device id "
            << m_cpiDeviceId
            << " constructor."
            << std::flush;

      try {

#ifdef PRE_PORT
        // FIXME: pass polled parameter as PValue?
        CC::Interface *container =
          CC::Driver::getContainer(cpiDeviceType.c_str(), cpiDeviceUnit.c_str(),
                                   endpoint.c_str(), 0);
#endif

        CPI::Util::PValue cprops[] = {CPI::Util::PVString("endpoint",(char*)endpoint.c_str() ),
                                      CPI::Util::PVBool("polling",polled),
                                      CPI::Util::PVEnd };
        CC::Interface *container = static_cast< CC::Interface * >( 
                                  m_driverManager.getDevice( cprops, cpiDeviceUnit.c_str() ));




        if (!container)
          throw CC::ApiError("Couldn't find or create container of type \"",
                             cpiDeviceType.c_str(), "\" with name \"", identifier.c_str(), "\"", NULL);

        try { // catch errors to release container. Note it will take everything else down

#ifdef PRE_PORT
          m_application = container->createApplication(processorName.c_str());
#endif

          m_application = container->createApplication();

#if 0

          /*
           * Start the dispatch thread.
           */
          DispatchThreadData * dtd = new DispatchThreadData;
          
          try {
            dtd->container = m_container;
            dtd->eventManager = m_eventManager;
            m_dispatchThreadMgr.start (dispatchThread, dtd);
            
            /*
             * Start the container.
             */
            container->start (m_eventManager);
          } catch (...) {
            delete dtd;
            throw;
          }
#endif
        } catch (...) {
          delete container;
          throw;
        }
        m_container = container;
      } catch (const CPI::Util::EmbeddedException & oops) {
        const char * auxInfo = oops.getAuxInfo ();
        std::string msg = "Error creating RPL container: error code ";
        msg += CPI::Util::Misc::unsignedToString (static_cast<unsigned int> (oops.getErrorCode()));
        
        if (auxInfo && *auxInfo) {
          msg += ": ";
          msg += auxInfo;
        }
        throw msg;
      } catch (const std::bad_alloc & oops) {
        throw std::string (oops.what());
      }
    }

    Cp289ExecutableDevice::
    ~Cp289ExecutableDevice ()
      throw ()
    {
      CPI::Logger::DebugLogger debug (m_out);
      debug << m_logProducerName
            << "CPI SCA CP289 Executable Device for device id "
            << m_cpiDeviceId
            << " destructor."
            << std::flush;

#if 0
      /*
       * Join the dispatch thread.
       */

      try {
        m_dispatchThreadMgr.join ();
      }
      catch (...) {
      }
#endif

      /*
       * Clean up.
       */

      delete m_container;
    }

    void
    Cp289ExecutableDevice::
    configure (const CF::Properties & props)
      throw (CF::PropertySet::InvalidConfiguration,
             CF::PropertySet::PartialConfiguration,
             CORBA::SystemException)
    {
      CPI::Util::AutoMutex mutex (m_mutex);
      CPI::Logger::DebugLogger debug (m_out);

      CORBA::ULong numProps = props.length ();
      CF::Properties invalidProperties;
      CORBA::ULong numInvalidProperties = 0;
      
      debug << m_logProducerName
            << CPI::Logger::Verbosity (2)
            << "configure (";

      for (CORBA::ULong dpi=0; dpi<numProps; dpi++) {
        if (dpi > 0) {
          debug << ", ";
        }

        debug << props[dpi].id;
      }

      debug << ")" << std::flush;

      // FIXME:  should these be in devicebase?

      for (CORBA::ULong pi=0; pi<numProps; pi++) {
        const CF::DataType & property = props[pi];
        const char * propertyId = property.id.in ();

        if (std::strcmp (propertyId, "PRODUCER_LOG_LEVEL") == 0) {
          if (!configureProducerLogLevel (property)) {
            invalidProperties.length (numInvalidProperties + 1);
            invalidProperties[numInvalidProperties++] = property;
          }
        }
        else {
          invalidProperties.length (numInvalidProperties + 1);
          invalidProperties[numInvalidProperties++] = property;
        }
      }

      if (numInvalidProperties) {
        m_out << CPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Configuration failed for "
              << ((numInvalidProperties != 1) ? "properties " : "property ");

        for (CORBA::ULong dpi=0; dpi<numInvalidProperties; dpi++) {
          if (dpi > 0 && dpi+1 == numInvalidProperties) {
            m_out << " and ";
          }
          else if (dpi > 0) {
            m_out << ", ";
          }

          m_out << invalidProperties[dpi].id;
        }

        m_out << "." << std::flush;
      }

      if (numInvalidProperties > 0 && numInvalidProperties == numProps) {
        // No property was successfully configured.
        CF::PropertySet::InvalidConfiguration ic;
        ic.msg = "configure failed";
        ic.invalidProperties = invalidProperties;
        throw ic;
      }
      else if (numInvalidProperties) {
        // Some, but not all properties were successfully configured.
        CF::PropertySet::PartialConfiguration pc;
        pc.invalidProperties = invalidProperties;
        throw pc;
      }

      m_out << CPI::Logger::Level::ADMINISTRATIVE_EVENT
            << m_logProducerName
            << "Configuration complete."
            << std::flush;
    }

    void
    Cp289ExecutableDevice::
    query (CF::Properties & props)
      throw (CF::UnknownProperties,
             CORBA::SystemException)
    {
      CPI::Util::AutoMutex mutex (m_mutex);
      CPI::Logger::DebugLogger debug (m_out);

      CORBA::ULong numProps = props.length ();
      CORBA::ULong numInvalidProperties = 0;
      CF::UnknownProperties up;

      debug << m_logProducerName
            << CPI::Logger::Verbosity (2)
            << "query (";

      for (CORBA::ULong dpi=0; dpi<numProps; dpi++) {
        if (dpi > 0) {
          debug << ", ";
        }

        debug << props[dpi].id;
      }

      debug << ")" << std::flush;

      if (!numProps) {
        CU::PValue *pvals = m_container->getProperties();
        for (numProps = 0; pvals[numProps].name; numProps++)
          ;
        /*
         * We're meant to return all properties.  Initialize props with all
         * property names, so that the code below fills them in.
         */
        props.length (numProps + 4);
        for (numProps = 0; pvals[numProps].name; numProps++)
          props[numProps].id = pvals[numProps].name;
        props[++numProps].id = "DCE:b59fa5e6-5eb4-44f6-90f6-0548508f2ba2"; // CPIDeviceId allocation capability
        props[++numProps].id = "DCE:c788404e-b9f5-4532-8c7d-3588d328fff0"; // CPIDeviceType allocation capability
        props[++numProps].id = "DCE:c4b738d8-fbe6-4893-81cd-1bb7a77bfb43"; // CPIContainerType allocation capability
        props[++numProps].id = "PRODUCER_LOG_LEVEL"; // SCA required
      }

      for (CORBA::ULong pi=0; pi<numProps; pi++) {
        CF::DataType & property = props[pi];
        const char * propertyId = property.id.in ();

        if (CPI::Util::Misc::caseInsensitiveStringCompare (propertyId, "DCE:b59fa5e6-5eb4-44f6-90f6-0548508f2ba2") == 0)
          property.value <<= static_cast<CORBA::ULong> (m_cpiDeviceId);
        else if (CPI::Util::Misc::caseInsensitiveStringCompare (propertyId, "DCE:c788404e-b9f5-4532-8c7d-3588d328fff0") == 0)
          property.value <<= m_cpiDeviceType.c_str ();
        else if (CPI::Util::Misc::caseInsensitiveStringCompare (propertyId, "DCE:c4b738d8-fbe6-4893-81cd-1bb7a77bfb43") == 0)
          property.value <<= "RPL";
        else if (std::strcmp (propertyId, "PRODUCER_LOG_LEVEL") == 0)
          queryProducerLogLevel (property);
        else {
          CU::PValue *p = m_container->getProperty(propertyId);
          if (p) {
            switch (p->type) {
#undef CPI_DATA_TYPE_H
#define CPI_DATA_TYPE_H(sca,corba,letter,bits,run,pretty,store)                \
              SCA_SIMPLE(sca,corba,letter,bits,CORBA::Any::from_##sca(typed_value), pretty, run)
#define CPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)                \
              SCA_SIMPLE(sca,corba,letter,bits,typed_value, pretty, run)
#undef CPI_DATA_TYPE_S
#undef SCA_SIMPLE
#define SCA_SIMPLE(l,c,t,n,h,pt,run)                        \
              case CPI::Metadata::Property::CPI_##pt: {        \
              CORBA::c typed_value = p->v##pt;                \
              property.value <<= h; \
              break; }
#define CPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)        \
              case CPI::Metadata::Property::CPI_String:        {        \
              property.value <<= p->vString; \
              break; }
              CPI_PROPERTY_DATA_TYPES
            default:;
            }
          } else {
            up.invalidProperties.length (numInvalidProperties + 1);
            up.invalidProperties[numInvalidProperties++] = property;
          }
        }
      }

      if (numInvalidProperties) {
        m_out << CPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Query failed for "
              << ((numInvalidProperties != 1) ? "properties " : "property ");

        for (CORBA::ULong dpi=0; dpi<numInvalidProperties; dpi++) {
          if (dpi > 0 && dpi+1 == numInvalidProperties) {
            m_out << " and ";
          }
          else if (dpi > 0) {
            m_out << ", ";
          }

          m_out << up.invalidProperties[dpi].id;
        }

        m_out << "." << std::flush;
        throw up;
      }
    }
  
    void
    Cp289ExecutableDevice::
    load (CF::FileSystem_ptr fileSystem,
          const char * fileName,
          CF::LoadableDevice::LoadType loadKind)
      throw (CF::LoadableDevice::InvalidLoadKind,
             CF::LoadableDevice::LoadFail,
             CF::Device::InvalidState,
             CF::InvalidFileName,
             CORBA::SystemException)
    {
      CPI::Util::AutoMutex mutex (m_mutex);
      CPI::Logger::DebugLogger debug (m_out);

      debug << m_logProducerName
            << CPI::Logger::Verbosity (2)
            << "load (\""
            << fileName
            << "\", "
            << CPI::CFUtil::loadTypeToString (loadKind)
            << ")"
            << std::flush;

      if (m_adminState != CF::Device::UNLOCKED ||
          m_operationalState == CF::Device::DISABLED) {
        /*
         * Spec says that we should raise the InvalidState exception in
         * this case.
         */

        std::string msg;

        if (m_adminState != CF::Device::UNLOCKED) {
          msg = "Administrative state is ";
          msg += CPI::CFUtil::adminTypeToString (m_adminState);
        }
        else {
          msg = "Operational state is ";
          msg += CPI::CFUtil::operationalTypeToString (m_operationalState);
        }

        CF::Device::InvalidState is;
        is.msg = msg.c_str ();
        throw is;
      }

      if (loadKind != CF::LoadableDevice::EXECUTABLE) {
        m_out << CPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << "Can not load "
              << CPI::CFUtil::loadTypeToString (loadKind)
              << " \""
              << fileName
              << "\": only \"executable\" files are supported."
              << std::flush;

        throw CF::LoadableDevice::InvalidLoadKind ();
      }

      try {
        // We need to correlate future "execute" operations with the load operations
        // We do this by insisting that the primary artificat name be supplied as an argument
        // to the execute operation.  Since loading the same file twice is fine with the container,
        // We'll just load the primary artifact again later.  It will be forgotten when the
        // Application is torn down.
        // FIXME:  we might tear down the app when Nworkers goes to 0 from non-zero...
        m_application->loadArtifact(fileName, 0);
      } catch (CC::ApiError &e) {
        std::string msg;
        msg  = "Can not load file \"";
        msg += fileName;
        msg += "\": ";
        msg += e.m_auxInfo;
        msg += "\"";

        m_out << CPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << msg << "."
              << std::flush;

        CF::LoadableDevice::LoadFail lf;
        lf.errorNumber = CF::CF_EBUSY;
        lf.msg = msg.c_str ();
        throw lf;
      }
    }

    void
    Cp289ExecutableDevice::
    unload (const char * fileName)
      throw (CF::Device::InvalidState,
             CF::InvalidFileName,
             CORBA::SystemException)
    {
      CPI::Util::AutoMutex mutex (m_mutex);
      CPI::Logger::DebugLogger debug (m_out);

      debug << m_logProducerName
            << CPI::Logger::Verbosity (2)
            << "unload (\""
            << fileName
            << "\")"
            << std::flush;

      if (m_adminState != CF::Device::UNLOCKED ||
          m_operationalState == CF::Device::DISABLED) {
        /*
         * Spec says that we should raise the InvalidState exception in
         * this case.
         */

        std::string msg;

        if (m_adminState != CF::Device::UNLOCKED) {
          msg = "Administrative state is ";
          msg += CPI::CFUtil::adminTypeToString (m_adminState);
        }
        else {
          msg = "Operational state is ";
          msg += CPI::CFUtil::operationalTypeToString (m_operationalState);
        }

        CF::Device::InvalidState is;
        is.msg = msg.c_str ();
        throw is;
      }
      // Since the application owns the loaded files, there is no point in doing anything here.
      // FIXME:  teach the container to have an unload function that deals with refcounts etc.
      // try/catch: m_container.unloadArtifact(fileName);
    }
    

    /*
     * CF::ExecutableDevice
     */

    CF::ExecutableDevice::ProcessID_Type
    Cp289ExecutableDevice::
    execute (const char * functionName,
             const CF::Properties & options,
             const CF::Properties & parameters)
      throw (CF::ExecutableDevice::InvalidFunction,
             CF::ExecutableDevice::InvalidParameters,
             CF::ExecutableDevice::InvalidOptions,
             CF::ExecutableDevice::ExecuteFail,
             CF::Device::InvalidState,
             CF::InvalidFileName,
             CORBA::SystemException)
    {
      CPI::Util::AutoMutex mutex (m_mutex);
      CPI::Logger::DebugLogger debug (m_out);

      debug << m_logProducerName
            << CPI::Logger::Verbosity (2)
            << "execute (\""
            << functionName
            << "\")"
            << std::flush;

      if (m_adminState != CF::Device::UNLOCKED ||
          m_operationalState == CF::Device::DISABLED) {
        /*
         * Spec says that we should raise the InvalidState exception in
         * this case.
         */

        std::string msg;

        if (m_adminState != CF::Device::UNLOCKED) {
          msg = "Administrative state is ";
          msg += CPI::CFUtil::adminTypeToString (m_adminState);
        }
        else {
          msg = "Operational state is ";
          msg += CPI::CFUtil::operationalTypeToString (m_operationalState);
        }

        CF::Device::InvalidState is;
        is.msg = msg.c_str ();
        throw is;
      }

      /*
       * Validate that the options look legit.
       */

      {
        CF::ExecutableDevice::InvalidOptions io;
        CORBA::ULong numInvalidOptions = 0;

        for (CORBA::ULong oi=0; oi<options.length(); oi++) {
          const CF::DataType & option = options[oi];
          const char * optionId = option.id.in ();

          if (std::strcmp (optionId, "STACK_SIZE") == 0) {
            CORBA::ULong value;

            if (!(option.value >>= value)) {
              io.invalidOpts.length (numInvalidOptions + 1);
              io.invalidOpts[numInvalidOptions++] = option;
            }

            /* STACK_SIZE value is ignored */
          }
          else if (std::strcmp (optionId, "PRIORITY") == 0) {
            CORBA::ULong value;

            if (!(option.value >>= value)) {
              io.invalidOpts.length (numInvalidOptions + 1);
              io.invalidOpts[numInvalidOptions++] = option;
            }

            /* PRIORITY value is ignored */
          }
          else {
            io.invalidOpts.length (numInvalidOptions + 1);
            io.invalidOpts[numInvalidOptions++] = option;
          }
        }

        if (numInvalidOptions) {
          m_out << CPI::Logger::Level::EXCEPTION_ERROR
                << m_logProducerName
                << "execute: invalid options: ";

          for (CORBA::ULong ioi=0; ioi<numInvalidOptions; ioi++) {
            if (ioi) {
              m_out << ", ";
            }
            m_out << io.invalidOpts[ioi].id;
          }

          m_out << "." << std::flush;
          throw io;
        }
      }

      /*
       * To do: more error checking here.
       */

      std::string namingContextIor,  nameBinding, componentIdentifier;
      std::string componentInstantiationIdentifier, codeLocalFileName, codeInstanceName;
      int debugLevel = 0;

      {
        CF::ExecutableDevice::InvalidParameters ip;
        CORBA::ULong numInvalidParameters = 0;

        for (CORBA::ULong pi=0; pi<parameters.length(); pi++) {
          const CF::DataType & parameter = parameters[pi];
          const char * parameterId = parameter.id.in ();
          const char * value;

          if (!(parameter.value >>= value)) {
            ip.invalidParms.length (numInvalidParameters + 1);
            ip.invalidParms[numInvalidParameters++] = parameter;
            continue;
          }

          debug << m_logProducerName
                << CPI::Logger::Verbosity (2)
                << "Parameter " << pi << ": "
                << parameter.id << " = \""
                << value
                << "\""
                << std::flush;

          if (std::strcmp (parameterId, "debugLevel") == 0) {
            debugLevel = atoi (value);
          }
          else if (std::strcmp (parameterId, "NAMING_CONTEXT_IOR") == 0) {
            namingContextIor = value;
          }
          else if (std::strcmp (parameterId, "NAME_BINDING") == 0) {
            nameBinding = value;
          }
          else if (std::strcmp (parameterId, "COMPONENT_IDENTIFIER") == 0) {
            std::string::size_type colonIdx;

            componentIdentifier = value;

            if ((colonIdx = componentIdentifier.rfind (':')) == std::string::npos) {
              ip.invalidParms.length (numInvalidParameters + 1);
              ip.invalidParms[numInvalidParameters++] = parameter;
            }

            componentInstantiationIdentifier = componentIdentifier.substr (0, colonIdx);
          }
          else if (std::strcmp (parameterId, "COMPONENT_NAME") == 0) {
            /* Passed by SCARI++ 2.2.  Ignore. */
          }
          else if (std::strcmp (parameterId, "CODE_LOCAL_FILE_NAME") == 0) {
            codeLocalFileName = value;
          }
          else if (std::strcmp (parameterId, "CODE_INSTANCE_NAME") == 0) {
            codeInstanceName = value;
          }
          else {
            ip.invalidParms.length (numInvalidParameters + 1);
            ip.invalidParms[numInvalidParameters++] = parameter;
          }
        }

        if (numInvalidParameters) {
          m_out << CPI::Logger::Level::EXCEPTION_ERROR
                << m_logProducerName
                << "Invalid execution parameters: ";

          for (CORBA::ULong ipi=0; ipi<numInvalidParameters; ipi++) {
            if (ipi) {
              m_out << ", ";
            }
            m_out << ip.invalidParms[ipi].id;
          }

          m_out << "." << std::flush;
          throw ip;
        }
      }
      if (!namingContextIor.length() || !nameBinding.length()) {
        std::string msg = "Execution parameters \"NAMING_CONTEXT_IOR\" or \"NAME_BINDING\" missing";

        m_out << CPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << msg
              << "."
              << std::flush;

        CF::ExecutableDevice::ExecuteFail ef;
        ef.errorNumber = CF::CF_EINVAL;
        ef.msg = msg.c_str ();
        throw ef;
      }

      // Enable debug logging for this id at this level
#if !defined (NDEBUG)
      CPI::Logger::debug (componentIdentifier, debugLevel);
#endif
      CF::ExecutableDevice::ProcessID_Type pid;
      Cp289GenericProxy *gp;

      try {
        gp = new Cp289GenericProxy(m_orb, m_poa, componentIdentifier, codeLocalFileName.c_str(),
                                   functionName, codeInstanceName.c_str(), *m_application,
                                   0, false, false);
        pid = gp->getPid();
      } catch (const CPI::Util::EmbeddedException & oops) {
        const char * auxInfo = oops.getAuxInfo ();
        std::string msg = "Error executing worker with id \"";
        msg += componentIdentifier;
        msg += "\", function name \"";
        msg += functionName;
        msg += "\" ";
        msg += CPI::Util::Misc::unsignedToString (static_cast<unsigned int> (oops.getErrorCode()));

        if (auxInfo && *auxInfo) {
          msg += ": ";
          msg += auxInfo;
        }

        m_out << CPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << msg
              << "."
              << std::flush;

        CF::InvalidFileName ifn;
        ifn.errorNumber = CF::CF_EINVAL;
        ifn.msg = msg.c_str ();
        throw ifn;
      } catch (const CORBA::Exception & ex) {
        std::string msg = "Failed to register generic proxy in Naming Service: ";
        msg += CPI::CORBAUtil::Misc::stringifyCorbaException (ex);

        m_out << CPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << msg
              << "."
              << std::flush;
        CF::ExecutableDevice::ExecuteFail ef;
        ef.errorNumber = CF::CF_EINVAL;
        ef.msg = msg.c_str ();
        throw ef;
      }

      m_out << CPI::Logger::Level::ADMINISTRATIVE_EVENT
            << m_logProducerName
            << "Started \""
            << functionName
            << "\", process id "
            << pid
            << "."
            << std::flush;

      m_pidMap[pid] = gp;
      return pid;
    }

    void
    Cp289ExecutableDevice::
    terminate (CF::ExecutableDevice::ProcessID_Type pid)
      throw (CF::ExecutableDevice::InvalidProcess,
             CF::Device::InvalidState,
             CORBA::SystemException)
    {
      CPI::Util::AutoMutex mutex (m_mutex);
      CPI::Logger::DebugLogger debug (m_out);

      debug << m_logProducerName
            << CPI::Logger::Verbosity (2)
            << "terminate ("
            << pid
            << ")"
            << std::flush;
      
      if (m_adminState != CF::Device::UNLOCKED ||
          m_operationalState == CF::Device::DISABLED) {
        /*
         * Spec says that we should raise the InvalidState exception in
         * this case.
         */

        std::string msg;

        if (m_adminState != CF::Device::UNLOCKED) {
          msg = "Administrative state is ";
          msg += CPI::CFUtil::adminTypeToString (m_adminState);
        }
        else {
          msg = "Operational state is ";
          msg += CPI::CFUtil::operationalTypeToString (m_operationalState);
        }

        CF::Device::InvalidState is;
        is.msg = msg.c_str ();
        throw is;
      }

      PidMap::iterator it = m_pidMap.find(pid);
      if (it == m_pidMap.end()) {
        std::string msg = "Invalid pid ";
        msg += CPI::Util::Misc::integerToString (pid);
        m_out << CPI::Logger::Level::EXCEPTION_ERROR
              << m_logProducerName
              << msg
              << "."
              << std::flush;
        
        CF::ExecutableDevice::InvalidProcess ef;
        ef.errorNumber = CF::CF_EINVAL;
        ef.msg = msg.c_str ();
        throw ef;
      }
      Cp289GenericProxy *gp = it->second;
      
      try {
        gp->releaseObject();
      }
      catch (...) {
      }
      gp->_remove_ref();
      m_pidMap.erase(it);

      m_out << CPI::Logger::Level::ADMINISTRATIVE_EVENT
            << m_logProducerName
            << "Terminated worker "
            << pid
            << "."
            << std::flush;
    }

  }
}

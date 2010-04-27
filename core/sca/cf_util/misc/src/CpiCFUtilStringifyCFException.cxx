/**
 * \file
 * \brief Map an SCA exception to a string.
 *
 * Revision History:
 *
 *     04/14/2009 - Frank Pilhofer
 *                  Add support for SCA 2.2.
 *
 *     10/13/2008 - Frank Pilhofer
 *                  Initial version.
 */

#include <string>
#include <CF.h>
#include <CpiUtilMisc.h>
#include <CpiCFUtilMisc.h>
#include <CpiStringifyCorbaException.h>
#include "CpiCFUtilStringifyCFException.h"

#define EX_NO_DATA(type,text) \
	{ \
	  const type * ex = type::_downcast (&cex); \
	  if (ex) { \
	    return text; \
	  } \
	}

#define EX_WITH_MSG(type,text) \
	{ \
	  const type * ex = type::_downcast (&cex); \
	  if (ex) { \
	    reason = text; \
	    reason += ": "; \
	    reason += ex->msg.in (); \
	    return reason; \
	  } \
	}

#define EX_WITH_ERRNO_MSG(type,text) \
	{ \
	  const type * ex = type::_downcast (&cex); \
	  if (ex) { \
	    reason = text; \
	    reason += ": "; \
	    reason += CPI::CFUtil::errorNumberToString (ex->errorNumber); \
	    reason += ": "; \
	    reason += ex->msg.in (); \
	    return reason; \
	  } \
	}

#define EX_WITH_PROPERTIES(type,text,props) \
	{ \
	  const type * ex = type::_downcast (&cex); \
	  if (ex) { \
	    CORBA::ULong np = ex-> props .length (); \
	    reason = text; \
	    reason += ": \""; \
	    for (CORBA::ULong i=0; i<np; i++) { \
	      reason += ex-> props [i].id.in (); \
	      if (i+1 < np) { \
	        reason += "\", \""; \
	      } \
	    } \
	    reason += "\""; \
	    return reason; \
	  } \
	}

#define EX_WITH_PROPERTIES_AND_MSG(type,text,props) \
	{ \
	  const type * ex = type::_downcast (&cex); \
	  if (ex) { \
	    CORBA::ULong np = ex-> props .length (); \
	    reason = text; \
	    reason += ": "; \
	    reason += ex->msg.in (); \
	    reason += ": \""; \
	    for (CORBA::ULong i=0; i<np; i++) { \
	      reason += ex-> props [i].id.in (); \
	      if (i+1 < np) { \
	        reason += "\", \""; \
	      } \
	    } \
	    reason += "\""; \
	    return reason; \
	  } \
	}

std::string
CPI::CFUtil::
stringifyCFException (const CORBA::Exception & cex)
  throw ()
{
  std::string reason;

  EX_NO_DATA (CF::InvalidProfile, "Invalid profile");
  EX_WITH_MSG (CF::InvalidObjectReference, "Invalid object reference");
  EX_WITH_PROPERTIES (CF::UnknownProperties, "Unknown properties", invalidProperties);
  EX_WITH_ERRNO_MSG (CF::InvalidFileName, "Invalid file name");
  EX_WITH_ERRNO_MSG (CF::FileException, "File exception");

  EX_WITH_PROPERTIES (CF::FileSystem::UnknownFileSystemProperties, "Unknown file system properties", invalidProperties);
  EX_WITH_ERRNO_MSG (CF::File::IOException, "File I/O exception");
  EX_NO_DATA (CF::File::InvalidFilePointer, "Invalid file pointer");

  EX_NO_DATA (CF::ResourceFactory::InvalidResourceId, "Invalid resource id");
  EX_WITH_MSG (CF::ResourceFactory::ShutdownFailure, "Shutdown failure");
  EX_WITH_ERRNO_MSG (CF::ResourceFactory::CreateResourceFailure, "Resource creation failure");

  EX_NO_DATA (CF::FileManager::NonExistentMount, "Non-existent mount point");
  EX_NO_DATA (CF::FileManager::InvalidFileSystem, "Invalid file system");
  EX_NO_DATA (CF::FileManager::MountPointAlreadyExists, "Mount point already exists");

  {
    const CF::Port::InvalidPort * ex =
      CF::Port::InvalidPort::_downcast (&cex);

    if (ex) {
      reason  = "Invalid port: ";
      reason += CPI::Util::Misc::unsignedToString (static_cast<unsigned int> (ex->errorCode));
      reason += ": ";
      reason += ex->msg.in ();
      return reason;
    }
  }

  EX_NO_DATA (CF::Port::OccupiedPort, "Occupied port");

  {
    const CF::LifeCycle::InitializeError * ex =
      CF::LifeCycle::InitializeError::_downcast (&cex);

    if (ex) {
      CORBA::ULong nip = ex->errorMessages.length ();

      reason = "Initialize error: \"";

      for (CORBA::ULong i=0; i<nip; i++) {
	reason += ex->errorMessages[i].in();

	if (i+1 < nip) {
	  reason += "\", \"";
	}
      }

      reason += "\"";
      return reason;
    }
  }

  {
    const CF::LifeCycle::ReleaseError * ex =
      CF::LifeCycle::ReleaseError::_downcast (&cex);

    if (ex) {
      CORBA::ULong nip = ex->errorMessages.length ();

      reason = "Release error: \"";

      for (CORBA::ULong i=0; i<nip; i++) {
	reason += ex->errorMessages[i].in();

	if (i+1 < nip) {
	  reason += "\", \"";
	}
      }

      reason += "\"";
      return reason;
    }
  }

  EX_NO_DATA (CF::TestableObject::UnknownTest, "Unknown test");
  EX_WITH_PROPERTIES_AND_MSG (CF::PropertySet::InvalidConfiguration, "Invalid configuration", invalidProperties);
  EX_WITH_PROPERTIES (CF::PropertySet::PartialConfiguration, "Partial configuration", invalidProperties);

  EX_WITH_ERRNO_MSG (CF::DomainManager::ApplicationInstallationError, "Application installation error");
#if ! defined (CPI_USES_SCA22)
  EX_NO_DATA (CF::DomainManager::ApplicationAlreadyInstalled, "Application already installed");
#endif
  EX_NO_DATA (CF::DomainManager::InvalidIdentifier, "Invalid identifier");
  EX_NO_DATA (CF::DomainManager::DeviceManagerNotRegistered, "Device manager not registered");
  EX_WITH_ERRNO_MSG (CF::DomainManager::ApplicationUninstallationError, "Application uninstallation error");
  EX_WITH_ERRNO_MSG (CF::DomainManager::RegisterError, "Registration error");
  EX_WITH_ERRNO_MSG (CF::DomainManager::UnregisterError, "Unregistration error");
  EX_NO_DATA (CF::DomainManager::AlreadyConnected, "Already connected");
  EX_NO_DATA (CF::DomainManager::InvalidEventChannelName, "Invalid event channel name");
  EX_NO_DATA (CF::DomainManager::NotConnected, "Not connected");

  {
    const CF::ApplicationFactory::CreateApplicationRequestError * ex =
      CF::ApplicationFactory::CreateApplicationRequestError::_downcast (&cex);

    if (ex) {
      CORBA::ULong nip = ex->invalidAssignments.length ();

      reason = "Application creation request error: ";

      if (nip != 1) {
	reason += "Invalid assignments for: \"";
      }
      else {
	reason += "Invalid assignment for: \"";
      }

      for (CORBA::ULong i=0; i<nip; i++) {
	reason += ex->invalidAssignments[i].componentId.in ();

	if (i+1 < nip) {
	  reason += "\", \"";
	}
      }

      reason += "\"";
      return reason;
    }
  }

  EX_WITH_ERRNO_MSG (CF::ApplicationFactory::CreateApplicationError, "Application creation error");
  EX_WITH_PROPERTIES (CF::ApplicationFactory::InvalidInitConfiguration, "Invalid initial configuration", invalidProperties);

  EX_NO_DATA (CF::PortSupplier::UnknownPort, "Unknown port");
  EX_WITH_ERRNO_MSG (CF::Resource::StartError, "Start error");
  EX_WITH_ERRNO_MSG (CF::Resource::StopError, "Stop error");

  EX_WITH_MSG (CF::Device::InvalidState, "Invalid state");
  EX_WITH_PROPERTIES_AND_MSG (CF::Device::InvalidCapacity, "Invalid capacities", capacities);

  EX_NO_DATA (CF::LoadableDevice::InvalidLoadKind, "Invalid load type");
  EX_WITH_ERRNO_MSG (CF::LoadableDevice::LoadFail, "Load failed");

  EX_WITH_ERRNO_MSG (CF::ExecutableDevice::InvalidProcess, "Invalid process");
  EX_NO_DATA (CF::ExecutableDevice::InvalidFunction, "Invalid function");
  EX_WITH_PROPERTIES (CF::ExecutableDevice::InvalidParameters, "Invalid parameters", invalidParms);
  EX_WITH_PROPERTIES (CF::ExecutableDevice::InvalidOptions, "Invalid options", invalidOpts);
  EX_WITH_ERRNO_MSG (CF::ExecutableDevice::ExecuteFail, "Execution failed");

  return CPI::CORBAUtil::Misc::stringifyCorbaException (cex);
}

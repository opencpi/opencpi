
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
 * Various SCA related helpers.
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
#include "OcpiCFUtilLegacyErrorNumbers.h"
#include "OcpiCFUtilMisc.h"

std::string
OCPI::CFUtil::
usageTypeToString (CF::Device::UsageType usageType)
  throw ()
{
  std::string type;

  switch (usageType) {
  case CF::Device::IDLE:   type = "IDLE";   break;
  case CF::Device::ACTIVE: type = "ACTIVE"; break;
  case CF::Device::BUSY:   type = "BUSY";   break;
  default:                 type = "???";    break;
  }

  return type;
}

std::string
OCPI::CFUtil::
adminTypeToString (CF::Device::AdminType adminType)
  throw ()
{
  std::string type;

  switch (adminType) {
  case CF::Device::LOCKED:        type = "LOCKED";        break;
  case CF::Device::SHUTTING_DOWN: type = "SHUTTING_DOWN"; break;
  case CF::Device::UNLOCKED:      type = "UNLOCKED";      break;
  default:                        type = "???";           break;
  }

  return type;
}

std::string
OCPI::CFUtil::
operationalTypeToString (CF::Device::OperationalType opType)
  throw ()
{
  std::string type;

  switch (opType) { 
  case CF::Device::ENABLED:  type = "ENABLED";  break;
  case CF::Device::DISABLED: type = "DISABLED"; break;
  default:                   type = "???";      break;
  }

  return type;
}

std::string
OCPI::CFUtil::
loadTypeToString (CF::LoadableDevice::LoadType loadType)
  throw ()
{
  std::string type;

  switch (loadType) { 
  case CF::LoadableDevice::KERNEL_MODULE:  type = "KERNEL_MODULE";  break;
  case CF::LoadableDevice::DRIVER:         type = "DRIVER";         break;
  case CF::LoadableDevice::SHARED_LIBRARY: type = "SHARED_LIBRARY"; break;
  case CF::LoadableDevice::EXECUTABLE:     type = "EXECUTABLE";     break;
  default:                                 type = "???";            break;
  }

  return type;
}

std::string
OCPI::CFUtil::
errorNumberToString (CF::ErrorNumberType ent)
  throw ()
{
  std::string type;

  switch (ent) {
  case CF::CF_NOTSET: type = "CF_NOTSET"; break;
  case CF::CF_E2BIG: type = "CF_E2BIG"; break;
  case CF::CF_EACCES: type = "CF_EACCES"; break;
  case CF::CF_EAGAIN: type = "CF_EAGAIN"; break;
  case CF::CF_EBADF: type = "CF_EBADF"; break;
  case CF::CF_EBADMSG: type = "CF_EBADMSG"; break;
  case CF::CF_EBUSY: type = "CF_EBUSY"; break;
  case CF::CF_ECANCELED: type = "CF_ECANCELED"; break;
  case CF::CF_ECHILD: type = "CF_ECHILD"; break;
  case CF::CF_EDEADLK: type = "CF_EDEADLK"; break;
  case CF::CF_EDOM: type = "CF_EDOM"; break;
  case CF::CF_EEXIST: type = "CF_EEXIST"; break;
  case CF::CF_EFAULT: type = "CF_EFAULT"; break;
  case CF::CF_EFBIG: type = "CF_EFBIG"; break;
  case CF::CF_EINPROGRESS: type = "CF_EINPROGRESS"; break;
  case CF::CF_EINTR: type = "CF_EINTR"; break;
  case CF::CF_EINVAL: type = "CF_EINVAL"; break;
  case CF::CF_EIO: type = "CF_EIO"; break;
  case CF::CF_EISDIR: type = "CF_EISDIR"; break;
  case CF::CF_EMFILE: type = "CF_EMFILE"; break;
  case CF::CF_EMLINK: type = "CF_EMLINK"; break;
  case CF::CF_EMSGSIZE: type = "CF_EMSGSIZE"; break;
  case CF::CF_ENAMETOOLONG: type = "CF_ENAMETOOLONG"; break;
  case CF::CF_ENFILE: type = "CF_ENFILE"; break;
  case CF::CF_ENODEV: type = "CF_ENODEV"; break;
  case CF::CF_ENOENT: type = "CF_ENOENT"; break;
  case CF::CF_ENOEXEC: type = "CF_ENOEXEC"; break;
  case CF::CF_ENOLCK: type = "CF_ENOLCK"; break;
  case CF::CF_ENOMEM: type = "CF_ENOMEM"; break;
  case CF::CF_ENOSPC: type = "CF_ENOSPC"; break;
  case CF::CF_ENOSYS: type = "CF_ENOSYS"; break;
  case CF::CF_ENOTDIR: type = "CF_ENOTDIR"; break;
  case CF::CF_ENOTEMPTY: type = "CF_ENOTEMPTY"; break;
  case CF::CF_ENOTSUP: type = "CF_ENOTSUP"; break;
  case CF::CF_ENOTTY: type = "CF_ENOTTY"; break;
  case CF::CF_ENXIO: type = "CF_ENXIO"; break;
  case CF::CF_EPERM: type = "CF_EPERM"; break;
  case CF::CF_EPIPE: type = "CF_EPIPE"; break;
  case CF::CF_ERANGE: type = "CF_ERANGE"; break;
  case CF::CF_EROFS: type = "CF_EROFS"; break;
  case CF::CF_ESPIPE: type = "CF_ESPIPE"; break;
  case CF::CF_ESRCH: type = "CF_ESRCH"; break;
  case CF::CF_ETIMEDOUT: type = "CF_ETIMEDOUT"; break;
  case CF::CF_EXDEV: type = "CF_EXDEV"; break;
  default: type = "???";
  }

  return type;
}


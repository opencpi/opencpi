
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
 * Abstact:
 *   This file contains the Interface for all SMB transfer types.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DATATRANSFER_EXCEPTIONS_H_
#define OCPI_DATATRANSFER_EXCEPTIONS_H_

#include <OcpiOsDataTypes.h>
#include <OcpiUtilException.h>

namespace DataTransfer {

  // Our error code defintiions
  const OCPI::OS::uint32_t DT_EX_SOURCE_ID = 0x03;
  const OCPI::OS::uint32_t MAIL_BOX_NOT_ALLOCATED   = (DT_EX_SOURCE_ID << 16) + 1;
  const OCPI::OS::uint32_t PROPERTY_NOT_SET         = (DT_EX_SOURCE_ID << 16) + 2;
  const OCPI::OS::uint32_t RESOURCE_EXCEPTION           = (DT_EX_SOURCE_ID << 16) + 3;
  const OCPI::OS::uint32_t UNSUPPORTED_ENDPOINT           = (DT_EX_SOURCE_ID << 16) + 4;
  const OCPI::OS::uint32_t NO_MORE_SMB                   = (DT_EX_SOURCE_ID << 16) + 5;
  const OCPI::OS::uint32_t SMB_MAP_ERROR                 = (DT_EX_SOURCE_ID << 16) + 6;
  const OCPI::OS::uint32_t EM_NOT_SUPPORTED_FOR_EP  = (DT_EX_SOURCE_ID << 16) + 7; // Event Manager not supported

  /**********************************
   * Data Transfer exception base class
   *********************************/
  class DataTransferEx : public OCPI::Util::EmbeddedException {
  public:
  DataTransferEx(const OCPI::OS::uint32_t errorCode, const char* aux=0)
    :OCPI::Util::EmbeddedException(errorCode,NULL){setAuxInfo(aux);};
    ~DataTransferEx(){};
  };
        
}
#endif






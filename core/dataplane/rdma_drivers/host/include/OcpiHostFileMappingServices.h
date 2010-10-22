
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
 *   This file contain the "C" implementation for the NT specific functions
 *   to support file mapping.
 *
 * Author: Tony Anzelmo
 *
 * Date: 10/16/02
 *
 */
#ifndef OcpiNT_FILE_MAPPING_SERVICES_H_
#define OcpiNT_FILE_MAPPING_SERVICES_H_

#include <OcpiOsDataTypes.h>

namespace DataTransfer {

  // File mapping services
  class OcpiFileMappingServices
  {
  public:
    typedef enum { ReadWriteAccess, ReadOnlyAccess, CopyAccess, AllAccess } AccessType;

    // Create a mapping to a named file.
    //        Arguments:
    //                strFilePath - Path to a file. If null, no backing store.
    //                strMapName        - Name of the mapping. Can be null.
    //                eAccess                - The type of access desired.
    //                iMaxSize        - Maximum size of mapping object.
    //        Returns:
    //                0 for success; platform dependent error code otherwise.
    //        Throws:
    //                DataTransferEx for all other exception conditions
    virtual int CreateMapping (const char*  strFilePath, const char* strMapName, AccessType eAccess, OCPI::OS::int64_t iMaxSize) = 0;

    // Open an existing mapping to a named file.
    //        Arguments:
    //                strMapName        - Name of the mapping.
    //                eAccess                - The type of access desired.
    //        Returns:
    //                0 for success; platform dependent error code otherwise.
    //        Throws:
    //                DataTransferEx for all other exception conditions
    virtual int OpenMapping (const char* strMapName, AccessType eAccess) = 0;

    // Close an existing mapping.
    //        Arguments:
    //        Returns:
    //                0 for success; platform dependent error code otherwise.
    //        Throws:
    //                DataTransferEx for all other exception conditions
    virtual int CloseMapping () = 0;

    // Map a segment of the file into the address space.
    //        Arguments:
    //                iOffset                - Byte offset into file for this view.
    //                lLength                - Number of bytes to map.
    //        Returns:
    //                virtual address or 0 if failure
    //        Throws:
    //                DataTransferEx for all other exception conditions
    virtual void* MapView (OCPI::OS::uint64_t iOffset, OCPI::OS::uint64_t lLength, AccessType eAccess) = 0;

    // Unmap a segment of a file from the address space.
    //        Arguments:
    //                pVA                - Virtual address of view to unmap
    //        Returns:
    //                Returns 0 for success or a platform specific error number
    //        Throws:
    //                DataTransferEx for all other exception conditions
    virtual int UnMapView (void *pVA) = 0;

    // Return the last error that occurred for a file mapping operation.
    //        Arguments:
    //        Returns:
    //                Returns 0 for success or a platform specific error number
    //        Throws:
    //                DataTransferEx for all other exception conditions
    virtual int GetLastError () = 0;

    // Destructor
    virtual ~OcpiFileMappingServices () {};
  };

  OcpiFileMappingServices* CreateFileMappingServices ();

};


#endif



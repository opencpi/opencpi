
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



/**
  @brief
    Contains the definition of the wci_strerror() error code translation
    function.

  The WCI APIs will return error indications, of the type ôWCI_errorö,
  which will have the value zero upon success, and non-zero on error,
  thus amenable to logical boolean tests in C.  Error codes can be
  converted to (const)  strings with the wci_strerror() function.  If
  wci_strerror() are not called by an application, no error strings will be
  linked or referenced.

  Revision History:

    10/13/2008 - Michael Pepe
                 Initial version.

************************************************************************** */

#include "wci.h"

#include <cstdio>
#include <cassert>

typedef struct wci__error_info
{
  WCI_error d_number;
  //!< WCI error code value.
  const char* d_message;
  //!< Error message that explains the error code.
} WCI__error_info;

const WCI_error WCI_ERROR_UNKNOWN_ERROR_NUMBER = 1000;

static WCI__error_info s_error_info [ ] =
{
  { WCI_SUCCESS,
    "Successful operation." },
  { WCI_FAILURE,
    "Generic error condition." },
  { WCI_ERROR_TIMEOUT,
    "Timeout exceeded." },
  { WCI_ERROR_OUT_OF_MEMORY,
    "Memory allocation failure." },
  { WCI_ERROR_SUB32_ACCESS,
    "Sub-32 bit property space access is not supported." },
  { WCI_ERROR_NULL_HANDLE,
    "WCI_worker handle is null." },
  { WCI_ERROR_INVALID_HANDLE,
    "WCI_worker handle is invalid." },
  { WCI_ERROR_UNKNOWN_WORKER_TYPE,
    "String does not identify a valid Worker type." },
  { WCI_ERROR_UNKNOWN_WORKER,
    "String does not identify a valid Worker." },
  { WCI_ERROR_PROPERTY_OVERRUN,
    "Attempt to access an invalid property location." },
  { WCI_ERROR_REQUEST_ERROR,
    "Last non-NOP request completed with a Worker indicating an error." },
  { WCI_ERROR_UNABLE_TO_FIND_WORKERS,
    "Unable to find workers. Is the bitstream loaded?" },
  { WCI_ERROR_UNABLE_TO_MAP_WORKER,
    "Unable to memory map Worker. Is the device ID valid?" },
  { WCI_ERROR_INTERNAL_NON_WCI_ERROR,
    "Internal error. A non-OCPI operation returned an error." },
  { WCI_ERROR_UNABLE_TO_MAP_DEVICE,
    "Unable to memory map the device. Is the Worker ID valid?" },
  { WCI_ERROR_INVALID_CONTROL_SEQUENCE,
    "Control operations have been issued in the wrong order." },
  { WCI_ERROR_UNUSABLE_WORKER,
    "Worker is unusable due to an error. Reload the Worker." },
  { WCI_ERROR_WORKER_NOT_INITIALIZED,
    "Initialize control operation must be issued before Worker is usable." },
  { WCI_ERROR_TEST_NOT_IMPLEMENTED,
    "The \"test\" control operation is not supported by the worker." },
  { WCI_ERROR_RELEASE_NOT_IMPLEMENTED,
    "The \"release\" control operation is not supported by the worker." },
  { WCI_ERROR_NOTIFICATION_SETUP,
    "Error setting up for notifications." },
  { WCI_ERROR_UNKNOWN,
    "An unexpected error occurred." },
  /* This must be the last error in the table */
  { WCI_ERROR_UNKNOWN_ERROR_NUMBER,
    "Unknown error number." }
};

const char* wci_strerror ( WCI_error error_number )
{
  int n = 0;

  for ( n = 0;
        s_error_info [ n ].d_number != WCI_ERROR_UNKNOWN_ERROR_NUMBER;
        n++ )
  {
    if ( s_error_info [ n ].d_number == error_number )
    {
      break;
    }
  }

  return s_error_info [ n ].d_message;
}

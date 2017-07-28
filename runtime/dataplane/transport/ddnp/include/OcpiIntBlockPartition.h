
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
 * Abstract:
 *   This file contains the Interface for the block partition class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2004
 *    Revision Detail: Created
 *
 */

#ifndef DataTransport_Interface_BlockPartition_H_
#define DataTransport_Interface_BlockPartition_H_

#include <OcpiIntDataPartition.h>

namespace  OCPI {

  namespace DataTransport {


    /**********************************
     * Block partition base class
     **********************************/
    class BlockPartition : public DataPartition
    {

    public:

      /**********************************
       * Constructors
       **********************************/
      BlockPartition();

      /**********************************
       * Destructor
       **********************************/
      virtual ~BlockPartition();

      /**********************************
       * Given the inherit distribution and partition information
       * calculate the offsets into the requested buffers for distribution.
       *
       * returns 0 on success.
       **********************************/
      virtual OCPI::OS::int32_t calculateBufferOffsets( 
                                                      OCPI::OS::uint32_t                   sequence,        // In - Transfer sequence
                                                      Buffer     *src_buf,                            // In - Source buffer
                                                      Buffer     *target_buf,                        // In - Target buffer
                                                      BufferInfo **target_buf_info);      // Out - Target buffer offset information


    private:

    };

  }

}


#endif



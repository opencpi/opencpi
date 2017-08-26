/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Abstact:
 *   This file contains the Interface for the indivisible partition class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2004
 *    Revision Detail: Created
 *
 */

#ifndef DataTransport_Interface_IndivisiblePartition_H_
#define DataTransport_Interface_IndivisiblePartition_H_

#include <OcpiIntDataPartition.h>

namespace OCPI {

  namespace DataTransport {

    class IndivisiblePartition : public DataPartition
    {

    public:

      /**********************************
       *  Constructors
       *********************************/
      IndivisiblePartition();

      /**********************************
       *  Destructor
       *********************************/
      virtual ~IndivisiblePartition();

      /**********************************
       * Given the inherit distribution and partition information
       * calculate the offsets into the requested buffers for distribution.
       *
       * returns 0 on success.
       **********************************/
      virtual OCPI::OS::int32_t calculateBufferOffsets( 
                                                      OCPI::OS::uint32_t           sequence,
                                                      Buffer     *src_buf,              
                                                      Buffer     *input_buf,                
                                                      BufferInfo **input_buf_info);    

    private:

    };

  }

}


#endif



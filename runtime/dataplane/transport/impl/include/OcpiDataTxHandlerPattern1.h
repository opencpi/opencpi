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
 *   This file contains the Interface for the OCPI data transfer handler class
 *   for transfers of type 1.  
 * 
 *   Transfers handled by this class are:
 *      1-1
 *      3-1
 *      1-3
 *      3-3
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_DataTxHandlerPattern1_H_
#define OCPI_DataTransport_DataTxHandlerPattern1_H_

#include <OcpiCircuit.h>
#include <OcpiDataTxHandlerBase.h>

namespace OCPI {

  namespace DataTransport {


    /**********************************
     **********************************/
    class OcpiDataTxHandlerPattern1 : public OcpiDataTxHandlerBase
    {

    public:


      /**********************************
       *  Constructors
       **********************************/
      OcpiDataTxHandlerPattern1( OCPI::DataTransport::Circuit* circuit );


      /**********************************
       *  Destructor
       **********************************/
      virtual ~OcpiDataTxHandlerPattern1();


      /**********************************
       * Initialize the transfers
       **********************************/
      virtual void initTransfers();


    private:

      // Initialize the source buffers
      void initSourceBuffers();

    };

  }

}


#endif



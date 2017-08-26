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
 *   This file contains the Interface for the OCPI data transfer handler base class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_DataTxHandlerBase_H_
#define OCPI_DataTransport_DataTxHandlerBase_H_

#include <OcpiIntTransferTemplate.h>
#include <OcpiCircuit.h>

namespace OCPI {

  namespace DataTransport {


    /**********************************
     * 
     **********************************/
    class OcpiDataTxHandlerBase : public TxController
    {

    public:

      /**********************************
       *  Constructors
       **********************************/
      OcpiDataTxHandlerBase( OCPI::DataTransport::Circuit* circuit )
      {
        ( void ) circuit;
      }

      /**********************************
       *  Destructor
       **********************************/
      virtual ~OcpiDataTxHandlerBase(){};

      /**********************************
       * Initialize the transfers
       **********************************/
      virtual void initTransfers(){};

      /**********************************
       * Creates or retreives an existing transfer handle. Based upon our rank and
       * the distribution type, this template will be created with the proper offset(s) into
       * the source, offsets into the target(s) and appropriate control structures.
       **********************************/
      TransferTemplate* getTxTemplate( Buffer* src );

    protected:

      // Our Circuit
      OCPI::DataTransport::Circuit* m_circuit;


    };
  }
}


#endif



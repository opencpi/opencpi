// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

/*
 * Abstact:
 *   This file contains the Interface for the CPI data transfer handler class
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

#ifndef CPI_DataTransport_DataTxHandlerPattern1_H_
#define CPI_DataTransport_DataTxHandlerPattern1_H_

#include <CpiCircuit.h>
#include <CpiDataTxHandlerBase.h>

namespace CPI {

  namespace DataTransport {


    /**********************************
     **********************************/
    class CpiDataTxHandlerPattern1 : public CpiDataTxHandlerBase
    {

    public:


      /**********************************
       *  Constructors
       **********************************/
      CpiDataTxHandlerPattern1( CPI::DataTransport::Circuit* circuit );


      /**********************************
       *  Destructor
       **********************************/
      virtual ~CpiDataTxHandlerPattern1();


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



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
 *   This file contains the Interface for the transfer template class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2004
 *    Revision Detail: Created
 *
 */

#ifndef CPI_DataTransport_Int_TransferTemplate_H_
#define CPI_DataTransport_Int_TransferTemplate_H_

#include <CpiUtilException.h>
#include <CpiBuffer.h>

namespace CPI {

namespace DataTransport {

    class TransferTemplate;

    class TxController
    {

    public:

      /**********************************
       *  Constructors
       **********************************/
      TxController(){};

      /**********************************
       *  Destructor
       **********************************/
      virtual ~TxController(){};

      /**********************************
       * This method allows a transfer controller to determine if a specific transfer is allowed 
       * to start.  This is implementation and data transfer specific.  For example, the most generic 
       * form of this method would be a whole/parallel source/target combination, which would simply 
       * check to see if all of the targets were ready for data.  A more complicated transfer controller 
       * might also need to test the source token to determine whether or not it has the "next" transfer.
       **********************************/
      virtual bool canTransfer( TransferTemplate* temp )=0;

      /**********************************
       * Implementation specific transfer from the source side.
       **********************************/
      virtual bool produce( TransferTemplate* temp )=0;

      /**********************************
       * Implementation specific transfer from the target side, idicating that
       * the target is now available
       **********************************/
      virtual CPI::DataTransport::Buffer*  consume( TransferTemplate* temp )=0;

    };

    class TransferTemplate
    {

    public:

      /**********************************
       * Constructors
       *********************************/
      TransferTemplate(){};

      /**********************************
       * Destructor
       *********************************/
      virtual ~TransferTemplate(){};

      /**********************************
       * Is this transfer pending
       *********************************/
      virtual bool isPending()=0;

      /**********************************
       * Is this transfer in use
       *********************************/
      virtual bool isComplete()=0;

      /**********************************
       * Start the produce
       *********************************/
      virtual void produce()=0;

      /**********************************
       * Start the consume
       *********************************/
      virtual CPI::DataTransport::Buffer*  consume()=0;

    protected:

      /**********************************
       * Gets/Sets the transfer controller
       *********************************/
      void setTxController( TxController* controller );
      TxController* getTxController();

    private:

      // Our transfer control object
      TxController* m_controller;

    };


    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
    inline void TransferTemplate::setTxController( TxController* controller ){m_controller=controller;}
    inline TxController* TransferTemplate::getTxController(){return m_controller;}

  }

}


#endif

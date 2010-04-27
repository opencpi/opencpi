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
 *   This file contains the Interface for the transfer template manager class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2004
 *    Revision Detail: Created
 *
 */

#ifndef CPI_DataTransport_Int_TransferTemplateManager_H_
#define CPI_DataTransport_Int_TransferTemplateManager_H_

#include <CpiUtilException.h>

namespace CPI {

namespace DataTransport {

    // Maximum number of transfers per template
    const int MAX_m_templatesPER_TRANSFER = 20;

    class TransferTemplateManager
    {

    public:

      /**********************************
       * Constructors
       *********************************/
      TransferTemplateManager()
	:m_templates(0){}

      /**********************************
       * Destructor
       *********************************/
      virtual ~TransferTemplateManager(){};

      /**********************************
       * Add Template
       *********************************/
      void add( TransferTemplate* temp );

      /**********************************
       * Is this transfer in use
       *********************************/
      bool isComplete();

      /**********************************
       * Start the transfer
       *********************************/
      void start();

    private:

      // Our templates
      CPI::Util::VList m_templates;

    };


    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
    inline void TransferTemplateManager::add( TransferTemplate* temp ){m_templates.push_back(temp);}

  }

}


#endif


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
 *   This file contains the Interface for the transfer template manager class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2004
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_Int_TransferTemplateManager_H_
#define OCPI_DataTransport_Int_TransferTemplateManager_H_

#include <OcpiUtilException.h>

namespace OCPI {

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
      OCPI::Util::VList m_templates;

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

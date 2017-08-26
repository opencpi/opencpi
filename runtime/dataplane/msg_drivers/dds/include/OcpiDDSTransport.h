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
 *   This file contains the Interface for the OCPI DDS port.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 8/2011
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DDS_TRANSPORT_H_
#define OCPI_DDS_TRANSPORT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OcpiParentChild.h>
#include <DtMsgDriver.h>


namespace OCPI {

  namespace Msg {
    
    namespace DDS {

      class TopicManager;
      class Topic;
      class Port;
      class Writer;
      class Reader;
      class TransCoder;

      



    }
  }
}

#endif


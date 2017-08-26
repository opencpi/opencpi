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
 * Abstract:
 *   This file contains the Interface for the connection meta data class
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_ConnectionMetaData_H_
#define OCPI_DataTransport_ConnectionMetaData_H_

#include <OcpiPortSetMetaData.h>
#include <OcpiList.h>
#include <OcpiParentChild.h>
namespace CU = ::OCPI::Util;

namespace OCPI {

  namespace RDT {
    struct Descriptors;
  }

  namespace DataTransport {

    class ConnectionMetaData : public OCPI::Util::Parent<PortSetMetaData>
    {
    public:

      // port meta data
      OCPI::Util::VList m_portSetMd;

      // Connection data distribution
      DataDistribution* dataDistribution;

      /**********************************
       * Constructors
       *********************************/
      ConnectionMetaData(DataTransfer::EndPoint *output_ep, DataTransfer::EndPoint *input_ep,
			 int buf_count, int buf_size );
      ConnectionMetaData(DataTransfer::EndPoint &ep, OCPI::RDT::Descriptors& sPort);
      ConnectionMetaData();


      /**********************************
       * Add a target port set
       *********************************/
      void addPortSet( PortSetMetaData* psmd );

      /**********************************
       * Destructor
       *********************************/
      virtual ~ConnectionMetaData();

    };
  }
}


#endif


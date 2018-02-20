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
 *   This file contains the Interface for the Ocpi port meta data class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_PortSetMetaData_H_
#define OCPI_DataTransport_PortSetMetaData_H_

#include <string>
#include <stdio.h>
#include <OcpiOsDataTypes.h>
#include <OcpiList.h>
#include <OcpiPortMetaData.h>
#include <OcpiParentChild.h>
namespace CU = ::OCPI::Util;

namespace OCPI {

  namespace RDT {
    struct Descriptors;
  }

  namespace DataTransport {

    class DataDistribution;
    class ConnectionMetaData;

    struct PortSetMetaData :  public OCPI::Util::Child<ConnectionMetaData,PortSetMetaData>, public OCPI::Util::Parent<PortMetaData>
    {

      // Output port
      bool output;

      // Our Connection
      ConnectionMetaData* m_connection;

      // Data distribution class
      DataDistribution  *dataDistribution;

      // buffer count, the number of buffers for all ports within a port set
      // must be the same
      uint32_t bufferCount;

      // Buffer length of all buffers in the set
      uint32_t bufferLength;

      // Port Set id
      int32_t        portSetId;

      inline PortMetaData* getPortInfo(int32_t idx )
      {return static_cast<PortMetaData*>(m_portMd[idx]);}

      // User data
      void* user_data;

      // Ocpi port meta data;
      OCPI::Util::VList m_portMd;


      /**********************************
       * Constuctors
       **********************************/
      PortSetMetaData(    bool src, 
                          int ps_id, 
                          DataDistribution* dd, 
                          int buffer_count,
                          int buffer_size,
                          ConnectionMetaData* c );

      PortSetMetaData( bool src, 
                       OCPI::OS::uint32_t ps_id, 
                       DataDistribution* dd, 
                       ConnectionMetaData* c,
		       DataTransfer::EndPoint &inputEp,
                       const OCPI::RDT::Descriptors* inputDesc, 
                       int port_count,
                       int buffer_count,
                       int buffer_size,
                       DataTransfer::EndPoint &outputEp);

      virtual ~PortSetMetaData();

      /**********************************
       * Add a new port definition
       **********************************/
      PortMetaData* addPort( PortMetaData* pmd );

    };
  }

}


#endif

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
 *   This file contains the Interface for the Cpi port meta data class.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef CPI_DataTransport_PortSetMetaData_H_
#define CPI_DataTransport_PortSetMetaData_H_

#include <string>
#include <stdio.h>
#include <CpiOsDataTypes.h>
#include <CpiList.h>
#include <CpiPortMetaData.h>
#include <CpiParentChild.h>
namespace CU = ::CPI::Util;

namespace CPI {

  namespace RDT {
    struct Descriptors;
  }

  namespace DataTransport {

    class DataDistribution;
    class ConnectionMetaData;

    struct PortSetMetaData :  public CPI::Util::Child<ConnectionMetaData,PortSetMetaData>, public CPI::Util::Parent<PortMetaData>
    {

      // Output port
      bool output;

      // Our Connection
      ConnectionMetaData* m_connection;

      // Data distribution class
      DataDistribution  *dataDistribution;

      // buffer count, the number of buffers for all ports within a port set
      // must be the same
      CPI::OS::uint32_t bufferCount;

      // Buffer length of all buffers in the set
      CPI::OS::uint32_t bufferLength;

      // Port Set id
      CPI::OS::int32_t        portSetId;

      inline PortMetaData* getPortInfo( CPI::OS::int32_t idx )
      {return static_cast<PortMetaData*>(m_portMd[idx]);}

      // User data
      void* user_data;

      // Cpi port meta data;
      CPI::Util::VList m_portMd;


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
                       CPI::OS::uint32_t ps_id, 
                       DataDistribution* dd, 
                       ConnectionMetaData* c,
                       CPI::RDT::Descriptors* port_dep, 
                       CPI::OS::uint32_t cid,
                       int port_count,
                       int buffer_count,
                       int buffer_size,
                       const char* our_ep);

      virtual ~PortSetMetaData();

      /**********************************
       * Add a new port definition
       **********************************/
      PortMetaData* addPort( PortMetaData* pmd );

    };
  }

}


#endif

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
 *   This file contains the Interface for the connection meta data class
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef CPI_DataTransport_ConnectionMetaData_H_
#define CPI_DataTransport_ConnectionMetaData_H_

#include <CpiPortSetMetaData.h>
#include <CpiList.h>
#include <CpiParentChild.h>
namespace CU = ::CPI::Util;

namespace CPI {

  namespace RDT {
    class Descriptors;
  }

  namespace DataTransport {

    class ConnectionMetaData : public CPI::Util::Parent<PortSetMetaData>
    {
    public:

      // port meta data
      CPI::Util::VList m_portSetMd;

      // Connection data distribution
      DataDistribution* dataDistribution;

      /**********************************
       * Constructors
       *********************************/
      ConnectionMetaData( const char* output_ep, const char* input_ep, int buf_count, int buf_size );
      ConnectionMetaData( CPI::RDT::Descriptors& sPort );
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


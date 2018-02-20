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
 *   This file contains the implementation for the OCPI data transfer handler class.
 *
 * Author: John F. Miller
 *
 * Date: 7/20/04
 *
*/

#include <OcpiDataTxHandlerPattern1.h>

using namespace OCPI::DataTransport;
using namespace DataTransfer;

/**********************************
 *  Constructors
 **********************************/
OcpiDataTxHandlerPattern1::OcpiDataTxHandlerPattern1( OCPI::DataTransport::Circuit* circuit )
  : OcpiDataTxHandlerBase( circuit ){}
      
/**********************************
 *  Destructor
 **********************************/
OcpiDataTxHandlerPattern1::~OcpiDataTxHandlerPattern1(){}


/**********************************
 * Initialize the transfers
 **********************************/
void OcpiDataTxHandlerPattern1::initTransfers()
{
  OcpiDataTxHandlerBase::initTransfers();
}



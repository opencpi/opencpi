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


/**
  @brief
    Contains the implementation of the WorkerIdentifier class.

  Revision History:

    10/13/2008 - Michael Pepe
                 Initial version.

************************************************************************** */

#include "CpiWciWorkerIdentifier.h"

#include <string>
#include <sstream>

using namespace CPI;

namespace
{
  unsigned int string_to_uint32 ( const std::string& s )
  throw ( )
  {
    std::istringstream s_stream ( s );

    unsigned int value = 0;

    s_stream >> value;

    return value;
  }

} // End: namespace <unamed>

WCI::WorkerIdentifier::WorkerIdentifier ( const char* p_path )
throw ( std::string )
  : d_uri ( p_path ),
    d_worker_type ( d_uri.getScheme ( ) ),
    d_transport_type ( d_uri.getHost ( ) ),
    d_device_id ( string_to_uint32 ( d_uri.getPort ( ) ) ),
    d_worker_id ( string_to_uint32 ( d_uri.getFileName ( ) ) )
{
  // Empty
}

unsigned int WCI::WorkerIdentifier::deviceId ( ) const
throw ( )
{
  return d_device_id;
}

unsigned int WCI::WorkerIdentifier::workerId ( ) const
throw ( )
{
  return d_worker_id;
}

const std::string& WCI::WorkerIdentifier::workerType ( ) const
throw ( )
{
  return d_worker_type;
}

const std::string& WCI::WorkerIdentifier::transportType ( ) const
throw ( )
{
  return d_transport_type;
}

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
  @file

  @brief
    The CPI::WCI::WorkerIdentifier class.

  This file contains the declaration of the CPI::WCI::WorkerIdentifier
  class. This class holds and parses the URI used to identify a particular
  Worker.

  Revision History:

    10/13/2008 - Michael Pepe
                 Initial version.

************************************************************************** */

#ifndef INCLUDED_CPI_WCI_WORKER_IDENTIFIER_H
#define INCLUDED_CPI_WCI_WORKER_IDENTIFIER_H

#include "CpiUtilUri.h"

namespace CPI
{
  namespace WCI
  {
    /**
      @class WorkerIdentifier

      @brief
        The CPI::WCI::WorkerIdentifier class.

      The class CPI::WCI::WorkerIdentifier contains and parses the URI
      used to identify a particular RCC or RPL Worker. This URI string
      is passed to the WCI API function wci_open() to establish a
      connection to a Worker.

      The URI must be in the following format.

      RPL Worker : WCI-rpl://[rapidio|pcie]:<deviceId>/<workerId>
      RCC Worker : WCI-rcc://[rapidio|pcie]:<deviceId>/<workerId>

    ********************************************************************** */

    class WorkerIdentifier
    {
      public:

        /**
          @brief
            Constructs a WorkerIdentifier instance from a valid URI.

          The function WorkerIdentifier() constructs a WorkerIdentifier
          object from a valid URI. The object can later be queried for
          information about the URI such as Worker ID or transport type.

          @param [ in ] p_path
                        URI that identifies a particular Worker.

        ****************************************************************** */

        explicit WorkerIdentifier ( const char* p_path )
        throw ( std::string );

        /**
          @brief
            Returns the device ID portion of the URI.

          The function deviceId() returns the device ID portion of the
          URI.

          @return Device ID portion of the URI.

        ****************************************************************** */

        unsigned int deviceId ( ) const throw ( );

        /**
          @brief
            Returns the Worker ID portion of the URI.

          The function workerId() returns the Worker ID portion of the
          URI.

          @return Worker ID portion of the URI.

        ****************************************************************** */

        unsigned int workerId ( ) const throw ( );

        /**
          @brief
            Returns the Worker type portion of the URI.

          The function workerType() returns the Worker type (WCI-rcc or
          WCI-rpl) portion of the URI as a string.

          @return Worker type portion of the URI.

        ****************************************************************** */

        const std::string& workerType ( ) const throw ( );

        /**
          @brief
            Returns the transport type portion of the URI.

          The function transportType() returns the transport type
          (rapidio or pcie) portion of the URI as a string.

          @return transport type portion of the URI.

        ****************************************************************** */

        const std::string& transportType ( ) const throw ( );

      private:

        CPI::Util::Uri d_uri;
        //!< Contains and parses the URI.

        std::string d_worker_type;
        //!< String representation of Worker type (WCI-rcc or WCI-rpl).

        std::string d_transport_type;
        //!< String representation of transport type (rapidio or pcie).

        unsigned int d_device_id;
        //!< Device ID

        unsigned int d_worker_id;
        //!< Worker ID (ranges from 1 to number of workers)

    }; // End: class WorkerIdentifier

  } // End: namespace WCI

} // End: namespace CPI

#endif // End: #ifndef INCLUDED_CPI_WCI_WORKER_IDENTIFIER_H


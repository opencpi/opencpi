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
    The CPI::WCI::WorkerHandle class.

  This file contains the declaration of the CPI::WCI::WorkerHandle class
  that inherits from the wci_worker structure. The CPI::WCI::WorkerHandle
  class contains the pointer the Worker's implementation object.

  Revision History:

    10/13/2008 - Michael Pepe
                 Initial version.

************************************************************************** */

#ifndef INCLUDED_CPI_WCI_WORKER_HANDLE_H
#define INCLUDED_CPI_WCI_WORKER_HANDLE_H

#include "wci.h"

#include <memory>

namespace CPI
{
  namespace WCI
  {
    /**
      @struct WorkerHandle

      @brief
        The CPI::WCI::WorkerHandle class.

      The class CPI::WCI::WorkerHandle inherits from the wci_worker
      structure. The WCI__worker structure is presented to the WCI API
      user as the WCI_worker typedef.

      The class CPI::WCI::WorkerHandle inherits data members for the
      Worker's attributes, property space size, and pointer to the
      Worker's property space and is responsible for initializing these
      members.

    ********************************************************************** */

    struct WorkerHandle : public WCI__worker
    {
      public:

        //!< Constructor
        WorkerHandle ( )
        throw ( )
        {
          // Empty
        }

        //!< Constructor
        ~WorkerHandle ( )
        throw ( )
        {
          // Empty
        }

        std::auto_ptr<Worker> d_p_impl;
        //!< Pointer to the Worker implementation object (RCC or RPL).
    };

  } // End: namespace WCI

} // End: namespace CPI

#endif // End: #ifndef INCLUDED_CPI_WCI_WORKER_HANDLE_H


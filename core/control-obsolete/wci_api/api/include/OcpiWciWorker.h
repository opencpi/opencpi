
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



/**
  @file

  @brief
    The OCPI::WCI::Worker abstract base class for WCI RPL and RCC workers.

  This file contains the declaration of the OCPI::WCI::Worker abstract base
  class for WCI RPL and RCC workers providing the control plane API
  for OCPI workers.

  The implementations of the Worker interface map control plane
  operations to a concrete Worker implementation.

  Revision History:

    10/13/2008 - Michael Pepe
                 Initial version.

************************************************************************** */

#ifndef INCLUDED_OCPI_WCI_WORKER_H
#define INCLUDED_OCPI_WCI_WORKER_H

#include "wci.h"

namespace OCPI
{
  namespace WCI
  {
    /**
      @class Worker

      @brief
        The abstract base class for WCI workers.

        The abstract base class OCPI::WCI::Worker provides an interface
        for the WCI control, property access, and status operations on
        a single Worker.

      @sa OCPI::WCI::WorkerRpl

    ********************************************************************** */

    class Worker
    {
      public:

        //! Constructor. Not callable by the user.
        Worker ( )

        {
          // Empty
        }

        //! Destructor.
        virtual ~Worker ( )

        {
          // Empty
        }

        /**
          @brief
            Request a Worker to perform a control operation.

          The function control() sends a request to a WCI Worker for
          the specified operation. The function will block until the
          Worker responds indicating the success or failure of the
          operation or a timeout occurs.

          @param [ in ] h_worker
                        The Worker handle provided to wci_control().

          @param [ in ] operation
                        The WCI_control operation provided to wci_control().

          @param [ in ] options
                        The WCI_options option provided to wci_control().

          @retval Success WCI_SUCCESS
          @retval Error   Error code that describes the error. Error
                          code can be decoded with wci_strerror().

        ****************************************************************** */

        virtual WCI_error control ( WCI_worker h_worker,
                                    WCI_control operation,
                                    WCI_options options ) = 0;

        /**
          @brief
            Query the Worker's status information.

          The function status() returns the status information for a
          Worker in the WCI_status structure.

          @param [ in ] h_worker
                        The Worker handle provided to wci_status().

          @param [ out ] p_status
                        The WCI_status operation provided to wci_status().
                        Will contain a snapshot of the Worker's status
                        information after the call completes.

          @retval Success WCI_SUCCESS
          @retval Error   Error code that describes the error. Error
                          code can be decoded with wci_strerror().

        ****************************************************************** */

        virtual WCI_error status ( WCI_worker h_worker,
                                   WCI_status* p_status ) = 0;

        /**
          @brief
            Perform a property space read request.

          The function read() reads the specified number of bytes from
          the Worker's property space. The function will block until the
          Worker responds indicating the success or failure of the
          operation or a timeout occurs.

          @param [ in ] h_worker
                        The Worker handle provided to the WCI API function
                        that initiated the read operation.

          @param [ in ] offset
                        Offset into the property space to start the read.

          @param [ in ] nBytes
                        Number of bytes to read from the property space.

          @param [ in ] data_type
                        WCI_data_type value that specifies the type
                        of data being read.

          @param [ in ] options
                        WCI_options argument passed to the WCI API function
                        that initiated the read operation.

          @param [ out ] p_data
                         Upon successful completion the data read from
                         the Worker's property space will be stored in
                         this buffer. The caller is responsible for
                         ensuring the provided buffer is large enough
                         to hold the result of the read request.

          @retval Success WCI_SUCCESS
          @retval Error   Error code that describes the error. Error
                          code can be decoded with wci_strerror().

        ****************************************************************** */

        virtual WCI_error read ( WCI_worker h_worker,
                                 WCI_u32 offset,
                                 WCI_u32 nBytes,
                                 WCI_data_type data_type,
                                 WCI_options options,
                                 void* p_data ) = 0;

        /**
          @brief
            Perform a property space write.

          The function write() writes the specified number of bytes into
          the Worker's property space. The function will block until the
          Worker responds indicating the success or failure of the
          operation or a timeout occurs.

          @param [ in ] h_worker
                        The Worker handle provided to the WCI API function
                        that initiated the write operation.

          @param [ in ] offset
                        Offset into the property space to start writing.

          @param [ in ] nBytes
                        Number of bytes to write into the property space.

          @param [ in ] data_type
                        WCI_data_type value that specifies the type
                        of data being written.

          @param [ in ] options
                        WCI_options argument passed to the WCI API function
                        that initiated the write operation.

          @param [ in ] p_data
                        Buffer that contains the data to be written into
                        the Worker's property space.

          @retval Success WCI_SUCCESS
          @retval Error   Error code that describes the error. Error
                          code can be decoded with wci_strerror().

        ****************************************************************** */

        virtual WCI_error write ( WCI_worker h_worker,
                                  WCI_u32 offset,
                                  WCI_u32 nBytes,
                                  WCI_data_type data_type,
                                  WCI_options options,
                                  const void* p_data ) = 0;

        /**
          @brief
            Ends a session created by wci_open() and releases resources.

          The function close() frees the resources allocated for the
          Worker.  If it fails, the resources may not all be recovered.
          By default, the call to close () asserts the reset signal on the
          RPL worker unless the WCI_DO_NOT_RESET option is specified.

          @param [ in ] h_worker
                        The Worker handle provided to the WCI API function
                        that initiated the close operation.

          @param [ in ] options
                        WCI_options argument passed to the WCI API function
                        that initiated the close operation.

          @retval Success WCI_SUCCESS
          @retval Error   Error code that describes the error. Error
                          code can be decoded with wci_strerror().

        ****************************************************************** */

        virtual WCI_error close ( WCI_worker h_worker,
                                  WCI_options options ) = 0;

    }; // End: class Worker;



    class WorkerOO
    {
      public:

        //! Constructor. Not callable by the user.
      WorkerOO ( )

        {
          // Empty
        }

        //! Destructor.
        virtual ~WorkerOO ( )

        {
          // Empty
        }

        /**
          @brief
            Request a Worker to perform a control operation.

          The function control() sends a request to a WCI Worker for
          the specified operation. The function will block until the
          Worker responds indicating the success or failure of the
          operation or a timeout occurs.

          @param [ in ] operation
                        The WCI_control operation provided to wci_control().

          @param [ in ] options
                        The WCI_options option provided to wci_control().

          @retval Success WCI_SUCCESS
          @retval Error   Error code that describes the error. Error
                          code can be decoded with wci_strerror().

        ****************************************************************** */

        virtual WCI_error control (
                                    WCI_control operation,
                                    WCI_options options ) = 0;

        /**
          @brief
            Query the Worker's status information.

          The function status() returns the status information for a
          Worker in the WCI_status structure.

          @param [ out ] p_status
                        The WCI_status operation provided to wci_status().
                        Will contain a snapshot of the Worker's status
                        information after the call completes.

          @retval Success WCI_SUCCESS
          @retval Error   Error code that describes the error. Error
                          code can be decoded with wci_strerror().

        ****************************************************************** */

        virtual WCI_error status ( 
                                   WCI_status* p_status ) = 0;

        /**
          @brief
            Perform a property space read request.

          The function read() reads the specified number of bytes from
          the Worker's property space. The function will block until the
          Worker responds indicating the success or failure of the
          operation or a timeout occurs.

          @param [ in ] offset
                        Offset into the property space to start the read.

          @param [ in ] nBytes
                        Number of bytes to read from the property space.

          @param [ in ] data_type
                        WCI_data_type value that specifies the type
                        of data being read.

          @param [ in ] options
                        WCI_options argument passed to the WCI API function
                        that initiated the read operation.

          @param [ out ] p_data
                         Upon successful completion the data read from
                         the Worker's property space will be stored in
                         this buffer. The caller is responsible for
                         ensuring the provided buffer is large enough
                         to hold the result of the read request.

          @retval Success WCI_SUCCESS
          @retval Error   Error code that describes the error. Error
                          code can be decoded with wci_strerror().

        ****************************************************************** */

        virtual WCI_error read ( 
                                 WCI_u32 offset,
                                 WCI_u32 nBytes,
                                 WCI_data_type data_type,
                                 WCI_options options,
                                 void* p_data ) = 0;

        /**
          @brief
            Perform a property space write.

          The function write() writes the specified number of bytes into
          the Worker's property space. The function will block until the
          Worker responds indicating the success or failure of the
          operation or a timeout occurs.

          @param [ in ] offset
                        Offset into the property space to start writing.

          @param [ in ] nBytes
                        Number of bytes to write into the property space.

          @param [ in ] data_type
                        WCI_data_type value that specifies the type
                        of data being written.

          @param [ in ] options
                        WCI_options argument passed to the WCI API function
                        that initiated the write operation.

          @param [ in ] p_data
                        Buffer that contains the data to be written into
                        the Worker's property space.

          @retval Success WCI_SUCCESS
          @retval Error   Error code that describes the error. Error
                          code can be decoded with wci_strerror().

        ****************************************************************** */

        virtual WCI_error write ( 
                                  WCI_u32 offset,
                                  WCI_u32 nBytes,
                                  WCI_data_type data_type,
                                  WCI_options options,
                                  const void* p_data ) = 0;

        /**
          @brief
            Ends a session created by wci_open() and releases resources.

          The function close() frees the resources allocated for the
          Worker.  If it fails, the resources may not all be recovered.
          By default, the call to close () asserts the reset signal on the
          RPL worker unless the WCI_DO_NOT_RESET option is specified.

          @param [ in ] h_worker
                        The Worker handle provided to the WCI API function
                        that initiated the close operation.

          @param [ in ] options
                        WCI_options argument passed to the WCI API function
                        that initiated the close operation.

          @retval Success WCI_SUCCESS
          @retval Error   Error code that describes the error. Error
                          code can be decoded with wci_strerror().

        ****************************************************************** */

        virtual WCI_error close ( 
                                  WCI_options options ) = 0;

    }; // End: class Worker;



  } // End: namespace WCI

} // End: namespace OCPI

#endif // End: #ifndef INCLUDED_OCPI_WCI_WORKER_H


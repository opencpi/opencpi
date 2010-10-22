
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
  @brief
    Contains the definitions of the WCI interface. The actual WCI operation
    is delegated to a WCI implementation function.

  Revision History:

    10/13/2008 - Michael Pepe
                 Initial version.

************************************************************************** */

#include "OcpiWciWorker.h"

#ifdef WCI_IMPL
#include "OcpiWciWorkerRpl.h"
#endif

#include "OcpiWciWorkerHandle.h"

#include "OcpiWciWorkerIdentifier.h"

#include "OcpiUtilException.h"

#include <iostream>

using namespace OCPI;

namespace
{
  WCI::WorkerHandle* worker_to_handle ( WCI__worker* p )
  throw ( )
  {
    return static_cast<WCI::WorkerHandle*>( p );
  }

  WCI__worker* handle_to_worker ( WCI::WorkerHandle* p )
  throw ( )
  {
    return static_cast<WCI__worker*>( p );
  }

  WCI_error wci_impl_factory ( const char* p_name,
                               WCI_timeout n_msecs,
                               WCI_options options,
                               WCI_worker* ph_worker )
  throw ( OCPI::Util::EmbeddedException )
  {
    ( void ) n_msecs;
    ( void ) options;
    WCI_error rc ( WCI_SUCCESS );

    WCI::WorkerIdentifier path ( p_name );

    *ph_worker = 0;

    try
    {
      if ( path.workerType ( ) == "wci-rpl" )
      {
        std::auto_ptr<WCI::WorkerHandle> p ( new WCI::WorkerHandle ( ) );

#ifdef WCI_IMPL
        p->d_p_impl.reset ( new WCI::WorkerRpl ( p_name,
                                                 n_msecs,
                                                 options,
                                                 p.get ( ) ) );

        *ph_worker = handle_to_worker ( p.release ( ) );
#endif

      }
      else if ( path.workerType ( ) == "wci-rcc" )
      {
        std::cerr << "Worker type RCC is not implemented." << std::endl;
        throw OCPI::Util::EmbeddedException ( WCI_ERROR_UNKNOWN_WORKER_TYPE,
                                             "RCC Worker not implemented.",
                                             0 );
      }
      else
      {
        rc = WCI_ERROR_UNKNOWN_WORKER_TYPE;
      }

    }
    catch ( std::bad_alloc )
    {
      rc = WCI_ERROR_OUT_OF_MEMORY;
    }
    catch ( const OCPI::Util::EmbeddedException& e )
    {
      std::cerr << e.getAuxInfo ( ) << std::endl;

      rc = e.getErrorCode ( );
    }
    catch ( ... )
    {
      rc = WCI_ERROR_UNKNOWN;
    }

    return rc;
  }

} // End: namespace <unamed>

WCI_error wci_open ( const char* p_name,
                     WCI_timeout n_msecs,
                     WCI_options options,
                     WCI_worker* ph_worker )
{
  WCI_error rc ( WCI_SUCCESS );

  if ( ph_worker == 0 )
  {
    rc = WCI_ERROR_NULL_HANDLE;
  }
  else
  {
    rc = wci_impl_factory ( p_name, n_msecs, options, ph_worker );
  }

  return rc;
}


WCI_error wci_close ( WCI_worker h_worker,
                      WCI_options options )
{
  WCI_error rc ( WCI_SUCCESS );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  p->d_p_impl->close ( h_worker, options );

  try
  {
    delete p;
  }
  catch ( const OCPI::Util::EmbeddedException& e )
  {
    std::cerr << e.getAuxInfo ( ) << std::endl;

    rc = e.getErrorCode ( );
  }
  catch ( ... )
  {
    rc = WCI_ERROR_UNKNOWN;
  }

  return rc;
}


WCI_error wci_control ( WCI_worker h_worker,
                        WCI_control operation,
                        WCI_options options )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->control ( h_worker, operation, options );
  }

  return rc;
}


WCI_error wci_status ( WCI_worker h_worker,
                       WCI_status* p_status )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->status ( h_worker, p_status );
  }

  return rc;
}


WCI_error wci_read ( WCI_worker h_worker,
                     WCI_u32 offset,
                     WCI_u32 n_bytes,
                     WCI_data_type data_type,
                     WCI_options options,
                     void* p_data )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->read ( h_worker,
                             offset,
                             n_bytes,
                             data_type,
                             options,
                             p_data );
  }

  return rc;
}


WCI_error wci_write ( WCI_worker h_worker,
                      WCI_u32 offset,
                      WCI_u32 n_bytes,
                      WCI_data_type data_type,
                      WCI_options options,
                      const void* p_data )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->write ( h_worker,
                              offset,
                              n_bytes,
                              data_type,
                              options,
                              p_data );
  }

  return rc;
}


WCI_error wci_read_list ( WCI_worker h_worker,
                          const WCI_access* p_accesses,
                          WCI_u32 n_list_elems,
                          WCI_options options )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    for ( WCI_u32 n = 0; n != n_list_elems; ++n )
    {
      rc = p->d_p_impl->read ( h_worker,
                               p_accesses [ n ].offset,
                               p_accesses [ n ].n_bytes,
                               p_accesses [ n ].data_type,
                               options,
                               p_accesses [ n ].p_data );
      if ( rc )
      {
        break;
      }
    }
  }

  return rc;
}


WCI_error wci_write_list ( WCI_worker h_worker,
                           const WCI_access* p_accesses,
                           WCI_u32 n_list_elems,
                           WCI_options options )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    for ( WCI_u32 n = 0; n != n_list_elems; ++n )
    {
      rc = p->d_p_impl->write ( h_worker,
                                p_accesses [ n ].offset,
                                p_accesses [ n ].n_bytes,
                                p_accesses [ n ].data_type,
                                options,
                                p_accesses [ n ].p_data );
      if ( rc )
      {
        break;
      }
    }
  }

  return rc;
}


WCI_error wci_set_u8 ( WCI_worker h_worker,
                       WCI_u32 offset,
                       WCI_u8 value )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->write ( h_worker,
                              offset,
                              sizeof ( WCI_u8 ),
                              WCI_DATA_TYPE_U8,
                              WCI_DEFAULT,
                              ( void* ) &value );
  }

  return rc;
}


WCI_error wci_set_u16 ( WCI_worker h_worker,
                        WCI_u32 offset,
                        WCI_u16 value )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->write ( h_worker,
                              offset,
                              sizeof ( WCI_u16 ),
                              WCI_DATA_TYPE_U16,
                              WCI_DEFAULT,
                              ( void* ) &value );
  }

  return rc;
}


WCI_error wci_set_u32 ( WCI_worker h_worker,
                        WCI_u32 offset,
                        WCI_u32 value )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->write ( h_worker,
                              offset,
                              sizeof ( WCI_u32 ),
                              WCI_DATA_TYPE_U32,
                              WCI_DEFAULT,
                              ( void* ) &value );
  }

  return rc;
}


WCI_error wci_set_u64 ( WCI_worker h_worker,
                        WCI_u32 offset,
                        WCI_u64 value )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->write ( h_worker,
                              offset,
                              sizeof ( WCI_u64 ),
                              WCI_DATA_TYPE_U64,
                              WCI_DEFAULT,
                              ( void* ) &value );
  }

  return rc;
}


WCI_error wci_set_f32 ( WCI_worker h_worker,
                        WCI_u32 offset,
                        WCI_f32 value )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->write ( h_worker,
                              offset,
                              sizeof ( WCI_f32 ),
                              WCI_DATA_TYPE_F32,
                              WCI_DEFAULT,
                              ( void* ) &value );
  }

  return rc;
}


WCI_error wci_set_f64 ( WCI_worker h_worker,
                        WCI_u32 offset,
                        WCI_f64 value )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->write ( h_worker,
                              offset,
                              sizeof ( WCI_f64 ),
                              WCI_DATA_TYPE_F64,
                              WCI_DEFAULT,
                              ( void* ) &value );
  }

  return rc;
}


WCI_error wci_get_u8 ( WCI_worker h_worker,
                       WCI_u32 offset,
                       WCI_u8* p_value )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->read ( h_worker,
                             offset,
                             sizeof ( WCI_u8 ),
                             WCI_DATA_TYPE_U8,
                             WCI_DEFAULT,
                             static_cast<void*> ( p_value ) );
  }

  return rc;
}


WCI_error wci_get_u16 ( WCI_worker h_worker,
                        WCI_u32 offset,
                        WCI_u16* p_value )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->read ( h_worker,
                             offset,
                             sizeof ( WCI_u16 ),
                             WCI_DATA_TYPE_U16,
                             WCI_DEFAULT,
                             static_cast<void*> ( p_value ) );
  }

  return rc;
}


WCI_error wci_get_u32 ( WCI_worker h_worker,
                        WCI_u32 offset,
                        WCI_u32* p_value )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->read ( h_worker,
                             offset,
                             sizeof ( WCI_u32 ),
                             WCI_DATA_TYPE_U32,
                             WCI_DEFAULT,
                             static_cast<void*> ( p_value ) );
  }

  return rc;
}


WCI_error wci_get_u64 ( WCI_worker h_worker,
                        WCI_u32 offset,
                        WCI_u64* p_value )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->read ( h_worker,
                             offset,
                             sizeof ( WCI_u64 ),
                             WCI_DATA_TYPE_U64,
                             WCI_DEFAULT,
                             static_cast<void*> ( p_value ) );
  }

  return rc;
}


WCI_error wci_get_f32 ( WCI_worker h_worker,
                        WCI_u32 offset,
                        WCI_f32* p_value )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->read ( h_worker,
                             offset,
                             sizeof ( WCI_f32 ),
                             WCI_DATA_TYPE_F32,
                             WCI_DEFAULT,
                             static_cast<void*> ( p_value ) );
  }

  return rc;
}


WCI_error wci_get_f64 ( WCI_worker h_worker,
                        WCI_u32 offset,
                        WCI_f64* p_value )
{
  WCI_error rc ( WCI_ERROR_NULL_HANDLE );

  WCI::WorkerHandle* p = worker_to_handle ( h_worker );

  if ( p )
  {
    rc = p->d_p_impl->read ( h_worker,
                             offset,
                             sizeof ( WCI_f64 ),
                             WCI_DATA_TYPE_F64,
                             WCI_DEFAULT,
                             static_cast<void*> ( p_value ) );
  }

  return rc;
}



#include "DemoWorkerFacade.h"
#include "OcpiApi.h"
#include "OcpiPropertyTypesApi.h"
#include "OcpiPValueApi.h"
#include <iomanip>
#include <iostream>

namespace OA = OCPI::API;


/* ---- Find an RCC container -------------------------------------------- */

OCPI::API::Container* Demo::get_rcc_interface ( )
{
  OCPI::API::PValue props [ 2 ] = { OCPI::API::PVBool ( "polling", 1 ),
                                    OCPI::API::PVEnd };
  OCPI::API::Container *d = OCPI::API::ContainerManager::find("rcc", NULL, props);
  if ( !d )  {
    throw std::string ( "No RCC containers found.\n" );
  }
  return static_cast<OCPI::API::Container*> ( d );
}


/* ---- Function to print OCPI::Container::Property objects --------------- */

Demo::WorkerFacade::WorkerFacade ( const char* msg,
				   OA::ContainerApplication* app,
				   const char* worker_name,
				   const char* inst_name
				   )
  : d_msg ( msg ),
    d_worker ( app->createWorker ( inst_name,
                                   worker_name
                                   ) ),
    d_ports ( )
{

}

Demo::WorkerFacade::~WorkerFacade ( )
{
  // Empty
}

void Demo::WorkerFacade::set_properties ( const OCPI::API::PValue* values )
{
  for ( std::size_t n = 0; values && values [ n ].name; n++ )
  {
    OA::Property property ( d_worker, values [ n ].name );

    if ( !property.isWritable ( ) )
    {
      std::cerr << "Property named "
                << values [ n ].name
                << " is read-only."
                << std::endl;
      continue;
    }

    switch ( property.type ( ) )
    {
      case OCPI::API::OCPI_Bool:
        property.setBoolValue ( values [ n ].vBool );
        break;
      case OCPI::API::OCPI_Char:
        property.setCharValue ( values [ n ].vChar );
        break;
      case OCPI::API::OCPI_UChar:
        property.setUCharValue ( values [ n ].vUChar );
        break;
      case OCPI::API::OCPI_Short:
        property.setShortValue ( values [ n ].vShort );
        break;
      case OCPI::API::OCPI_UShort:
        property.setUShortValue ( values [ n ].vUShort );
        break;
      case OCPI::API::OCPI_Long:
        property.setLongValue ( values [ n ].vLong );
        break;
      case OCPI::API::OCPI_ULong:
        property.setULongValue ( values [ n ].vULong );
        break;
      case OCPI::API::OCPI_LongLong:
        property.setLongLongValue ( values [ n ].vLongLong );
        break;
      case OCPI::API::OCPI_ULongLong:
        property.setULongLongValue ( values [ n ].vULongLong );
        break;
      case OCPI::API::OCPI_Double:
        property.setDoubleValue ( values [ n ].vDouble );
        break;
      case OCPI::API::OCPI_Float:
        property.setFloatValue ( values [ n ].vFloat );
        break;
      case OCPI::API::OCPI_String:
        property.setStringValue ( values [ n ].vString );
        break;
      default:
        std::cerr << "error : unknown property type : "
                  << property.type ( )
                  << std::endl;
        break;

    } // End: switch ( property.getType ( ) )

  } // End: for ( std::size_t n = 0; values && values [ n ].name; ...
}

void Demo::WorkerFacade::get_properties ( OCPI::API::PValue* values ) const
{
  for ( std::size_t n = 0; values && values [ n ].name; n++ )
  {
    OA::Property property ( d_worker, values [ n ].name );

    if ( !property.isReadable ( ) )
    {
      std::cerr << "Property named "
                << values [ n ].name
                << " is write-only."
                << std::endl;
      continue;
    }

    switch ( property.type ( ) )
    {
      case OCPI::API::OCPI_Bool:
        values [ n ].vBool = property.getBoolValue ( );
        break;
      case OCPI::API::OCPI_Char:
        values [ n ].vChar = property.getCharValue ( );
        break;
      case OCPI::API::OCPI_UChar:
        values [ n ].vUChar = property.getUCharValue ( );
        break;
      case OCPI::API::OCPI_Short:
        values [ n ].vShort = property.getShortValue ( );
        break;
      case OCPI::API::OCPI_UShort:
        values [ n ].vUShort = property.getUShortValue ( );
        break;
      case OCPI::API::OCPI_Long:
        values [ n ].vLong = property.getLongValue ( );
        break;
      case OCPI::API::OCPI_ULong:
        values [ n ].vULong = property.getULongValue ( );
        break;
      case OCPI::API::OCPI_LongLong:
        values [ n ].vLongLong = property.getLongLongValue ( );
        break;
      case OCPI::API::OCPI_ULongLong:
        values [ n ].vULongLong = property.getULongLongValue ( );
        break;
      case OCPI::API::OCPI_Double:
        values [ n ].vDouble = property.getDoubleValue ( );
        break;
      case OCPI::API::OCPI_Float:
        values [ n ].vFloat = property.getFloatValue ( );
        break;
      case OCPI::API::OCPI_String:
        property.getStringValue ( values [ n ].vString,
                                  sizeof ( values [ n ].vString ) );
        break;
      default:
        std::cerr << "error : unknown property type : "
                  << property.type ( )
                  << std::endl;
        break;

    } // End: switch ( property.getType ( ) )

  } // End: for ( std::size_t n = 0; values && values [ n ].name; ...
}


OA::Port& Demo::WorkerFacade::port ( const char* name )
{
  if ( d_ports [ name ] == 0 )
  {
    d_ports [ name ] = &( d_worker.getPort ( name ) );
  }

  return *( d_ports [ name ] );
}


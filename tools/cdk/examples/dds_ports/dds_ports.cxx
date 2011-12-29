
#include "OcpiContainerApi.h"
#include "OcpiPValueApi.h"
#include <iostream>
#include <unistd.h>


namespace OA = OCPI::API;

int main ( int argc, char* argv [ ] )
{
  try
  {
    ( void ) argc;
    ( void ) argv;


    OA::PValue props [ 2 ] = { OCPI::API::PVBool ( "polling", 1 ),
			       OCPI::API::PVEnd };
    OA::Container *container = OCPI::API::ContainerManager::find("rcc", NULL, props);
    if ( !container )  {
      throw std::string ( "No RCC containers found.\n" );
    }
    OA::ContainerApplication* app = container->createApplication ( );

    // Create the DDS producer worker
    // This worker produces DDS messages 
    OA::Worker & producer  = app->createWorker( "dds_producer", "dds_producer" );    

    // Create the DDS consumer worker
    // This worker reads DDS messages
    OA::Worker & consumer  = app->createWorker( "dds_consumer", "dds_consumer" );


    OA::Port & out = producer.getPort("out");
    OA::Port & in = consumer.getPort("in");

    in.connectURL( "ocpi-dds-msg://JTest_Msg1;JTest::Msg1;u1");
    //    out.connectURL( "ocpi-dds-msg://JTest_Msg1;JTest::Msg1;u1");

    out.connectURL( "ocpi-dds-msg://JTest_Msg1");


    producer.start();
    consumer.start();

    int count = 10;
    while( count > 0 ) 
      { 
	sleep(1); 
	count--;
      };

    producer.stop();
    consumer.stop();
    sleep(2);


  }
  catch ( const std::string& s )
  {
    std::cerr << "\n\nException(s): " << s << "\n" << std::endl;
  }
  catch ( const OCPI::API::Error& e )
  {
    std::cerr << "\nException(e): rc=" << e.error( ) << std::endl;
  }
  catch ( std::exception& g )
  {
    std::cerr << "\nException(g): " << typeid( g ).name( ) << " : "
                                       << g.what ( ) << "\n" << std::endl;
  }
  catch ( ... )
  {
    std::cerr << "\n\nException(u): unknown\n" << std::endl;
  }


  

  return 0;
}



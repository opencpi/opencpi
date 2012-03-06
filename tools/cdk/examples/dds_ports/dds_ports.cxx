
#include "OcpiContainerApi.h"
#include "OcpiPValueApi.h"
#include <iostream>
#include <unistd.h>


namespace OA = OCPI::API;

int main ( int argc, char* argv [ ] )
{

  bool passed=true;

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

    in.connectURL( "ocpi-dds-msg:JTest_Msg1;JTest::Msg1;u1");
    out.connectURL( "ocpi-dds-msg:JTest_Msg1");

    // Number of message the producer should publish
    producer.setProperty( "msgs_to_produce", "30" );
    
    producer.start();
    consumer.start();

    printf("\n\nWaiting for 10 seconds for standalone DDS messages\n");
    int count = 10;
    while( count > 0 ) 
      { 
	sleep(1); 
	count--;
      };

    producer.stop();
    consumer.stop();

    printf("\n\n\n");
    std::string n[2], v[2];    
    consumer.getProperty( 0, n[0], v[0] );    consumer.getProperty( 1, n[1], v[1] );
    printf("Got %s OCPI message, %s were good\n", v[0].c_str(),v[1].c_str());
    if (v[0] != v[1] ) passed = false;
    consumer.getProperty( 2, n[0], v[0] );    consumer.getProperty( 3, n[1], v[1] );
    printf("Got %s DDS message, %s were good\n", v[0].c_str(),v[1].c_str());
    if ( v[0] != v[1] ) passed = false;
    printf("******  The test has %s   *******\n", passed ? "Passed" : "Failed" );

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

  return passed ? 0 : 1;
}



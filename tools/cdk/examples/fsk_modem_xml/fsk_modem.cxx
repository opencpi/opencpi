#include <iostream>
#include "OcpiApi.h"
#include "OcpiContainerApi.h"
#include "OcpiPValueApi.h"


namespace OA = OCPI::API;
static const double pi = 3.14159265358979323846;


int main ( int argc, char* argv [ ] )
{
  std::string app_xml("<application>"
		      " <policy mapping='MaxProcessors' processors='0'/>"

		      "  <instance worker='file_read_real' name='file_reader' selection='model==\"rcc\"'>"
		      "    <property name='fileName' value='dataIn.dat'/> "		      
		      "    <property name='genTestFile' value='true'/> "		      
		      "    <property name='stepThruMsg' value='true'/> "
		      "    <property name='stepNow' value='true'/> "
		      "  </instance> "

		      "  <instance worker='sym_fir_real' name='tx_fir_r' selection='model==\"rcc\"'>"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='nTaps' value='256'/> "
		      "  </instance> "

		      "  <instance worker='fsk_mod_complex' name='fsk_mod' selection='model==\"rcc\"'>"
		      "  </instance> "

		      "  <instance worker='sym_fir_complex' name='tx_fir_c' selection='model==\"rcc\"'>"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='nTaps' value='256'/> "
		      "  </instance> "

		      "  <instance worker='cic_hpfilter_complex' name='tx_cic' selection='model==\"rcc\"'>"
		      "    <property name='M' value='2'/> "
		      "  </instance> "
			 

		      "  <instance worker='loopback_complex' name='loopback' selection='model==\"rcc\"'>"
		      "  </instance> "


		      "  <instance worker='dds_complex' name='ddc_dds' selection='model==\"rcc\"'>"
		      "    <property name='phaseIncrement' value='12345678'/> "
		      "    <property name='syncPhase' value='0'/> "
		      "  </instance> "

		      "  <instance worker='mixer_complex' name='ddc_mixer' selection='model==\"rcc\"'>"
		      "  </instance> "


		      "  <instance worker='cic_lpfilter_complex' name='rx_cic' selection='model==\"rcc\"'>"
		      "    <property name='M' value='2'/> "
		      "  </instance> "

		      "  <instance worker='sym_fir_complex' name='rx_fir_c' selection='model==\"rcc\"'>"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='nTaps' value='256'/> "
		      "  </instance> "

		      "  <instance worker='fm_demod_complex' name='fm_demod' selection='model==\"rcc\"'>"
		      "  </instance> "


		      "  <instance worker='sym_fir_real' name='rx_fir_r' selection='model==\"rcc\"'>"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='nTaps' value='256'/> "
		      "  </instance> "
"<!--"
"-->"

		      "  <instance worker='file_write' name='file_writer' selection='model==\"rcc\"'>"
		      "    <property name='fileName' value='dataOut.dat'/> "		      
		      "  </instance> "



		      "  <connection>"
		      "    <port instance='file_reader' name='out'/>"
		      "    <port instance='tx_fir_r' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='tx_fir_r' name='out'/>"
		      "    <port instance='fsk_mod' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='tx_fir_c' name='in'/>"
		      "    <port instance='fsk_mod' name='out'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='tx_fir_c' name='out'/>"
		      "    <port instance='tx_cic' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='loopback' name='in'/>"
		      "    <port instance='tx_cic' name='out'/>"
		      "  </connection>"
			 

		      "  <connection>"
		      "    <port instance='loopback' name='out'/>"
		      "    <port instance='ddc_mixer' name='in_if'/>"
		      "  </connection>"


		      "  <connection>"
		      "    <port instance='rx_cic' name='in'/>"
		      "    <port instance='ddc_mixer' name='out'/>"
		      "  </connection>"


		      "<!--   Connections internal to the DDC -->"
		      "  <connection>"
		      "    <port instance='ddc_mixer' name='out_sync_only'/>"
		      "    <port instance='ddc_dds' name='in'/>"
		      "  </connection>"


		      "  <connection>"
		      "    <port instance='ddc_mixer' name='in_dds'/>"
		      "    <port instance='ddc_dds' name='out'/>"
		      "  </connection>"



		      "  <connection>"
		      "    <port instance='rx_cic' name='out'/>"
		      "    <port instance='rx_fir_c' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='fm_demod' name='in'/>"
		      "    <port instance='rx_fir_c' name='out'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='fm_demod' name='out'/>"
		      "    <port instance='rx_fir_r' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='rx_fir_r' name='out'/>"
		      "    <port instance='file_writer' name='in'/>"
		      "  </connection>"			 

		      "</application>");
  

  OA::Application * app = NULL;
  try
    {      
      int nContainers = 1;
      // Create several containers to distribute the workers on
      for ( int n=0; n<nContainers; n++ ) {
	char buf[1024];
	sprintf(buf, "Rcc Container %d\n", n );
	(void)OA::ContainerManager::find("rcc",buf);
      }

      OCPI::API::PValue minp_policy[] = {
	OCPI::API::PVULong("MinProcessors",0),
	OCPI::API::PVEnd
      };
      
      app = new OA::Application( app_xml, minp_policy);	
      fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
      app->initialize();
      fprintf(stderr, "Application established: containers, workers, connections all created\n");
      printf(">>> DONE Initializing!\n");
      app->start();

      while ( 1 ) {
	std::string value;
	app->getProperty( "file_reader", "stepThruMsg", value);
        if ( value == "true" ) {
	  app->getProperty( "file_reader", "stepNow", value);
	  if ( value == "false" ) {
	    // wait for user
	    char c;
	    std::cout << "Hit any key to continue" << std::endl;
	    std::cin >> c;
	    app->setProperty("file_reader","stepNow","true");
	  }
	}

	sleep( 1 );
      }

}
catch ( const std::string& s )
  {
    std::cerr << "\n\nException(s): " << s << "\n" << std::endl;
  }
 catch ( std::exception& g )
   {
     std::cerr << "\nException(g): "
	       << typeid( g ).name( )
	       << " : "
	       << g.what ( )
	       << "\n"
	       << std::endl;
   }
 catch ( ... )
   {
     std::cerr << "\n\nException(u): unknown\n" << std::endl;
   }

return 0;
}



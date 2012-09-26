#include <stdio.h>
#include <iostream>
#include "OcpiApi.h"
#include "OcpiContainerApi.h"
#include "OcpiPValueApi.h"
#include "OcpiUtilCommandLineConfiguration.h"

namespace OA = OCPI::API;

// Command line Configuration
class UnitTestConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  UnitTestConfigurator ();

public:
  bool help;
  bool verbose;
  bool cont;
  bool step;
  std::string policy;
  int  nRCCCont;
  int M;
private:
  static CommandLineConfiguration::Option g_options[];
};
static  UnitTestConfigurator config;

UnitTestConfigurator::
UnitTestConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
    verbose (false),
    cont(false),
    step(false),
    policy("min"),
    nRCCCont(1),
    M(1)
{
}

OCPI::Util::CommandLineConfiguration::Option
UnitTestConfigurator::g_options[] = {
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    OCPI_CLC_OPT(&UnitTestConfigurator::verbose), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "cont", "Produce continuous data (duplicate last buffer for debug)",
    OCPI_CLC_OPT(&UnitTestConfigurator::cont), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "step", "Step thru data 1 buffer at a time (debug)",
    OCPI_CLC_OPT(&UnitTestConfigurator::step), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "policy", "Worker deployment policy {max,min}",
    OCPI_CLC_OPT(&UnitTestConfigurator::policy), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::LONG,
    "nRCCCont", "Number of RCC containers to create",
    OCPI_CLC_OPT(&UnitTestConfigurator::policy), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::LONG,
    "M", "Decimation/Interpolation factor for CIC filters",
    OCPI_CLC_OPT(&UnitTestConfigurator::M), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&UnitTestConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (UnitTestConfigurator & config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options]" << std::endl
            << "  options: " << std::endl;
  config.printOptions (std::cout);
}

static  OCPI::API::PValue minp_policy[] = {
  OCPI::API::PVULong("MinProcessors",0),
  OCPI::API::PVEnd
};

static OCPI::API::PValue maxp_policy[] = {
  OCPI::API::PVULong("MaxProcessors",0),
  OCPI::API::PVEnd
};


int main ( int argc, char* argv [ ] )
{
  const char * axml("<application>"

		      "  <instance worker='file_read_msg' >"
		      "    <property name='fileName' value='dataIn.dat'/> "		      
		      "    <property name='genTestFile' value='true'/> "		      
		      "    <property name='stepThruMsg' value='%s'/> "
		      "    <property name='stepNow' value='%s'/> "
		      "    <property name='continuous' value='%s'/> "
		      "  </instance> "

		      "  <instance worker='sym_fir_real' name='tx_fir_r' >"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='taps' valuefile='fir_real_coefs.xml'/> "
		      "  </instance> "

		      "  <instance worker='fsk_mod_complex' name='fsk_mod' >"
		      "  </instance> "

		      "  <instance worker='sym_fir_complex' name='tx_fir_c'>"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='taps' valuefile='fir_complex_coefs.xml'/> "
		      "  </instance> "

		      "  <instance worker='cic_hpfilter_complex' name='tx_cic' >"
		      "    <property name='M' value='%d'/> "
		      "  </instance> "
			 
		      "  <instance worker='loopback' >"
		      "  </instance> "

		      "  <instance worker='noise_gen_complex' >"
		      "    <property name='mask' value='0'/> "		      
		      "  </instance> "

#ifdef NEED_MIXER
		      "  <instance worker='dds_complex' name='ddc_dds' >"
		      "    <property name='phaseIncrement' value='12345678'/> "
		      "    <property name='syncPhase' value='0'/> "
		      "  </instance> "

		      "  <instance worker='mixer_complex' name='ddc_mixer' >"
		      "  </instance> "
#endif
		      "  <instance worker='cic_lpfilter_complex' name='rx_cic' >"
		      "    <property name='M' value='%d'/> "
		      "  </instance> "

		      "  <instance worker='sym_fir_complex' name='rx_fir_c' >"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='taps' valuefile='fir_complex_coefs.xml'/> "
		      "  </instance> "

		      "  <instance worker='fm_demod_complex' name='fm_demod' >"
		      "  </instance> "

		      "  <instance worker='sym_fir_real' name='rx_fir_r' >"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='taps' valuefile='fir_real_coefs.xml'/> "
		      "  </instance> "

		      "  <instance worker='file_write_msg' >"
		      "    <property name='fileName' value='dataOut.dat'/> "		      
		      "  </instance> "

		      "  <connection>"
		      "    <port instance='file_read_msg' name='out'/>"
		      "    <port instance='tx_fir_r' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='tx_fir_r' name='out'/>"
		      "    <port instance='fsk_mod' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='fsk_mod' name='out'/>"
		      "    <port instance='tx_fir_c' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='tx_fir_c' name='out'/>"
		      "    <port instance='tx_cic' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='tx_cic' name='out'/>"
		      "    <port instance='noise_gen_complex' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='noise_gen_complex' name='out'/>"
		      "    <port instance='loopback' name='in'/>"
		      "  </connection>"			 

#ifdef NEED_MIXER
		      "  <connection>"
		      "    <port instance='loopback' name='out'/>"
		      "    <port instance='ddc_mixer' name='in_if'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='rx_cic' name='in'/>"
		      "    <port instance='ddc_mixer' name='out'/>"
		      "  </connection>"
#else
		      "  <connection>"
		      "    <port instance='loopback' name='out'/>"
		      "    <port instance='rx_cic' name='in'/>"
		      "  </connection>"

#endif



#ifdef NEED_MIXER
		      "<!--   Connections internal to the DDC -->"
		      "  <connection>"
		      "    <port instance='ddc_mixer' name='out_sync_only'/>"
		      "    <port instance='ddc_dds' name='in'/>"
		      "  </connection>"


		      "  <connection>"
		      "    <port instance='ddc_mixer' name='in_dds'/>"
		      "    <port instance='ddc_dds' name='out'/>"
		      "  </connection>"
#endif

		      "  <connection>"
		      "    <port instance='rx_cic' name='out'/>"
		      "    <port instance='rx_fir_c' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='rx_fir_c' name='out'/>"
		      "    <port instance='fm_demod' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='fm_demod' name='out'/>"
		      "    <port instance='rx_fir_r' name='in'/>"
		      "  </connection>"

		      "  <connection>"
		      "    <port instance='rx_fir_r' name='out'/>"
		      "    <port instance='file_write_msg' name='in'/>"
		      "  </connection>"			 

		      "</application>");

  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cerr << "Error: " << oops << std::endl;
    return 0;
  }
  if (config.help) {
    printUsage (config, argv[0]);
    return 0;
  }
    

  OA::Application * app = NULL;
  try
    {      
      // Create several containers to distribute the workers on
      for ( int n=0; n<config.nRCCCont; n++ ) {
	char buf[1024];
	sprintf(buf, "Rcc Container %d\n", n );
	(void)OA::ContainerManager::find("rcc",buf);
      }

      const int xmlbsize = 10*1024;
      char as_xml[xmlbsize];
      snprintf( as_xml, xmlbsize, axml, config.step  ? "true" : "false", config.step ? "true" : "false",
		config.cont ? "true" : "false",
		config.M, config.M
		);
      std::string app_xml(as_xml);

      OCPI::API::PValue * policy;
      if ( config.policy == "max" ) {
	policy = maxp_policy;
      }
      else {
	policy = minp_policy;
      }
      
      app = new OA::Application( app_xml, policy);	
      fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
      app->initialize();
      fprintf(stderr, "Application established: containers, workers, connections all created\n");
      printf(">>> DONE Initializing!\n");
      app->start();

      unsigned count=0,
	mcount = 4;
      while ( (count++ < mcount) | config.cont ) {

	/****
	 *  DEBUG, step thru data
	 ****/
	if ( config.step ) {
	  std::string value;
	  app->getProperty( "file_read_msg", "stepNow", value);
	  if ( value == "false" ) {
	    // wait for user
	    char c;
	    std::cout << "Hit any key to continue" << std::endl;
	    std::cin >> c;
	    app->setProperty("file_read_msg","stepNow","true");
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



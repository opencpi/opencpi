#include <iostream>
#include "OcpiApi.h"
#include "OcpiContainerApi.h"
#include "OcpiPValueApi.h"
#include <OcpiUtilCommandLineConfiguration.h>

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
  bool real;
  std::string unit_test_name;
  std::string unit_test_props;
  std::string comparator_props;
  std::string model;  
  bool cont;
private:
  static CommandLineConfiguration::Option g_options[];
};
static  UnitTestConfigurator config;

UnitTestConfigurator::
UnitTestConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
    verbose (false),
    model("rcc"),
    cont(false)
{
}

OCPI::Util::CommandLineConfiguration::Option
UnitTestConfigurator::g_options[] = {
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "utname", "unit test component name",
    OCPI_CLC_OPT(&UnitTestConfigurator::unit_test_name), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "utp", "Unit test component properties",
    OCPI_CLC_OPT(&UnitTestConfigurator::unit_test_props), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "compp", "Comparator properties",
    OCPI_CLC_OPT(&UnitTestConfigurator::comparator_props), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    OCPI_CLC_OPT(&UnitTestConfigurator::verbose), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "real", "Real data connection or complex",
    OCPI_CLC_OPT(&UnitTestConfigurator::real), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "cont", "Produce continuous data (duplicate last buffer for debug)",
    OCPI_CLC_OPT(&UnitTestConfigurator::cont), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "model", "The model for the UUT, {rcc,hdl}",
    OCPI_CLC_OPT(&UnitTestConfigurator::model), 0 },
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


int main ( int argc, char* argv [ ] )
{
  int passed = 1;
  const char * xml("<application>"
		   " <policy mapping='MaxProcessors' processors='0'/>"

		   "  <instance worker='file_read_msg' name='fr_test_data' >"
		   "    <property name='fileName' value='testDataIn.dat'/> "		      
		   "    <property name='genTestFile' value='false'/> "		      
		   "    <property name='stepThruMsg' value='false'/> "
		   "    <property name='stepNow' value='true'/> "
		   "    <property name='continuous' value='%s'/> "
		   "  </instance> "

		   "  <instance worker='%s' name='unit_test'  selection='model==\"%s\"'>"
		   "  %s "
		   "  </instance> "

		   "  <instance worker='file_write_msg' name='fw_test_gen_data' >"
		   "    <property name='fileName' value='testDataOut.dat'/> "
		   "  </instance> "

		   "  <instance worker='file_read_msg' name='fr_expected_data'>"
		   "    <property name='fileName' value='expectedDataIn.dat'/> "		      
		   "    <property name='genTestFile' value='false'/> "		      
		   "    <property name='stepThruMsg' value='false'/> "
		   "    <property name='stepNow' value='true'/> "
		   "  </instance> "

		   "  <instance worker='file_write_msg' name='fw_delta_data' >"
		   "    <property name='fileName' value='deltaDataOut.dat'/> "
		   "  </instance> "

		   "  <instance worker='comparator_%s' name='comparator'>"
		   "    <property name='passed' value='false'/> "
		   " %s "
		   "  </instance> "

		   "  <connection>"
		   "    <port instance='fr_test_data' name='out'/>"
		   "    <port instance='unit_test' name='in'/>"
		   "  </connection>"

		   "  <connection>"
		   "    <port instance='unit_test' name='out'/>"
		   "    <port instance='comparator' name='in_unit_test'/>"
		   "  </connection>"

		   "  <connection>"
		   "    <port instance='comparator' name='in_expected'/>"
		   "    <port instance='fr_expected_data' name='out'/>"
		   "  </connection>"

		   "  <connection>"
		   "    <port instance='comparator' name='out_actual'/>"
		   "    <port instance='fw_test_gen_data' name='in'/>"
		   "  </connection>"

		   "  <connection>"
		   "    <port instance='comparator' name='out_delta'/>"
		   "    <port instance='fw_delta_data' name='in'/>"
		   "  </connection>"

		   "</application>");


  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cerr << "Error: " << oops << std::endl;
    return 0;
  }
  if (config.unit_test_name.empty()) {
    printUsage (config, argv[0]);
    return 0;
  }
  if (config.help) {
    printUsage (config, argv[0]);
    return 0;
  }
  
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

      char app_xml[4096];
      snprintf( app_xml, 4095, xml, config.cont ? "true" : "false",
		config.unit_test_name.c_str(), config.model.c_str(), 
		config.unit_test_props.c_str(),  config.real ? "real" : "complex", config.comparator_props.c_str() );

      printf("%s\n", app_xml );
      
      std::string s = app_xml;
      app = new OA::Application( s, minp_policy);	
      fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
      app->initialize();
      fprintf(stderr, "Application established: containers, workers, connections all created\n");
      printf(">>> DONE Initializing!\n");
      app->start();

      std::string value;
      int retries=5;
      while ( retries ) {

	// Make sure file reader processed all data
	app->getProperty( "fr_test_data", "finished", value);
        if ( value == "false" ) {
	  retries--;
	  sleep(1);
	  continue;
	}

	app->getProperty( "fr_expected_data", "finished", value);
        if ( value == "false" ) {
	  retries--;
	  sleep(1);
	  continue;
	}

	// At this point the file readers have pushed all their data so we will give it
	// time to move thru the pipe
	sleep( 2 );
	break;

      }

      app->getProperty( "comparator", "passed", value);
      if ( value == "true" ) {
	passed = 0;
      }
      printf("\n\n\nTEST %s \n\n\n", passed == 0 ? "passed" : "failed" );

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
  fflush( stdout );
  return passed;
}



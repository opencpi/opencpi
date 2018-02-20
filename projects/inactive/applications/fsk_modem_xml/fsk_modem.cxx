/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <typeinfo>
#include "OcpiApi.hh"
#include "OcpiContainerApi.h"
#include "OcpiPValueApi.h"

namespace OA = OCPI::API;

#define OCPI_OPTIONS_HELP \
  "Usage syntax is: fsk_model [options]\n" \
  "This command evaluates the expression, with the provided variables and result type.\n"

#define OCPI_OPTIONS \
  CMD_OPTION(verbose,   v, Bool,   0, "be verbose in describing what is happening")\
  CMD_OPTION(cont,      c, Bool,   0, "Produce continuous data (duplicate last buffer)") \
  CMD_OPTION(step,      s, Bool,   0, "Step thru data 1 buffer at a time (debug)") \
  CMD_OPTION(policy,    p, String, 0, "Worker deployment policy {max,min}") \
  CMD_OPTION(rcc_count, r, ULong,  0, "Number of RCC containers to create") \
  CMD_OPTION(M,         m, ULong,  0, "Decimation/Interpolation factor for CIC filters") \

#include "CmdOption.h"

static  OCPI::API::PValue minp_policy[] = {
  OCPI::API::PVBool("verbose",true),
  OCPI::API::PVBool("dump",true),
  OCPI::API::PVULong("MinProcessors",0),
  OCPI::API::PVEnd
};

static OCPI::API::PValue maxp_policy[] = {
  OCPI::API::PVBool("verbose",true),
  OCPI::API::PVBool("dump",true),
  OCPI::API::PVULong("MaxProcessors",0),
  OCPI::API::PVEnd
};


static int
mymain(const char **argv) {
  const char * axml("<application package='ocpi.inactive' done='file_write'>"

		      "  <instance component='ocpi.core.file_read' >"
		      "    <property name='fileName' value='dataIn.dat'/> "
		    //		      "    <property name='genTestFile' value='false'/> "
		    //		      "    <property name='stepThruMsg' value='%s'/> "
		    //		      "    <property name='stepNow' value='%s'/> "
		    //		      "    <property name='continuous' value='%s'/> "
		      "    <property name='messageSize' value='1024'/> "
		      "  </instance> "

		      "  <instance component='sym_fir_real' name='tx_fir_r' >"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='taps' valuefile='fir_real_coefs.xml'/> "
		      "  </instance> "

		      "  <instance component='fsk_mod_complex' name='fsk_mod' >"
		      "  </instance> "

		      "  <instance component='sym_fir_complex' name='tx_fir_c'>"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='taps' valuefile='fir_complex_coefs.xml'/> "
		      "  </instance> "

		      "  <instance component='cic_hpfilter_complex' name='tx_cic' >"
		      "    <property name='M' value='%d'/> "
		      "  </instance> "

		      "  <instance component='loopback' >"
		      "  </instance> "

		      "  <instance component='noise_gen_complex' >"
		      "    <property name='mask' value='0'/> "
		      "  </instance> "

#ifdef NEED_MIXER
		      "  <instance component='dds_complex' name='ddc_dds' >"
		      "    <property name='phaseIncrement' value='12345678'/> "
		      "    <property name='syncPhase' value='0'/> "
		      "  </instance> "

		      "  <instance component='mixer_complex' name='ddc_mixer' >"
		      "  </instance> "
#endif
		      "  <instance component='cic_lpfilter_complex' name='rx_cic' >"
		      "    <property name='M' value='%d'/> "
		      "  </instance> "

		      "  <instance component='sym_fir_complex' name='rx_fir_c' >"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='taps' valuefile='fir_complex_coefs.xml'/> "
		      "  </instance> "

		      "  <instance component='fm_demod_complex' name='fm_demod' >"
		      "  </instance> "

		      "  <instance component='sym_fir_real' name='rx_fir_r' >"
		      "    <property name='bypass' value='false'/> "
		      "    <property name='gain' value='1'/> "
		      "    <property name='taps' valuefile='fir_real_coefs.xml'/> "
		      "  </instance> "

		      "  <instance component='ocpi.core.file_write' >"
		      "    <property name='fileName' value='dataOut.dat'/> "
		      "  </instance> "

		      "  <connection>"
		      "    <port instance='file_read' name='out'/>"
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
		      "    <port instance='file_write' name='in'/>"
		      "  </connection>"

		      "</application>");

  try
    {
      // Create several containers to distribute the workers on
      for (unsigned n=0; n<options.rcc_count(); n++ ) {
	char buf[1024];
	sprintf(buf, "Rcc Container %d\n", n );
	(void)OA::ContainerManager::find("rcc",buf);
      }

      //      const int xmlbsize = 10*1024;
      char *as_xml;
      asprintf(&as_xml,
	       axml,// config.step  ? "true" : "false", config.step ? "true" : "false", config.cont ? "true" : "false",
	       options.M(), options.M()
		);
      std::string app_xml(as_xml);
      free(as_xml);

      OCPI::API::PValue *policy = options.policy() && !strcmp(options.policy(),"max") ?
	maxp_policy : minp_policy;

      OA::Application app( app_xml, policy);
      fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
      app.initialize();


      std::string value;
#ifdef TIME_PROPERTY_FUNTIONS
      // This here to simulate the AGC
      app.getProperty( "tx_fir_r", "gain", value);
      app.setProperty("tx_fir_r","gain","1");
      app.getProperty( "rx_fir_r", "gain", value);
      app.setProperty("rx_fir_r","gain","1");
      app.getProperty( "tx_fir_r", "gain", value);
      app.setProperty("tx_fir_r","gain","1");
      app.getProperty( "rx_fir_r", "gain", value);
      app.setProperty("rx_fir_r","gain","1");
#endif


      fprintf(stderr, "Application established: containers, workers, connections all created\n");
      printf(">>> DONE Initializing!\n");
      app.start();

      unsigned count=0,
	mcount = 4;
      while ( (count++ < mcount) | options.cont() ) {


	/****
	 *  DEBUG, step thru data
	 ****/
	if ( options.step() ) {
	  app.getProperty( "file_read_msg", "stepNow", value);
	  if ( value == "false" ) {
	    // wait for user
	    char c;
	    std::cout << "Hit any key to continue" << std::endl;
	    std::cin >> c;
	    app.setProperty("file_read_msg","stepNow","true");
	  }
	}

	sleep( 1 );
      }

}
catch ( const std::string& s )
  {
    std::cerr << "\n\nException(s): " << s << "\n" << std::endl;
     return 1;
  }
 catch ( std::exception& g )
   {
     std::cerr << "\nException(g): "
	       << typeid( g ).name( )
	       << " : "
	       << g.what ( )
	       << "\n"
	       << std::endl;
     return 1;
   }
 catch ( ... )
   {
     std::cerr << "\n\nException(u): unknown\n" << std::endl;
     return 1;
   }

  return 0;
}

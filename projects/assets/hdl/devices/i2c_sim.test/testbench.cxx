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

/*
 * This file is the Application Control Interface API Simulation testbench for the
 * Matchstiq I2C component. This file works with HDL implementations only. Input
 * parameters include the name of the OAS file (without the .xml extension).
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "OcpiApi.h"

namespace OA = OCPI::API;

int main(int argc, char **argv) {
  try {
    std::string appName, appNameXml, workerModel, tc, bypass, writevalue, readvalue;
    bool pass, hex;

    if (argc < 2) {
      printf("Program needs an OAS argument.\n");
      return 1;
    }

    // since appName is defined as an unknown string length above make it the
    // size of the input argument and convert to lower case text
    appName.resize(strlen(argv[1]));
    for(unsigned int i = 0; i < strlen(argv[1]); i++) {
      appName[i] = tolower(argv[1][i]);
    }

    printf("appName=%s\n",appName.c_str());

    // default case of HDL worker
    appNameXml = "sim_testbench_app_file.xml";
    printf("sim_testbench_app_file selected.\n");

    printf("appNameXml=%s\n",appNameXml.c_str());

    // define the OA::Application parameter values; enable verbose mode
    OA::PValue params[] = {
      //OA::PVBool("verbose",true),
      OA::PVBool("hex",true),
      OA::PVEnd
    };

    // construct the app and run
    OA::Application app(appNameXml.c_str(), params);
    app.initialize();
    printf("App initialized.\n");

    // start the app 
    app.start();
    printf("App started.\n");

    // perform a write and read to/from each i2c master
    pass=true;
    hex=true;
    writevalue="0xaa";
    app.setProperty("i2c_sim_master","test0", writevalue.c_str());
    app.getProperty("i2c_sim_master","test0", readvalue, hex);
    if(readvalue.compare(writevalue) == 0){
      printf("I2C Readback Test for 8 bit master: Passed.\n");
    }else{
      printf("I2C Readback Test for 8 bit master: Failed. %s != %s\n",writevalue.c_str(),readvalue.c_str());
      pass=false;
    }
    writevalue="0xaabb";
    app.setProperty("i2c_sim_master_16b_props","test01", writevalue.c_str());
    app.getProperty("i2c_sim_master_16b_props","test01", readvalue, hex);
    if(readvalue.compare(writevalue) == 0){
      printf("I2C Readback Test for 16 bit master: Passed.\n");
    }else{
      printf("I2C Readback Test for 16 bit master: Failed. %s != %s\n",writevalue.c_str(),readvalue.c_str());
      pass=false;
    }
    writevalue="0xaa";
    app.setProperty("i2c_sim_master_mix_props","test0", writevalue.c_str());
    app.getProperty("i2c_sim_master_mix_props","test0", readvalue, hex);
    if(readvalue.compare(writevalue) == 0){
      printf("I2C Readback Test for 8 bit mixed master: Passed.\n");
    }else{
      printf("I2C Readback Test for 8 bit mixed master: Failed. %s != %s\n",writevalue.c_str(),readvalue.c_str());
      pass=false;
    }
    writevalue="0xccdd";
    app.setProperty("i2c_sim_master_mix_props","test23", writevalue.c_str());
    app.getProperty("i2c_sim_master_mix_props","test23", readvalue, hex);
    if(readvalue.compare(writevalue) == 0){
      printf("I2C Readback Test for 16 bit mixed master: Passed.\n");
    }else{
      printf("I2C Readback Test for 16 bit mixed master: Failed. %s != %s\n",writevalue.c_str(),readvalue.c_str());
      pass=false;
    }
    writevalue="0xaabb";
    app.setProperty("i2c_sim_master_16b_ext_wr_props","test01", writevalue.c_str());
    app.getProperty("i2c_sim_master_16b_ext_wr_props","test01", readvalue, hex);
    if(readvalue.compare(writevalue) == 0){
      printf("I2C Readback Test for 16 bit extended write master: Passed.\n");
    }else{
      printf("I2C Readback Test for 16 bit extended write master: Failed. %s != %s\n",writevalue.c_str(),readvalue.c_str());
      pass=false;
    }

    // stop the app
    app.stop();
    printf("App stopped.\n");
    sleep(1);

    if(pass){
      printf("I2C Sim Testbench: Passed.\n");
      std::ofstream outfile ("i2c_sim_testbench.results");
      outfile << "PASSED" << std::endl;
      outfile.close();
    }

  }

  catch(std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return 1;
  }

  return 0;
}

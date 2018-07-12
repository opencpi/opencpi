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
 * This file is the Application Control Interface API testbench for the lime_dac_ts
 * component. This file works with an HDL implementation. Input parameters include
 * the name of the OAS file (without the .xml extension), the TX CLK_RATE property,
 * the Platform value, the Flush property, and the Number of Output Samples
 * property. The file works very much like OCPIRUN in that it runs in verbose mode
 * (which lists deployed instances) and dumps all initial and final property values,
 * but adds the ability to manipulate property values during runtime. The test is
 * complete when an EOF zero-length message is propagated through to the file_writer,
 * which indicates DONE.
 */

#include <unistd.h>
#include <iostream>
#include <algorithm>
#include "OcpiApi.h"

void bad(const char *str) {
    std::cerr << str << std::endl;
    exit(1);
}

namespace OA = OCPI::API;

int main(int argc, char **argv) {
  try {
    std::string appName, appNameXml, workerModel, clk_rate, flush, num_output_samples;

    if (argc < 2)
      bad("Program needs an OAS argument.");
    else if (argc < 3)
      bad("Program needs a CLK_RATE argument.");
    else if (argc < 4)
      bad("Program needs a PLATFORM argument.");
    else if (argc < 5)
      bad("Program needs a FLUSH argument.");
    else if (argc < 6)
      bad("Program needs a NUM_OUTPUT_SAMPLES argument.");

    // appName is input argument and convert to lower case text
    appName = argv[1];
    std::transform(appName.begin(), appName.end(), appName.begin(), ::tolower);

    // printf("appName=%s\n",appName.c_str());

    appNameXml = appName + ".xml";
    // printf("appNameXml=%s\n",appNameXml.c_str());

    clk_rate = argv[2];
    flush = argv[4];
    num_output_samples = argv[5];

    // force the model to HDL
    workerModel = "qdac_ts=hdl";

    std::string p("=");
    p += argv[3];
    // define the OA::Application parameter values; enable verbose mode
    OA::PValue params[] = {
      OA::PVString("model", workerModel.c_str()),
      OA::PVBool("verbose",true),
      OA::PVString("platform", p.c_str()),
      OA::PVEnd
    };

    // construct the app and run
    OA::Application app(appNameXml.c_str(), params);
    app.initialize();
    printf("App initialized.\n");
    // here is where a runtime property is set, but could also happen after
    // start following some amount of time but before stop
    app.setProperty("lime_dac_ts_em", "tx_clk_rate", clk_rate.c_str());
    app.setProperty("lime_dac_ts_em", "numOutputSamples", num_output_samples.c_str());

    // dump all inital properties following initalization but before start
    std::string name, value;
    bool isParameter, hex = false;
    fprintf(stderr, "Dump of all initial property values:\n");
    for (unsigned n = 0; app.getProperty(n, name, value, hex, &isParameter); n++) {
      fprintf(stderr, "Property %2u: %s = \"%s\"%s\n", n, name.c_str(), value.c_str(), isParameter ? " (parameter)" : "");
    }

    // start the app and wait for the file_writer to signal Done
    app.start();
    if (flush == "true") {
      app.setProperty("qdac_ts","flush", "true");
    }
    printf("App started.\n");
    app.wait();

    // stop the app and wait
    app.stop();
    printf("App stopped.\n");
    sleep(1);

    // dump all final property values
    fprintf(stderr, "Dump of all final property values:\n");
    for (unsigned n = 0; app.getProperty(n, name, value, hex, &isParameter); n++) {
      if (!isParameter) {
        fprintf(stderr, "Property %2u: %s = \"%s\"\n", n, name.c_str(), value.c_str());
      }
    }
  }

  catch(std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return 1;
  }

  return 0;
}

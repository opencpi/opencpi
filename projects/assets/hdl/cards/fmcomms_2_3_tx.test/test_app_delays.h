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

#ifndef _TEST_APP_DELAYS_H
#define _TEST_APP_DELAYS_H

#include <string>     // std::string
#include "OcpiApi.hh" // OCPI::API namespace
#include "ocpi_component_prop_type_helpers.h" // ocpi_ushort_t
#include "test_app_common.h"// APP_DEFAULT_... macros

void get_FPGA_bitstream_FB_CLK_Delay(OCPI::API::Application& app,
    ocpi_ushort_t& FB_CLK_Delay) {
  std::string FB_CLK_Delay_str;
  app.getProperty(APP_DEFAULT_XML_INST_NAME_DATA_SUB, "FB_CLK_Delay", FB_CLK_Delay_str);
  FB_CLK_Delay = (ocpi_ushort_t) (strtol(FB_CLK_Delay_str.c_str(), NULL, 0) & 0xffff);
}

void get_FPGA_bitstream_TX_Data_Delay(OCPI::API::Application& app,
    ocpi_ushort_t& TX_Data_Delay) {
  std::string TX_Data_Delay_str;
  app.getProperty(APP_DEFAULT_XML_INST_NAME_DATA_SUB, "TX_Data_Delay", TX_Data_Delay_str);
  TX_Data_Delay = (ocpi_ushort_t) (strtol(TX_Data_Delay_str.c_str(), NULL, 0) & 0xffff);
}

bool did_pass_test_ocpi_app_AD9361_FB_CLK_Delay_enforcement()
{
  printf("TEST: AD9361 FB_CLK_Delay autonomous assignment\n");
  try
  {
    {
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();
      app.stop();

      // expected val (note this is FPGA bitstream-dependent)
      ocpi_ushort_t expected_FB_CLK_Delay; // from ad9361_data_sub;
      get_FPGA_bitstream_FB_CLK_Delay(app, expected_FB_CLK_Delay);

      // actual assigned val
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "FB_CLK_Delay");
      ocpi_ushort_t FB_CLK_Delay_ad9361_config_proxy = p.getUShortValue();
      
      TEST_EXPECTED_VAL(expected_FB_CLK_Delay, FB_CLK_Delay_ad9361_config_proxy);
    }

    {
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();
      app.stop();

      // expected val (note this is FPGA bitstream-dependent)
      ocpi_ushort_t expected_FB_CLK_Delay; // from ad9361_data_sub;
      get_FPGA_bitstream_FB_CLK_Delay(app, expected_FB_CLK_Delay);

      // actual assigned val
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "FB_CLK_Delay");
      ocpi_ushort_t FB_CLK_Delay_ad9361_config_proxy = p.getUShortValue();
      
      TEST_EXPECTED_VAL(expected_FB_CLK_Delay, FB_CLK_Delay_ad9361_config_proxy);
    }
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }

  return true;
}

bool did_pass_test_ocpi_app_AD9361_Tx_Data_Delay_enforcement()
{
  printf("TEST: AD9361 Tx_Data_Delay autonomous assignment\n");
  try
  {
    {
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();
      app.stop();

      // expected val (note this is FPGA bitstream-dependent
      ocpi_ushort_t expected_Tx_Data_Delay; // from ad9361_data_sub
      get_FPGA_bitstream_TX_Data_Delay(app, expected_Tx_Data_Delay);

      // actual assigned val
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "Tx_Data_Delay");
      ocpi_ushort_t Tx_Data_Delay_ad9361_config_proxy = p.getUShortValue();
      
      TEST_EXPECTED_VAL(Tx_Data_Delay_ad9361_config_proxy, expected_Tx_Data_Delay);
    }

    {
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();
      app.stop();

      // expected val (note this is FPGA bitstream-dependent
      ocpi_ushort_t expected_Tx_Data_Delay; // from ad9361_data_sub
      get_FPGA_bitstream_TX_Data_Delay(app, expected_Tx_Data_Delay);

      // actual assigned val
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "Tx_Data_Delay");
      ocpi_ushort_t Tx_Data_Delay_ad9361_config_proxy = p.getUShortValue();
      
      TEST_EXPECTED_VAL(Tx_Data_Delay_ad9361_config_proxy, expected_Tx_Data_Delay);
    }
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }

  return true;
}

#endif // _TEST_APP_DELAYS_H

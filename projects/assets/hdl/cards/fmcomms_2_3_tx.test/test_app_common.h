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

#ifndef _TEST_APP_COMMON_H
#define _TEST_APP_COMMON_H

#include "OcpiApi.hh" // OCPI::API namespace
#include "ocpi_component_prop_type_helpers.h" // ocpi_..._t types
#include "worker_prop_parsers_ad9361_config_proxy.h" // ad9361_config_proxy_tx_attenuation_t
#include "test_helpers.h" // TEST_EXPECTED_VAL(), TEST_EXPECTED_VAL_DIFF()

#define APP_DEFAULT_FMCOMMS2_XML        "app_default_fmcomms2.xml"
#define APP_DEFAULT_FMCOMMS3_XML        "app_default_fmcomms3.xml"
#define APP_DEFAULT_XML_INST_NAME_RX    "rx"
#define APP_DEFAULT_XML_INST_NAME_TX    "tx"
#define APP_DEFAULT_XML_INST_NAME_PROXY "ad9361_config_proxy"
#define APP_DEFAULT_XML_INST_NAME_DATA_SUB "ad9361_data_sub"

/*bool did_pass_test_expected_value_rx_rf_gain(
    OCPI::API::Application& app,
    ocpi_long_t expected_ad9361_config_proxy_val)
{
  try
  {
    {
      std::string str;
      app.getProperty(APP_DEFAULT_XML_INST_NAME_PROXY, "rx_rf_gain", str);
      printf("************x=%s",str.c_str());
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "rx_rf_gain");

      ocpi_long_t actual_val = p.getLongValue();
      TEST_EXPECTED_VAL(actual_val, expected_ad9361_config_proxy_val);
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}*/

bool did_pass_test_expected_value_proxy(
    OCPI::API::Application& app, const char* prop_name,
    ocpi_ulong_t expected_ad9361_config_proxy_val)
{
  try
  {
    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, prop_name);

      ocpi_ulong_t actual_val = p.getULongValue();
      TEST_EXPECTED_VAL(actual_val, expected_ad9361_config_proxy_val);
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}




bool did_pass_test_expected_value_rf_gain_dB(
    OCPI::API::Application& app,
    double expected_tx_val,
    ocpi_ulong_t expected_ad9361_config_proxy_val)
{
  std::string prop("rf_gain_dB");
  try
  {
    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, prop.c_str());
      double actual_tx_val = p.getDoubleValue();
      TEST_EXPECTED_VAL_DIFF(actual_tx_val, expected_tx_val, 0.5);
    }

    {
      std::string str;
      app.getProperty(APP_DEFAULT_XML_INST_NAME_PROXY, "tx_attenuation", str);
      ad9361_config_proxy_tx_attenuation_t tx_attenuation;
      const char* err = parse(str.c_str(), tx_attenuation);
      if(err != 0) { printf("%s",err); return false; }
      TEST_EXPECTED_VAL(tx_attenuation[0], expected_ad9361_config_proxy_val);
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_expected_value_rx_rf_gain_dB(
    OCPI::API::Application& app,
    double expected_rx_val,
    ocpi_long_t expected_ad9361_config_proxy_val)
{
  std::string prop("rf_gain_dB");
  try
  {
    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, prop.c_str());
      double actual_rx_val = p.getDoubleValue();
      TEST_EXPECTED_VAL_DIFF(actual_rx_val, expected_rx_val, 0.5);
    }

    {
      std::string str;
      app.getProperty(APP_DEFAULT_XML_INST_NAME_PROXY, "rx_rf_gain", str);
      ad9361_config_proxy_rx_rf_gain_t rx_rf_gain;
      const char* err = parse(str.c_str(), rx_rf_gain);
      if(err != 0) { printf("%s",err); return false; }
      TEST_EXPECTED_VAL(rx_rf_gain[0], expected_ad9361_config_proxy_val);
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}



bool did_pass_test_expected_value_bb_gain_dB(
    OCPI::API::Application& app,
    double expected_tx_val)
{
  std::string prop("bb_gain_dB");
  try
  {
    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, prop.c_str());
      double actual_tx_val = p.getDoubleValue();
      // some wiggle room due to double precision rounding 0.00000000000001 was
      // arbitrarily chosen to be "very little wiggle room"
      TEST_EXPECTED_VAL_DIFF(actual_tx_val, expected_tx_val, 0.00000000000001);
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_expected_value_rx_frequency_MHz(
    OCPI::API::Application& app,
    double expected_rx_val,
    ocpi_ulonglong_t expected_ad9361_config_proxy_val)
{
  std::string prop("frequency_MHz");
  try
  {
    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, prop.c_str());
      double actual_rx_val = p.getDoubleValue();
      TEST_EXPECTED_VAL_DIFF(actual_rx_val, expected_rx_val, 0.000004769); // rounding up from 4.768 in fmcomms_2_3_rx.xml
    }

    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "rx_lo_freq");
      ocpi_ulonglong_t actual_ad9361_config_proxy_val = p.getULongLongValue();
      //TEST_EXPECTED_VAL(actual_ad9361_config_proxy_val, expected_ad9361_config_proxy_val);
      TEST_EXPECTED_VAL_DIFF((double)actual_ad9361_config_proxy_val, (double)expected_ad9361_config_proxy_val, 4.769); // rounding up from 4.768 in fmcomms_2_3.xml
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_expected_value_frequency_MHz(
    OCPI::API::Application& app,
    double expected_tx_val,
    ocpi_ulonglong_t expected_ad9361_config_proxy_val)
{
  std::string prop("frequency_MHz");
  try
  {
    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, prop.c_str());
      double actual_tx_val = p.getDoubleValue();
      TEST_EXPECTED_VAL_DIFF(actual_tx_val, expected_tx_val, 0.000004769); // rounding up from 4.768 in fmcomms_2_3_rx.xml
    }

    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "tx_lo_freq");
      ocpi_ulonglong_t actual_ad9361_config_proxy_val = p.getULongLongValue();
      //TEST_EXPECTED_VAL(actual_ad9361_config_proxy_val, expected_ad9361_config_proxy_val);
      TEST_EXPECTED_VAL_DIFF((double)actual_ad9361_config_proxy_val, (double)expected_ad9361_config_proxy_val, 4.769); // rounding up from 4.768 in fmcomms_2_3.xml
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_expected_value_sample_rate_MHz(
    OCPI::API::Application& app,
    double expected_tx_val,
    ocpi_ulong_t expected_ad9361_config_proxy_val)
{
  std::string prop("sample_rate_MHz");
  try
  {
    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, prop.c_str());
      double actual_tx_val = p.getDoubleValue();
      TEST_EXPECTED_VAL_DIFF(actual_tx_val, expected_tx_val, 0.0000005);
    }

    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "tx_sampling_freq");
      ocpi_ulong_t actual_ad9361_config_proxy_val = p.getULongValue();
      TEST_EXPECTED_VAL(actual_ad9361_config_proxy_val, expected_ad9361_config_proxy_val);
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_expected_value_rx_sample_rate_MHz(
    OCPI::API::Application& app,
    double expected_rx_val,
    ocpi_ulong_t expected_ad9361_config_proxy_val)
{
  std::string prop("sample_rate_MHz");
  try
  {
    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, prop.c_str());
      double actual_rx_val = p.getDoubleValue();
      TEST_EXPECTED_VAL_DIFF(actual_rx_val, expected_rx_val, 0.0000005);
    }

    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "rx_sampling_freq");
      ocpi_ulong_t actual_ad9361_config_proxy_val = p.getULongValue();
      TEST_EXPECTED_VAL(actual_ad9361_config_proxy_val, expected_ad9361_config_proxy_val);
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}


bool did_pass_test_expected_value_rf_cutoff_frequency_MHz(
    OCPI::API::Application& app,
    double expected_tx_val)
{
  std::string prop("rf_cutoff_frequency_MHz");
  try
  {
    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, prop.c_str());
      double actual_tx_val = p.getDoubleValue();
      // some wiggle room due to double precision rounding 0.00000000000001 was
      // arbitrarily chosen to be "very little wiggle room"
      TEST_EXPECTED_VAL_DIFF(actual_tx_val, expected_tx_val, 0.00000000000001);
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_expected_value_bb_cutoff_frequency_MHz(
    OCPI::API::Application& app,
    double expected_tx_val,
    ocpi_ulong_t expected_ad9361_config_proxy_val)
{
  std::string prop("bb_cutoff_frequency_MHz");
  try
  {
    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, prop.c_str());
      double actual_tx_val = p.getDoubleValue();
      TEST_EXPECTED_VAL_DIFF(actual_tx_val, expected_tx_val, 0.01); // just trying 10 kHz step for now
    }

    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "tx_rf_bandwidth");
      ocpi_ulong_t actual_ad9361_config_proxy_val = p.getULongValue();
      TEST_EXPECTED_VAL(actual_ad9361_config_proxy_val, expected_ad9361_config_proxy_val);
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_expected_value_rx_bb_cutoff_frequency_MHz(
    OCPI::API::Application& app,
    double expected_rx_val,
    ocpi_ulong_t expected_ad9361_config_proxy_val)
{
  std::string prop("bb_cutoff_frequency_MHz");
  try
  {
    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, prop.c_str());
      double actual_rx_val = p.getDoubleValue();
      TEST_EXPECTED_VAL_DIFF(actual_rx_val, expected_rx_val, 0.01); // just trying 10 kHz step for now
    }

    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "rx_rf_bandwidth");
      ocpi_ulong_t actual_ad9361_config_proxy_val = p.getULongValue();
      TEST_EXPECTED_VAL(actual_ad9361_config_proxy_val, expected_ad9361_config_proxy_val);
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}



#endif // _TEST_APP_COMMON_H

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
#include "worker_prop_parsers_ad9361_config_proxy.h" // ad9361_config_proxy_rx_rf_gain_t
#include "test_helpers.h" // TEST_EXPECTED_VAL(), TEST_EXPECTED_VAL_DIFF()

#define APP_DEFAULT_FMCOMMS2_XML           "app_default_fmcomms2.xml"
#define APP_DEFAULT_FMCOMMS3_XML           "app_default_fmcomms3.xml"
#define APP_DEFAULT_XML_INST_NAME_RX       "rx"
#define APP_DEFAULT_XML_INST_NAME_TX       "tx"
#define APP_DEFAULT_XML_INST_NAME_PROXY    "ad9361_config_proxy"
#define APP_DEFAULT_XML_INST_NAME_DATA_SUB "ad9361_data_sub"
#define APP_DEFAULT_XML_INST_NAME_ADC_SUB  "ad9361_adc_sub"

namespace OA = OCPI::API;

bool did_pass_test_expected_value_tx_rf_gain_dB(
    OA::Application& app,
    double expected_val,
    OA::ULong expected_ad9361_config_proxy_val)
{
  std::string prop("rf_gain_dB");
  try
  {
    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, prop.c_str());
      double actual_tx_val = p.getDoubleValue();
      TEST_EXPECTED_VAL_DIFF(actual_tx_val, expected_val, 0.5);
    }

    {
      std::string str;
      app.getProperty(APP_DEFAULT_XML_INST_NAME_PROXY, "tx_attenuation", str);
      ad9361_config_proxy_tx_attenuation_t tx_attenuation;
      parse(str.c_str(), tx_attenuation);
      TEST_EXPECTED_VAL(tx_attenuation[0], expected_ad9361_config_proxy_val);
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_expected_value_tx_frequency_MHz(
    OA::Application& app,
    double expected_val,
    OA::ULongLong expected_ad9361_config_proxy_val)
{
  std::string prop("frequency_MHz");
  try
  {
    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, prop.c_str());
      double actual_tx_val = p.getDoubleValue();
      TEST_EXPECTED_VAL_DIFF(actual_tx_val, expected_val, 0.000004769); // rounding up from 4.768 in fmcomms_2_3_rx.xml
    }

    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "tx_lo_freq");
      OA::ULongLong actual_ad9361_config_proxy_val = p.getULongLongValue();
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

bool did_pass_test_expected_value_tx_bb_cutoff_frequency_MHz(
    OA::Application& app,
    double expected_val,
    OA::ULong expected_ad9361_config_proxy_val)
{
  std::string prop("bb_cutoff_frequency_MHz");
  try
  {
    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, prop.c_str());
      double actual_tx_val = p.getDoubleValue();
      TEST_EXPECTED_VAL_DIFF(actual_tx_val, expected_val, 0.01); // just trying 10 kHz step for now
    }

    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "tx_rf_bandwidth");
      OA::ULong actual_ad9361_config_proxy_val = p.getULongValue();
      TEST_EXPECTED_VAL(actual_ad9361_config_proxy_val, expected_ad9361_config_proxy_val);
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_expected_value_rf_gain_dB(
    OA::Application& app,
    double expected_rx_val,
    OA::Long expected_ad9361_config_proxy_val)
{
  std::string prop("rf_gain_dB");
  try
  {
    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, prop.c_str());
      double actual_rx_val = p.getDoubleValue();
      TEST_EXPECTED_VAL_DIFF(actual_rx_val, expected_rx_val, 0.5);
    }

    {
      std::string str;
      app.getProperty(APP_DEFAULT_XML_INST_NAME_PROXY, "rx_rf_gain", str);
      ad9361_config_proxy_rx_rf_gain_t rx_rf_gain;
      parse(str.c_str(), rx_rf_gain);
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
    OA::Application& app,
    double expected_rx_val)
{
  std::string prop("bb_gain_dB");
  try
  {
    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, prop.c_str());
      double actual_rx_val = p.getDoubleValue();
      // some wiggle room due to double precision rounding 0.00000000000001 was
      // arbitrarily chosen to be "very little wiggle room"
      TEST_EXPECTED_VAL_DIFF(actual_rx_val, expected_rx_val, 0.00000000000001);
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_expected_value_frequency_MHz(
    OA::Application& app,
    double expected_rx_val,
    OA::ULongLong expected_ad9361_config_proxy_val)
{
  std::string prop("frequency_MHz");
  try
  {
    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, prop.c_str());
      double actual_rx_val = p.getDoubleValue();
      TEST_EXPECTED_VAL_DIFF(actual_rx_val, expected_rx_val, 0.000004769); // rounding up from 4.768 in fmcomms_2_3_rx.xml
    }

    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "rx_lo_freq");
      OA::ULongLong actual_ad9361_config_proxy_val = p.getULongLongValue();
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
    OA::Application& app,
    double expected_rx_val,
    OA::ULong expected_ad9361_config_proxy_val)
{
  std::string prop("sample_rate_MHz");
  try
  {
    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, prop.c_str());
      double actual_rx_val = p.getDoubleValue();
      TEST_EXPECTED_VAL_DIFF(actual_rx_val, expected_rx_val, 0.0000005);
    }

    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "rx_sampling_freq");
      OA::ULong actual_ad9361_config_proxy_val = p.getULongValue();
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
    OA::Application& app,
    double expected_rx_val)
{
  std::string prop("rf_cutoff_frequency_MHz");
  try
  {
    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, prop.c_str());
      double actual_rx_val = p.getDoubleValue();
      // some wiggle room due to double precision rounding 0.00000000000001 was
      // arbitrarily chosen to be "very little wiggle room"
      TEST_EXPECTED_VAL_DIFF(actual_rx_val, expected_rx_val, 0.00000000000001);
    }
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_expected_value_bb_cutoff_frequency_MHz(
    OA::Application& app,
    double expected_rx_val,
    OA::ULong expected_ad9361_config_proxy_val)
{
  std::string prop("bb_cutoff_frequency_MHz");
  try
  {
    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, prop.c_str());
      double actual_rx_val = p.getDoubleValue();
      TEST_EXPECTED_VAL_DIFF(actual_rx_val, expected_rx_val, 0.01); // just trying 10 kHz step for now
    }

    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_PROXY, "rx_rf_bandwidth");
      OA::ULong actual_ad9361_config_proxy_val = p.getULongValue();
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

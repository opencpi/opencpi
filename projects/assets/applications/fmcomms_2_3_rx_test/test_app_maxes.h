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

#ifndef _TEST_APP_MAXES_H
#define _TEST_APP_MAXES_H

#include <sstream> // std::ostringstream
#include <string> // std::string
#include "OcpiApi.hh" // OA namespace
#include "test_app_common.h"// APP_DEFAULT_... macros, did_pass_test_expected_value_<prop>() functions

namespace OA = OA;

bool did_pass_test_ocpi_app_max_value_rf_gain_dB()
{
  printf("TEST: max     value for rf_gain_dB\n");
  bool did_pass;
  try
  {
    OA::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
    app.initialize();
    //app.setProperty(APP_DEFAULT_XML_INST_NAME_RX, "enable_log_info",  "true");
    //app.setProperty(APP_DEFAULT_XML_INST_NAME_RX, "enable_log_trace", "true");
    //app.setProperty(APP_DEFAULT_XML_INST_NAME_RX, "enable_log_debug", "true");
    app.start();

    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "frequency_MHz");
      p.setDoubleValue(1299.999999);
    }
    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "rf_gain_dB");
      try {
        p.setDoubleValue(74.);
        did_pass = false; // set to 74. should not have succeeded
      }
      catch(...) {
        did_pass = true;
      }
      TEST_EXPECTED_VAL(did_pass, true);
      p.setDoubleValue(73.);
    }
    did_pass = did_pass_test_expected_value_frequency_MHz(app, 1299.999999, (OA::ULongLong) 1299999999);
    if(!did_pass) { return false; }
    did_pass = did_pass_test_expected_value_rf_gain_dB(app, 73., (OA::Long) 73);
    if(!did_pass) { return false; }

    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "frequency_MHz");
      p.setDoubleValue(2400.);
    }
    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "rf_gain_dB");
      try {
        p.setDoubleValue(72.);
        did_pass = false; // set to 72. should not have succeeded
      }
      catch(...) {
        did_pass = true;
      }
      TEST_EXPECTED_VAL(did_pass, true);
      if(!did_pass) { return false; }
      p.setDoubleValue(71.);
    }
    did_pass = did_pass_test_expected_value_frequency_MHz(app, 2400., (OA::ULongLong) 2400000000);
    if(!did_pass) { return false; }
    did_pass = did_pass_test_expected_value_rf_gain_dB(app, 71., (OA::Long) 71);
    if(!did_pass) { return false; }

    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "frequency_MHz");
      p.setDoubleValue(4000.000005); // rounded up to nearest Hz from 4GHz +4.768 Hz
    }
    {
      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "rf_gain_dB");
      try {
        p.setDoubleValue(63.);
        did_pass = false; // set to 63. should not have succeeded
      }
      catch(...) {
        did_pass = true;
      }
      TEST_EXPECTED_VAL(did_pass, true);
      p.setDoubleValue(62.);
    }
    did_pass = did_pass_test_expected_value_frequency_MHz(app, 4000.000001, (OA::ULongLong) 4000000001);
    if(!did_pass) { return false; }
    did_pass = did_pass_test_expected_value_rf_gain_dB(app, 62., (OA::Long) 62);
    if(!did_pass) { return false; }
 
    app.stop();
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_ocpi_app_max_value_bb_gain_dB()
{
  printf("TEST: max     value for bb_gain_dB\n");

  auto rx = APP_DEFAULT_XML_INST_NAME_RX;

  std::vector<std::string> apps;
  apps.push_back(APP_DEFAULT_FMCOMMS2_XML);
  apps.push_back(APP_DEFAULT_FMCOMMS3_XML);

  for(auto it = apps.begin(); it != apps.end(); ++it) {
    try
    {
      OA::Application app(*it, NULL);
      app.initialize();
      app.start();

      double max = app.getPropertyValue<double>(rx, "bb_gain_max_dB");

      // test for exception
      app.setPropertyValue<double>(rx, "bb_gain_dB", max);
    }
    catch (std::string &e)
    {
      fprintf(stderr, "Exception thrown: %s\n", e.c_str());
      return false;
    }
  }

  return did_pass_test_ocpi_app_default_value_bb_gain_dB();
}

bool did_pass_test_ocpi_app_max_value_frequency_MHz()
{
  printf("TEST: max     value for frequency_MHz\n");

  auto rx = APP_DEFAULT_XML_INST_NAME_RX;

  std::vector<std::string> apps;
  apps.push_back(APP_DEFAULT_FMCOMMS2_XML);
  apps.push_back(APP_DEFAULT_FMCOMMS3_XML);

  for(auto it = apps.begin(); it != apps.end(); ++it) {
    try
    {
      OA::Application app(*it, NULL);
      app.initialize();
      app.start();

      double max = app.getPropertyValue<double>(rx, "frequency_max_MHz");

      // test for exception
      app.setPropertyValue<double>(rx, "frequency_MHz", max);

      bool did_pass = false;
      if(*it == APP_DEFAULT_FMCOMMS2_XML) {
        did_pass = did_pass_test_expected_value_frequency_MHz(app, 2500., (OA::ULongLong) 2500000000);
      }
      else if(*it == APP_DEFAULT_FMCOMMS3_XML) {
        did_pass = did_pass_test_expected_value_frequency_MHz(app, 6000., (OA::ULongLong) 6000000000);
      }
      if(!did_pass) {
        return false;
      }
    }
    catch (std::string &e)
    {
      fprintf(stderr, "Exception thrown: %s\n", e.c_str());
      return false;
    }
  }

  return true;
}

bool did_pass_test_ocpi_app_max_value_sample_rate_MHz()
{
  printf("TEST: max     value for sample_rate_MHz\n");

  auto rx = APP_DEFAULT_XML_INST_NAME_RX;

  std::vector<std::string> apps;
  apps.push_back(APP_DEFAULT_FMCOMMS2_XML);
  apps.push_back(APP_DEFAULT_FMCOMMS3_XML);

  for(auto it = apps.begin(); it != apps.end(); ++it) {
    try
    {
      OA::Application app(*it, NULL);
      app.initialize();
      app.start();

      double max = app.getPropertyValue<double>(rx, "sample_rate_max_MHz");

      // test for exception
      app.setPropertyValue<double>(rx, "sample_rate_MHz", max);

      bool did_pass;
      did_pass = did_pass_test_expected_value_sample_rate_MHz(app, 61.44, (OA::ULong) 61440000);
      if(!did_pass) {
        return false;
      }
    }
    catch (std::string &e)
    {
      fprintf(stderr, "Exception thrown: %s\n", e.c_str());
      return false;
    }
  }

  return true;
}

bool did_pass_test_ocpi_app_max_value_rf_cutoff_frequency_MHz()
{
  printf("TEST: max     value for rf_cutoff_frequency_MHz\n");

  auto rx = APP_DEFAULT_XML_INST_NAME_RX;

  std::vector<std::string> apps;
  apps.push_back(APP_DEFAULT_FMCOMMS2_XML);
  apps.push_back(APP_DEFAULT_FMCOMMS3_XML);

  for(auto it = apps.begin(); it != apps.end(); ++it) {
    try
    {
      OA::Application app(*it, NULL);
      app.initialize();
      app.start();

      double max = app.getPropertyValue<double>(rx, "rf_cutoff_frequency_max_MHz");

      // test for exception
      app.setPropertyValue<double>(rx, "rf_cutoff_frequency_MHz", max);
    }
    catch (std::string &e)
    {
      fprintf(stderr, "Exception thrown: %s\n", e.c_str());
      return false;
    }
  }

  return did_pass_test_ocpi_app_default_value_rf_cutoff_frequency_MHz();
}

bool did_pass_test_ocpi_app_max_value_bb_cutoff_frequency_MHz()
{
  printf("TEST: max     value for bb_cutoff_frequency_MHz\n");

  auto rx = APP_DEFAULT_XML_INST_NAME_RX;

  std::vector<std::string> apps;
  apps.push_back(APP_DEFAULT_FMCOMMS2_XML);
  apps.push_back(APP_DEFAULT_FMCOMMS3_XML);

  for(auto it = apps.begin(); it != apps.end(); ++it) {
    try
    {
      OA::Application app(*it, NULL);
      app.initialize();
      app.start();

      double max = app.getPropertyValue<double>(rx, "bb_cutoff_frequency_max_MHz");

      // test for exception
      app.setPropertyValue<double>(rx, "bb_cutoff_frequency_MHz", max);

      bool did_pass;
      did_pass = did_pass_test_expected_value_bb_cutoff_frequency_MHz(app, 39.2, (OA::ULong) 39200000);
      if(!did_pass) {
        return false;
      }
    }
    catch (std::string &e)
    {
      fprintf(stderr, "Exception thrown: %s\n", e.c_str());
      return false;
    }
  }

  return true;
}

bool did_pass_test_ocpi_app_max_values()
{
  if(!did_pass_test_ocpi_app_max_value_rf_gain_dB())              { return false; }
  if(!did_pass_test_ocpi_app_max_value_bb_gain_dB())              { return false; }
  if(!did_pass_test_ocpi_app_max_value_frequency_MHz())           { return false; }
  if(!did_pass_test_ocpi_app_max_value_sample_rate_MHz())         { return false; }
  if(!did_pass_test_ocpi_app_max_value_rf_cutoff_frequency_MHz()) { return false; }
  if(!did_pass_test_ocpi_app_max_value_bb_cutoff_frequency_MHz()) { return false; }

  return true;
}

#endif // _TEST_APP_MAXES_H

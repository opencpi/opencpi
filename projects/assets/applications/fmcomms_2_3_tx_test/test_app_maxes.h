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

#include "OcpiApi.hh" // OCPI::API namespace

namespace OA = OCPI::API;

bool did_pass_test_ocpi_app_max_value_rf_gain_dB()
{
  printf("TEST: max     value for rf_gain_dB\n");

  auto tx = APP_DEFAULT_XML_INST_NAME_TX;

  std::vector<std::string> apps;
  apps.push_back(APP_DEFAULT_FMCOMMS2_XML);
  apps.push_back(APP_DEFAULT_FMCOMMS3_XML);

  for(auto it = apps.begin(); it != apps.end(); ++it) {
    try
    {
      OA::Application app(*it, NULL);
      app.initialize();
      app.start();

      app.setPropertyValue<double>(tx, "rf_gain_dB", 0);

      bool did_pass;
      did_pass = did_pass_test_expected_value_rf_gain_dB(app, 0, (OA::ULong) 0);
      if(!did_pass) {
        return false;
      }

      double max = app.getPropertyValue<double>(tx, "rf_gain_max_dB");

      // test for exception
      app.setPropertyValue<double>(tx, "rf_gain_dB", max);
    }
    catch (std::string &e)
    {
      fprintf(stderr, "Exception thrown: %s\n", e.c_str());
      return false;
    }
  }

  return true;
}

bool did_pass_test_ocpi_app_max_value_bb_gain_dB()
{
  printf("TEST: max     value for bb_gain_dB\n");

  auto tx = APP_DEFAULT_XML_INST_NAME_TX;

  std::vector<std::string> apps;
  apps.push_back(APP_DEFAULT_FMCOMMS2_XML);
  apps.push_back(APP_DEFAULT_FMCOMMS3_XML);

  for(auto it = apps.begin(); it != apps.end(); ++it) {
    try
    {
      OA::Application app(*it, NULL);
      app.initialize();
      app.start();

      double max = app.getPropertyValue<double>(tx, "bb_gain_max_dB");

      // test for exception
      app.setPropertyValue<double>(tx, "bb_gain_dB", max);
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

  auto tx = APP_DEFAULT_XML_INST_NAME_TX;

  std::vector<std::string> apps;
  apps.push_back(APP_DEFAULT_FMCOMMS2_XML);
  apps.push_back(APP_DEFAULT_FMCOMMS3_XML);

  for(auto it = apps.begin(); it != apps.end(); ++it) {
    try
    {
      OA::Application app(*it, NULL);
      app.initialize();
      app.start();

      double max = app.getPropertyValue<double>(tx, "frequency_max_MHz");

      // test for exception
      app.setPropertyValue<double>(tx, "frequency_MHz", max);

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

  auto tx = APP_DEFAULT_XML_INST_NAME_TX;

  std::vector<std::string> apps;
  apps.push_back(APP_DEFAULT_FMCOMMS2_XML);
  apps.push_back(APP_DEFAULT_FMCOMMS3_XML);

  for(auto it = apps.begin(); it != apps.end(); ++it) {
    try
    {
      OA::Application app(*it, NULL);
      app.initialize();
      app.start();

      app.setPropertyValue<double>(tx, "sample_rate_MHz", 61.44);

      bool did_pass;
      did_pass = did_pass_test_expected_value_sample_rate_MHz(app, 61.44, (OA::ULong) 61440000);
      if(!did_pass) {
        return false;
      }

      double max = app.getPropertyValue<double>(tx, "sample_rate_max_MHz");

      // test for exception
      app.setPropertyValue<double>(tx, "sample_rate_MHz", max);
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

  auto tx = APP_DEFAULT_XML_INST_NAME_TX;

  std::vector<std::string> apps;
  apps.push_back(APP_DEFAULT_FMCOMMS2_XML);
  apps.push_back(APP_DEFAULT_FMCOMMS3_XML);

  for(auto it = apps.begin(); it != apps.end(); ++it) {
    try
    {
      OA::Application app(*it, NULL);
      app.initialize();
      app.start();

      double max = app.getPropertyValue<double>(tx, "rf_cutoff_frequency_max_MHz");

      // test for exception
      app.setPropertyValue<double>(tx, "rf_cutoff_frequency_MHz", max);
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

  auto tx = APP_DEFAULT_XML_INST_NAME_TX;

  std::vector<std::string> apps;
  apps.push_back(APP_DEFAULT_FMCOMMS2_XML);
  apps.push_back(APP_DEFAULT_FMCOMMS3_XML);

  for(auto it = apps.begin(); it != apps.end(); ++it) {
    try
    {
      OA::Application app(*it, NULL);
      app.initialize();
      app.start();

      app.setPropertyValue<double>(tx, "bb_cutoff_frequency_MHz", 12.5);

      bool did_pass;
      did_pass = did_pass_test_expected_value_bb_cutoff_frequency_MHz(app, 12.5, (OA::ULong) 12500000);
      if(!did_pass) {
        return false;
      }

      double max = app.getPropertyValue<double>(tx, "bb_cutoff_frequency_max_MHz");

      // test for exception
      app.setPropertyValue<double>(tx, "bb_cutoff_frequency_MHz", max);
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

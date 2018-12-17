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

#ifndef _TEST_APP_DEFAULTS_H
#define _TEST_APP_DEFAULTS_H

#include <string>     // std::string
#include "OcpiApi.hh" // OA namespace
#include "test_app_common.h"// APP_DEFAULT_... macros, did_pass_test_expected_value_<prop>() functions

namespace OA = OCPI::API;

bool did_pass_test_ocpi_app_default_value_rf_gain_dB()
{
  printf("TEST: default value for rf_gain_dB\n");
  bool did_pass;
  try
  {
    {
      OA::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();
      app.stop();
      did_pass = did_pass_test_expected_value_rf_gain_dB(app, 1., (OA::Long) 1);
      if(!did_pass) { return false; }
    }
    
    {
      OA::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();
      app.stop();
      did_pass = did_pass_test_expected_value_rf_gain_dB(app, 1., (OA::Long) 1);
      if(!did_pass) { return false; }
    }
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_ocpi_app_default_value_bb_gain_dB()
{
  printf("TEST: default value for bb_gain_dB\n");
  bool did_pass;
  try
  {
    {
      OA::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();
      app.stop();
      did_pass = did_pass_test_expected_value_bb_gain_dB(app, -1.);
      if(!did_pass) { return false; }
    }

    {
      if(!did_pass) { return false; }
      OA::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();
      app.stop();
      did_pass = did_pass_test_expected_value_bb_gain_dB(app, -1.);
      if(!did_pass) { return false; }
    }
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_ocpi_app_default_value_frequency_MHz()
{
  printf("TEST: default value for frequency_MHz\n");
  bool did_pass;
  try
  {
    {
      OA::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();
      app.stop();
      did_pass = did_pass_test_expected_value_frequency_MHz(app, 2400., (OA::ULongLong) 2400000000);
      if(!did_pass) { return false; }
    }

    {
      OA::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();
      app.stop();
      did_pass = did_pass_test_expected_value_frequency_MHz(app, 2400., (OA::ULongLong) 2400000000);
      if(!did_pass) { return false; }
    }
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_ocpi_app_default_value_sample_rate_MHz()
{
  printf("TEST: default value for sample_rate_MHz\n");
  bool did_pass;
  try
  {
    {
      OA::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();
      app.stop();
      did_pass = did_pass_test_expected_value_sample_rate_MHz(app, 30.72, (OA::ULong) 30720000);
      if(!did_pass) { return false; }
    }

    {
      OA::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();
      app.stop();
      did_pass = did_pass_test_expected_value_sample_rate_MHz(app, 30.72, (OA::ULong) 30720000);
      if(!did_pass) { return false; }
    }
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_ocpi_app_default_value_rf_cutoff_frequency_MHz()
{
  printf("TEST: default value for rf_cutoff_frequency_MHz\n");
  bool did_pass;
  try
  {
    {
      OA::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();
      app.stop();
      did_pass = did_pass_test_expected_value_rf_cutoff_frequency_MHz(app, -1.);
      if(!did_pass) { return false; }
    }

    {
      OA::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();
      app.stop();
      did_pass = did_pass_test_expected_value_rf_cutoff_frequency_MHz(app, -1.);
      if(!did_pass) { return false; }
    }
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_ocpi_app_default_value_bb_cutoff_frequency_MHz()
{
  printf("TEST: default value for bb_cutoff_frequency_MHz\n");
  bool did_pass;
  try
  {
    {
      OA::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();
      app.stop();
      did_pass = did_pass_test_expected_value_bb_cutoff_frequency_MHz(app, 18., (OA::ULong) 18000000);
      if(!did_pass) { return false; }
    }

    {
      OA::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();
      app.stop();
      did_pass = did_pass_test_expected_value_bb_cutoff_frequency_MHz(app, 18., (OA::ULong) 18000000);
      if(!did_pass) { return false; }
    }
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_ocpi_app_default_values()
{
  if(!did_pass_test_ocpi_app_default_value_rf_gain_dB())              { return false; }
  if(!did_pass_test_ocpi_app_default_value_bb_gain_dB())              { return false; }
  if(!did_pass_test_ocpi_app_default_value_frequency_MHz())           { return false; }
  if(!did_pass_test_ocpi_app_default_value_sample_rate_MHz())         { return false; }
  if(!did_pass_test_ocpi_app_default_value_rf_cutoff_frequency_MHz()) { return false; }
  if(!did_pass_test_ocpi_app_default_value_bb_cutoff_frequency_MHz()) { return false; }

  return true;
}

#endif // _TEST_APP_DEFAULTS_H

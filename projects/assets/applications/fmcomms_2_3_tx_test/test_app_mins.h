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

#ifndef _TEST_APP_MINS_H
#define _TEST_APP_MINS_H

#include <sstream> // std::ostringstream
#include <string> // std::string>

#include "OcpiApi.hh" // OCPI::API namespace

namespace OA = OCPI::API;

bool did_pass_test_ocpi_app_min_value_rf_gain_dB()
{
  printf("TEST: min     value for rf_gain_dB\n");

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

      app.setPropertyValue<double>(tx, "rf_gain_dB", -89.75);

      bool did_pass;
      did_pass = did_pass_test_expected_value_rf_gain_dB(app, -89.75, (OA::ULong) 89750);
      if(!did_pass) {
        return false;
      }

      double min = app.getPropertyValue<double>(tx, "rf_gain_min_dB");

      // test for exception
      app.setPropertyValue<double>(tx, "rf_gain_dB", min);
    }
    catch (std::string &e)
    {
      fprintf(stderr, "Exception thrown: %s\n", e.c_str());
      return false;
    }
  }

  return true;
}

bool did_pass_test_ocpi_app_min_value_bb_gain_dB()
{
  printf("TEST: min     value for bb_gain_dB\n");

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

      double min = app.getPropertyValue<double>(tx, "bb_gain_min_dB");

      // test for exception
      app.setPropertyValue<double>(tx, "bb_gain_dB", min);
    }
    catch (std::string &e)
    {
      fprintf(stderr, "Exception thrown: %s\n", e.c_str());
      return false;
    }
  }

  return did_pass_test_ocpi_app_default_value_bb_gain_dB();
}

bool did_pass_test_ocpi_app_min_value_frequency_MHz()
{
  printf("TEST: min     value for frequency_MHz\n");
  bool did_pass;
  try
  {
    {
      OA::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();

      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, "frequency_MHz");
      p.setDoubleValue(2400.);

      did_pass = did_pass_test_expected_value_frequency_MHz(app, 2400., (OA::ULongLong) 2400000000);
      if(!did_pass) { return false; }

      {
      OA::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "frequency_min_MHz");
      double min = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << min;
      std::string tmp_str(oss.str());
      app.setProperty("tx", "frequency_MHz", tmp_str.c_str()); // test for exception
      }
   
      app.stop();
    }

    {
      OA::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();

      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, "frequency_MHz");
      p.setDoubleValue(70.);

      did_pass = did_pass_test_expected_value_frequency_MHz(app, 70., (OA::ULongLong) 70000000);
      if(!did_pass) { return false; }

      {
      OA::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "frequency_min_MHz");
      double min = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << min;
      std::string tmp_str(oss.str());
      app.setProperty("tx", "frequency_MHz", tmp_str.c_str()); // test for exception
      }
   
      app.stop();
    }
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_ocpi_app_min_value_sample_rate_MHz()
{
  printf("TEST: min     value for sample_rate_MHz\n");
  bool did_pass;
  try
  {
    {
      OA::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();

      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, "sample_rate_MHz");

      //! @todo TODO/FIXME comment back on once bug is fixed where framework doesn't remove precision
      //p.setDoubleValue(2.083334); // assumes FIR is disabled

      // p.setDoubleValue(2.08334) will fail with error:
      // Worker tx produced error during execution: sample_rate_MHz too low ("2.0833399999999989") can only be in the range [ 2.0833399999999993 to 61.439999999999998 ]
      // due to double floating point rounding
      //
      // ... so this value is used instead: 2.0833400000000001917044301080750301480293273925781250
      // double x = MIN_SAMP_RATE_FIR_DISABLED_MHZ;
      // std::cout << "x=" << std::hex << *(uint64_t*)&x << "\n";
      // output:
      // x=4000aaae297396d1
      // compare against: https://www.binaryconvert.com/convert_double.html?decimal=050046048056051051052
      // /r/theydidthemath
#define MIN_SAMP_RATE_FIR_DISABLED_MHZ 2.0833400000000001917044301080750301480293273925781250

      p.setDoubleValue(MIN_SAMP_RATE_FIR_DISABLED_MHZ);

      //! @todo TODO/FIXME comment back on once bug is fixed where framework doesn't remove precision
      //did_pass = did_pass_test_expected_value_sample_rate_MHz(app, 2.083334, (OA::ULong) 2083334); // assumes FIR is disabled
      did_pass = did_pass_test_expected_value_sample_rate_MHz(app, 2.08334, (OA::ULong) 2083340); // assumes FIR is disabled
      if(!did_pass) { return false; }

      {
      OA::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "sample_rate_min_MHz");
      double min = pp.getValue<double>();

      // test for exception
      app.setPropertyValue<double>("tx", "sample_rate_MHz", min);
      }
   
      app.stop();
    }

    {
      OA::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();

      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, "sample_rate_MHz");

      //! @todo TODO/FIXME comment back on once bug is fixed where framework doesn't remove precision
      //p.setDoubleValue(2.083334);

      p.setDoubleValue(MIN_SAMP_RATE_FIR_DISABLED_MHZ);

      //! @todo TODO/FIXME comment back on once bug is fixed where framework doesn't remove precision
      //did_pass = did_pass_test_expected_value_sample_rate_MHz(app, 2.083334, (OA::ULong) 2083334);
      did_pass = did_pass_test_expected_value_sample_rate_MHz(app, 2.08334, (OA::ULong) 2083340);
      if(!did_pass) { return false; }

      {
      OA::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "sample_rate_min_MHz");
      double min = pp.getValue<double>();

      // test for exception
      app.setPropertyValue<double>("tx", "sample_rate_MHz", min);
      }
   
      app.stop();
    }
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_ocpi_app_min_value_rf_cutoff_frequency_MHz()
{
  printf("TEST: min     value for rf_cutoff_frequency_MHz\n");

  try
  {
  OA::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
  app.initialize();
  app.start();
  OA::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "rf_cutoff_frequency_min_MHz");
  double min = pp.getValue<double>();

  std::ostringstream oss;
  oss << std::setprecision(17) << min;
  std::string tmp_str(oss.str());
  app.setProperty("tx", "rf_cutoff_frequency_MHz", tmp_str.c_str()); // test for exception
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }

  try
  {
  OA::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
  app.initialize();
  app.start();
  OA::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "rf_cutoff_frequency_min_MHz");
  double min = pp.getValue<double>();

  std::ostringstream oss;
  oss << std::setprecision(17) << min;
  std::string tmp_str(oss.str());
  app.setProperty("tx", "rf_cutoff_frequency_MHz", tmp_str.c_str()); // test for exception
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }

  return did_pass_test_ocpi_app_default_value_rf_cutoff_frequency_MHz();
}

bool did_pass_test_ocpi_app_min_value_bb_cutoff_frequency_MHz()
{
  printf("TEST: min     value for bb_cutoff_frequency_MHz\n");
  bool did_pass;
  try
  {
    {
      OA::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();

      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, "bb_cutoff_frequency_MHz");
      p.setDoubleValue(1.);

      did_pass = did_pass_test_expected_value_bb_cutoff_frequency_MHz(app, 1., (OA::ULong) 1000000);
      if(!did_pass) { return false; }

      {
      OA::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "bb_cutoff_frequency_min_MHz");
      double min = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << min;
      std::string tmp_str(oss.str());
      app.setProperty("tx", "bb_cutoff_frequency_MHz", tmp_str.c_str()); // test for exception
      }
   
      app.stop();
    }

    {
      OA::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();

      OA::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, "bb_cutoff_frequency_MHz");
      p.setDoubleValue(1.);

      did_pass = did_pass_test_expected_value_bb_cutoff_frequency_MHz(app, 1., (OA::ULong) 1000000);
      if(!did_pass) { return false; }

      {
      OA::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "bb_cutoff_frequency_min_MHz");
      double min = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << min;
      std::string tmp_str(oss.str());
      app.setProperty("tx", "bb_cutoff_frequency_MHz", tmp_str.c_str()); // test for exception
      }
   
      app.stop();
    }
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  return true;
}

bool did_pass_test_ocpi_app_min_values()
{
  if(!did_pass_test_ocpi_app_min_value_rf_gain_dB())              { return false; }
  if(!did_pass_test_ocpi_app_min_value_bb_gain_dB())              { return false; }
  if(!did_pass_test_ocpi_app_min_value_frequency_MHz())           { return false; }
  if(!did_pass_test_ocpi_app_min_value_sample_rate_MHz())         { return false; }
  if(!did_pass_test_ocpi_app_min_value_rf_cutoff_frequency_MHz()) { return false; }
  if(!did_pass_test_ocpi_app_min_value_bb_cutoff_frequency_MHz()) { return false; }

  return true;
}

#endif // _TEST_APP_MINS_H

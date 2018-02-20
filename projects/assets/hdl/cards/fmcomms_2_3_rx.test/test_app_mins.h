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

#include "OcpiApi.hh" // OCPI::API namespace
#include "ocpi_component_prop_type_helpers.h" // ocpi_..._t types
#include "test_app_common.h"// APP_DEFAULT_... macros, did_pass_test_expected_value_<prop>() functions

bool did_pass_test_ocpi_app_min_value_rf_gain_dB()
{
  printf("TEST: min     value for rf_gain_dB\n");
  bool did_pass;
  try
  {
    OCPI::API::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
    app.initialize();
    app.start();

    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "frequency_MHz");
      p.setDoubleValue(2400.);
      OCPI::API::Property p2(app, APP_DEFAULT_XML_INST_NAME_RX, "rf_gain_dB");
      p2.setDoubleValue(-4.);
    }
    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "rf_gain_dB");
      p.setDoubleValue(-4.);
    }
    did_pass = did_pass_test_expected_value_frequency_MHz(app, 2400., (ocpi_ulonglong_t) 2400000000);
    if(!did_pass) { return false; }
    did_pass = did_pass_test_expected_value_rf_gain_dB(app, -4., (ocpi_long_t) -4);
    if(!did_pass) { return false; }

    app.stop();
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }
  try
  {
    OCPI::API::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
    app.initialize();
    //app.setProperty(APP_DEFAULT_XML_INST_NAME_RX, "enable_log_info",  "true");
    //app.setProperty(APP_DEFAULT_XML_INST_NAME_RX, "enable_log_trace", "true");
    //app.setProperty(APP_DEFAULT_XML_INST_NAME_RX, "enable_log_debug", "true");
    app.start();

    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "frequency_MHz");
      p.setDoubleValue(1299.999999);
      OCPI::API::Property p2(app, APP_DEFAULT_XML_INST_NAME_RX, "rf_gain_dB");
      p2.setDoubleValue(1.);
    }
    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "rf_gain_dB");
      p.setDoubleValue(1.);
    }
    did_pass = did_pass_test_expected_value_frequency_MHz(app, 1299.999999, (ocpi_ulonglong_t) 1299999999);
    if(!did_pass) { return false; }
    did_pass = did_pass_test_expected_value_rf_gain_dB(app, 1., (ocpi_long_t) 1);
    if(!did_pass) { return false; }

    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "frequency_MHz");
      p.setDoubleValue(2400.);
    }
    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "rf_gain_dB");
      p.setDoubleValue(-4.);
    }
    did_pass = did_pass_test_expected_value_frequency_MHz(app, 2400., (ocpi_ulonglong_t) 2400000000);
    if(!did_pass) { return false; }
    did_pass = did_pass_test_expected_value_rf_gain_dB(app, -4., (ocpi_long_t) -4);
    if(!did_pass) { return false; }

    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "frequency_MHz");
      p.setDoubleValue(4000.000005); // rounded up to nearest Hz from 4GHz +4.768 Hz
    }
    {
      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "rf_gain_dB");
      p.setDoubleValue(-10.);
    }
    did_pass = did_pass_test_expected_value_frequency_MHz(app, 4000.000001, (ocpi_ulonglong_t) 4000000001);
    if(!did_pass) { return false; }
    did_pass = did_pass_test_expected_value_rf_gain_dB(app, -10., (ocpi_long_t) -10);
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

bool did_pass_test_ocpi_app_min_value_bb_gain_dB()
{
  printf("TEST: min     value for bb_gain_dB\n");

  try
  {
  OCPI::API::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
  app.initialize();
  app.start();
  OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_RX, "bb_gain_min_dB");
  double min = pp.getValue<double>();

  std::ostringstream oss;
  oss << std::setprecision(17) << min;
  app.setProperty("rx", "bb_gain_dB", oss.str().c_str()); // test for exception
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }

  try
  {
  OCPI::API::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
  app.initialize();
  app.start();
  OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_RX, "bb_gain_min_dB");
  double min = pp.getValue<double>();

  std::ostringstream oss;
  oss << std::setprecision(17) << min;
  app.setProperty("rx", "bb_gain_dB", oss.str().c_str()); // test for exception
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
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
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();

      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "frequency_MHz");
      p.setDoubleValue(2400.);

      did_pass = did_pass_test_expected_value_frequency_MHz(app, 2400., (ocpi_ulonglong_t) 2400000000);
      if(!did_pass) { return false; }

      {
      OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_RX, "frequency_min_MHz");
      double min = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << min;
      app.setProperty("rx", "frequency_MHz", oss.str().c_str()); // test for exception
      }
   
      app.stop();
    }

    {
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();

      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "frequency_MHz");
      p.setDoubleValue(70.);

      did_pass = did_pass_test_expected_value_frequency_MHz(app, 70., (ocpi_ulonglong_t) 70000000);
      if(!did_pass) { return false; }

      {
      OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_RX, "frequency_min_MHz");
      double min = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << min;
      app.setProperty("rx", "frequency_MHz", oss.str().c_str()); // test for exception
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
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();

      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "sample_rate_MHz");

      //! @todo TODO/FIXME comment back on once bug is fixed where framework doesn't remove precision
      //p.setDoubleValue(2.083334); // assumes FIR is disabled

      p.setDoubleValue(2.08334); // assumes FIR is disabled

      //! @todo TODO/FIXME comment back on once bug is fixed where framework doesn't remove precision
      //did_pass = did_pass_test_expected_value_sample_rate_MHz(app, 2.083334, (ocpi_ulong_t) 2083334); // assumes FIR is disabled
      did_pass = did_pass_test_expected_value_sample_rate_MHz(app, 2.08334, (ocpi_ulong_t) 2083340); // assumes FIR is disabled
      if(!did_pass) { return false; }

      {
      OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_RX, "sample_rate_min_MHz");
      double min = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << min;
      app.setProperty("rx", "sample_rate_MHz", oss.str().c_str()); // test for exception
      }
   
      app.stop();
    }

    {
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();

      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "sample_rate_MHz");

      //! @todo TODO/FIXME comment back on once bug is fixed where framework doesn't remove precision
      //p.setDoubleValue(2.083334);

      p.setDoubleValue(2.08334);

      //! @todo TODO/FIXME comment back on once bug is fixed where framework doesn't remove precision
      //did_pass = did_pass_test_expected_value_sample_rate_MHz(app, 2.083334, (ocpi_ulong_t) 2083334);
      did_pass = did_pass_test_expected_value_sample_rate_MHz(app, 2.08334, (ocpi_ulong_t) 2083340);
      if(!did_pass) { return false; }

      {
      OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_RX, "sample_rate_min_MHz");
      double min = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << min;
      app.setProperty("rx", "sample_rate_MHz", oss.str().c_str()); // test for exception
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
  OCPI::API::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
  app.initialize();
  app.start();
  OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_RX, "rf_cutoff_frequency_min_MHz");
  double min = pp.getValue<double>();

  std::ostringstream oss;
  oss << std::setprecision(17) << min;
  app.setProperty("rx", "rf_cutoff_frequency_MHz", oss.str().c_str()); // test for exception
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }

  try
  {
  OCPI::API::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
  app.initialize();
  app.start();
  OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_RX, "rf_cutoff_frequency_min_MHz");
  double min = pp.getValue<double>();

  std::ostringstream oss;
  oss << std::setprecision(17) << min;
  app.setProperty("rx", "rf_cutoff_frequency_MHz", oss.str().c_str()); // test for exception
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
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();

      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "bb_cutoff_frequency_MHz");
      p.setDoubleValue(2.8);

      did_pass = did_pass_test_expected_value_bb_cutoff_frequency_MHz(app, 2.8, (ocpi_ulong_t) 2800000);
      if(!did_pass) { return false; }

      {
      OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_RX, "bb_cutoff_frequency_min_MHz");
      double min = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << min;
      app.setProperty("rx", "bb_cutoff_frequency_MHz", oss.str().c_str()); // test for exception
      }
   
      app.stop();
    }

    {
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();

      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_RX, "bb_cutoff_frequency_MHz");
      p.setDoubleValue(2.8);

      did_pass = did_pass_test_expected_value_bb_cutoff_frequency_MHz(app, 2.8, (ocpi_ulong_t) 2800000);
      if(!did_pass) { return false; }
 
      {
      OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_RX, "bb_cutoff_frequency_min_MHz");
      double min = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << min;
      app.setProperty("rx", "bb_cutoff_frequency_MHz", oss.str().c_str()); // test for exception
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

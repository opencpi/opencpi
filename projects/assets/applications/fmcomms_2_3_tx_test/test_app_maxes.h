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

bool did_pass_test_ocpi_app_max_value_rf_gain_dB()
{
  printf("TEST: max     value for rf_gain_dB\n");
  bool did_pass;
  try
  {
    {
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();

      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, "rf_gain_dB");
      p.setDoubleValue(0);

      did_pass = did_pass_test_expected_value_rf_gain_dB(app, 0, (ocpi_ulong_t) 0);
      if(!did_pass) { return false; }

      {
      OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "rf_gain_max_dB");
      double max = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << max;
      app.setProperty("tx", "rf_gain_dB", oss.str().c_str()); // test for exception
      }

      app.stop();
    }

    {
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();

      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, "rf_gain_dB");
      p.setDoubleValue(0);

      did_pass = did_pass_test_expected_value_rf_gain_dB(app, 0, (ocpi_ulong_t) 0);
      if(!did_pass) { return false; }

      {
      OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "rf_gain_max_dB");
      double max = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << max;
      app.setProperty("tx", "rf_gain_dB", oss.str().c_str()); // test for exception
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

bool did_pass_test_ocpi_app_max_value_bb_gain_dB()
{
  printf("TEST: max     value for bb_gain_dB\n");

  try
  {
  OCPI::API::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
  app.initialize();
  app.start();
  OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "bb_gain_max_dB");
  double max = pp.getValue<double>();

  std::ostringstream oss;
  oss << std::setprecision(17) << max;
  app.setProperty("tx", "bb_gain_dB", oss.str().c_str()); // test for exception
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
  OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "bb_gain_max_dB");
  double max = pp.getValue<double>();

  std::ostringstream oss;
  oss << std::setprecision(17) << max;
  app.setProperty("tx", "bb_gain_dB", oss.str().c_str()); // test for exception
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }

  return did_pass_test_ocpi_app_default_value_bb_gain_dB();
}

bool did_pass_test_ocpi_app_max_value_frequency_MHz()
{
  printf("TEST: max     value for frequency_MHz\n");
  bool did_pass;
  try
  {
    {
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();

      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, "frequency_MHz");
      p.setDoubleValue(2500.);

      did_pass = did_pass_test_expected_value_frequency_MHz(app, 2500., (ocpi_ulonglong_t) 2500000000);
      if(!did_pass) { return false; }

      {
      OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "frequency_max_MHz");
      double max = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << max;
      app.setProperty("tx", "frequency_MHz", oss.str().c_str()); // test for exception
      }
   
      app.stop();
    }

    {
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();

      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, "frequency_MHz");
      p.setDoubleValue(6000.);

      did_pass = did_pass_test_expected_value_frequency_MHz(app, 6000., (ocpi_ulonglong_t) 6000000000);
      if(!did_pass) { return false; }
 
      {
      OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "frequency_max_MHz");
      double max = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << max;
      app.setProperty("tx", "frequency_MHz", oss.str().c_str()); // test for exception
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

bool did_pass_test_ocpi_app_max_value_sample_rate_MHz()
{
  printf("TEST: max     value for sample_rate_MHz\n");
  bool did_pass;
  try
  {
    {
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();

      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, "sample_rate_MHz");
      p.setDoubleValue(61.44);

      did_pass = did_pass_test_expected_value_sample_rate_MHz(app, 61.44, (ocpi_ulong_t) 61440000);
      if(!did_pass) { return false; }
 
      {
      OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "sample_rate_max_MHz");
      double max = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << max;
      app.setProperty("tx", "sample_rate_MHz", oss.str().c_str()); // test for exception
      }
 
      app.stop();
    }

    {
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();

      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, "sample_rate_MHz");
      p.setDoubleValue(61.44);

      did_pass = did_pass_test_expected_value_sample_rate_MHz(app, 61.44, (ocpi_ulong_t) 61440000);
      if(!did_pass) { return false; }
  
      {
      OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "sample_rate_max_MHz");
      double max = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << max;
      app.setProperty("tx", "sample_rate_MHz", oss.str().c_str()); // test for exception
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

bool did_pass_test_ocpi_app_max_value_rf_cutoff_frequency_MHz()
{
  printf("TEST: max     value for rf_cutoff_frequency_MHz\n");

  try
  {
  OCPI::API::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
  app.initialize();
  app.start();
  OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "rf_cutoff_frequency_max_MHz");
  double max = pp.getValue<double>();

  std::ostringstream oss;
  oss << std::setprecision(17) << max;
  app.setProperty("tx", "rf_cutoff_frequency_MHz", oss.str().c_str()); // test for exception
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
  OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "rf_cutoff_frequency_max_MHz");
  double max = pp.getValue<double>();

  std::ostringstream oss;
  oss << std::setprecision(17) << max;
  app.setProperty("tx", "rf_cutoff_frequency_MHz", oss.str().c_str()); // test for exception
  }
  catch (std::string &e)
  {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }

  return did_pass_test_ocpi_app_default_value_rf_cutoff_frequency_MHz();
}

bool did_pass_test_ocpi_app_max_value_bb_cutoff_frequency_MHz()
{
  printf("TEST: max     value for bb_cutoff_frequency_MHz\n");
  bool did_pass;
  try
  {
    {
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS2_XML, NULL);
      app.initialize();
      app.start();

      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, "bb_cutoff_frequency_MHz");
      p.setDoubleValue(12.5);

      did_pass = did_pass_test_expected_value_bb_cutoff_frequency_MHz(app, 12.5, (ocpi_ulong_t) 12500000);
      if(!did_pass) { return false; }
  
      {
      OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "bb_cutoff_frequency_max_MHz");
      double max = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << max;
      app.setProperty("tx", "bb_cutoff_frequency_MHz", oss.str().c_str()); // test for exception
      }
   
      app.stop();
    }

    {
      OCPI::API::Application app(APP_DEFAULT_FMCOMMS3_XML, NULL);
      app.initialize();
      app.start();

      OCPI::API::Property p(app, APP_DEFAULT_XML_INST_NAME_TX, "bb_cutoff_frequency_MHz");
      p.setDoubleValue(12.5);

      did_pass = did_pass_test_expected_value_bb_cutoff_frequency_MHz(app, 12.5, (ocpi_ulong_t) 12500000);
      if(!did_pass) { return false; }
  
      {
      OCPI::API::Property pp(app, APP_DEFAULT_XML_INST_NAME_TX, "bb_cutoff_frequency_max_MHz");
      double max = pp.getValue<double>();

      std::ostringstream oss;
      oss << std::setprecision(17) << max;
      app.setProperty("tx", "bb_cutoff_frequency_MHz", oss.str().c_str()); // test for exception
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

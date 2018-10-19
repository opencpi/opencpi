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

#ifndef _TEST_APP_RX_VALUE_CORRUPTION_H
#define _TEST_APP_RX_VALUE_CORRUPTION_H

#include "OcpiApi.hh" // OCPI::API namespace
#include "ocpi_component_prop_type_helpers.h" // ocpi_..._t types

#define APP_RX_CORRUPTION_FMCOMMS2_XML "app_rx_corruption_fmcomms2.xml"
#define APP_RX_CORRUPTION_FMCOMMS3_XML "app_rx_corruption_fmcomms3.xml"

bool did_pass_test_ocpi_app_rx_corruption_rx_rf_gain_dB()
{
  printf("TEST: ensure no corruption of rx rf_gain_dB\n");
  try
  {
    OCPI::API::Application app(APP_RX_CORRUPTION_FMCOMMS2_XML, NULL);
    app.initialize();
    app.start();
    if(!did_pass_test_expected_value_rx_rf_gain_dB(app, 13., (ocpi_long_t) 13)) { return false; }
    app.stop();
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }

  try
  {
    OCPI::API::Application app(APP_RX_CORRUPTION_FMCOMMS3_XML, NULL);
    app.initialize();
    app.start();
    if(!did_pass_test_expected_value_rx_rf_gain_dB(app, 13., (ocpi_long_t) 13)) { return false; }
    app.stop();
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }

  return true;
}

bool did_pass_test_ocpi_app_rx_corruption_rx_frequency_MHz()
{
  printf("TEST: ensure no corruption of rx frequency_MHz\n");
  try
  {
    OCPI::API::Application app(APP_RX_CORRUPTION_FMCOMMS2_XML, NULL);
    app.initialize();
    app.start();
    if(!did_pass_test_expected_value_rx_frequency_MHz(app, 2468.123, (ocpi_ulonglong_t) 2468123000)) { return false; }
    app.stop();
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }

  try
  {
    OCPI::API::Application app(APP_RX_CORRUPTION_FMCOMMS3_XML, NULL);
    app.initialize();
    app.start();
    if(!did_pass_test_expected_value_rx_frequency_MHz(app, 2468.123, (ocpi_ulonglong_t) 2468123000)) { return false; }
    app.stop();
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }

  return true;
}

bool did_pass_test_ocpi_app_rx_corruption_rx_bb_cutoff_frequency_MHz()
{
  printf("TEST: ensure no corruption of rx bb_cutoff_frequency_MHz\n");
  try
  {
    OCPI::API::Application app(APP_RX_CORRUPTION_FMCOMMS2_XML, NULL);
    app.initialize();
    app.start();
    if(!did_pass_test_expected_value_rx_bb_cutoff_frequency_MHz(app, 1.234567, (ocpi_ulong_t) 1234567)) { return false; }
    app.stop();
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }

  try
  {
    OCPI::API::Application app(APP_RX_CORRUPTION_FMCOMMS3_XML, NULL);
    app.initialize();
    app.start();
    if(!did_pass_test_expected_value_rx_bb_cutoff_frequency_MHz(app, 1.234567, (ocpi_ulong_t) 1234567)) { return false; }
    app.stop();
  }
  catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return false;
  }

  return true;

}

bool did_pass_test_ocpi_app_rx_value_corruption()
{
  if(!did_pass_test_ocpi_app_rx_corruption_rx_rf_gain_dB())             { return false; }
  // bb_gain_dB is unused for the fmcomms_2_3_rx.rcc worker
  if(!did_pass_test_ocpi_app_rx_corruption_rx_frequency_MHz())          { return false; }
  // we do not test rx_sample_rate_MHz because the ocpi.core.tx worker's sample_rate_MHz's value will always override the ocpi.core.rx worker's sample_rate_MHz property value (this is not a shortcoming of any worker but rather the very nature of how the AD961 works)
  // bb_cutoff_frequency_MHz is unused for the fmcomms_2_3_rx.rcc worker
  if(!did_pass_test_ocpi_app_rx_corruption_rx_bb_cutoff_frequency_MHz()) { return false; }

  return true;
}

#endif // _TEST_APP_RX_VALUE_CORRUPTION_H

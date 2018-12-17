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

#ifndef _TEST_PARSERS_H
#define _TEST_PARSERS_H

#include <iostream> // std::cerr
#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE
#include <string>  // std::string
#include <vector>  // std::vector

#include "OcpiApi.hh" // OCPI::API namespace
#include "test_helpers.h"                     // TEST_EXPECTED_VAL()
#include "worker_prop_parsers_ad9361_config_proxy.h" // parse()

namespace OA = OCPI::API;

bool did_pass_test_parsing_array_uchars()
{
  // example string: "1,2,122,9"

  std::string array_uchars_str("1,2,122,9");
  std::cout << "TEST: parsing string '" << array_uchars_str << "\n";

  std::vector<OA::UChar> array_uchars;

  try {
    parse(array_uchars_str.c_str(), array_uchars);
  }
  catch(std::string& e) {
    std::cerr << "ERROR: " << e << "\n";
    return false;
  }
  catch(...) {
    std::cerr << "ERROR: unknown exception\n";
    return false;
  }

  TEST_EXPECTED_VAL(array_uchars.size(),(size_t)    4  );
  TEST_EXPECTED_VAL(array_uchars[0],    (OA::UChar) 1  );
  TEST_EXPECTED_VAL(array_uchars[1],    (OA::UChar) 2  );
  TEST_EXPECTED_VAL(array_uchars[2],    (OA::UChar) 122);
  TEST_EXPECTED_VAL(array_uchars[3],    (OA::UChar) 9  );

  return true;
}

bool did_pass_test_parsing_array_longs()
{
  // example string: "1,-2,122,-2943"

  std::string array_longs_str("1,-2,122,-2943");
  std::cout << "TEST: parsing string '" << array_longs_str << "\n";

  std::vector<OA::Long> array_longs;

  try {
    parse(array_longs_str.c_str(), array_longs);
  }
  catch(std::string& e) {
    std::cerr << "ERROR: " << e << "\n";
    return false;
  }
  catch(...) {
    std::cerr << "ERROR: unknown exception\n";
    return false;
  }

  TEST_EXPECTED_VAL(array_longs.size(),(size_t)   4    );
  TEST_EXPECTED_VAL(array_longs[0],    (OA::Long) 1    );
  TEST_EXPECTED_VAL(array_longs[1],    (OA::Long) -2   );
  TEST_EXPECTED_VAL(array_longs[2],    (OA::Long) 122  );
  TEST_EXPECTED_VAL(array_longs[3],    (OA::Long) -2943);

  return true;
}

bool did_pass_test_parsing_array_ulongs()
{
  // example string: "1,2,122,2943"

  std::string array_ulongs_str("1,2,122,2943");
  std::cout << "TEST: parsing string '" << array_ulongs_str << "\n";

  std::vector<OA::ULong> array_ulongs;

  try {
    parse(array_ulongs_str.c_str(), array_ulongs);
  }
  catch(std::string& e) {
    std::cerr << "ERROR: " << e << "\n";
    return false;
  }
  catch(...) {
    std::cerr << "ERROR: unknown exception\n";
    return false;
  }

  TEST_EXPECTED_VAL(array_ulongs.size(),(size_t)    4   );
  TEST_EXPECTED_VAL(array_ulongs[0],    (OA::ULong) 1   );
  TEST_EXPECTED_VAL(array_ulongs[1],    (OA::ULong) 2   );
  TEST_EXPECTED_VAL(array_ulongs[2],    (OA::ULong) 122 );
  TEST_EXPECTED_VAL(array_ulongs[3],    (OA::ULong) 2943);

  return true;
}

bool did_pass_test_parsing_clk_refin()
{
  // example string: "rate 40000000"

  std::string clk_refin_str("rate 40000000");
  std::cout << "TEST: parsing string '" << clk_refin_str << "\n";

  struct ad9361_config_proxy_clk_refin clk_refin;

  try {
    parse(clk_refin_str.c_str(), clk_refin);
  }
  catch(std::string& e) {
    std::cerr << "ERROR: " << e << "\n";
    return false;
  }
  catch(...) {
    std::cerr << "ERROR: unknown exception\n";
    return false;
  }

  TEST_EXPECTED_VAL(clk_refin.rate, (OA::ULong) 40000000);

  return true;
}

bool did_pass_test_parsing_pdata()
{
  // example string: "rx2tx2 false,fdd true,use_extclk false,dcxo_coarse 8,dcxo_fine 5920,rx1tx1_mode_use_rx_num 1,rx1tx1_mode_use_tx_num 1"

  std::string pdata_str("rx2tx2 false,fdd true,use_extclk false,dcxo_coarse 8,dcxo_fine 5920,rx1tx1_mode_use_rx_num 1,rx1tx1_mode_use_tx_num 1");
  std::cout << "TEST: parsing string '" << pdata_str << "\n";

  struct ad9361_config_proxy_pdata pdata;

  try {
    parse(pdata_str.c_str(), pdata);
  }
  catch(std::string& e) {
    std::cerr << "ERROR: " << e << "\n";
    return false;
  }
  catch(...) {
    std::cerr << "ERROR: unknown exception\n";
    return false;
  }

  TEST_EXPECTED_VAL(pdata.rx2tx2,                  (OA::Bool)  false);
  TEST_EXPECTED_VAL(pdata.fdd,                     (OA::Bool)  true );
  TEST_EXPECTED_VAL(pdata.use_extclk,              (OA::Bool)  false);
  TEST_EXPECTED_VAL(pdata.dcxo_coarse,             (OA::ULong) 8    );
  TEST_EXPECTED_VAL(pdata.dcxo_fine,               (OA::ULong) 5920 );
  TEST_EXPECTED_VAL(pdata.rx1tx1_mode_use_rx_num , (OA::ULong) 1    );
  TEST_EXPECTED_VAL(pdata.rx1tx1_mode_use_tx_num , (OA::ULong) 1    );

  return true;
}

bool did_pass_test_parsing_ad9361_rf_phy()
{
  // example string: "clk_refin {rate 40000000},pdata {rx2tx2 false,fdd true,use_extclk false,dcxo_coarse 8,dcxo_fine 5920,rx1tx1_mode_use_rx_num 1,rx1tx1_mode_use_tx_num 1}"

  std::string ad9361_rf_phy_str("clk_refin {rate 40000000},pdata {rx2tx2 false,fdd true,use_extclk false,dcxo_coarse 8,dcxo_fine 5920,rx1tx1_mode_use_rx_num 1,rx1tx1_mode_use_tx_num 1}");
  std::cout << "TEST: parsing string '" << ad9361_rf_phy_str << "\n";

  struct ad9361_config_proxy_ad9361_rf_phy ad9361_rf_phy;

  try {
    parse(ad9361_rf_phy_str.c_str(), ad9361_rf_phy);
  }
  catch(std::string& e) {
    std::cerr << "ERROR: " << e << "\n";
    return false;
  }
  catch(...) {
    std::cerr << "ERROR: unknown exception\n";
    return false;
  }

  TEST_EXPECTED_VAL(ad9361_rf_phy.clk_refin.rate,                (OA::ULong) 40000000);
  TEST_EXPECTED_VAL(ad9361_rf_phy.pdata.rx2tx2,                  (OA::Bool)  false   );
  TEST_EXPECTED_VAL(ad9361_rf_phy.pdata.fdd,                     (OA::Bool)  true    );
  TEST_EXPECTED_VAL(ad9361_rf_phy.pdata.use_extclk,              (OA::Bool)  false   );
  TEST_EXPECTED_VAL(ad9361_rf_phy.pdata.dcxo_coarse,             (OA::ULong) 8       );
  TEST_EXPECTED_VAL(ad9361_rf_phy.pdata.dcxo_fine,               (OA::ULong) 5920    );
  TEST_EXPECTED_VAL(ad9361_rf_phy.pdata.rx1tx1_mode_use_rx_num , (OA::ULong) 1       );
  TEST_EXPECTED_VAL(ad9361_rf_phy.pdata.rx1tx1_mode_use_tx_num , (OA::ULong) 1       );

  return true;
}

bool did_pass_test_parsing_ad9361_init()
{
  // example string: "reference_clk_rate 40000000,one_rx_one_tx_mode_use_rx_num 1,one_rx_one_tx_mode_use_tx_num 1,frequency_division_duplex_mode_enable 1,xo_disable_use_ext_refclk_enable 0,two_t_two_r_timing_enable 0,pp_tx_swap_enable 1,pp_rx_swap_enable 1,tx_channel_swap_enable 1,rx_channel_swap_enable 1,delay_rx_data 0,rx_data_clock_delay 0,rx_data_delay 4,tx_fb_clock_delay 7,tx_data_delay 0"

  std::string ad9361_init_str("reference_clk_rate 40000000,one_rx_one_tx_mode_use_rx_num 1,one_rx_one_tx_mode_use_tx_num 1,frequency_division_duplex_mode_enable 1,xo_disable_use_ext_refclk_enable 0,two_t_two_r_timing_enable 0,pp_tx_swap_enable 1,pp_rx_swap_enable 1,tx_channel_swap_enable 1,rx_channel_swap_enable 1,delay_rx_data 0,rx_data_clock_delay 0,rx_data_delay 4,tx_fb_clock_delay 7,tx_data_delay 0");

  std::cout << "TEST: parsing string '" << ad9361_init_str << "\n";

  struct ad9361_config_proxy_ad9361_init ad9361_init;

  try {
    parse(ad9361_init_str.c_str(), ad9361_init);
  }
  catch(std::string& e) {
    std::cerr << "ERROR: " << e << "\n";
    return false;
  }
  catch(...) {
    std::cerr << "ERROR: unknown exception\n";
    return false;
  }

  TEST_EXPECTED_VAL(ad9361_init.reference_clk_rate,                    (OA::ULong) 40000000);
  TEST_EXPECTED_VAL(ad9361_init.one_rx_one_tx_mode_use_rx_num,         (OA::UChar) 1       );
  TEST_EXPECTED_VAL(ad9361_init.one_rx_one_tx_mode_use_tx_num,         (OA::UChar) 1       );
  TEST_EXPECTED_VAL(ad9361_init.frequency_division_duplex_mode_enable, (OA::UChar) 1       );
  TEST_EXPECTED_VAL(ad9361_init.xo_disable_use_ext_refclk_enable,      (OA::UChar) 0       );
  TEST_EXPECTED_VAL(ad9361_init.two_t_two_r_timing_enable,             (OA::Bool)  false   );
  TEST_EXPECTED_VAL(ad9361_init.pp_tx_swap_enable,                     (OA::UChar) 1       );
  TEST_EXPECTED_VAL(ad9361_init.pp_rx_swap_enable,                     (OA::UChar) 1       );
  TEST_EXPECTED_VAL(ad9361_init.tx_channel_swap_enable,                (OA::UChar) 1       );
  TEST_EXPECTED_VAL(ad9361_init.rx_channel_swap_enable,                (OA::UChar) 1       );
  TEST_EXPECTED_VAL(ad9361_init.delay_rx_data,                         (OA::ULong) 0       );
  TEST_EXPECTED_VAL(ad9361_init.rx_data_clock_delay,                   (OA::ULong) 0       );
  TEST_EXPECTED_VAL(ad9361_init.rx_data_delay,                         (OA::ULong) 4       );
  TEST_EXPECTED_VAL(ad9361_init.tx_fb_clock_delay,                     (OA::ULong) 7       );
  TEST_EXPECTED_VAL(ad9361_init.tx_data_delay,                         (OA::ULong) 0       );

  return true;
}

bool did_pass_test_parsing_rx_gain_control_mode()
{
  // example string: "0,0"

  std::string rx_gain_control_mode_str("0,0");
  std::cout << "TEST: parsing string '" << rx_gain_control_mode_str << "\n";

  ad9361_config_proxy_rx_gain_control_mode_t ad9361_config_proxy_rx_gain_control_mode;

  try {
    parse(rx_gain_control_mode_str.c_str(), ad9361_config_proxy_rx_gain_control_mode);
  }
  catch(std::string& e) {
    std::cerr << "ERROR: " << e << "\n";
    return false;
  }
  catch(...) {
    std::cerr << "ERROR: unknown exception\n";
    return false;
  }

  TEST_EXPECTED_VAL(ad9361_config_proxy_rx_gain_control_mode[0], (OA::UChar) 0);
  TEST_EXPECTED_VAL(ad9361_config_proxy_rx_gain_control_mode[1], (OA::UChar) 0);

  return true;
}

bool did_pass_test_parsing_rx_rf_gain()
{
  // example string: "1,1"

  std::string rx_rf_gain_str("1,1");
  std::cout << "TEST: parsing string '" << rx_rf_gain_str << "\n";

  ad9361_config_proxy_rx_rf_gain_t ad9361_config_proxy_rx_rf_gain;

  try {
    parse(rx_rf_gain_str.c_str(), ad9361_config_proxy_rx_rf_gain);
  }
  catch(std::string& e) {
    std::cerr << "ERROR: " << e << "\n";
    return false;
  }
  catch(...) {
    std::cerr << "ERROR: unknown exception\n";
    return false;
  }

  TEST_EXPECTED_VAL(ad9361_config_proxy_rx_rf_gain[0], (OA::Long) 1);
  TEST_EXPECTED_VAL(ad9361_config_proxy_rx_rf_gain[1], (OA::Long) 1);

  return true;
}

bool did_pass_test_parsing_tx_attenuation()
{
  // example string: "1000,1000"

  std::string tx_attenuation_str("1000,1000");
  std::cout << "TEST: parsing string '" << tx_attenuation_str << "\n";

  ad9361_config_proxy_tx_attenuation_t ad9361_config_proxy_tx_attenuation;

  try {
    parse(tx_attenuation_str.c_str(), ad9361_config_proxy_tx_attenuation);
  }
  catch(std::string& e) {
    std::cerr << "ERROR: " << e << "\n";
    return false;
  }
  catch(...) {
    std::cerr << "ERROR: unknown exception\n";
    return false;
  }

  TEST_EXPECTED_VAL(ad9361_config_proxy_tx_attenuation[0], (OA::ULong) 1000);
  TEST_EXPECTED_VAL(ad9361_config_proxy_tx_attenuation[1], (OA::ULong) 1000);

  return true;
}

bool did_pass_test_stringifying_ad9361_init()
{
  struct ad9361_config_proxy_ad9361_init ad9361_init;
  ad9361_init.reference_clk_rate                    = (OA::ULong) 40000000;
  ad9361_init.one_rx_one_tx_mode_use_rx_num         = (OA::UChar) 1       ;
  ad9361_init.one_rx_one_tx_mode_use_tx_num         = (OA::UChar) 1       ;
  ad9361_init.frequency_division_duplex_mode_enable = (OA::UChar) 1       ;
  ad9361_init.xo_disable_use_ext_refclk_enable      = (OA::UChar) 0       ;
  ad9361_init.two_t_two_r_timing_enable             = (OA::Bool)  false   ;
  ad9361_init.pp_tx_swap_enable                     = (OA::UChar) 1       ;
  ad9361_init.pp_rx_swap_enable                     = (OA::UChar) 1       ;
  ad9361_init.tx_channel_swap_enable                = (OA::UChar) 1       ;
  ad9361_init.rx_channel_swap_enable                = (OA::UChar) 1       ;
  ad9361_init.delay_rx_data                         = (OA::ULong) 0       ;
  ad9361_init.rx_data_clock_delay                   = (OA::ULong) 0       ;
  ad9361_init.rx_data_delay                         = (OA::ULong) 4       ;
  ad9361_init.tx_fb_clock_delay                     = (OA::ULong) 7       ;
  ad9361_init.tx_data_delay                         = (OA::ULong) 0       ;

  std::string ad9361_init_stringified = to_string(ad9361_init);

  TEST_EXPECTED_VAL(ad9361_init_stringified, std::string("reference_clk_rate 40000000,one_rx_one_tx_mode_use_rx_num 1,one_rx_one_tx_mode_use_tx_num 1,frequency_division_duplex_mode_enable 1,xo_disable_use_ext_refclk_enable 0,two_t_two_r_timing_enable 0,pp_tx_swap_enable 1,pp_rx_swap_enable 1,tx_channel_swap_enable 1,rx_channel_swap_enable 1,delay_rx_data 0,rx_data_clock_delay 0,rx_data_delay 4,tx_fb_clock_delay 7,tx_data_delay 0"));

  return true;
}

/*! @brief Does not require an OpenCPI application.
 ******************************************************************************/
bool did_pass_test_no_ocpi_app_parsers()
{
  if(!did_pass_test_parsing_array_uchars())         { goto failed; }
  if(!did_pass_test_parsing_array_longs())          { goto failed; }
  if(!did_pass_test_parsing_array_ulongs())         { goto failed; }
  if(!did_pass_test_parsing_clk_refin())            { goto failed; }
  if(!did_pass_test_parsing_pdata())                { goto failed; }
  if(!did_pass_test_parsing_ad9361_rf_phy())        { goto failed; }
  if(!did_pass_test_parsing_ad9361_init())          { goto failed; }
  if(!did_pass_test_parsing_rx_gain_control_mode()) { goto failed; }
  if(!did_pass_test_parsing_rx_rf_gain())           { goto failed; }
  if(!did_pass_test_parsing_tx_attenuation())       { goto failed; }
  if(!did_pass_test_stringifying_ad9361_init())     { goto failed; }
 
  return true;

  failed:
  return false;
}

#endif // _TEST_PARSERS_H

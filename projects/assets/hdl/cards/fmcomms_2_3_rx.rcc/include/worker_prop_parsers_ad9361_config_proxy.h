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

#ifndef _WORKER_PROP_PARSERS_AD9361_CONFIG_PROXY_H
#define _WORKER_PROP_PARSERS_AD9361_CONFIG_PROXY_H

#include <string>    // std::string
#include <cstdio>    // sscanf()
#include <exception> // std::exception
#include <sstream>   // std::ostringstream
#include <vector>    // std::vector

#include "ocpi_component_prop_type_helpers.h"

// "SSCANF" for "O"pen"CPI" component property of and put parsed value
// in member of struct s (string is of format "<prop> <ocpi_t value>")
#define OCPI_SSCANF_STRUCT(ptr,s,cnt,prop) ocpi_sscanf(ptr,&s.prop,cnt,#prop);

// this is C data structure representation of the ad9361_config_proxy worker's
// ad9361_rf_phy struct property's clk_refin member
struct ad9361_config_proxy_clk_refin
{
  ocpi_ulong_t rate;
};

// this is C data structure representation of the ad9361_config_proxy worker's
// ad9361_rf_phy struct property's pdata member
struct ad9361_config_proxy_pdata
{
  ocpi_bool_t  rx2tx2;
  ocpi_bool_t  fdd;
  ocpi_bool_t  use_extclk;
  ocpi_ulong_t dcxo_coarse;
  ocpi_ulong_t dcxo_fine;
  ocpi_ulong_t rx1tx1_mode_use_rx_num;
  ocpi_ulong_t rx1tx1_mode_use_tx_num;
};

// this is C data structure representation of the ad9361_config_proxy worker's
// ad9361_rf_phy struct property
struct ad9361_config_proxy_ad9361_rf_phy
{
  struct ad9361_config_proxy_clk_refin clk_refin;
  struct ad9361_config_proxy_pdata     pdata;
};

// this is C data structure representation of the ad9361_config_proxy worker's
// ad9361_init struct property
struct ad9361_config_proxy_ad9361_init
{
  ocpi_ulong_t reference_clk_rate;
  ocpi_uchar_t one_rx_one_tx_mode_use_rx_num;
  ocpi_uchar_t one_rx_one_tx_mode_use_tx_num;
  ocpi_uchar_t frequency_division_duplex_mode_enable;
  ocpi_uchar_t xo_disable_use_ext_refclk_enable;
  ocpi_bool_t  two_t_two_r_timing_enable;
  ocpi_uchar_t pp_tx_swap_enable;
  ocpi_uchar_t pp_rx_swap_enable;
  ocpi_uchar_t tx_channel_swap_enable;
  ocpi_uchar_t rx_channel_swap_enable;
  ocpi_ulong_t delay_rx_data;
  ocpi_ulong_t rx_data_clock_delay;
  ocpi_ulong_t rx_data_delay;
  ocpi_ulong_t tx_fb_clock_delay;
  ocpi_ulong_t tx_data_delay;
};

// this is C data structure representation of the ad9361_config_proxy worker's
// rx_gain_control_mode property
typedef struct ad9361_config_proxy_rx_gain_control_mode_t
{
  ocpi_uchar_t  operator[](int i) const { return data[i]; }
  ocpi_uchar_t& operator[](int i)       { return data[i]; }
  ocpi_uchar_t  data[2];
} ad9361_config_proxy_rx_gain_control_mode_t;

// this is C data structure representation of the ad9361_config_proxy worker's
// rx_rf_gain property
typedef struct ad9361_config_proxy_rx_rf_gain_t
{
  ocpi_long_t  operator[](int i) const { return data[i]; }
  ocpi_long_t& operator[](int i)       { return data[i]; }
  ocpi_long_t  data[2];
} ad9361_config_proxy_rx_rf_gain_t;

// this is C data structure representation of the ad9361_config_proxy worker's
// tx_attenuation property
typedef struct ad9361_config_proxy_tx_attenuation_t
{
  ocpi_ulong_t  operator[](int i) const { return data[i]; }
  ocpi_ulong_t& operator[](int i)       { return data[i]; }
  ocpi_ulong_t  data[2];
} ad9361_config_proxy_tx_attenuation_t;

const char* parse(const char* cstr_to_parse,
    struct ad9361_config_proxy_clk_refin& s, int* count = 0)
{
  int tmp_count;
  int count_sum = 0;
  int num_filled;
  char* ptr = (char*) cstr_to_parse;

  // commence parsing
  // example string: "rate 40000000"

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, rate);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  if(count != 0)
  {
    *count = count_sum;
  }
  return 0;

  error:
    std::string err_str;
    err_str = "Malformed clk_refin struct string read ";
    err_str += "from 'ad9361_config_proxy' worker: '";
    err_str += std::string(cstr_to_parse) + "'";
    return err_str.c_str();
}

const char* parse(const char* cstr_to_parse,
    struct ad9361_config_proxy_pdata& s, int* count = 0)
{
  int tmp_count;
  int count_sum = 0;
  int num_filled;
  char* ptr = (char*) cstr_to_parse;

  // commence parsing
  // example string: "rx2tx2 false,fdd true,use_extclk false,dcxo_coarse 8,dcxo_fine 5920,rx1tx1_mode_use_rx_num 1,rx1tx1_mode_use_tx_num 1"

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, rx2tx2);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, fdd);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, use_extclk);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, dcxo_coarse);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, dcxo_fine);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, rx1tx1_mode_use_rx_num);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, rx1tx1_mode_use_tx_num);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  if(count != 0)
  {
    *count = count_sum;
  }
  return 0;
  
  error:
    std::string err_str;
    err_str = "Malformed pdata struct string read ";
    err_str += "from 'ad9361_config_proxy' worker: '";
    err_str += std::string(cstr_to_parse) + "'";
    return err_str.c_str();
}

const char* parse(const char* cstr_to_parse,
    struct ad9361_config_proxy_ad9361_rf_phy& s, int* count = 0)
{
  int tmp_count;
  int count_sum = 0;
  int num_filled;
  char* ptr = (char*) cstr_to_parse;
  char* err;

  // commence parsing
  // example string: "clk_refin {rate 40000000},pdata {rx2tx2 false,fdd true,use_extclk false,dcxo_coarse 8,dcxo_fine 5920,rx1tx1_mode_use_rx_num 1,rx1tx1_mode_use_tx_num 1}"

  num_filled = sscanf(ptr, "clk_refin {%n", &tmp_count);
  if((num_filled != 0) and (tmp_count != 11)) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  err = (char*) parse(ptr, s.clk_refin, &tmp_count);
  if(err != 0) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, "}%n", &tmp_count);
  if(((num_filled != 0) and tmp_count != 1)) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(((num_filled != 0) and tmp_count != 1)) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, "pdata {%n", &tmp_count);
  if(((num_filled != 0) and tmp_count != 7)) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  err = (char*) parse(ptr, s.pdata, &tmp_count);
  if(err != 0) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, "}%n", &tmp_count);
  if(((num_filled != 0) and tmp_count != 1)) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  if(count != 0)
  {
    *count = count_sum;
  }
  return 0;
  
  error:
    std::string err_str;
    err_str = "Malformed ad9361_rf_phy property value string read ";
    err_str += "from 'ad9361_config_proxy' worker: '";
    err_str += std::string(cstr_to_parse) + "'";
    return err_str.c_str();
}

const char* parse(const char* cstr_to_parse,
    struct ad9361_config_proxy_ad9361_init& s, int* count = 0)
{
  int tmp_count;
  int count_sum = 0;
  int num_filled;
  char* ptr = (char*) cstr_to_parse;

  // commence parsing
  // example string: "reference_clk_rate 40000000,one_rx_one_tx_mode_use_rx_num 1,one_rx_one_tx_mode_use_tx_num 1,frequency_division_duplex_mode_enable 1,xo_disable_use_ext_refclk_enable 0,two_t_two_r_timing_enable 0,pp_tx_swap_enable 1,pp_rx_swap_enable 1,tx_channel_swap_enable 1,rx_channel_swap_enable 1,delay_rx_data 0,rx_data_clock_delay 0,rx_data_delay 4,tx_fb_clock_delay 7,tx_data_delay 0"

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, reference_clk_rate);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, one_rx_one_tx_mode_use_rx_num);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, one_rx_one_tx_mode_use_tx_num);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, frequency_division_duplex_mode_enable);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, xo_disable_use_ext_refclk_enable);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, two_t_two_r_timing_enable);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, pp_tx_swap_enable);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, pp_rx_swap_enable);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, tx_channel_swap_enable);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, rx_channel_swap_enable);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, delay_rx_data);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, rx_data_clock_delay);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, rx_data_delay);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, tx_fb_clock_delay);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, tx_data_delay);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  if(count != 0)
  {
    *count = count_sum;
  }
  return 0;
  
  error:
    std::string err_str;
    err_str = "Malformed ad9361_init property value string read ";
    err_str += "from 'ad9361_config_proxy' worker: '";
    err_str += std::string(cstr_to_parse) + "'";
    return err_str.c_str();
}

const char* parse(const char* cstr_to_parse,
    ad9361_config_proxy_rx_gain_control_mode_t& prop, int* count = 0)
{
  // commence parsing
  // example string: "0,0"

  std::vector<ocpi_uchar_t> tmp;
  const char* err = parse(cstr_to_parse, tmp, count);
  if(err != 0) { return err; }
  if(tmp.size() != 2) { goto error; }
  prop[0] = tmp[0];
  prop[1] = tmp[1];

  return 0;

  error:
    std::string err_str;
    err_str = "Malformed rx_gain_control_mode property value string read ";
    err_str += "from 'ad9361_config_proxy' worker: '";
    err_str += std::string(cstr_to_parse) + "'";
    return err_str.c_str();
}

const char* parse(const char* cstr_to_parse,
    ad9361_config_proxy_rx_rf_gain_t& prop, int* count = 0)
{
  // commence parsing
  // example string: "1,1"

  std::vector<ocpi_long_t> tmp;
  const char* err = parse(cstr_to_parse, tmp, count);
  if(err != 0) { return err; }
  if(tmp.size() != 2) { goto error; }
  prop[0] = tmp[0];
  prop[1] = tmp[1];

  return 0;

  error:
    std::string err_str;
    err_str = "Malformed rx_rf_gain property value string read ";
    err_str += "from 'ad9361_config_proxy' worker: '";
    err_str += std::string(cstr_to_parse) + "'";
    return err_str.c_str();
}

const char* parse(const char* cstr_to_parse,
   ad9361_config_proxy_tx_attenuation_t& prop, int* count = 0)
{
  // commence parsing
  // example string: "1000,1000"

  std::vector<ocpi_ulong_t> tmp;
  const char* err = parse(cstr_to_parse, tmp, count);
  if(err != 0) { return err; }
  if(tmp.size() != 2) { goto error; }
  prop[0] = tmp[0];
  prop[1] = tmp[1];

  return 0;

  error:
    std::string err_str;
    err_str = "Malformed tx_attenuation property value string read ";
    err_str += "from 'ad9361_config_proxy' worker: '";
    err_str += std::string(cstr_to_parse) + "'";
    return err_str.c_str();
}

std::string to_string(const struct ad9361_config_proxy_ad9361_init & s)
{
  std::ostringstream oss;

  oss <<              "reference_clk_rate ";
  append_to_oss(oss, s.reference_clk_rate                   );

  oss <<             ",one_rx_one_tx_mode_use_rx_num ";
  append_to_oss(oss, s.one_rx_one_tx_mode_use_rx_num        );

  oss <<             ",one_rx_one_tx_mode_use_tx_num ";
  append_to_oss(oss, s.one_rx_one_tx_mode_use_tx_num        );

  oss <<             ",frequency_division_duplex_mode_enable ";
  append_to_oss(oss, s.frequency_division_duplex_mode_enable);

  oss <<             ",xo_disable_use_ext_refclk_enable ";
  append_to_oss(oss, s.xo_disable_use_ext_refclk_enable     );

  oss <<             ",two_t_two_r_timing_enable ";
  append_to_oss(oss, s.two_t_two_r_timing_enable            );

  oss <<             ",pp_tx_swap_enable ";
  append_to_oss(oss, s.pp_tx_swap_enable                    );

  oss <<             ",pp_rx_swap_enable ";
  append_to_oss(oss, s.pp_rx_swap_enable                    );

  oss <<             ",tx_channel_swap_enable ";
  append_to_oss(oss, s.tx_channel_swap_enable               );

  oss <<             ",rx_channel_swap_enable ";
  append_to_oss(oss, s.rx_channel_swap_enable               );

  oss <<             ",delay_rx_data ";
  append_to_oss(oss, s.delay_rx_data                        );

  oss <<             ",rx_data_clock_delay ";
  append_to_oss(oss, s.rx_data_clock_delay                  );

  oss <<             ",rx_data_delay ";
  append_to_oss(oss, s.rx_data_delay                        );

  oss <<             ",tx_fb_clock_delay ";
  append_to_oss(oss, s.tx_fb_clock_delay                    );

  oss <<             ",tx_data_delay ";
  append_to_oss(oss, s.tx_data_delay                        );

  std::string str = oss.str();
  return str;
}

std::string to_string(const ad9361_config_proxy_rx_gain_control_mode_t& prop)
{
  std::ostringstream oss;

  append_to_oss(oss, prop[0]);
  oss << ",";
  append_to_oss(oss, prop[1]);

  std::string str = oss.str();
  return str;
}

std::string to_string(const ad9361_config_proxy_rx_rf_gain_t& prop)
{
  std::ostringstream oss;

  append_to_oss(oss, prop[0]);
  oss << ",";
  append_to_oss(oss, prop[1]);

  std::string str = oss.str();
  return str;
}

std::string to_string(const ad9361_config_proxy_tx_attenuation_t& prop)
{
  std::ostringstream oss;

  append_to_oss(oss, prop[0]);
  oss << ",";
  append_to_oss(oss, prop[1]);

  std::string str = oss.str();
  return str;
}

#endif // _WORKER_PROP_PARSERS_AD9361_CONFIG_PROXY_H

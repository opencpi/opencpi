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

#ifndef _WORKER_PROP_PARSERS_FMCOMMS_2_3_RX_H
#define _WORKER_PROP_PARSERS_FMCOMMS_2_3_RX_H

#include "ocpi_component_prop_type_helpers.h" // ocpi_double_t, ocpi_bool_t

enum class ocpi_fmcomms_2_3_rx_duplex_mode_t {TDD, FDD};
enum class ocpi_fmcomms_2_3_rx_SMA_channel_t {RX1A, RX2A};

// "SSCANF" for "O"pen"CPI" component property of and put parsed value
// in member of struct s (string is of format "<prop> <ocpi_t value>")
#define OCPI_SSCANF_STRUCT(ptr,s,cnt,prop) ocpi_sscanf(ptr,&s.prop,cnt,#prop);

// this is C data structure representation of the fmcomms_2_3_rx worker's
// config struct property
struct fmcomms_2_3_rx_config
{
  ocpi_double_t                     reference_clk_rate_Hz;
  ocpi_fmcomms_2_3_rx_duplex_mode_t duplex_mode;
  ocpi_bool_t                       are_using_REF_CLK_SMA;
  ocpi_fmcomms_2_3_rx_SMA_channel_t SMA_channel;
};

/*! @brief sscanf string of the format "<prop_name> <prop_value>" for OpenCPI
 *         fmcomms_2_3_rx worker's config struct property's duplex_mode member
 *         enum.
 *
 *  @param[in]  s                   pointer to char array which contains the
 *                                  property name/value to be parsed,
 *                                  expected format: "<prop_name> <prop_value>"
 *  @param[out] prop_val            pointer to variable whose value will be
 *                                  assigned to the parsed value
 *  @param[out] count               Number of characters read
 *  @param[in]  prop_chars          pointer to char array containing property
 *                                  name name to parse for
 *  @return Return value of sccanf call.
 ******************************************************************************/
int ocpi_sscanf(const char* s, ocpi_fmcomms_2_3_rx_duplex_mode_t* prop_val,
    int* count, const char* prop_chars)
{
  int num_filled_1, num_filled_2;
  int tmp_count_1, tmp_count_2;
  char* tmp_s = (char*) s;

  num_filled_1 = ocpi_sscanf_prop_name(tmp_s, prop_chars, &tmp_count_1);
  tmp_s += tmp_count_1;
  if(num_filled_1 == EOF) { return num_filled_1; }
  
  char buffer[32];
  num_filled_2 = sscanf(tmp_s, " %32[^,]%n", buffer, &tmp_count_2);
  const std::string TDD("TDD");
  const std::string FDD("FDD");
  if(TDD.compare(buffer) == 0)
  {
    *prop_val = ocpi_fmcomms_2_3_rx_duplex_mode_t::TDD;
  }
  else if(FDD.compare(buffer) == 0)
  {
    *prop_val = ocpi_fmcomms_2_3_rx_duplex_mode_t::FDD;
  }
  else
  {
    return EOF; //! @todo - TODO/FIXME implement less lazy error enforcement
  }
  *count = (tmp_count_1 + tmp_count_2);
  if(num_filled_2 == EOF) { return num_filled_2; }

  return num_filled_1 + num_filled_2;
}

/*! @brief sscanf string of the format "<prop_name> <prop_value>" for OpenCPI
 *         fmcomms_2_3_rx worker's config struct property's SMA_channel member
 *         enum.
 *
 *  @param[in]  s                   pointer to char array which contains the
 *                                  property name/value to be parsed,
 *                                  expected format: "<prop_name> <prop_value>"
 *  @param[out] prop_val            pointer to variable whose value will be
 *                                  assigned to the parsed value
 *  @param[out] count               Number of characters read
 *  @param[in]  prop_chars          pointer to char array containing property
 *                                  name name to parse for
 *  @return Return value of sccanf call.
 ******************************************************************************/
int ocpi_sscanf(const char* s, ocpi_fmcomms_2_3_rx_SMA_channel_t* prop_val,
    int* count, const char* prop_chars)
{
  int num_filled_1, num_filled_2;
  int tmp_count_1, tmp_count_2;
  char* tmp_s = (char*) s;

  num_filled_1 = ocpi_sscanf_prop_name(tmp_s, prop_chars, &tmp_count_1);
  tmp_s += tmp_count_1;
  if(num_filled_1 == EOF) { return num_filled_1; }
  
  char buffer[32];
  num_filled_2 = sscanf(tmp_s, " %32[^,]%n", buffer, &tmp_count_2);
  const std::string RX1A("RX1A");
  const std::string RX2A("RX2A");
  if(RX1A.compare(buffer) == 0)
  {
    *prop_val = ocpi_fmcomms_2_3_rx_SMA_channel_t::RX1A;
  }
  else if(RX2A.compare(buffer) == 0)
  {
    *prop_val = ocpi_fmcomms_2_3_rx_SMA_channel_t::RX2A;
  }
  else
  {
    return EOF; //! @todo - TODO/FIXME implement less lazy error enforcement
  }
  *count = (tmp_count_1 + tmp_count_2);
  if(num_filled_2 == EOF) { return num_filled_2; }

  return num_filled_1 + num_filled_2;
}

const char* parse(const char* cstr_to_parse,
    struct fmcomms_2_3_rx_config& s, int* count = 0)
{
  int tmp_count;
  int count_sum = 0;
  int num_filled;
  char* ptr = (char*) cstr_to_parse;

  // commence parsing
  // example string: "reference_clk_rate_Hz 40000000,duplex_mode FDD,are_using_REF_CLK_SMA false,SMA_channel RX2A"

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, reference_clk_rate_Hz);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, duplex_mode);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, are_using_REF_CLK_SMA);
  if(num_filled != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = sscanf(ptr, ",%n", &tmp_count);
  if(num_filled == EOF) { goto error; }
  if(tmp_count != 1) { goto error; }
  ptr += tmp_count;
  count_sum += tmp_count;

  num_filled = OCPI_SSCANF_STRUCT(ptr, s, &tmp_count, SMA_channel);
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
    err_str = "Malformed 'config' property value string read ";
    err_str += "from 'fmcomms_2_3_rx' worker: '";
    err_str += std::string(cstr_to_parse) + "'";
    return err_str.c_str();
}


#endif // _WORKER_PROP_PARSERS_FMCOMMS_2_3_RX_H

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

#ifndef _OCPI_COMPONENT_PROP_TYPE_HELPERS_H
#define _OCPI_COMPONENT_PROP_TYPE_HELPERS_H

/*! @file
 *  @brief Provides macros and methods for interacting with C mappings for
 *         OpenCPI component property types.
 ******************************************************************************/

#include <cstdint>   // int8_t, uint8_t, int16_t, etc...
#include <string>    // std::string
#include <sstream>   // std::ostringstream
#include <cinttypes> // SCN... types
#include <vector>    // std::vector

#include "OcpiApi.hh" // OCPI::API namespace

#define SCNOCPICHAR       SCNi8
#define SCNOCPIUCHAR      SCNu8
#define SCNOCPISHORT      SCNi16
#define SCNOCPIUSHORT     SCNu16
#define SCNOCPILONG       SCNi32
#define SCNOCPIULONG      SCNu32
#define SCNOCPILONGLONG   SCNi64
#define SCNOCPIULONGLONG  SCNu64

namespace OA = OCPI::API;

int ocpi_sscanf_prop_name(const char* s, const char* prop_chars, int* count)
{
  std::string format_str(prop_chars);
  format_str += "%n";

  int ret = sscanf(s, format_str.c_str(), count);

  return ret;
}

/*! @brief sscanf string of the format "<prop_name> <prop_value>" for OpenCPI
 *         bool property value.
 *
 *  @param[in]  s                   pointer to char array which contains the
 *                                  property name/value to be parsed,
 *                                  expected format: "<prop_name> <prop_value>"
 *  @param[out] prop_val            pointer to variable whose value will be
 *                                  assigned to the parsed value
 *  @param[out] count               Number of characters read
 *  @param[in]  prop_chars          pointer to char array containing property
 *                                  name name to parse for
 *  @return On success, the function returns the number of items in the argument
 *          list successfully filled. This count can match the expected number
 *          of items or be less (even zero) in the case of a matching failure.
 *          In the case of an input failure before any data could be
 *          successfully interpreted, EOF is returned.
 ******************************************************************************/
int ocpi_sscanf(const char* s, OA::Bool* prop_val, int* count,
    const char* prop_chars)
{
  int num_filled_1, num_filled_2;
  int tmp_count_1, tmp_count_2;
  char* tmp_s = (char*) s;

  num_filled_1 = ocpi_sscanf_prop_name(tmp_s, prop_chars, &tmp_count_1);
  tmp_s += tmp_count_1;
  if(num_filled_1 == EOF) { return num_filled_1; }

  char buffer[32];
  num_filled_2 = sscanf(tmp_s, " %32[^,]%n", buffer, &tmp_count_2);
  const std::string t("true");
  const std::string f("false");
  const std::string one("1");
  const std::string zero("0");
  if((t.compare(buffer) == 0) || (one.compare(buffer) == 0))
  {
    *prop_val = true;
  }
  else if((f.compare(buffer) == 0) || (zero.compare(buffer) == 0))
  {
    *prop_val = false;
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
 *         double property value.
 *
 *  @param[in]  s                   pointer to char array which contains the
 *                                  property name/value to be parsed,
 *                                  expected format: "<prop_name> <prop_value>"
 *  @param[out] prop_val            pointer to variable whose value will be
 *                                  assigned to the parsed value
 *  @param[out] count               Number of characters read
 *  @param[in]  prop_chars          pointer to char array containing property
 *                                  name name to parse for
 *  @return On success, the function returns the number of items in the argument
 *          list successfully filled. This count can match the expected number
 *          of items or be less (even zero) in the case of a matching failure.
 *          In the case of an input failure before any data could be
 *          successfully interpreted, EOF is returned.
 ******************************************************************************/
int ocpi_sscanf(const char* s, double* prop_val, int* count,
    const char* prop_chars)
{
  int num_filled_1, num_filled_2;
  int tmp_count_1, tmp_count_2;
  char* tmp_s = (char*) s;

  num_filled_1 = ocpi_sscanf_prop_name(tmp_s, prop_chars, &tmp_count_1);
  tmp_s += tmp_count_1;
  if(num_filled_1 == EOF) { return num_filled_1; }

  char buffer[32];
  num_filled_2 = sscanf(tmp_s, " %32[^,]%n", buffer, &tmp_count_2);
  buffer[tmp_count_2] = EOF;
  *prop_val = strtod(buffer, NULL);
  *count = (tmp_count_1 + tmp_count_2);
  if(num_filled_2 == EOF) { return num_filled_2; }

  return num_filled_1 + num_filled_2;
}

/*! @brief sscanf string of the format "<prop_name> <prop_value>" for OpenCPI
 *         char property value.
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
int ocpi_sscanf(const char* s, OA::Char* prop_val, int* count,
    const char* prop_chars)
{
  int ret;
  int tmp_count_1, tmp_count_2;
  char* tmp_s = (char*) s;

  ret = ocpi_sscanf_prop_name(tmp_s, prop_chars, &tmp_count_1);
  tmp_s += tmp_count_1;
  if(ret == EOF) { return ret; }
  
  ret = sscanf(tmp_s, " %" SCNOCPICHAR "%n", prop_val, &tmp_count_2);
  *count = (tmp_count_1 + tmp_count_2);

  return ret;
}

/*! @brief sscanf string of the format "<prop_name> <prop_value>" for OpenCPI
 *         uchar property value.
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
int ocpi_sscanf(const char* s, OA::UChar* prop_val, int* count,
    const char* prop_chars)
{
  int ret;
  int tmp_count_1, tmp_count_2;
  char* tmp_s = (char*) s;

  ret = ocpi_sscanf_prop_name(tmp_s, prop_chars, &tmp_count_1);
  tmp_s += tmp_count_1;
  if(ret == EOF) { return ret; }
  
  ret = sscanf(tmp_s, " %" SCNOCPIUCHAR "%n", prop_val, &tmp_count_2);
  *count = (tmp_count_1 + tmp_count_2);

  return ret;
}

/*! @brief sscanf string of the format "<prop_name> <prop_value>" for OpenCPI
 *         short property value.
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
int ocpi_sscanf(const char* s, OA::Short* prop_val, int* count,
    const char* prop_chars)
{
  int ret;
  int tmp_count_1, tmp_count_2;
  char* tmp_s = (char*) s;

  ret = ocpi_sscanf_prop_name(tmp_s, prop_chars, &tmp_count_1);
  tmp_s += tmp_count_1;
  if(ret == EOF) { return ret; }
  
  ret = sscanf(tmp_s, " %" SCNOCPISHORT "%n", prop_val, &tmp_count_2);
  *count = (tmp_count_1 + tmp_count_2);

  return ret;
}

/*! @brief sscanf string of the format "<prop_name> <prop_value>" for OpenCPI
 *         ushort property value.
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
int ocpi_sscanf(const char* s, OA::UShort* prop_val, int* count,
    const char* prop_chars)
{
  int ret;
  int tmp_count_1, tmp_count_2;
  char* tmp_s = (char*) s;

  ret = ocpi_sscanf_prop_name(tmp_s, prop_chars, &tmp_count_1);
  tmp_s += tmp_count_1;
  if(ret == EOF) { return ret; }
  
  ret = sscanf(tmp_s, " %" SCNOCPIUSHORT "%n", prop_val, &tmp_count_2);
  *count = (tmp_count_1 + tmp_count_2);

  return ret;
}

/*! @brief sscanf string of the format "<prop_name> <prop_value>" for OpenCPI
 *         long property value.
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
int ocpi_sscanf(const char* s, OA::Long* prop_val, int* count,
    const char* prop_chars)
{
  int ret;
  int tmp_count_1, tmp_count_2;
  char* tmp_s = (char*) s;

  ret = ocpi_sscanf_prop_name(tmp_s, prop_chars, &tmp_count_1);
  tmp_s += tmp_count_1;
  if(ret == EOF) { return ret; }
  
  ret = sscanf(tmp_s, " %" SCNOCPILONG "%n", prop_val, &tmp_count_2);
  *count = (tmp_count_1 + tmp_count_2);

  return ret;
}

/*! @brief sscanf string of the format "<prop_name> <prop_value>" for OpenCPI
 *         ulong property value.
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
int ocpi_sscanf(const char* s, OA::ULong* prop_val, int* count,
    const char* prop_chars)
{
  int ret;
  int tmp_count_1, tmp_count_2;
  char* tmp_s = (char*) s;

  ret = ocpi_sscanf_prop_name(tmp_s, prop_chars, &tmp_count_1);
  tmp_s += tmp_count_1;
  if(ret == EOF) { return ret; }
  
  ret = sscanf(tmp_s, " %" SCNOCPIULONG "%n", prop_val, &tmp_count_2);
  *count = (tmp_count_1 + tmp_count_2);

  return ret;
}

/*! @brief sscanf string of the format "<prop_name> <prop_value>" for OpenCPI
 *         longlong property value.
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
int ocpi_sscanf(const char* s, OA::LongLong* prop_val, int* count,
    const char* prop_chars)
{
  int ret;
  int tmp_count_1, tmp_count_2;
  char* tmp_s = (char*) s;

  ret = ocpi_sscanf_prop_name(tmp_s, prop_chars, &tmp_count_1);
  tmp_s += tmp_count_1;
  if(ret == EOF) { return ret; }
  
  ret = sscanf(tmp_s, " %" SCNOCPILONGLONG "%n", prop_val, &tmp_count_2);
  *count = (tmp_count_1 + tmp_count_2);

  return ret;
}

/*! @brief sscanf string of the format "<prop_name> <prop_value>" for OpenCPI
 *         ulonglong property value.
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
int ocpi_sscanf(const char* s, OA::ULongLong* prop_val, int* count,
    const char* prop_chars)
{
  int ret;
  int tmp_count_1, tmp_count_2;
  char* tmp_s = (char*) s;

  ret = ocpi_sscanf_prop_name(tmp_s, prop_chars, &tmp_count_1);
  tmp_s += tmp_count_1;
  if(ret == EOF) { return ret; }
  
  ret = sscanf(tmp_s, " %" SCNOCPIULONGLONG "%n", prop_val, &tmp_count_2);
  *count = (tmp_count_1 + tmp_count_2);

  return ret;
}

/*! @brief Parses OpenCPI component property which is array of uchars.
 ******************************************************************************/
void parse(const char* cstr_to_parse,
    std::vector<OA::UChar>& prop, int* count = 0)
{
  int tmp_count;
  int count_sum = 0;
  int num_filled;
  char* ptr = (char*) cstr_to_parse;

  // commence parsing
  // example string: "1,2,122,9"
  
  do
  {
    OA::UChar tmp_val;
    num_filled = sscanf(ptr, "%" SCNOCPIUCHAR "%n", &tmp_val, &tmp_count);
    prop.push_back(tmp_val);
    if(num_filled != 1) { goto error; }
    ptr += tmp_count;
    count_sum += tmp_count;

    num_filled = sscanf(ptr, ",%n", &tmp_count);
    if(num_filled == EOF) { break; }
    ptr += tmp_count;
    count_sum += tmp_count;
  }
  while(tmp_count == 1);

  if(count != 0)
  {
    *count = count_sum;
  }
  return;

  error:
    std::string err_str;
    err_str = "Malformed array of uchars string read ";
    err_str += "from worker: '";
    err_str += std::string(cstr_to_parse) + "'";
    throw err_str;
}

/*! @brief Parses OpenCPI component property which is array of longs.
 ******************************************************************************/
void parse(const char* cstr_to_parse,
    std::vector<OA::Long>& prop, int* count = 0)
{
  int tmp_count;
  int count_sum = 0;
  int num_filled;
  char* ptr = (char*) cstr_to_parse;

  // commence parsing
  // example string: "1,-2,122,-2943"

  do
  {
    OA::Long tmp_val;
    num_filled = sscanf(ptr, "%" SCNOCPILONG "%n", &tmp_val, &tmp_count);
    prop.push_back(tmp_val);
    if(num_filled != 1) { goto error; }
    ptr += tmp_count;
    count_sum += tmp_count;

    num_filled = sscanf(ptr, ",%n", &tmp_count);
    if(num_filled == EOF) { break; }
    ptr += tmp_count;
    count_sum += tmp_count;
  }
  while(tmp_count == 1);

  if(count != 0)
  {
    *count = count_sum;
  }
  return;

  error:
    std::string err_str;
    err_str = "Malformed array of longs string read ";
    err_str += "from worker: '";
    err_str += std::string(cstr_to_parse) + "'";
    throw err_str;
}

/*! @brief Parses OpenCPI component property which is array of ulongs.
 ******************************************************************************/
void parse(const char* cstr_to_parse,
    std::vector<OA::ULong>& prop, int* count = 0)
{
  int tmp_count;
  int count_sum = 0;
  int num_filled;
  char* ptr = (char*) cstr_to_parse;

  // commence parsing
  // example string: "1,2,122,2943"

  do
  {
    OA::Long tmp_val;
    num_filled = sscanf(ptr, "%" SCNOCPIULONG "%n", &tmp_val, &tmp_count);
    prop.push_back(tmp_val);
    if(num_filled != 1) { goto error; }
    ptr += tmp_count;
    count_sum += tmp_count;

    num_filled = sscanf(ptr, ",%n", &tmp_count);
    if(num_filled == EOF) { break; }
    ptr += tmp_count;
    count_sum += tmp_count;
  }
  while(tmp_count == 1);

  if(count != 0)
  {
    *count = count_sum;
  }
  return;

  error:
    std::string err_str;
    err_str = "Malformed array of longs string read ";
    err_str += "from worker: '";
    err_str += std::string(cstr_to_parse) + "'";
    throw err_str;
}

void inline append_to_oss(std::ostringstream& oss, OA::Char val)
{
  oss << (int) val;
}

void inline append_to_oss(std::ostringstream& oss, OA::UChar val)
{
  oss << (unsigned int) val;
}

void inline append_to_oss(std::ostringstream& oss, OA::Short val)
{
  oss << val;
}

void inline append_to_oss(std::ostringstream& oss, OA::UShort val)
{
  oss << val;
}

void inline append_to_oss(std::ostringstream& oss, OA::Long val)
{
  oss << val;
}

void inline append_to_oss(std::ostringstream& oss, OA::ULong val)
{
  oss << val;
}

void inline append_to_oss(std::ostringstream& oss, OA::LongLong val)
{
  oss << val;
}

void inline append_to_oss(std::ostringstream& oss, OA::ULongLong val)
{
  oss << val;
}

#endif // _OCPI_COMPONENT_PROP_TYPE_HELPERS_H

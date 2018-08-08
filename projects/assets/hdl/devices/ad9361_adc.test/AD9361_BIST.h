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

#ifndef _AD9361_BIST_H
#define _AD9361_BIST_H
#include <cstdio> // needed for fgetc, feof, ferror, etc.
#include <cerrno> // needed for errno
#include <vector>
#include "OcpiApi.h"
#include "util.h" // for GETBIT, ...

//#define DISABLE_LOG // uncomment to remove logging code

#define ADC_BIT_WIDTH 12

#define PRINT_LFSR_HISTORIC_TABLE(v, t, p) print_LFSR_historic_table(v,#v,t,p)

enum non_errno_failure_t {ERROR_NONE,
                          ERROR_FILE_WAS_EMPTY,
                          ERROR_FILE_ENDED_IN_MIDDLE_OF_AN_IQ_SAMPLE};

namespace OA = OCPI::API;

/* @returns POSIX error code (positive integer)
 ******************************************************************************/
inline int read_iqstream_sample_from_file(FILE*& fd, uint16_t& I, uint16_t& Q,
                                          bool& got_eof,
                                          bool& read_partial_sample,
                                          size_t& num_total_bits_read_from_file,
                                          const bool log_debug=false)
{
  got_eof = false;
  read_partial_sample = false;
  I = 0;
  Q = 0;

#define SAMP_BYTE_LENGTH 4
  static char c_array[SAMP_BYTE_LENGTH];
  static char* pc;
  static char c;
  pc = &(c_array[0]);

  for(int ii=0; ii<SAMP_BYTE_LENGTH; ii++)
  {
    if((fd == NULL) || (ferror(fd)))
    {
#if 0 // EBADFD is not portable, not POSIX
      return EBADFD;
#else
      return EBADF;
#endif
    }
    c = fgetc(fd);
    if(feof(fd))
    {
      got_eof = true;
      read_partial_sample = (ii != 0);
      return 0;
    }
    num_total_bits_read_from_file += 8;
    *pc++ = c;
  } 

  I  = (c_array[0] & 0xff);
  I |= (c_array[1] << 8  );
  Q  = (c_array[2] & 0xff);
  Q |= (c_array[3] << 8  );

  if(log_debug)
  {
#ifndef DISABLE_LOG
    static size_t sample_count = 0;
    std::cout << "DEBUG: read sample " << sample_count <<", I=d'" << I <<
                 ",\tQ=d'" << Q << ",\tI=b'";
    sample_count++;
    for(int ii=SAMP_BYTE_LENGTH/2*8-1; ii>=0; ii--)
    {
      std::cout << GETBIT(ii,I);
    }
    std::cout << ",\tQ=b'";
    for(int ii=SAMP_BYTE_LENGTH/2*8-1; ii>=0; ii--)
    {
      std::cout << GETBIT(ii,Q);
    }
    std::cout << std::endl;
#endif
  }

  return 0;
}

inline bool do_skip_for_reg_sync(long long& reg_sync_val,
                                 const uint16_t& I, const uint16_t& Q,
                                 const bool& log_debug)
{
  bool do_skip = (reg_sync_val != -1) &&
                 (I != (uint16_t)(reg_sync_val & 0x0000ffff)) &&
                 (Q != (uint16_t)((reg_sync_val & 0xffff0000) >> 16));
  // when !do_skip - sync either just occured or was already disabled so
  // now disabled it forever
  reg_sync_val = do_skip ? reg_sync_val : -1;
  if(do_skip)
  {
    if(log_debug)
    {
#ifndef DISABLE_LOG
      std::cout << "DEBUG: skipping I value of " << I << std::endl;
      std::cout << "DEBUG: skipping Q value of " << Q << std::endl;
#endif
    }
  }
  return do_skip;
}

void print_LFSR_historic_table(const std::vector<uint16_t>& table,
                               const std::string& var_name,
                               long t = -1, bool print_debug = false)
{
  std::cout << (print_debug ? "DEBUG: " : "");
  std::cout << var_name << " = [";
  std::vector<uint16_t> _table = table; // only because I wanted to keep
                                        // reference passed in a const
  long idx = 0;
  for(std::vector<uint16_t>::iterator it=_table.begin(); it != _table.end();
      ++it)
  {
    const uint16_t row = *it;
    for(int ii=15; ii>=0; ii--)
    {
      std::cout << GETBIT(ii,row) << " ";
    }
    if((t >= 0) && (t == idx))
    {
      std::cout << " <--- t";
    }
    std::cout << std::endl;
    std::cout << (print_debug ? "DEBUG: " : "");
    std::cout << "                   ";
    idx++;
  }
  std::cout << "]" << std::endl;
}

/* @brief This function is intended to be executed once for every bit shifted
 *        through the PRBS LFSR that was received.
 *****************************************************************************/
void calculate_BER_for_LFSR_bit(const std::vector<uint16_t>& tableI,
                                const std::vector<uint16_t>& tableQ,
                                const bool& started_on_last_table_row,
                                size_t& num_processed_bits_in_file,
                                size_t& cumulative_num_bit_errors,
                                bool& last_bit_done,
                                const bool log_debug=false)
{
  static size_t num_rows_with_data = 1;
  static size_t idx_min = 0;
  static size_t start_row = 0;

  size_t num_ones_I = 0;
  size_t num_ones_Q = 0;

  if(started_on_last_table_row)
  {
    start_row = (start_row == 0) ? ADC_BIT_WIDTH-1 : start_row-1;
  }
  num_rows_with_data -= started_on_last_table_row ? 1 : 0;
  size_t row = start_row;
  size_t num_rows_processed = 0;
  idx_min += started_on_last_table_row ? 1 : 0;
  size_t idx_max = idx_min + num_rows_with_data - 1;
  idx_max = (idx_max > ADC_BIT_WIDTH-1) ? ADC_BIT_WIDTH-1 : idx_max;
  for(size_t idx = idx_min; idx <= idx_max; idx++)
  {
    num_ones_I += GETBIT(idx,                 tableI[row]);
    num_processed_bits_in_file++;

    num_ones_Q += GETBIT(ADC_BIT_WIDTH-1-idx, tableQ[row]);
    num_processed_bits_in_file++;

    num_rows_processed++;
    row = (row == 0) ? ADC_BIT_WIDTH-1 : row-1; // handles row jump
  }

  const size_t num_zeros_I = num_rows_processed-num_ones_I;
  const size_t num_zeros_Q = num_rows_processed-num_ones_Q;

  const size_t num_bit_errors_I = std::min(num_ones_I, num_zeros_I);
  const size_t num_bit_errors_Q = std::min(num_ones_Q, num_zeros_Q);
  cumulative_num_bit_errors += num_bit_errors_I + num_bit_errors_Q;

  num_rows_with_data += (num_rows_with_data == ADC_BIT_WIDTH) ? 0 : 1;
  start_row++;
  start_row %= ADC_BIT_WIDTH; // handles row jump
  last_bit_done = (idx_min >= ADC_BIT_WIDTH-1);

  if(log_debug)
  {
#ifndef DISABLE_LOG
    static size_t LFSR_bit = 0;
    std::cout << "DEBUG: LFSR I bit " << LFSR_bit << " was received " <<
                 num_rows_processed << " times, " << num_bit_errors_I <<
                 " of which had bit errors, and most likely had a value of " <<
                 ((num_ones_I > num_zeros_I) ? "1" : "0") << std::endl;
    std::cout << "DEBUG: LFSR Q bit " << LFSR_bit << " was received " <<
                 num_rows_processed << " times, " << num_bit_errors_Q <<
                 " of which had bit errors, and most likely had a value of " <<
                 ((num_ones_Q > num_zeros_Q) ? "1" : "0") << std::endl;
    LFSR_bit++;
#endif
  }
}

/* @brief This function verifies the BIST PRBS functionality of the 
 *        ad9361_config_proxy / ad9361_adc workers by reading a file containing
 *        raw I/Q samples and calculating the Bit Error Rate (BER).
 ******************************************************************************/
int calculate_BIST_PRBS_RX_BER(const std::string& rx_filename_str,
                               double& bit_error_rate,
                               size_t& num_LFSR_bits,
                               size_t& num_total_bits_in_file,
                               size_t& num_processed_bits_in_file,
                               const long long max_num_samps_to_process,
                               long long reg_sync_val,
                               non_errno_failure_t& non_errno_failure,
                               bool log_debug = false)
{
  // initialization
  num_LFSR_bits = 0;
  num_total_bits_in_file = 0;
  num_processed_bits_in_file = 0;
  non_errno_failure = ERROR_NONE;
  bit_error_rate = -1;
  std::vector<uint16_t> LFSR_I_historic; // index is in time direction
  std::vector<uint16_t> LFSR_Q_historic; // index is in time direction
  size_t cumulative_num_bit_errors = 0;
  LFSR_I_historic.assign(ADC_BIT_WIDTH, 0);
  LFSR_Q_historic.assign(ADC_BIT_WIDTH, 0);

  // open file
  if(log_debug)
  {
#ifndef DISABLE_LOG
    std::cout << "DEBUG: attempting to open file with following path: " <<
                 rx_filename_str << std::endl;
#endif
  }
  FILE * fd = fopen(rx_filename_str.c_str(), "r");
  if(fd == NULL)
  {
    return errno;
  }

  // loop to read samples and process
  bool last_table_row_done = false;
  bool last_bit_done = false;
  size_t t = 0;
  bool no_num_samps_limit = (max_num_samps_to_process == 0);
  long long n = max_num_samps_to_process;
  do
  {
    // read sample
    uint16_t I;
    uint16_t Q;
    bool got_eof;
    bool read_partial_sample;
    do
    {
      int ret_errno = read_iqstream_sample_from_file(fd, I, Q,
          got_eof, read_partial_sample, num_total_bits_in_file, log_debug);
      if(ret_errno != 0)
      {
        goto exit;
      }
      if(got_eof && (!read_partial_sample)) // i.e. got eof on first char
      {
        if(n == max_num_samps_to_process) // i.e. first loop execution
        {
          non_errno_failure = ERROR_FILE_WAS_EMPTY;
          goto exit;
        }
        else
        {
          last_table_row_done = true;
        }
      }
      else if(read_partial_sample)
      {
        non_errno_failure = ERROR_FILE_ENDED_IN_MIDDLE_OF_AN_IQ_SAMPLE;
        goto exit;
      }
    } while(do_skip_for_reg_sync(reg_sync_val, I, Q,log_debug) && (!got_eof));

    if(!(got_eof && (!read_partial_sample)))
    {
      // insert sample into tables for processing
      LFSR_I_historic[t] = I & 0x7fff;
      LFSR_Q_historic[t] = Q & 0x7fff;
      if(log_debug)
      {
  #ifndef DISABLE_LOG
        PRINT_LFSR_HISTORIC_TABLE(LFSR_I_historic, (long)t, true);
        PRINT_LFSR_HISTORIC_TABLE(LFSR_Q_historic, (long)t, true);
  #endif
      }
      t++;
      t %= ADC_BIT_WIDTH;
    }

    // process and perform calculation
    do
    {
      calculate_BER_for_LFSR_bit(LFSR_I_historic, LFSR_Q_historic,
                                 last_table_row_done,
                                 num_processed_bits_in_file,
                                 cumulative_num_bit_errors,
                                 last_bit_done, log_debug);
      num_LFSR_bits++;
    } while (last_table_row_done && (!last_bit_done));
    n--;
  } while(((n != 0) || no_num_samps_limit) & (!last_bit_done));
  bit_error_rate = ((double)cumulative_num_bit_errors)/
                   ((double)num_processed_bits_in_file);

  exit:
    if(fd != NULL)
    {
      fclose(fd);
    }
    return errno;
}

#endif


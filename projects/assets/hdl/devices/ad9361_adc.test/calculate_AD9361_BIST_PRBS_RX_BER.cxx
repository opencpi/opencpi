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

#include <stdlib.h> // needed for strtol
#include "AD9361_BIST.h"
#include <cstring> // needed for strerror()

void print_usage(char* argv0)
{
  std::cout << "usage: " << argv0 << " <filename> <optional-debug> <optional-max-num-bits_shifted_through_LFSR-to-process> <optional-reg-sync-value>"
            << std::endl
            << "  e.g. " << argv0 << " app.out" << std::endl
            << "  e.g. " << argv0 << " app.out 1" << std::endl
            << "  e.g. " << argv0 << " app.out 0 1024" << std::endl
            << "  e.g. " << argv0 << " app.out 0 1024 4294967295 (this will not start processing input file contents until a register value of d'4294967295 (0xffffffff) is observed)";
  std::cout << std::endl;
  std::cout << "report:" << std::endl <<
    "  number_of_processed_bits_shifted_through_LFSR <#>-note a single LFSR bit"
    << std::endl <<
    "                                                    will exist in file up"
    << std::endl <<
    "                                                    to 12 times [due to"
    << std::endl <<
    "                                                    ADC width of 12])"
    << std::endl <<
    "  number_of_total_bits_in_file <#> ---------------- this is expected to"
    << std::endl <<
    "                                                    match filesize"
    << std::endl <<
    "  number_of_processed_bits_in_file <#> ------------ when"
    << std::endl <<
    "                                                    number_of_processed_bits_shifted_through_LFSR"
    << std::endl <<
    "                                                    is NOT specified, this is"
    << std::endl <<
    "                                                    expected to equal "
    << std::endl <<
    "                                                    filesize*12/16"
    << std::endl <<
    "                                                    [because 4 sign"
    << std::endl <<
    "                                                    extension bits are"
    << std::endl <<
    "                                                    ignored, i.e. not"
    << std::endl <<
    "                                                    processed]"
    << std::endl <<
    "  estimated_number_of_bit_errors <#> -------------  number of processed"
    << std::endl <<
    "                                                    bits with errors"
    << std::endl <<
    "  BER <#> ----------------------------------------- calculated as "
    << std::endl <<
    "                                       estimated_number_of_bit_errors/"
    << std::endl <<
    "                                       number_of_processed_bits_in_file)"
    << std::endl;
}

int main(int argc, char **argv)
{
  
  if(argc < 2)
  {
    print_usage(argv[0]);
    return 1;
  }

  const std::string rx_filename_str(argv[1]);
  long long max_num_samps_to_process = 0;
  bool log_debug = false;
  long long reg_sync_val = -1;
  if(argc >= 3)
  {
    long log_debug_long = strtol(argv[2], NULL, 0);
    if((log_debug_long != 0L) & (log_debug_long != 1L))
    {
      print_usage(argv[0]);
    }
    log_debug = (log_debug_long == 1L);
#ifdef DISABLE_LOG
    if(log_debug)
    {
      std::cout << "WARNING: debug logging requested but was excluded "
                << "during compilation" << std::endl;
    }
#endif
  }
  if(argc >= 4)
  {
    max_num_samps_to_process = strtoll(argv[3], NULL, 0);
    if(errno == ERANGE)
    {
      print_error(strerror(errno), false);
      std::cout << " (max_num_samps_to_process was " << argv[3] << ")";
      std::cout << std::endl;
      return -errno;
    }
    else if(max_num_samps_to_process <= 0)
    {
      print_error("num samples to process was either <= 0 or an invalid string", false);
      std::cout << " (max_num_samps_to_process was " << argv[3] << ")";
      std::cout << std::endl;
      std::cout << std::endl;
      print_usage(argv[0]);
      return 2;
    }
  }
  if(argc == 5)
  {
    reg_sync_val = strtoll(argv[4], NULL, 0);
  }

  double estimated_BER;
  size_t num_LFSR_bits;
  size_t num_total_bits_in_file;
  size_t num_processed_bits_in_file;
  non_errno_failure_t non_errno_failure;
  int ret_errno = calculate_BIST_PRBS_RX_BER(rx_filename_str, estimated_BER,
                                             num_LFSR_bits,
                                             num_total_bits_in_file,
                                             num_processed_bits_in_file,
                                             max_num_samps_to_process,
                                             reg_sync_val, non_errno_failure,
                                             log_debug);
  if(ret_errno != 0)
  {
    print_error(strerror(ret_errno), false);
    if((ret_errno == ENOENT) || (ret_errno == EISDIR))
    {
      std::cout << " (attempted to open filename: '" << rx_filename_str << "')";
    }
    std::cout << std::endl;
    std::cout << std::endl;
  }
  else if(non_errno_failure == ERROR_FILE_WAS_EMPTY)
  {
    if(reg_sync_val == -1)
    {
      print_error("file was empty", false);
      std::cout << " (file read was " << rx_filename_str << ")";
      std::cout << std::endl;
    }
    else
    {
      print_error("reg sync val could not be found", false);
      std::cout << " (file read was " << rx_filename_str << ")";
      std::cout << std::endl;
    }
  }
  else if(non_errno_failure == ERROR_FILE_ENDED_IN_MIDDLE_OF_AN_IQ_SAMPLE)
  {
    print_error("file ended in the middle of an I/Q sample", false);
    std::cout << " (file read was " << rx_filename_str << ")";
    std::cout << std::endl;
  }
 
  if((ret_errno == 0) && (non_errno_failure == ERROR_NONE))
  {
    const size_t num_bit_errors = (size_t)(((double)num_processed_bits_in_file)*
                                  estimated_BER);
    std::cout << "filename : " << rx_filename_str << std::endl;
    std::cout << "number_of_processed_bits_shifted_through_LFSR : " << num_LFSR_bits << std::endl;
    std::cout << "number_of_total_bits_in_file : " << num_total_bits_in_file << std::endl;
    std::cout << "number_of_processed_bits_in_file : " << num_processed_bits_in_file << std::endl;
    std::cout << "estimated_number_of_bit_errors : " << num_bit_errors << std::endl;
    std::cout << "estimated_BER : " << (estimated_BER*100.) << "%" << std::endl;
  }

  return -ret_errno;
}


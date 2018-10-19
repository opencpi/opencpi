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

#include <iostream> // std::cerr
#include <cstdlib> // system(), exit(), EXIT_SUCCESS, EXIT_FAILURE
#include <string> // std::string

std::string cmd_str;

void print_first_arg_error_msg() {

  std::cerr << "first arg must be lowercase a or b indicating HSMC slot\n";
}

void print_second_arg_error_msg() {

  std::string str("second arg (optional) must be lowercase p, which forces ");
  str += "prompting for LEDs\n";
  std::cerr << str;
}

/*! @brief Will exit application if args are improperly formatted.
 *****************************************************************************/
void parse_args(int argc, char **argv) {

  cmd_str.assign("./scripts/test_hsmc_debug_header_breakout_is_plugged_");

  if(argc < 2) {
    print_first_arg_error_msg();
    exit(EXIT_FAILURE);
  }

  if(argc > 3) {
    std::cerr << "too many args\n";
    print_first_arg_error_msg();
    print_second_arg_error_msg();
    exit(EXIT_FAILURE);
  }

  if(*argv[1] == 'a') {
    cmd_str += "into_port_a.sh";
  }
  else if(*argv[1] == 'b') {
    cmd_str += "into_port_b.sh";
  }
  else {
    print_first_arg_error_msg();
    exit(EXIT_FAILURE);
  }

  if(argc == 3) {
    if(*argv[2] == 'p') {
      cmd_str += " p";
    }
    else {
      print_second_arg_error_msg();
      exit(EXIT_FAILURE);
    }
  }
}

/*! @return Return value indicates completion (0 indicates completion, 1
 *          indicates no completion), not whether or not the test was
 *          successful. Test success is determined by visually inspect LEDs
 *          while executing (also inspect stdout while executing).
 *****************************************************************************/
int main(int argc, char **argv) {

  parse_args(argc, argv);

  int ret = system(cmd_str.c_str());

  if(WIFEXITED(ret)) {
    if(WEXITSTATUS(ret) != 0) {
      return EXIT_FAILURE;
    }
  }
  else {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

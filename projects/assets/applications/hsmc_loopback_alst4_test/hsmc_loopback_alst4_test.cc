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

#include <cstdlib> // system(), exit(), EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cout, std::cerr

#define EXIT_COULD_NOT_COMPLETE_ 100

void exit_if_bad_ret(int ret, bool failure_indicates_test_incomplete = true) {

  if(WIFEXITED(ret)) {
    if(WEXITSTATUS(ret) != 0) {
      if(failure_indicates_test_incomplete) {
        exit(EXIT_COULD_NOT_COMPLETE_);
      }
      else {
        std::cerr << "FAILED\n";
        exit(EXIT_FAILURE);
      }
    }
  }
  else {
    if(failure_indicates_test_incomplete) {
      exit(EXIT_COULD_NOT_COMPLETE_);
    }
    else {
      std::cerr << "FAILED\n";
      exit(EXIT_FAILURE);
    }
  }
}

/*! @return Return value indicates success (0 indicates success, 1
 *          indicates failure, 100 indicates test unexpectedly could not
 *          complete).
 *****************************************************************************/
int main() {

  exit_if_bad_ret(system("rm -rf odata/stdout.bin"));
  exit_if_bad_ret(system("mkdir -p odata"));

  std::string cmd_str("set -o pipefail; ./scripts/test.sh ");
  cmd_str += "| tee odata/stdout.bin";
  exit_if_bad_ret(system(cmd_str.c_str()));

  int ret = system("diff idata/stdout.golden.bin odata/stdout.bin");
  exit_if_bad_ret(ret, false);

  std::cout << "PASSED\n";
  return EXIT_SUCCESS;
}

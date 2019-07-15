#!/bin/bash
# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

set -e
if [ -n "$HDL_TEST_PLATFORM" ]; then
   sims=$HDL_TEST_PLATFORM
else
   sims=(`ocpirun -C --only-platforms | grep '.*-.*xsim' | sed s/^.*-//`)
   [ -z "$sims" ] && {
       echo This test requires a simulator for building, and there are none so we are skipping it.
       exit 0
  }
  echo Available simulators are: ${sims[*]}, using $sims.
  export HDL_TEST_PLATFORM=$sims
fi
echo Using sim platform: $sims

fail() {
  echo "Did not receive an error running this test: this command should not work"
  exit 1
}

OCPIDEV="$OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin/ocpidev"

$OCPIDEV build worker two_in_four_out.hdl -d ../../av-test --hdl-platform $HDL_TEST_PLATFORM
$OCPIDEV build test two_in_four_out.test -d ../../av-test --hdl-platform $HDL_TEST_PLATFORM
$OCPIDEV run test two_in_four_out.test -d ../../av-test
$OCPIDEV clean test two_in_four_out.test -d ../../av-test
$OCPIDEV clean worker two_in_four_out.hdl -d ../../av-test

$OCPIDEV build worker optional_output.hdl -d ../../av-test --hdl-platform $HDL_TEST_PLATFORM
$OCPIDEV build test optional_output.test -d ../../av-test --hdl-platform $HDL_TEST_PLATFORM
$OCPIDEV run test optional_output.test -d ../../av-test
$OCPIDEV clean test optional_output.test -d ../../av-test
$OCPIDEV clean worker optional_output.hdl -d ../../av-test

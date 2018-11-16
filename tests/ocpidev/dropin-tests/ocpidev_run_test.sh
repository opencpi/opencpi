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
   sims=(`ocpirun -C --only-platforms | grep '.*-.*sim' | sed s/^.*-//`)
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

# OCPIDEV="coverage3 run --append $OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin/ocpidev_run.py -d ../../av-test"
OCPIDEV="$OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin/ocpidev run -d ../../av-test"

$OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin/ocpidev build -d ../../av-test --hdl-platform $HDL_TEST_PLATFORM
echo "Running Test 1"
$OCPIDEV
echo "Running Test 2"
$OCPIDEV tests
echo "Running Test 3"
$OCPIDEV applications
echo "Running Test 4"
$OCPIDEV application aci_property_test_app
echo "Running Test 5"
$OCPIDEV test test_worker
echo "Running Test 6"
$OCPIDEV library components
echo "Running Test 7"
$OCPIDEV test test_worker.test -l components
echo "Running Test 8"
$OCPIDEV test test_worker --mode clean_all
echo "Running Test 9"
$OCPIDEV test test_worker --mode gen_build
echo "Running Test 10"
$OCPIDEV test test_worker --mode prep_run_verify
echo "Running Test 11"
$OCPIDEV test test_worker --mode clean_all
echo "Running Test 12"
$OCPIDEV test test_worker --mode gen_build --case '0.*'
echo "Running Test 13"
$OCPIDEV test test_worker --mode prep
echo "Running Test 14"
$OCPIDEV test test_worker --mode run -v --accumulate-errors --keep-simulations
echo "Running Test 15"
$OCPIDEV test test_worker --mode verify
echo "Running Test 16"
$OCPIDEV test test_worker --mode clean_sim
echo "Running Test 17"
$OCPIDEV test test_worker --mode clean_run
echo "Running Test 18"
$OCPIDEV test test_worker.test --hdl-platform $HDL_TEST_PLATFORM
echo "Running Test 19"
$OCPIDEV test test_worker --mode clean_all
$OCPIDEV test test_worker.test --only-platform $HDL_TEST_PLATFORM
echo "Running Test 20"
$OCPIDEV test test_worker --mode clean_all
$OCPIDEV test test_worker.test --exclude-platform $HDL_TEST_PLATFORM
# need to add things we expect to fail to this test as well
set +e
echo "Running Test 21: Expecting Error"
$OCPIDEV junk test_worker.test && fail
echo "Running Test 22: Expecting Error"
$OCPIDEV test test_worker -l components/dsp_comps && fail
echo "Running Test 23: Expecting Error"
$OCPIDEV test -l components/dsp_comps && fail
echo "Running Test 24: Expecting Error"
$OCPIDEV project --junk && fail
echo "Tests Passed!"

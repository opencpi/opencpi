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
if [ "" == "$OCPIDEV_UTILIZATION_TEST" ]; then
   echo This test requires OCPIDEV_UTILIZATION_TEST to be set and have non-simulator hdl so we are skipping it.
   exit 0
fi

if [ "" == "$HDL_TEST_PLATFORM" ]; then
   HDL_TEST_PLATFORM=zed
fi

echo Using hdl platform: $HDL_TEST_PLATFORM

fail() {
  echo "Did not receive an error running this test: this command should not work"
  exit 1
}

# OCPIDEV="coverage3 run --append $OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin/ocpidev_run.py -d ../../av-test"
OCPIDEV="$OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin/ocpidev utilization -d ../../av-test"

$OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin/ocpidev build -d ../../av-test --hdl-platform $HDL_TEST_PLATFORM
echo "Running Test 1"
$OCPIDEV
echo "Running Test 2"
$OCPIDEV workers
echo "Running Test 3"
$OCPIDEV worker test_worker.hdl
echo "Running Test 4"
$OCPIDEV library components
echo "Running Test 5"
$OCPIDEV project
echo "Running Test 6"
$OCPIDEV hdl platforms
echo "Running Test 7"
$OCPIDEV hdl assemblies
echo "Running Test 8"
$OCPIDEV hdl assembly test_assy
echo "Running Test 9"
$OCPIDEV worker test_worker.hdl --library components --format latex
echo "Running Test 10"
$OCPIDEV worker test_worker.hdl --hdl-platform $HDL_TEST_PLATFORM

set +e
echo "Running Fail Test 1: Expecting Error"
$OCPIDEV junk test_worker.hdl && fail
echo "Running Fail Test 2: Expecting Error"
$OCPIDEV worker  test_worker.hdl -l components/dsp_comps && fail
echo "Running Fail Test 3: Expecting Error"
$OCPIDEV worker -l components/dsp_comps && fail
echo "Running Fail Test 4: Expecting Error"
$OCPIDEV project --junk && fail
echo "Running Fail Test 1: Expecting Error"
$OCPIDEV worker test_worker && fail
echo "Tests Passed!"

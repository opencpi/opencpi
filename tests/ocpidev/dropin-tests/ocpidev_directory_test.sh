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
fail() {
  echo "Did not receive an error running this test: this command should not work"
  exit 1
}

ex_ocpidev(){
  $OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin/ocpidev $VERB -d $OCPI_WD $1
}

OCPIDEV="$OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin/ocpidev $VERB -d $OCPI_WD"
ocpidev create project temp_proj
cd temp_proj
ocpidev create library sub_lib
ocpidev create component -l sub_lib temp_comp
ocpidev create worker -l sub_lib temp_comp.rcc
ocpidev create worker -l sub_lib temp_comp.hdl
ocpidev create test -l sub_lib temp_comp.test
ocpidev create component -h devices dev_comp
ocpidev create worker -h devices dev_comp.rcc
ocpidev create worker -h devices dev_comp.hdl
ocpidev create test -h devices dev_comp.test
ocpidev create hdl platform temp_plat
ocpidev create component -P temp_plat plat_comp
ocpidev create worker -P temp_plat plat_comp.rcc
ocpidev create worker -P temp_plat plat_comp.hdl
ocpidev create test -P temp_plat plat_comp.test
ocpidev create hdl assembly temp_assy
cd ..

# need to add things we expect to fail to this test as well
set +e
VERB="run"
OCPI_WD="../../av-test"
#application
echo "Running Failure Test 1"
ex_ocpidev "application --hdl-libary adapters multislave_test" && fail
echo "Running Failure Test 2"
ex_ocpidev "application" && fail
OCPI_WD="../../av-tes/components"
echo "Running Failure Test 3"
ex_ocpidev "application multislave_test" && fail

#applications
VERB="run"
OCPI_WD="../../av-test"
echo "Running Failure Test 4"
ex_ocpidev "applications --hdl-libary adapters" && fail
OCPI_WD="../../av-tes/components"
echo "Running Failure Test 5"
ex_ocpidev "applications" && fail

#assembly
VERB="utilization"
OCPI_WD="../../av-test"
echo "Running Failure Test 6"
ex_ocpidev "assembly --hdl-libary adapters param_test_assy" && fail
echo "Running Failure Test 7"
ex_ocpidev "assembly" && fail
OCPI_WD="../../av-tes/components"
echo "Running Failure Test 8"
ex_ocpidev "assembly param_test_assy" && fail

#assemblies
VERB="utilization"
OCPI_WD="../../av-test"
echo "Running Failure Test 9"
ex_ocpidev "assemblies --hdl-libary adapters" && fail
echo "Running Failure Test 10"
ex_ocpidev "assemblies param_test_assy" && fail
OCPI_WD="../../av-tes/components"
echo "Running Failure Test 11"
ex_ocpidev "assemblies" && fail

#library/libraries
VERB="run"
OCPI_WD="../../av-test"
echo "Running Failure Test 12"
ex_ocpidev "library -l components --hdl-libary adapters -P my_plat" && fail
echo "Running Failure Test 13"
ex_ocpidev "library -l components my_lib_name" && fail
echo "Running Failure Test 14"
ex_ocpidev "library" && fail
OCPI_WD="temp_proj"
echo "Running Failure Test 15"
ex_ocpidev "library -l components --hdl-libary adapters -P my_plat" && fail
echo "Running Failure Test 16"
ex_ocpidev "library -l components my_lib_name" && fail
echo "Running Failure Test 17"
ex_ocpidev "library" && fail

#hdl platform
VERB="show"
OCPI_WD="temp_proj"
echo "Running Failure Test 18"
ex_ocpidev "hdl platform --hdl-library adapters temp_plat" && fail
echo "Running Failure Test 19"
ex_ocpidev "hdl platform" && fail
OCPI_WD="temp_proj/components"
echo "Running Failure Test 20"
ex_ocpidev "hdl platform temp_plat" && fail

#hdl platforms
VERB="utilization"
OCPI_WD="temp_proj"
echo "Running Failure Test 21"
ex_ocpidev "hdl platforms --hdl-library adapters" && fail
echo "Running Failure Test 22"
ex_ocpidev "hdl platform" && fail
OCPI_WD="temp_proj/components"
echo "Running Failure Test 23"
ex_ocpidev "hdl platform temp_plat" && fail

#test
VERB="run"
OCPI_WD="../../av-test"
echo "Running Failure Test 24"
ex_ocpidev "test -l components --hdl-libary adapters -P my_plat test_worker" && fail
echo "Running Failure Test 25"
ex_ocpidev "test -l components my_lib_name test_worker" && fail
echo "Running Failure Test 26"
ex_ocpidev "test" && fail
OCPI_WD="../../av-test/components/test_worker.hdl"
echo "Running Failure Test 27"
ex_ocpidev "test test_worker" && fail
OCPI_WD="temp_proj"
echo "Running Failure Test 28"
ex_ocpidev "test temp_comp" && fail

#worker
VERB="show"
OCPI_WD="../../av-test"
echo "Running Failure Test 29"
ex_ocpidev "worker -l components --hdl-libary adapters -P my_plat test_worker.hdl" && fail
echo "Running Failure Test 30"
ex_ocpidev "worker -l components my_lib_name test_worker.hdl" && fail
echo "Running Failure Test 31"
ex_ocpidev "worker" && fail
OCPI_WD="../../av-test/components/test_worker.test"
echo "Running Failure Test 32"
ex_ocpidev "worker test_worker" && fail
OCPI_WD="temp_proj"
echo "Running Failure Test 33"
ex_ocpidev "worker temp_comp.hdl" && fail

#component
VERB="show"
OCPI_WD="../../av-test"
echo "Running Failure Test 34"
ex_ocpidev "component -l components --hdl-libary adapters -P my_plat test_worker.hdl" && fail
echo "Running Failure Test 35"
ex_ocpidev "component -l components my_lib_name test_worker.hdl" && fail
echo "Running Failure Test 36"
ex_ocpidev "component" && fail
OCPI_WD="../../av-test/components/test_worker.test"
echo "Running Failure Test 37"
ex_ocpidev "component test_worker" && fail

set -e

VERB="run"
OCPI_WD="."
echo "Running Test 1"
ex_ocpidev "project temp_proj"
echo "Running Test 2"
OCPI_WD="../../av-test/components"
ex_ocpidev ""
echo "Running Test 3"
ex_ocpidev "library"
echo "Running Test 4"
ex_ocpidev "tests"
echo "Running Test 5"
ex_ocpidev "test unit_tester.test"
echo "Running Test 6"
OCPI_WD="../../av-test"
ex_ocpidev "library components"
OCPI_WD="temp_proj"
ex_ocpidev "library components"
OCPI_WD="../../av-test/applications"
echo "Running Test 7"
ex_ocpidev ""
echo "Running Test 8"
ex_ocpidev "application multislave_test"
echo "Running Test 9"
ex_ocpidev "applications"
OCPI_WD="../../av-test/applications/multislave_test"
echo "Running Test 10"
ex_ocpidev ""
echo "Running Test 11"
ex_ocpidev "application"
ex_ocpidev "application multislave_test"
OCPI_WD="temp_proj/components"
echo "Running Test 12"
ex_ocpidev ""
echo "Running Test 13.1"
ex_ocpidev "library sub_lib"
echo "Running Test 13.2"
ex_ocpidev "library"
echo "Running Test 13.3"
ex_ocpidev "library components"
echo "Running Test 14"
ex_ocpidev "tests"
echo "Running Test 15"
ex_ocpidev "-l sub_lib test temp_comp"
OCPI_WD="temp_proj/components/sub_lib"
echo "Running Test 16"
ex_ocpidev ""
echo "Running Test 17"
ex_ocpidev "library sub_lib"
ex_ocpidev "library"
echo "Running Test 18"
ex_ocpidev "tests"
echo "Running Test 19.1"
ex_ocpidev "test temp_comp"
echo "Running Test 19.2"
ex_ocpidev "test temp_comp.test"
echo "Running Test ???.1"

VERB="show"
OCPI_WD="."
echo "Running Test 20"
ex_ocpidev "project temp_proj"
echo "Running Test 21"
ex_ocpidev "platforms"
echo "Running Test 22"
ex_ocpidev "projects"
echo "Running Test 23"
ex_ocpidev "registry"
echo "Running Test 24"
ex_ocpidev "components"
echo "Running Test 25"
ex_ocpidev "targets"
echo "Running Test 26"
ex_ocpidev "rcc targets"
echo "Running Test 27"
ex_ocpidev "hdl targets"
echo "Running Test 28"
ex_ocpidev "rcc platforms"
echo "Running Test 29"
ex_ocpidev "hdl platforms"
echo "Running Test 30"
ex_ocpidev "workers"
OCPI_WD="temp_proj"
echo "Running Test 31"
ex_ocpidev ""
OCPI_WD="temp_proj/components"
echo "Running Test 32"
ex_ocpidev "-l sub_lib components temp_comp"
echo "Running Test 33"
ex_ocpidev "-l sub_lib worker temp_comp.rcc"
OCPI_WD="../../av-test/components"
echo "Running Test 34.1"
ex_ocpidev "component test_worker"
echo "Running Test 34.2"
ex_ocpidev "component test_worker-spec.xml"
echo "Running Test 34."
ex_ocpidev "component test_worker-spec"
echo "Running Test 35"
ex_ocpidev "worker test_worker.rcc"
OCPI_WD="temp_proj/components/sub_lib"
echo "Running Test 36"
ex_ocpidev "component temp_comp"
echo "Running Test 37"
ex_ocpidev "worker temp_comp.rcc"
OCPI_WD="temp_proj/components/sub_lib/temp_comp.rcc"
echo "Running Test 38"
ex_ocpidev ""
echo "Running Test 39"
ex_ocpidev "worker"
ex_ocpidev "worker temp_comp.rcc"

VERB="utilization"
echo "Running Test 40"
OCPI_WD="../../av-test"
ex_ocpidev "library components"
OCPI_WD="temp_proj"
ex_ocpidev "library components"
echo "Running Test 41"
ex_ocpidev ""
echo "Running Test 42"
ex_ocpidev "hdl platform temp_plat"
OCPI_WD="temp_proj/components"
echo "Running Test 43"
ex_ocpidev ""
echo "Running Test 44"
ex_ocpidev "library sub_lib"
echo "Running Test 45"
ex_ocpidev "-l sub_lib worker temp_comp.hdl"
echo "Running Test 46"
ex_ocpidev "workers"
OCPI_WD="../../av-test/components"
echo "Running Test 47"
ex_ocpidev ""
echo "Running Test 48"
ex_ocpidev "library"
ex_ocpidev "library components"
echo "Running Test 49"
ex_ocpidev "worker unit_tester.hdl"
echo "Running Test 50"
ex_ocpidev "workers"
OCPI_WD="temp_proj/components/sub_lib"
echo "Running Test 51"
ex_ocpidev ""
echo "Running Test 52"
ex_ocpidev "library"
ex_ocpidev "library sub_lib"
echo "Running Test 53"
ex_ocpidev "worker temp_comp.hdl"
echo "Running Test 54"
ex_ocpidev "workers"
OCPI_WD="temp_proj/components/sub_lib/temp_comp.hdl"
echo "Running Test 55"
ex_ocpidev ""
echo "Running Test 56"
ex_ocpidev "worker"
ex_ocpidev "worker temp_comp.hdl"
OCPI_WD="temp_proj/hdl/platforms/temp_plat"
echo "Running Test 57"
ex_ocpidev ""
echo "Running Test 58"
ex_ocpidev "worker plat_comp.hdl"
echo "Running Test 60"
ex_ocpidev "hdl platform"
ex_ocpidev "hdl platform temp_plat"
OCPI_WD="temp_proj/hdl/platforms"
echo "Running Test 61"
ex_ocpidev ""
echo "Running Test 63"
ex_ocpidev "hdl platform temp_plat"
echo "Running Test 64"
ex_ocpidev "hdl platforms"
OCPI_WD="temp_proj/hdl/assemblies/temp_assy"
echo "Running Test 65"
ex_ocpidev ""
echo "Running Test 66"
ex_ocpidev "hdl assembly"
ex_ocpidev "hdl assembly temp_assy"
OCPI_WD="temp_proj/hdl/assemblies"
echo "Running Test 67"
ex_ocpidev ""
echo "Running Test 68"
ex_ocpidev "hdl assembly temp_assy"
echo "Running Test 69"
ex_ocpidev "hdl assemblies"
OCPI_WD="."
echo "Running Test 70"
ex_ocpidev "project temp_proj"

ocpidev delete -f project temp_proj

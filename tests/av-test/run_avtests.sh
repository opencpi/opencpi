#!/bin/bash --noprofile
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

##########################################################################################
# Run the av-tests, assuming a clean tree
set -e
[ -z "$OCPI_TARGET_PLATFORM" ] && {
  # Set just enough target variables to run runtime tests
  export OCPI_TARGET_PLATFORM=$OCPI_TOOL_PLATFORM
  export OCPI_TARGET_OS=$OCPI_TOOL_OS
  export OCPI_TARGET_DIR=$OCPI_TOOL_DIR
}
shopt -s expand_aliases
alias odev="$OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin/ocpidev -v"
echo Cleaning the project
make cleaneverything
echo Building the components
odev build rcc
echo Building the mulislave_test application
odev build application mulislave_test
echo Running the mulislave_test application
(cd applications/mulislave_test &&
  OCPI_LIBRARY_PATH=../../:$OCPI_LIBRARY_PATH ./target-$OCPI_TARGET_DIR/mulislave_test)
echo Building the aci_property_test_app application
odev build application aci_property_test_app
echo Running the aci_property_test_app application
(cd applications/aci_property_test_app &&
  OCPI_LIBRARY_PATH=../../:$OCPI_LIBRARY_PATH ./target-$OCPI_TARGET_DIR/test_app)
cd components
odev build worker prop_mem_align_info.rcc
odev build test prop_mem_align_info.test

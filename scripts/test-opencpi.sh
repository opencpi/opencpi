#!/bin/sh
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

# Run all the go-no-go tests we have
set -e
if test "$OCPI_CDK_DIR" != ""; then
  echo Since OCPI_CDK_DIR is set, we will use the existing environment.
else
  # We're being run in an uninitialized environment
  if test ! -d env; then
    echo It appears that this script is not being run at the top level of OpenCPI.
    exit 1
  fi
fi
OCPI_BOOTSTRAP=`pwd`/exports/scripts/ocpibootstrap.sh; . $OCPI_BOOTSTRAP
test $? = 0 || exit 1; 
source $OCPI_CDK_DIR/scripts/util.sh
echo ======================= Loading the OpenCPI Linux Kernel driver. &&
(test "$(ocpiGetToolOS)" = macos || $OCPI_CDK_DIR/scripts/ocpidriver load) &&
echo ======================= Running Unit Tests &&
tests/target-$OCPI_TOOL_DIR/ocpitests &&
echo ======================= Running Datatype/protocol Tests &&
tools/cdk/ocpidds/target-$OCPI_TOOL_DIR/ocpidds -t 10000 > /dev/null &&
echo ======================= Running Container Tests &&
(cd runtime/ctests/target-$OCPI_TOOL_DIR && ${OCPI_TOOL_MODE:+../}../src/run_tests.sh) &&
echo All tests passed.

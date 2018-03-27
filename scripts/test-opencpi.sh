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
  echo Use \"unset OCPI_CDK_DIR\" to cause this script to initialize the environment for OpenCPI.
  OCPI_BOOTSTRAP=$OCPI_CDK_DIR/scripts/ocpibootstrap.sh
  . $OCPI_BOOTSTRAP
  if test -n "$1" -a "$1" != $OCPI_TOOL_PLATFORM; then
    echo Skipping testing since we are cross-building for $1 on $OCPI_TOOL_PLATFORM.
    exit 0
  fi      
else
  # We're being run in an uninitialized environment
  if test ! -d scripts; then
    echo It appears that this script is not being run at the top level of OpenCPI.
    exit 1
  fi
  OCPI_BOOTSTRAP=`pwd`/exports/scripts/ocpibootstrap.sh
  . $OCPI_BOOTSTRAP
fi
test $? = 0 || exit 1; 
source $OCPI_CDK_DIR/scripts/util.sh
[ "$OCPI_TOOL_OS" != macos ] && {
  echo ======================= Loading the OpenCPI Linux Kernel driver. &&
    $OCPI_CDK_DIR/scripts/ocpidriver load
}
bin=$OCPI_CDK_DIR/bin/$OCPI_TOOL_DIR
echo ======================= Running Unit Tests &&
$bin/ocpitests &&
echo ======================= Running Datatype/protocol Tests &&
$bin/ocpidds -t 10000 > /dev/null &&
echo ======================= Running Container Tests &&
$bin/ctests/run_tests.sh &&
echo ======================= Running unit tests in project/core &&
make -C $OCPI_CDK_DIR/../projects/core runtest &&
echo ======================= Running Application tests in project/assets &&
make -C $OCPI_CDK_DIR/../projects/assets/applications run &&
echo ======================= Running Application tests in project/assets &&
make -C $OCPI_CDK_DIR/../projects/inactive/applications run &&
echo All tests passed.

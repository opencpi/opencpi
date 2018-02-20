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

# Run all the go/no-go tests we have
set -e
if test -z "$OCPI_CDK_DIR"; then
  echo Since OCPI_CDK_DIR is not set, refusing to continue
  exit 1
fi

# echo ======================= Loading the OpenCPI Linux Kernel driver. &&
# (test "$OCPI_TOOL_OS" = macos || ocpidriver load) &&

echo ======================= Running Unit Tests &&
ocpitests &&
echo ======================= Running Datatype/protocol Tests &&
ocpidds -t 100000 > /dev/null &&
echo ======================= Running Container Tests &&
($OCPI_CDK_DIR/bin/$(${OCPI_CDK_DIR}/platforms/getPlatform.sh | awk '{print $4}')/ctests/run_tests.sh) &&
echo All tests passed.

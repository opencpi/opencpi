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
# Ensure exports
source ./scripts/core-init.sh
# Ensure CDK and TOOL variables
OCPI_BOOTSTRAP=`pwd`/exports/scripts/ocpibootstrap.sh; source $OCPI_BOOTSTRAP
# Ensure TARGET variables
source tools/cdk/scripts/ocpitarget.sh "$1"

echo Installing all prerequisites for platform: $OCPI_TARGET_PLATFORM
if test -d /opt/opencpi; then
  echo The /opt/opencpi directory is already created.
else
  echo We will create /opt/opencpi and make it read/write for everyone
  sudo mkdir -p /opt/opencpi
  sudo chmod a+rwx /opt/opencpi
  mkdir -p /opt/opencpi/prerequisites
fi
if test $OCPI_TOOL_PLATFORM = $OCPI_TARGET_PLATFORM; then
  echo ================================================================================
  echo Installing the standard packages for $OCPI_TOOL_PLATFORM.
  projects/core/rcc/platforms/$OCPI_TOOL_PLATFORM/$OCPI_TOOL_PLATFORM-packages.sh
  echo ================================================================================
  echo All basic prerequisites are installed in the system.
  echo ================================================================================
  echo Installing the patchelf utility under /opt/opencpi/prerequisites
  scripts/install-patchelf.sh
fi
echo ================================================================================
echo We are running in `pwd` where the git clone of opencpi has been placed.
echo Next, before building OpenCPI, we will install some prerequisites in /opt/opencpi.
echo ================================================================================
echo Installing Google test '(gtest)' under /opt/opencpi/prerequisites
scripts/install-gtest.sh
echo ================================================================================
echo Installing the LZMA compression library '(lzma)' under /opt/opencpi/prerequisites
scripts/install-lzma.sh
echo ================================================================================
echo Installing the GMP numeric library '(gmp)' under /opt/opencpi/prerequisites
scripts/install-gmp.sh
echo ================================================================================
echo Installing the ad9361 library under /opt/opencpi/prerequisites
scripts/install-ad9361.sh
echo ================================================================================
echo All OpenCPI prerequisites have been installed.

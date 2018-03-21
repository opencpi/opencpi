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
source scripts/setup-prereq-dirs.sh
echo ================================================================================
echo We are running in `pwd` where the git clone of opencpi has been placed.
echo We will build non-standard or cross-compiled prerequisites in $OCPI_PREREQUISITES_BUILD_DIR.
echo We will install them in $OCPI_PREREQUISITES_INSTALL_DIR.
echo Standard development packages installed on development hosts will be normal global installs.
if test $OCPI_TOOL_PLATFORM = $OCPI_TARGET_PLATFORM; then
  echo ================================================================================
  echo Installing the standard packages for $OCPI_TOOL_PLATFORM.
  # FIXME - this assumes dev platforms are all in the core project - bad
  projects/core/rcc/platforms/$OCPI_TOOL_PLATFORM/$OCPI_TOOL_PLATFORM-packages.sh
  echo All standard prerequisites are now installed in the system.
  echo ================================================================================
  echo Installing the patchelf utility under $OCPI_PREREQUISITES_INSTALL_DIR
  echo This package is only used for development hosts and is never cross compiled.
  scripts/install-patchelf.sh
fi
echo ================================================================================
echo Installing Google test '(gtest)' under $OCPI_PREREQUISITES_INSTALL_DIR/gtest
scripts/install-gtest.sh
echo ================================================================================
echo Installing the LZMA compression library '(lzma)' under $OCPI_PREREQUISITES_INSTALL_DIR/lzma
scripts/install-lzma.sh
echo ================================================================================
echo Installing the GMP numeric library '(gmp)' under $OCPI_PREREQUISITES_INSTALL_DIR/gmp
scripts/install-gmp.sh
echo ================================================================================
echo Installing the ad9361 library under $OCPI_PREREQUISITES_INSTALL_DIR/ad9361
scripts/install-ad9361.sh
echo ================================================================================
echo All OpenCPI prerequisites have been installed.

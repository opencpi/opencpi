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

#############################################################################################
# This script installs any installable-from-network-repository software packages that OpenCPI
# depends on for a development platform.  It calls a file specific to the actual rcc/platform for
# finding out which commands (e.g. yum vs apt-get) to issue.

set -e
# Ensure exports (or cdk) exists and has scripts
source ./scripts/init-opencpi.sh
# Ensure CDK and TOOL variables
OCPI_BOOTSTRAP=`pwd`/cdk/scripts/ocpibootstrap.sh; source $OCPI_BOOTSTRAP
# Ensure TARGET variables
source cdk/scripts/ocpitarget.sh "$1"
echo We will now globally install any packages from package repositories for the $OCPI_TARGET_PLATFORM platform.
if test "$OCPI_TOOL_PLATFORM" = "$OCPI_TARGET_PLATFORM"; then
  script=$OCPI_TOOL_PLATFORM_DIR/$OCPI_TOOL_PLATFORM-packages.sh
else
  script=$OCPI_TARGET_PLATFORM_DIR/$OCPI_TOOL_PLATFORM=$OCPI_TARGET_PLATFORM-packages.sh
fi
if [ ! -f $script ]; then
  echo "Since there is no $OCPI_TARGET_PLATFORM-packages.sh script for $OCPI_TARGET_PLATFORM, no packages will be installed."
  exit 0
fi
echo "Installing the packages for building $OCPI_TARGET_PLATFORM (when running on $OCPI_TOOL_PLATFORM)..."
bash $script
echo All required standard repository packages are now installed for building $OCPI_TARGET_PLATFORM.

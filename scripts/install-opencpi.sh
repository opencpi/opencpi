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

# Just check if it looks like we are in the source tree.
[ -d runtime -a -d build -a -d scripts -a -d tools ] || {
  echo "Error:  this script ($0) is not being run from the top level of the OpenCPI source tree."
  exit 1
}
set -e
# We do some bootstrapping here (that is also done in the scripts we call), in order to know
# whether the platform we are building

# Ensure exports (or cdk) exists and has scripts
source ./scripts/init-opencpi.sh
# Ensure CDK and TOOL variables
OCPI_BOOTSTRAP=`pwd`/cdk/scripts/ocpibootstrap.sh; source $OCPI_BOOTSTRAP
if test "$OCPI_TOOL_PLATFORM" != "$1"; then
  ./scripts/install-packages.sh $OCPI_TOOL_PLATFORM
  # This should check if a successful prereq install has been done
  ./scripts/install-prerequisites.sh $OCPI_TOOL_PLATFORM
fi
# Allow this to build for platforms defined in the inactive project
[ -z "$OCPI_PROJECT_PATH" ] && export OCPI_PROJECT_PATH=`pwd`/projects/inactive
./scripts/install-packages.sh $1
./scripts/install-prerequisites.sh $1
./scripts/build-opencpi.sh $1
./scripts/test-opencpi.sh $1

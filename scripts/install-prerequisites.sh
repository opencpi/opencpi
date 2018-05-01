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

#############################################################################################
# This script builds and installs any prerequisite software packages required by OpenCPI
# These are packages that are not generally/easily available for all platforms from
# network-based package repositories.
set -e
# Ensure exports
source ./scripts/init-opencpi.sh
# Ensure CDK and TOOL variables
source ./cdk/opencpi-setup.sh -e
# Ensure TARGET variables
source $OCPI_CDK_DIR/scripts/ocpitarget.sh "$1"
source $OCPI_CDK_DIR/scripts/setup-prereq-dirs.sh
# Get the prerequisites list
topprereqs=$(sed -n 's/^ *prerequisites *\(.*\) *$/\1/p' build/places)
[ -n "$topprereqs" ] || {
  echo Cannot get prerequisites from the build/places files.
  exit 1
}
echo Building/installing prerequisites for the $OCPI_TARGET_PLATFORM platform, now running on $OCPI_TOOL_PLATFORM.
echo Building prerequisites in $OCPI_PREREQUISITES_BUILD_DIR.
echo Installing them in $OCPI_PREREQUISITES_INSTALL_DIR.
if [ -n "$OCPI_TARGET_PREREQUISITES" ]; then
  echo -------------------------------------------------------------------------------------------
  echo "Before building/installing common OpenCPI prerequisites, there are $OCPI_TARGET_PLATFORM-specific ones: $OCPI_TARGET_PREREQUISITES"
  # First build the target-specific script for the TOOL platform

  for p in $OCPI_TARGET_PREREQUISITES; do
    read preq tool <<<$(echo $p | tr : ' ')
    if [ -z "$tool" -o "$tool" = $OCPI_TOOL_PLATFORM ]; then
      echo --- Building/installing the $OCPI_TARGET_PLATFORM-specific prerequisite $preq for executing on $OCPI_TOOL_PLATFORM.
      script=$OCPI_TARGET_PLATFORM_DIR/install-$preq.sh
      [ -x $script ] || (echo No executable installation script found in $script. && exit 1)
      (for e in $(env | grep OCPI_TARGET_|sed 's/=.*//'); do unset $e; done &&
        $script $OCPI_TOOL_PLATFORM)
    fi
    if [ -z "$tools" ]; then
      echo --- Building/installing $OCPI_TARGET_PLATFORM-specific prerequisites for executing on $OCPI_TARGET_PLATFORM.
      $OCPI_TARGET_PLATFORM_DIR/install-$preq.sh $OCPI_TARGET_PLATFORM
    fi
  done
fi
echo -------------------------------------------------------------------------------------------
echo "Now installing the generic (for all platforms) prerequisites for $OCPI_TARGET_PLATFORM."
for p in $topprereqs; do
  echo -------------------------------------------------------------------------------------------
  script=scripts/install-$p.sh
  if [ -x $script ] ; then
    $script $1
  else
    echo "The installation script for the $p ($script) is missing or not executable." >&2
  fi
done
echo -------------------------------------------------------------------------------------------
echo All these OpenCPI prerequisites have been successfully installed for $OCPI_TARGET_PLATFORM: $top_prereqs

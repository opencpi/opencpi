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
# This script builds and installs any prerequisite software packages required by OpenCPI
# These are packages that are not generally/easily available for all platforms from
# network-based package repositories.
set -e
[ "$1" = -f ] && force=1 && shift
# Ensure exports
source ./scripts/init-opencpi.sh
# Ensure CDK and TOOL variables
source ./cdk/opencpi-setup.sh -e
# Ensure TARGET variables
source $OCPI_CDK_DIR/scripts/ocpitarget.sh "$1"
source $OCPI_CDK_DIR/scripts/setup-prereq-dirs.sh
# arg is where a directory of prerequisite directories might be found
function add_prereq_dirs {
  shopt -s nullglob
  for p in $1/prerequisites/*; do
    [[ -d $p && $p != *.hold && -x $p/install-$(basename $p).sh ]] && topprereqs+=" $p"
  done
}
add_prereq_dirs build
for p in project-registry/*; do
  add_prereq_dirs $p
done
timestamp=$OCPI_PREREQUISITES_INSTALL_DIR/built-timestamp-$OCPI_TARGET_PLATFORM
if [ -f $timestamp ]; then
  echo It appears that prerequisites were successfully built for $OCPI_TARGET_PLATFORM on $(< $timestamp).
  if [ -n "$force" ]; then
    echo "Since the -f option (force) was supplied, we will rebuild anyway."
    rm $timestamp
  else
    echo "So we will skip building prerequisites for platform: $OCPI_TARGET_PLATFORM."
    echo "Use the -f option to $0 to force rebuilding prerequisite."
    exit 0
  fi
fi
echo Building/installing prerequisites for the $OCPI_TARGET_PLATFORM platform, now running on $OCPI_TOOL_PLATFORM.
echo Building prerequisites in $OCPI_PREREQUISITES_BUILD_DIR.
echo Installing them in $OCPI_PREREQUISITES_INSTALL_DIR.
if [ -n "$OCPI_TARGET_PREREQUISITES" ]; then
  echo -------------------------------------------------------------------------------------------
  echo "Before building/installing common OpenCPI prerequisites, there are $OCPI_TARGET_PLATFORM-specific ones: $OCPI_TARGET_PREREQUISITES"
  # First build the target-specific script for the TOOL platform

  for p in $OCPI_TARGET_PREREQUISITES; do
    read preq tool <<<$(echo $p | tr : ' ')
    script=$OCPI_TARGET_PLATFORM_DIR/install-$preq.sh
    [ -x $script ] || (echo No executable installation script found in $script. && exit 1)
    if [ -z "$tool" -o "$tool" = $OCPI_TOOL_PLATFORM ]; then
      echo --- Building/installing the $OCPI_TARGET_PLATFORM-specific prerequisite $preq for executing on $OCPI_TOOL_PLATFORM.
      (for e in $(env | grep OCPI_TARGET_|sed 's/=.*//'); do unset $e; done &&
        $script $OCPI_TOOL_PLATFORM)
    fi
    if [ -z "$tool" -o "$tool" = $OCPI_TARGET_PLATFORM ]; then
      echo --- Building/installing the $OCPI_TARGET_PLATFORM-specific prerequisite $preq for executing on $OCPI_TARGET_PLATFORM.
      $script $OCPI_TARGET_PLATFORM
    fi
    if [ -n "$tool" -a "$tool" != $OCPI_TOOL_PLATFORM -a "$tool" != $OCPI_TARGET_PLATFORM ] ; then
      echo --- Skipping the prequisite \"$p\" for $OCPI_TARGET_PLATFORM since we are running on $tool.
    fi
  done
fi
echo -------------------------------------------------------------------------------------------
echo "Now installing the generic (for all platforms) prerequisites for $OCPI_TARGET_PLATFORM."
for p in $topprereqs; do
  echo -------------------------------------------------------------------------------------------
  script=$p/install-$(basename $p).sh
  if [ -x $script ] ; then
    $script $1
  else
    echo "The installation script for the $p ($script) is missing or not executable." >&2
  fi
done
echo -------------------------------------------------------------------------------------------
echo All these OpenCPI prerequisites have been successfully installed for $OCPI_TARGET_PLATFORM:
printf "    "
for p in $topprereqs; do printf " "$(basename $p); done
echo
# Record that this was successfully built.  Very poor man's "make".
date > $timestamp

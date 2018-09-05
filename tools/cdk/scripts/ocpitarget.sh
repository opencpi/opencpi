# No /bin/bash here - this file should be sourced, not executed

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

################################################################################################
# Set up the SHELL environment to build for a single target platform.
# Usage is not RCC workers, not ACI programs, and not framework libraries or executables,
# but it is still used by:
# - Building and cleaning drivers
# - Creating RPMs
# - createOpenCPIZynqSD
# - gdbForZynq
# - install-packages
# - install-prerequisites
# - test-opencpi.sh
# - setup-install
# - ocpirh_export.sh
#
# So we are basically setting the OCPI_TARGET* environment variables
# Extract the target-related variables from the make context for use in the shell context.
# This file must be sourced since its purpose is to change the environment
# This is rarely needed since these variables are almost always used in the "make" context,
# where the initialization is done using setup-target-platform.mk
# A single argument is required which will become OCPI_TARGET_PLATFORM.
# If the single argument is empty, and OCPI_TARGET_PLATFORM is already set, it is used.
# If the single argument is empty and OCPI_TARGET_PLATFORM is not set, it will be set
# it as OCPI_TOOL_PLATFORM.
# Just some extra error checking
if [ -n "$OCPI_TARGET_PLATFORM" ]; then
  if [ -n "$1" ]; then
    if  [ "$1" != $OCPI_TARGET_PLATFORM ]; then
      echo Error:  ocpitarget.sh called with \"$1\" when OCPI_TARGET_PLATFORM already set to \"$OCPI_TARGET_PLATFORM\".
      exit 1
    fi
  elif [ $OCPI_TARGET_PLATFORM != $OCPI_TOOL_PLATFORM ]; then
    echo Error:  ocpitarget.sh called with no target when OCPI_TARGET_PLATFORM already set to \"$OCPI_TARGET_PLATFORM\".
    exit 1
  fi
elif [ -n "$1" ] ; then
  # OCPI_TARGET_PLATFORM reflects the argument which might be empty
  plat=(${1//-/ })
  export OCPI_TARGET_PLATFORM=$plat
  export OCPI_TARGET_DIR=$1  
else
  export OCPI_TARGET_PLATFORM=$OCPI_TOOL_PLATFORM
  export OCPI_TARGET_DIR=$OCPI_TOOL_DIR  
fi
# In bootstrap/early mode we just need to know the target platform's directory, not all
# the details, and we don't want to rely on "make"
[ -n "$2" ] && {
  if [ $OCPI_TARGET_PLATFORM = $OCPI_TOOL_PLATFORM ]; then
    export OCPI_TARGET_PLATFORM_DIR=$OCPI_TOOL_PLATFORM_DIR
  else    
    read v0 v1 v2 v3 v4 v5 <<< `$OCPI_CDK_DIR/scripts/getPlatform.sh $OCPI_TARGET_PLATFORM`
    [ -d "$v5" ] || {
      echo Error:  Cannot find the platform directory for platform $OCPI_TARGET_PLATFORM
      exit 1
    }
    export OCPI_TARGET_PLATFORM_DIR=$v5
  fi
  return 0
}
# Ensure we are really starting fresh for this target
unset `env | grep OCPI_TARGET | egrep -v 'OCPI_TARGET_(PLATFORM|DIR|KERNEL_DIR)' | sed 's/=.*//'`
source $OCPI_CDK_DIR/scripts/util.sh
setVarsFromMake $OCPI_CDK_DIR/include/setup-target-platform.mk ShellTargetVars=1 -v

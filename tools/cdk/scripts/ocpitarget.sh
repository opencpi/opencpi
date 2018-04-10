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
# Extract the target-related variables from the make context for use in the shell context.
# This file must be sourced since its purpose is to change the environment
# This is rarely needed since these variables are almost always used in the "make" context,
# where the initialization is done using ocpisetup.mk.
# A single argument is required which will become OCPI_TARGET_PLATFORM.
# If the single argument is empty, and OCPI_TARGET_PLATFORM is already set, it is used.
# If the single argument is empty and OCPI_TARGET_PLATFORM is not set, ocpisetup.mk will set
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
else
  # OCPI_TARGET_PLATFORM reflects the argument which might be empty
  export OCPI_TARGET_PLATFORM=$1
fi

# Ensure we are really starting fresh for this target
unset `env | grep OCPI_TARGET | grep -v OCPI_TARGET_PLATFORM | sed 's/=.*//'`

source $OCPI_CDK_DIR/scripts/util.sh
# Remove this until we figure out why it is here.
# platform searches already look in the right places...
#OCPI_PROJECT_PATH=`getProjectPathAndRegistered` \
#
setVarsFromMake $OCPI_CDK_DIR/include/ocpisetup.mk ShellTargetVars=1 -v

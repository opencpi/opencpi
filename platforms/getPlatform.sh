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

# This script determines the runtime platform and target variables
# The four variables are: OS OSVersion Processor Platform
# If a platform is added, this script should be updated to recognize it
# If it returns nothing (""), that is an error
# It depends on being located in the directory where platforms live

isCurPlatform()
{
  vars=($(sh $1-check.sh $HostSystem $HostProcessor))
  if test ${#vars[@]} = 3; then
    echo ${vars[@]}  ${vars[0]}-${vars[1]}-${vars[2]} $p
    exit 0
  fi
}

# These are universally available so far so we do this once and pass then to all probes.
HostSystem=`uname -s | tr A-Z a-z`
HostProcessor=`uname -m | tr A-Z a-z`

# Collect all known projects. Append with the default read-only core project
# in case this is a limited runtime-only system with no project registry
if [ -n "$OCPI_CDK_DIR" -a -e "$OCPI_CDK_DIR/scripts/util.sh" ]; then
  source $OCPI_CDK_DIR/scripts/util.sh
  projects="`getProjectPathAndRegistered`"
elif [ -n "$OCPI_PROJECT_PATH" ]; then
  # If the CDK is not set or util.sh does not exist, fall back on OCPI_PROJECT_PATH
  projects=$OCPI_PROJECT_PATH
elif [ -d projects ]; then
  # Probably running in a clean source tree
  projects=$(echo projects/*)
fi

if [ -n "$OCPI_CDK_DIR" ]; then
  projects="$projects:$OCPI_CDK_DIR/../projects/core"
else
  projects="$projects:/opt/opencpi/projects/core"
fi

# loop through all projects to determine right platform by calling isCurPlatform for each platform
shopt -s nullglob
for j in $(echo $projects | tr : ' '); do
  # First, assume this is an exported project check lib/rcc...
  # Next, assume this is a source project that is exported and check exports/lib/rcc,
  # Finally, just search the source rcc/platforms...
  if test -d $j/lib/rcc/platforms; then
    platforms_dir=$j/lib/rcc/platforms
  elif test -d $j/exports/lib/rcc/platforms; then
    platforms_dir=$j/exports/lib/rcc/platforms
  else
    platforms_dir=$j/rcc/platforms
  fi
  for i in $platforms_dir/*; do
    # For each existing platform directory, if the *-check.sh script exists,
    # call isCurPlatform to run that *-check.sh script and determine the platform/target
    if test -d $i; then
      p=$(basename $i)
      if test -f $i/$p-check.sh; then
        isCurPlatform $i/$p
      fi
    fi
  done
done

echo Cannot determine platform we are running on.  1>&2
exit 1

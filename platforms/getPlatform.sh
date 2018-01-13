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
    [ -n "$1" ] && echo succeeded.  Target is ${vars[0]}-${vars[1]}-${vars[2]}. 1>&2
    echo ${vars[@]}  ${vars[0]}-${vars[1]}-${vars[2]} $p
    exit 0
  fi
}

# These are universally available so far so we do this once and pass then to all probes.
HostSystem=`uname -s | tr A-Z a-z`
HostProcessor=`uname -m | tr A-Z a-z`
# Each recognizable platform has a script <platform>-check.sh
# If there is a file platform-list, then we look for them in that order.
# Otherwise, we just look in alphabetical order
mydir=$(dirname $0)

if [ -n "$OCPI_CDK_DIR" -a -e "$OCPI_CDK_DIR/scripts/util.sh" ]; then
  source $OCPI_CDK_DIR/scripts/util.sh
  projects=`getProjectPathAndRegistered`
else
  projects=$OCPI_PROJECT_PATH
fi
# loop through all options to determine right platform by calling isCurPlatform for each platform
for j in $(echo $projects | tr : ' '); do
  for i in $j/rcc/platforms/*; do
    if test -d $i; then
      p=$(basename $i)
      if test -f $i/$p-check.sh; then
        isCurPlatform $i/$p
      fi
    fi
  done
done
for i in $mydir/*; do
  if test -d $i; then
    p=$(basename $i)
    if test -f $i/$p-check.sh; then
      isCurPlatform $i/$p
    fi
  fi
done

echo Cannot determine platform we are running on.  1>&2
exit 1

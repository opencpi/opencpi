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

[ -z "$OCPI_CDK_DIR" ] && echo Environment variable OCPI_CDK_DIR not set && exit 1
source $OCPI_CDK_DIR/scripts/setup-prerequisite.sh \
       "$1" \
       myhostname \
       "library to inject a specific hostname by intercepting gethostname" \
       . \
       myhostname.c \
       . \
       0
# Only build/use this for centos for now
[[ "$OcpiPlatformOs" != linux || "$OcpiPlatformOsVersion" != c* ]] &&
  echo The myhostname package is not built for $OcpiPlatform, only CentOS*. Skipping it. &&
  exit 0
gcc -Wall -fPIC -shared ../myhostname.c -o libmyhostname.so
relative_link libmyhostname.so $OcpiInstallExecDir/lib

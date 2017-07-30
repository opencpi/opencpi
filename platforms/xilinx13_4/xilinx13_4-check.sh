#!/bin/sh --noprofile
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

if test "$1" = linux -a "$2" = armv7l -a -d /sys/class/xdevcfg; then
  # this "zynq" without further versioning is temporary until we add new os versions...
  dir=/mnt/card/opencpi/lib/*
  [ -d $dir ] && {
    osv=$(echo $(basename $dir) | sed 's/.*-\(.*\)-.*$/\1/')
    echo $1 $osv arm
    exit 0
  }
fi
exit 1



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

# Fix up the simulation executable by converting xilinx-related library dependencies to
# be relative, not absolute
echo doing $1
set -o pipefail
set -e
[ -x "$1" ] || ( echo $1 is not executable; exit 1 )
ldd $1 > /dev/null || ( echo The ldd command does not work; exit 1 )
libs=`ldd $1 | sed -n '/^[ 	]*\//s/ *(.*$//p'`
changes=
for l in $libs; do
  [[ $l = */ISE/* ]] && changes+=" --replace-needed $l $(basename $l)"
done    
[ -n "$changes" ] && {
    echo Fixing absolute xilinx library dependencies for: $1. Doing:
    echo $OCPI_PREREQUISITES_DIR/patchelf/$OCPI_TOOL_DIR/bin/patchelf $changes $1    
    $OCPI_PREREQUISITES_DIR/patchelf/$OCPI_TOOL_DIR/bin/patchelf $changes $1    
}
exit 0

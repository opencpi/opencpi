#!/bin/sh
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

set -e
if test "$1" = "" -o "$1" = "--help" -o "$1" = "-help"; then
  cat <<EOF
This script displays the history of the Xilinx Linux kernel source tree in chronological order
to show what repo tags are available for use in an OpenCPI Zynq release.  Normally you should
select one that corresponds to the version of Xilinx tools (actually the EDK) you are using.

This script has one argument, which is the directory in which the git repo has been placed.
Usually it is "git".
EOF
  exit 1
fi
if test ! -d $1 -o ! -d $1/linux-xlnx; then
  echo "The directory \"$1\" does not exist or does not contain the Xilinx source repo (linux-xlnx)." 1>&2
  exit 1
fi

cd $1/${2:-linux-xlnx}
git log --tags --simplify-by-decoration --pretty="format:%ai %d" |  sort -n | grep ') *$'

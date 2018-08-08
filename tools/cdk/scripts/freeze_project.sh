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

# This script freezes a project to allow it to be moved.
# If given an argument, uses that name as the basis for UNCOMPRESSED tarball (one level up).

set -e
set -o pipefail

fail() {
  echo $1
  false
}

if [ -n "$1" -a -e "../${1}.tar" ]; then
  fail "../${1}.tar already exists!"
fi

if [ ! -e Project.mk ]; then
  fail "This script should be run from the top level of an OpenCPI project!"
fi

if [ -z "${OCPI_CDK_DIR}" ]; then
  fail "OCPI_CDK_DIR is not set!"
fi

echo "Please wait..."
make cleanimports
printf "This project is now frozen!" "$(pwd)"

if [ -n "$1" ]; then
  tar cf ../${1}.tar .
  echo "../${1}.tar is an uncompressed tarball as well."
fi

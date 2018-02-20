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

# This script thaws a project that has been moved.
# If the project is a tarball, give two parameters:
#  scripts/thaw_project.sh base.tar ocpi_core
set -e
set -o pipefail

fail() {
  echo $1
  false
}

if [ -z "${OCPI_CDK_DIR}" ]; then
  fail "OCPI_CDK_DIR is not set!"
fi

if [ -n "$1" -a -z "$2" ]; then
  fail "Need two parameters for tarball mode."
fi

if [ -n "$1" -a -n "$2" ]; then
  if [ ! -f "$1" ]; then
    fail "$1 not found!"
  fi
  if [ -e "$2" ]; then
    fail "$2 already exists!"
  fi
  if [ "$(expr index "$2" ' ')" -ne 0 ]; then
    fail "Spaces in directory name will not work."
  fi
  echo "Extracting $1 to $2"
  mkdir "$2"
  cd "$2"
  tar xf ../$1
fi

if [ ! -e Project.mk ]; then
  fail "This script should be run from the top level of a frozen OpenCPI project!"
fi

echo "Please wait..."
make imports
echo "This project is now moved to '$(pwd)'."

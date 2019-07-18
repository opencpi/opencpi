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
# Set up the prereq install and build directories.
# This file is sourced either from install-prerequisites.sh or setup-prerequisite.sh
[ -z "$OCPI_PREREQUISITES_INSTALL_DIR" ] &&
    export OCPI_PREREQUISITES_INSTALL_DIR=$OCPI_PREREQUISITES_DIR
[ -z "$OCPI_PREREQUISITES_BUILD_DIR" ] && {
  export OCPI_PREREQUISITES_BUILD_DIR=$(dirname $OCPI_PREREQUISITES_INSTALL_DIR)/prerequisites-build
}

function ask {
  ans=''
  until [[ "$ans" == [yY] || "$ans" == [nN] ]]; do
    read -p "Are you sure you want to $* (y or n)? " ans
  done
  [[ "$ans" == [Nn] ]] && exit 1
  return 0
}

function checkdir {
  if [ ! -d $1 ]; then
    if mkdir -p $1; then
      echo Created $1 since it did not exist.
    else
      echo Could not create $1 or its parents without sudo.
      ask try to create $1 as root
      sudo mkdir -p $1 || exit 1
    fi
    echo "Do not delete this directory; there may be symlinks into it from elsewhere." > $1/README
  elif [ ! -w $1 ]; then
      echo You do not have permission for writing to $1.
      ask try to change permissions $1 as root
      sudo chmod a+w $1 || exit 1
  fi
}
checkdir $OCPI_PREREQUISITES_BUILD_DIR
checkdir $OCPI_PREREQUISITES_INSTALL_DIR

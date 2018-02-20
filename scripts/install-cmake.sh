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

OCPI_CMAKE_MAJOR_VERSION=3.7
OCPI_CMAKE_MINOR_VERSION=2
OCPI_CMAKE_VERSION=$OCPI_CMAKE_MAJOR_VERSION.$OCPI_CMAKE_MINOR_VERSION
dir=cmake-$OCPI_CMAKE_VERSION
source ./scripts/setup-install.sh \
       "$1" \
       cmake \
       $dir.tar.gz \
       https://cmake.org/files/v$OCPI_CMAKE_MAJOR_VERSION \
       $dir \
       0
tar xzf ../../$dir.tar.gz
mv $dir/* $dir/.??* .
./bootstrap --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/cmake/$OCPI_TARGET_DIR
make
make install

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

OCPI_GTEST_VERSION=1.7.0
[ -z "$OCPI_CDK_DIR" ] && echo Environment variable OCPI_CDK_DIR not set && exit 1
source $OCPI_CDK_DIR/scripts/setup-install.sh \
       "$1" \
       gtest \
       "Google C++ Test Library" \
       release-$OCPI_GTEST_VERSION.zip \
       https://github.com/google/googletest/archive \
       googletest-release-$OCPI_GTEST_VERSION \
       1
$CXX -fPIC -I../include -I.. -c ../src/gtest-all.cc
$AR -rs libgtest.a gtest-all.o
dname=libgtest.$OCPI_TARGET_DYNAMIC_SUFFIX
ldir=$OCPI_PREREQUISITES_INSTALL_DIR/gtest/$OCPI_TARGET_DIR/lib
iname=$ldir/$dname
[ "$OCPI_TARGET_OS" = macos ] && install_name="-install_name $iname"
$CXX $OCPI_TARGET_DYNAMIC_FLAGS $install_name -o $dname gtest-all.o
mkdir -p $ldir
relative_link `pwd`/libgtest.a $ldir
relative_link `pwd`/$dname $ldir
relative_link `cd ..;pwd`/include $OCPI_PREREQUISITES_INSTALL_DIR/gtest

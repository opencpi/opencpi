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

OCPI_GTEST_VERSION=1.8.0
[ -z "$OCPI_CDK_DIR" ] && echo Environment variable OCPI_CDK_DIR not set && exit 1
source $OCPI_CDK_DIR/scripts/setup-prerequisite.sh \
       "$1" \
       gtest \
       "Google C++ Test Library" \
       https://github.com/google/googletest/archive \
       release-$OCPI_GTEST_VERSION.zip \
       googletest-release-$OCPI_GTEST_VERSION \
       1
# Adjust things given that googletest is actually in a subdirectory here
base=$(basename `pwd`)
cd ..
rm -r -f googletest/$base
# Move the build directory into googletest
mv $base googletest
cd googletest
# Hopefully this is really one-time, platform-independent, but we're redoing it
# for each platform...
autoreconf -v -i && autoconf
cd $base
../configure ${cross_host+--host=$cross_host} \
	     --enable-static --prefix=$install_dir --libdir=$install_exec_dir
make lib/libgtest.la
relative_link lib/.libs/libgtest.a $install_exec_dir/lib
relative_link lib/.libs/libgtest$OCPI_TARGET_DYNAMIC_SUFFIX $install_exec_dir/lib
cd ..
relative_link include $install_dir
exit 0
# This was the previous non-auto-tools way
$CXX -fPIC -I../include -I.. -c ../src/gtest-all.cc
$AR -rs libgtest.a gtest-all.o
dname=libgtest$OCPI_TARGET_DYNAMIC_SUFFIX
ldir=$install_dir/lib
iname=$ldir/$dname
[ "$OCPI_TARGET_OS" = macos ] && install_name="-install_name $iname"
libs=($OCPI_TARGET_EXTRA_LIBS)
$CXX $OCPI_TARGET_DYNAMIC_FLAGS -o $dname gtest-all.o ${libs[@]/#/-l}
relative_link libgtest.a $ldir
relative_link $dname $ldir
cd ..
relative_link include $install_dir

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

gtest_version=1.8.0
[ -z "$OCPI_CDK_DIR" ] && echo Environment variable OCPI_CDK_DIR not set && exit 1
source $OCPI_CDK_DIR/scripts/setup-prerequisite.sh \
       "$1" \
       gtest \
       "Google C++ Test Library" \
       https://github.com/google/googletest/archive \
       release-$gtest_version.zip \
       googletest-release-$gtest_version \
       1
# According to their readme, they recommend simply compiling gtest-all.cc or using cmake
# The have older autotools stuff and declare that legacy and unmaintained.
# We use the first recommended way, which is SIMPLEST.  Their code does not use config.h etc.
dir=../googletest # srcdir
dynlib=libgtest$OcpiDynamicLibrarySuffix
$CXX -fPIC -I$dir/include -I$dir -c $dir/src/gtest-all.cc
$AR -rs libgtest.a gtest-all.o
$CXX $OcpiDynamicLibraryFlags -o$dynlib gtest-all.o -lpthread
relative_link $dir/include $OcpiInstallDir # each platform creates this same link
relative_link libgtest.a $OcpiInstallExecDir/lib
relative_link $dynlib $OcpiInstallExecDir/lib

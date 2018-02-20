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
OCPI_GTEST_VERSION=1.8.0
# set these for prereq_utils:
SPECFILE=gtest.spec
OCPI_BUNDLED_VERSION=${OCPI_GTEST_VERSION}
. ../prereq_utils.sh

find . -name 'googletest-*' -type d -print0 | xargs -r0 rm -rf
TARBALL=release-$OCPI_GTEST_VERSION.zip

prereq_init_tarball ${TARBALL} https://github.com/google/googletest/archive/${TARBALL}

# Parameters:
# 1: platform, e.g. arm-xilinx-linux-gnueabi
# 2: target host, e.g. linux-x13_3-arm
# 3: software platform, e.g. x13_3
# 4: RPM platform nice name, e.g. zynq
# 5: CFLAGS, e.g. "-O2 -g -pipe -Wall"
#    For the OCPI_CFLAGS, start with the ones in /usr/lib/rpm/redhat/macros for %__global_cflags and then take out
#    the ones that fail on the target platform.
#    Then add the ones that tune for the platform.
#    It goes through an "echo" to evaluate if things like ${CROSS_DIR} were passed in.
# 6-9: Any extra rpmbuild options, e.g. "--define=OCPI_AD9361_COMMIT_SHORT ${OCPI_AD9361_COMMIT_SHORT}"

if [ "$1" = "rpm" ]; then
    if [ -n "${CROSS_DIR}" ]; then
        echo "This script requires CROSS_DIR to set the platform as a suffix, e.g. CROSS_DIR_zynq"
        echo "Do not use CROSS_DIR without a suffix!"
        echo "Assuming you meant CROSS_DIR_zynq (which was previously correct)"
        export CROSS_DIR_zynq=${CROSS_DIR}
    fi
    if [ -e ${SPECFILE} ]; then
        rm -rf ~/rpmbuild/{BUILD,SOURCES}/ # Starting in 1.8.0, it's in BUILD/XXX/googletest and rpm doesn't clean XXX
        mkdir -p ~/rpmbuild/{BUILD,SOURCES}
        cp *.patch ${TARBALL} ~/rpmbuild/SOURCES
        skip_host || rpmbuild -ba ${SPECFILE} --define="OCPI_TARGET_HOST ${OCPI_TARGET_HOST}" --define="OCPI_BUNDLED_VERSION ${OCPI_BUNDLED_VERSION}"
        ### Cross compile for zynq
        cross_build arm-xilinx-linux-gnueabi linux-x13_3-arm x13_3 zynq "-O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions --param=ssp-buffer-size=4 -grecord-gcc-switches -mfpu=neon-fp16 -mfloat-abi=softfp -march=armv7-a -mtune=cortex-a9"
        ### Cross compile for picoflexor
        #   - Starting in 1.8.0, they used things that exposed differences in gcc versions between x13_3 and pico
        #   - The parameters are stupid because we want it to end up in "linux-zynq-arm_cs"
        cross_build arm-none-linux-gnueabi linux-zynq-arm_cs pico_t6a picoflexor "-O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions --param=ssp-buffer-size=4 -mfpu=neon-fp16 -mfloat-abi=softfp -march=armv7-a -mtune=cortex-a9"
        cp -v ~/rpmbuild/RPMS/*/ocpi-prereq-gtest* . || :
    else
        echo "Missing RPM spec file in `pwd`"
        exit 1
    fi
else
    echo Extracting...
    unzip -q ${TARBALL}
    cd googletest-release-$OCPI_BUNDLED_VERSION
    chmod a+w *
    patch -p1 < ../opencpi-gtest-$OCPI_BUNDLED_VERSION.patch
    autoreconf
    autoconf
    ./configure --enable-static --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/gtest --libdir=$OCPI_PREREQUISITES_INSTALL_DIR/gtest/OCPI_TARGET_HOST
    make -j
    make install
fi

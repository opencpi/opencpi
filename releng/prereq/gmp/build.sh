#!/bin/bash
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
OCPI_GMP_VERSION=6.1.2
# set these for prereq_utils:
SPECFILE=ocpi_gmp.spec
OCPI_BUNDLED_VERSION=${OCPI_GMP_VERSION}
. ../prereq_utils.sh

SUBDIR=gmp-$OCPI_GMP_VERSION
rm -rf $SUBDIR
TARBALL=$SUBDIR.tar.xz

prereq_init_tarball ${TARBALL} https://ftp.gnu.org/gnu/gmp/${TARBALL}

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
    if [ -e ${SPECFILE} ]; then
        mkdir -p ~/rpmbuild/SOURCES || :
        cp gmp{,-mparam}.h ${TARBALL} ~/rpmbuild/SOURCES
        skip_host || rpmbuild -ba ${SPECFILE} --define="OCPI_TARGET_HOST ${OCPI_TARGET_HOST}" --define="OCPI_BUNDLED_VERSION ${OCPI_BUNDLED_VERSION}"
        cross_build arm-xilinx-linux-gnueabi linux-x13_3-arm x13_3 zynq "-O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions --param=ssp-buffer-size=4 -grecord-gcc-switches -mfpu=neon-fp16 -mfloat-abi=softfp -march=armv7-a -mtune=cortex-a9"
        cp -v ~/rpmbuild/RPMS/*/ocpi-prereq-gmp* . || :
        rm -f *-debuginfo-*.rpm
    else
        echo "Missing RPM spec file in `pwd`"
        exit 1
    fi
else
    echo This script only builds RPMs. Try \"$0 rpm\".
    exit 99
fi

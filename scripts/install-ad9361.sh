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

################################################################################
# Import and prepare the ADI "no_OS" library for using the ad9361 with OpenCPI
# We build the C code into an external library to incorporate into proxies.
# We also derive the xml properties file from the ADI headers.
# The API headers need some tweaks to not introduce bad namespace pollution
# Priorities are:
#   Allow use of their API to program the device
#   Try not to touch their SW at all.
#   Enable repeated installation, refresh etc.
#   Support host and cross compilation
#   Be similar to all other such prereq/import/cross-compilations
################################################################################
# 1. Download/clone and setup directories in the prereq area
################################################################################
#OCPI_AD9361_VERSION=master
# As of 13 Oct 2017, 2016_R2 points to e9f8fe509cc0e3685cdca47998979f287be4c360 (AV-3478)
OCPI_AD9361_CURRENT_2016_R2_GIT_COMMIT_ID=e9f8fe509cc0e3685cdca47998979f287be4c360
OCPI_AD9361_VERSION=$OCPI_AD9361_CURRENT_2016_R2_GIT_COMMIT_ID
here=$(pwd)/scripts
if [ -z "${RPM_BUILD_ROOT}" ]; then
[ -z "$OCPI_CDK_DIR" ] && echo Environment variable OCPI_CDK_DIR not set && exit 1
source $OCPI_CDK_DIR/scripts/setup-install.sh \
       "$1" \
       ad9361 \
       "ADI ad9361 Convenience Library" \
       $OCPI_AD9361_VERSION \
       https://github.com/analogdevicesinc/no-OS.git \
       no-OS \
       1
else
# RPM building
set -e
OCPI_PREREQUISITES_INSTALL_DIR=.
fi
# End of no-RPM
cp -r ../ad9361/sw/* .

################################################################################
# 2. Patch their API headers so they actually act like API headers
#    I.e. the patched version doesn't pollute the caller's namespace
################################################################################
echo Patching API headers
dir=.
patch -p0 < $here/ad9361.patch

#################################################################################
# 3. Compile code into the library
################################################################################
# We are not depending on their IP
DEFS=-DAXI_ADC_NOT_PRESENT
SRCNAMES=(ad9361 ad9361_api ad9361_conv util)
SRCS=(${SRCNAMES[@]/%/.c})
INCS=(ad9361_api ad9361 config)
if [ -z "${RPM_BUILD_ROOT}" ]; then
echo $CC -std=c99 -fPIC -I. -I$dir/platform_generic -I$dir $DEFS -c ${SRCS[@]/#/$dir\/}
$CC -std=c99 -fPIC -I. -I$dir/platform_generic -I$dir $DEFS -c ${SRCS[@]/#/$dir\/}
$AR -rs libad9361.a ${SRCNAMES[@]/%/.o}

################################################################################
# 3. Install the deliverables:  OPS file, headers and library
################################################################################
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/ad9361/$OCPI_TARGET_DIR/lib
relative_link libad9361.a $OCPI_PREREQUISITES_INSTALL_DIR/ad9361/$OCPI_TARGET_DIR/lib
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/ad9361/include
for i in ${INCS[@]}; do
  relative_link $dir/$i.h $OCPI_PREREQUISITES_INSTALL_DIR/ad9361/include
done
else
# RPM Building
echo RPM CC: -fPIC -I. -I$dir/platform_generic -I$dir $DEFS -c ${SRCS[@]/#/$dir\/}
echo RPM AR: -rs libad9361.a ${SRCNAMES[@]/%/.o}
for i in ${INCS[@]}; do
  echo RPM INC: `pwd`/$dir/$i.h
done
fi

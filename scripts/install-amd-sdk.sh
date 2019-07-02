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

if [ "$UID" -eq 0 ]; then
  echo This script should not be run as root, since that would not be a sand-boxed installation.
  exit 1
fi
if test "$OCPI_TARGET_OS" != "linux"; then #linux-c6-x86_64"; then
 echo This script only works for Centos6 linux x86_64.
 exit 1
fi
OCPI_AMD_SDK_VERSION=3.0
OCPI_AMD_SDK_BUILD=130.135-GA
TARBALL=AMD-APP-SDKInstaller-v${OCPI_AMD_SDK_VERSION}.${OCPI_AMD_SDK_BUILD}-linux64.tar.bz2
INSTALLER=AMD-APP-SDK-v${OCPI_AMD_SDK_VERSION}.${OCPI_AMD_SDK_BUILD}-linux64.sh
AMD_SDK_INSTALL_DIR=$OCPI_PREREQUISITES_INSTALL_DIR/amd-sdk/$OCPI_TARGET_PLATFORM
. ./scripts/setup-install.sh
mkdir -p amd-sdk
cd amd-sdk
SUBDIR=amd-sdk-${OCPI_AMD_SDK_VERSION}
if test -f $TARBALL; then
  echo The distribution file for the AMD SDK, $TARBALL, exists and is being used.
  echo Remove `pwd`/$TARBALL afterwards if you want save space, and maybe download it again.
else
  echo You need to downloading the distribution file: $TARBALL
  echo It requires a click-through license.
  echo It is at http://developer.amd.com/tools-and-sdks/opencl-zone/amd-accelerated-parallel-processing-app-sdk
  echo Specify the ${OCPI_AMD_SDK_VERSION} version of the 64 bit linux SDK.
  echo Download the file and copy it to `pwd`/$TARBALL
  exit 1
fi
if test -d $SUBDIR; then
  echo The source directory $SUBDIR exists, using it for this target: $OCPI_TARGET_PLATFORM.
  cd $SUBDIR
else
  echo Unpacking download file $TARBALL into directory $SUBDIR.
  mkdir $SUBDIR
  cd $SUBDIR
  tar xjf ../$TARBALL
  if [ ! -x $INSTALLER ] ; then
    echo The tarball file $TARBALL did not contain the installer: $INSTALLER
    exit 1
  fi
  echo Running the self-executing tar file to extract all the installation files.
  ./$INSTALLER --noexec --target tmp
  rm $INSTALLER
  echo The installer files have been extracted into a tmp directory.
  (cd tmp; HOME=$OCPI_PREREQUISITES_INSTALL_DIR/amd-sdk/installed ./install.sh --silent --acceptEULA y)
  cd ..
  rm -r -f $SUBDIR
  rm -r -f $OCPI_TARGET_PLATFORM
  ln -s installed/AMDAPPSDK-$OCPI_AMD_SDK_VERSION $OCPI_TARGET_PLATFORM
fi
echo ============= amd-sdk for $OCPI_TARGET_PLATFORM built and installed in $OCPI_PREREQUISITES_INSTALL_DIR/amd-sdk/$OCPI_TARGET_PLATFORM
exit 0

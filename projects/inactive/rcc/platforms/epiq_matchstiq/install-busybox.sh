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

##########################################################################################
dir=busybox-1_25_1
me=busybox # could be from ${0} etc.

[ -z "$OCPI_CDK_DIR" ] && echo Environment variable OCPI_CDK_DIR not set && exit 1
source $OCPI_CDK_DIR/scripts/setup-prerequisite.sh \
       "$1" \
       $me \
       "Busy box used in embedded platforms" \
       https://git.busybox.net/busybox/snapshot/ \
       $dir.tar.gz \
       $dir \
       1
cd ..
export CROSS_COMPILE=$OcpiCrossCompile
# In ntpd from busy box you cannot use -c to give your own config file.
# This is a patch that will change the location to what we want and give us the
# option to add -c
if ! patch -R -p0 -s -f --dry-run < $OcpiThisPrerequisiteDir/prerequisites/$me/ntpd.patch > /dev/null; then
  patch -p0 < $OcpiThisPrerequisiteDir/prerequisites/$me/ntpd.patch
fi
cp $OcpiThisPrerequisiteDir/prerequisites/$me/busybox.config .config
# Adding appropriate sysroot to config file based off of OcpiCrossCompile variable
sysroot=$(echo $OcpiCrossCompile | sed "s|/bin[^/]*/$OcpiCrossHost-||")/$OcpiCrossHost/libc
sed -i "s|CONFIG_SYSROOT=.*|CONFIG_SYSROOT=\"${sysroot}\"|g" .config
make -j
# Reverse the patch so other sw platforms are not affected
patch -R -p0 < $OcpiThisPrerequisiteDir/prerequisites/$me/ntpd.patch
# Move contents produced to specific build dir
cd -
cp ../busybox .
cp ../busybox_unstripped .
# Move conf file to same build dir
cp $OcpiThisPrerequisiteDir/prerequisites/$me/ntp.conf .

# Make a directory for binaries produced from this script
mkdir -p $OcpiInstallExecDir/bin
# Only need the busybox executable so linking here
relative_link busybox $OcpiInstallExecDir/bin

# Link busy box commands here
# e.g. ln -sf busybox $OcpiInstallExecDir/<name_of_command>
ln -sf busybox $OcpiInstallExecDir/bin/ntpd
ln -sf busybox $OcpiInstallExecDir/bin/mdev

# Make a directory for configuration files
mkdir -p $OcpiInstallExecDir/conf
# Link ntpd configuration file to prerequisites dir
relative_link ntp.conf $OcpiInstallExecDir/conf

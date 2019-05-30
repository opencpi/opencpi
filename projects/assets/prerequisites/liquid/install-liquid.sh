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

liquid_version=1.3.1
[ -z "$OCPI_CDK_DIR" ] && echo Environment variable OCPI_CDK_DIR not set && exit 1
source $OCPI_CDK_DIR/scripts/setup-prerequisite.sh \
       "$1" \
       liquid \
       "DSP Math Library" \
       https://github.com/jgaeddert/liquid-dsp/archive \
       v$liquid_version.tar.gz \
       liquid-dsp-$liquid_version \
       1

# since this package does not use automake, it is not prepared for vpath mode
# (using ../configure from a build directory), so we have to snapshot the code for each platform
echo Copying git repo checkout contents for building in `pwd`
base=$(basename `pwd`)
(cd ..; cp -R $(ls . | grep -v ocpi-build-) $base)

# Patch malloc macros for cross-compilers ("undefined symbol: rpl_malloc")
patch < ${OcpiThisPrerequisiteDir}/malloc.patch

#ed bootstrap.sh <<-EOF
#	/^ *aclocal/s/\$/ -Iscripts/
#	w
#EOF
echo Performing '"./bootstrap.sh"'
# Even though configure.ac contains AC_CONFIG_MACRO_DIR, for at least the autoconf on centos6
# (autoconf version 2.63), this does not work so the -Iscripts is required below.
# Presumably they changed things "for the better", but broke centos6.
ACLOCAL="aclocal -Iscripts" ./bootstrap.sh

echo Performing '"./configure"'
./configure ${OcpiCrossHost:+--host=$OcpiCrossHost} \
  --prefix=$OcpiInstallDir --exec-prefix=$OcpiInstallExecDir \
  --includedir=$OcpiInstallDir/include --enable-fftoverride\
  CFLAGS=-g CXXFLAGS=-g
make
make install
rm -r -f $OcpiInstallExecDir/bin
echo rm -r -f $OcpiInstallExecDir/bin

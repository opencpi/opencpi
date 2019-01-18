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

# This server is unavailable:       https://ftp.gnu.org/gnu/gmp
# Since we don't look at multiple URLs/mirrors (yet)
# The one below is one of the advertised mirrors
me=rsync

[ -z "$OCPI_CDK_DIR" ] && echo Environment variable OCPI_CDK_DIR not set && exit 1
source $OCPI_CDK_DIR/scripts/setup-prerequisite.sh \
       "$1" \
       $me \
       "remote sync" \
       git://git.samba.org/rsync.git \
       master \
       $me \
       1

../configure --host=$OcpiCrossHost CFLAGS="-static $OcpiCFlags"
make -j

# Make a directory for binaries produced from this script
mkdir -p $OcpiInstallExecDir/bin
cp rsync rsync.bak
$OcpiCrossHost-strip rsync
relative_link rsync $OcpiInstallExecDir/bin

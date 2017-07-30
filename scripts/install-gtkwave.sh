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

OCPI_HOST_SYSTEM=linux-x86_64
set -e
. setup_install.sh
if test ! -d gtkwave ; then
mkdir gtkwave
fi
cd gtkwave
GTKWAVE_VERSION=3.3.30
TARFILE=gtkwave-$GTKWAVE_VERSION.tar.gz
if test ! -d $TARFILE ; then
curl -O http://gtkwave.sourceforge.net/$TARFILE
fi
rm -f gtkwave-$GTKWAVE_VERSION.tar
tar xzf $TARFILE
cd gtkwave-$GTKWAVE_VERSION
mkdir -p $OCPI_HOST_SYSTEM
./configure --disable-tcl --disable-xz --prefix=`pwd`/$OCPI_HOST_SYSTEM
make; make install










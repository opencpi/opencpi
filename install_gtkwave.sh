#!/bin/sh
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










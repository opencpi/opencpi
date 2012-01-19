#!/bin/sh
set -e
# no sandboxing here, we are just installing a new version of boost.
BOOST_VERSION=1.37.0
BOOST_VERSION_UNDER=`echo $BOOST_VERSION | tr . _`
BOOST_TAR_FILE=boost_$BOOST_VERSION_UNDER.tar.bz2
curl -v -O http://cdnetworks-us-2.dl.sourceforge.net/project/boost/boost/1.37.0/boost_1_37_0.tar.bz2
# Maybe try curl -L to follow locations
#curl -v -O http://downloads.sourceforge.net/project/boost/boost/1.37.0/boost_1_37_0.tar.bz2?r=&ts=1326925336&use_mirror=cdnetworks-us-2
#curl -v -O http://sourceforge.net/projects/boost/files/boost/$BOOST_VERSION/$BOOST_TAR_FILE/download
tar xf $BOOST_TAR_FILE
cd boost_$BOOST_VERSION_UNDER
./configure --prefix=/usr

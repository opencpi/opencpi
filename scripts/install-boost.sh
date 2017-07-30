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

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

if [ "$1" != "rpm" ]; then
  echo This script only builds RPMs
  exit 99
fi

if [ ! -e ocpi_build_support.spec ]; then
  echo "Missing RPM spec file in" `pwd`
  exit 1
fi

TARBALL=ocpi-prereq-build_support.tar

SUPPORT_FILES+=inode64

mkdir -p ~/rpmbuild/SOURCES || :
rm -rf ${TARBALL} ~/rpmbuild/RPMS || :
tar cf ${TARBALL} ${SUPPORT_FILES}
cp ${TARBALL} ~/rpmbuild/SOURCES

rpmbuild -ba ocpi_build_support.spec
cp -v ~/rpmbuild/RPMS/*/ocpi-prereq-build_support* .
rm -rf ${TARBALL} ~/rpmbuild/RPMS

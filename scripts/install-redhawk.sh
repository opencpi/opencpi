#!/bin/bash
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

OCPI_REDHAWK_VERSION=2.0.5
case `uname -r` in
    (*.el6.*) osv=6;;
    (*.el7.*) osv=7;;
    (*) echo Unsupported operating system for Redhawk installation; exit 1;;
esac
source ./scripts/setup-install.sh \
       "$1" \
       redhawk \
       redhawk-yum-${OCPI_REDHAWK_VERSION}-el${osv}-x86_64.tar.gz \
       https://github.com/RedhawkSDR/redhawk/releases/download/${OCPI_REDHAWK_VERSION} \
       redhawk-${OCPI_REDHAWK_VERSION}-el${osv}-x86_64 \
       0
# Remove the build directory called by setup-install.sh
cd ..
rm -r -f ocpi-build-*
echo ========= Ensure epel-release package is installed
yum list installed | grep epel-release.noarch > /dev/null ||
  sudo yum -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-${osv}.noarch.rpm
sudo yum -y update epel-release
echo ========= Establishing the RedHawk yum repository, locally in
cat<<EOF|sed 's@LDIR@'`pwd`'@g'|sudo tee /etc/yum.repos.d/redhawk.repo
[redhawk]
name=REDHAWK Repository
baseurl=file://LDIR/
enabled=1
gpgcheck=0
EOF
sudo yum -y install redhawk-devel redhawk-codegen bulkioInterfaces
echo ========= This is a global operation and is '*NOT*' sand-boxed for OpenCPI.
echo ========= Partial REDHAWK installation complete for supporting ocpirh_export


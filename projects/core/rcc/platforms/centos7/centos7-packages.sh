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

# Install prerequisite packages for Centos7
echo Installing standard extra packages using "yum"
sudo yum -y groupinstall "development tools"
CENTOS7_PKGS="mlocate tcl pax python-devel fakeroot which nfs-utils ocl-icd python-lxml"
echo Installing packages required or commonly used: $CENTOS7_PKGS
sudo yum -y install $CENTOS7_PKGS
echo Installing 32 bit libraries '(really only required for modelsim and/or quartus)'
sudo yum -y install glibc.i686 libXft.i686 libXext.i686 ncurses-libs.i686 libXdmcp.i686

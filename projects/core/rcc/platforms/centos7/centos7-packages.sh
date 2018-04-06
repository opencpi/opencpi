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

##########################################################################################
# Install prerequisite packages for Centos7
#
# First, for git cloning in the minimum centos7 CD image, installing git brings:
CENTOS7_PKGS="perl rsync libgnome-keyring perl-Git"
# Second, for the basic build/test (make prerequisites, make framework, make projects, test):
#    for framework and prereq build:
CENTOS7_PKGS+=" autoconf automake libtool gcc-c++ ed which"
#    for prerequisite downloading and building:
CENTOS7_PKGS+=" unzip patch"
#    for python and swig:
CENTOS7_PKGS+=" python swig python-devel python-lxml"
#    for driver: kernel-devel
CENTOS7_PKGS+=" kernel-devel"
#    for "make rpm":
CENTOS7_PKGS+=" rpm-build"
#    for building init root file systems for embedded systems
CENTOS7_PKGS+=" fakeroot"
#    for JTAG loading of FPGA bitstreams
CENTOS7_PKGS+=" libusb-devel"
#    for general configuration/installation flexibility
CENTOS7_PKGS+=" nfs-utils"
#    for OpenCL support (the switch for different actual drivers that are not installed here)
CENTOS7_PKGS+=" ocl-icd"
#    for various 32-bit software tools we end up using (e.g. modelsim)
CENTOS7_PKGS+=" glibc.i686 libXft.i686 libXext.i686 ncurses-libs.i686 libXdmcp.i686"

echo Installing standard extra packages using "sudo yum"
echo Installing packages required or commonly used: $CENTOS7_PKGS
sudo yum -y install $CENTOS7_PKGS

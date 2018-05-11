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
# Install required and available packages for Centos7
#
# First, for git cloning in the minimum centos7 CD image, installing git brings:
#PKGS="perl rsync libgnome-keyring perl-Git"
# While some manual installations require git manually installed before this,
# in other scenarios (bare docker containers), the git clone happens outside the container
# and thus we need to explicitly ask for git
PKGS=git
# Second, for the basic build/test (make prerequisites, make framework, make projects, test):
#    for framework and prereq build:
PKGS+=" make autoconf automake libtool gcc-c++ ed which"
#    for prerequisite downloading and building:
PKGS+=" unzip patch"
#    for python and swig:
PKGS+=" python swig python-devel python-lxml"
#    for kernel driver: kernel-devel
PKGS+=" kernel-devel"
#    for "make rpm":
PKGS+=" rpm-build"
#    for building init root file systems for embedded systems
PKGS+=" fakeroot"
#    for JTAG loading of FPGA bitstreams
PKGS+=" libusb-devel"
#    for general configuration/installation flexibility
PKGS+=" nfs-utils"
#    for OpenCL support (the switch for different actual drivers that are not installed here)
PKGS+=" ocl-icd"
#    for various 32-bit software tools we end up using (e.g. modelsim)
PKGS+=" glibc.i686 libXft.i686 libXext.i686 ncurses-libs.i686 libXdmcp.i686"
#    for the inode64 prerequisite
PKGS+=" glibc-devel.i686"
#    for various testing scripts
PKGS+=" numpy"
PKGS+=" epel-release"
#    for various testing scripts
EPEL_PKGS+=" python34-numpy"
[ "$1" = list ] && echo $PKGS && echo $EPEL_PKGS && exit 0
sudo yum -y install $PKGS
# Now those that depend on epel
sudo yum -y install $EPEL_PKGS

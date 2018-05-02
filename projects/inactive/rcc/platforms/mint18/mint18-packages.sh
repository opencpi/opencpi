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

# Install prerequisite packages for Linux Mint 18
echo Installing standard extra packages for mint18 using "sudo apt install"
PKGS=
# For the basic build/test (make prerequisites, make framework, make projects, test):
#    for framework and prereq build:
PKGS+=" autoconf automake libtool g++"
#    for prerequisite downloading and building:
#PKGS+=" unzip patch"
#    for python and swig:
#PKGS+=" python swig python-devel python-lxml"
PKGS+=" swig python-dev"
#    for driver: kernel-devel
#PKGS+=" kernel-devel"
#    for "make rpm":
#PKGS+=" rpm-build"
#    for building init root file systems for embedded systems
#PKGS+=" fakeroot"
#    for JTAG loading of FPGA bitstreams
#PKGS+=" libusb-devel"
#    for general configuration/installation flexibility
#PKGS+=" nfs-utils"
#    for OpenCL support (the switch for different actual drivers that are not installed here)
#PKGS+=" ocl-icd"
#    for various 32-bit software tools we end up using (e.g. modelsim)
#PKGS+=" glibc.i686 libXft.i686 libXext.i686 ncurses-libs.i686 libXdmcp.i686"
#    for various testing scripts
PKGS+=" python-numpy python3 python3-numpy"
sudo apt install $PKGS -y


#MINT18_PKGS="build-essential autoconf automake binutils bison flex gettext libtool make patch pkg-config mlocate tcl pax python-dev fakeroot nfs-common ocl-icd-dev libusb-dev"
#echo Installing 32 bit libraries '(really only required for modelsim and/or quartus)'
#sudo apt install libc6-i386 libxft2:i386 libxext6:i386 lib32ncurses5 libxdmcp6:i386 -y

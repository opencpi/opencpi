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
# Install or list required and available packages for Centos7
#
# The packages are really in four categories (and in 4 variables PKG{1,2,3,4}
# R. Simply required packages that can be yum-installed and rpm-required for runtime
#    -- note the driver package has separate requirements for driver rebuilding etc.
# D. Simply required packages that can be yum-installed and rpm-required for devel
# S. Convenience packages that will be yum-installed, but not rpm-required
#    -- Generally useful in a source installation, like rpmbuild, etc.
# E. Packages from other repos that are enabled as category #2 (e.g. use epel)
#    -- assumed needed for devel
#    -- thus they are installed after category #2 is installed

# 32 bit cross-architecture packages that, when rpm-required,
#    -- can only be rpm-required by mentioning some individual file in the package
#    -- we encode them as <package-name-for-yum>=<some-file-in-package-for-rpm>

##########################################################################################
# R. yum-installed and rpm-required for runtime - minimal
#    linux basics for general runtime scripts
PKGS_R+=(util-linux coreutils ed findutils sudo initscripts)
#    for JTAG loading of FPGA bitstreams
#    AV-3053 libusb.so is required to communicate with Xilinx programming dongle
#    For some reason, that is only in the libusb-devel package in both C6 and C7
PKGS_R+=(libusb-devel)
#    for bitstream manipulation at least
PKGS_R+=(unzip)
#    for python and swig testing
PKGS_R+=(python)

##########################################################################################
# D. yum-installed and rpm-required for devel (when users are doing their development).
#    for ACI and worker builds (and to support our project workers using autotools :-( )
PKGS_D+=(make autoconf automake libtool gcc-c++)
#    for our development scripts
PKGS_D+=(which)
#    for development and solving the "/lib/cpp failed the sanity check" a long shot
PKGS_D+=(glibc-static glibc-devel binutils)
#    for various building scripts for timing commands
PKGS_D+=(time)
#    for various project testing scripts - to allow users to use python2 - (we migrate to 3)
#    -- (AV-1261, AV-1299): still python 2 or just for users?
#    -- note that we also need python3 but that is from epel - below in $#4
PKGS_D+=(python-matplotlib scipy numpy)
#    enable other packages in the epel repo, some required for devel (e.g. python34)
PKGS_D+=(epel-release)
#    for various 32-bit software tools we end up supporting (e.g. modelsim) in devel (AV-567)
#    -- for rpm-required, we need a file-in-this-package too
PKGS_D+=(glibc.i686=/lib/ld-linux.so.2
         redhat-lsb-core.i686=/lib/ld-lsb.so.3
         ncurses-libs.i686=/usr/lib/libncurses.so.5
         libXft.i686=/usr/lib/libXft.so.2
         libXext.i686=/usr/lib/libXext.so.6)
#    for Quartus Pro 17 (AV-4318), we need specifically the 1.2 version of libpng
PKGS_D+=(libpng12)
#    to cleanup multiple copies of Linux kernel, etc. (AV-4802)
PKGS_D+=(hardlink)
# docker container missing this	libXdmcp.i686=/lib/libXdmcp.so.6) # AV-3645
#    for bash completion - a noarch package  (AV-2398)
PKGS_D+=(bash-completion=/etc/profile.d/bash_completion.sh)
#    Needed to build gdb
PKGS_D+=(bison)
#    Needed to build gdb
PKGS_D+=(flex)
##########################################################################################
# S. yum-installed and but not rpm-required - conveniences or required for source environment
# While some manual installations require git manually installed before this,
# in other scenarios (bare docker containers), the git clone happens outside the container
# and thus we need to explicitly ask for git inside the container
PKGS_S+=(git)
#    for prerequisite downloading and building:
PKGS_S+=(patch)
#    for building kernel drivers (separate from driver RPM)
PKGS_S+=(kernel-devel)
#    for "make rpm":
PKGS_S+=(rpm-build)
#    for creating swig
PKGS_S+=(swig python-devel)
#    for general configuration/installation flexibility
PKGS_S+=(nfs-utils)
#    for the inode64 prerequisite build (from source)
PKGS_S+=(glibc-devel.i686)
##########################################################################################
# E. installations that have to happen after we run yum-install once, and also rpm-required
#    for devel.  For RPM installations we somehow rely on the user pre-installing epel
#
#    for ocpidev
PKGS_E+=(python34 python34-jinja2)
#    for various testing scripts
#    AV-5478: If the minor version changes here, fix script below
PKGS_E+=(python34-numpy)
#    for building init root file systems for embedded systems (enabled in devel?)
PKGS_E+=(fakeroot)
#    for OpenCL support (the switch for different actual drivers that are not installed here)
PKGS_E+=(ocl-icd)

# functions to deal with arrays with <pkg>=<file> syntax
function rpkgs {
  eval echo \${$1[@]/#*=}
}
function ypkgs {
  eval echo \${$1[@]/%=*}
}
# The list for RPMs: first line
[ "$1" = list ] && rpkgs PKGS_R && rpkgs PKGS_D && rpkgs PKGS_S && rpkgs PKGS_E && exit 0
[ "$1" = yumlist ] && ypkgs PKGS_R && ypkgs PKGS_D && ypkgs PKGS_S && ypkgs PKGS_E && exit 0
sudo yum -y install $(ypkgs PKGS_R) $(ypkgs PKGS_D) $(ypkgs PKGS_S) --setopt=skip_missing_names_on_install=False
# Now those that depend on epel, e.g.
sudo yum -y install $(ypkgs PKGS_E) --setopt=skip_missing_names_on_install=False
# AV-5478: Make sure the python3 link is present
if ! command -v python3 >/dev/null; then
    if ! py34=$(command -v python3.4); then
      echo "Cannot find python3.4 after installing it" 2>&1
      exit 1
    fi
    if ! sudo ln -s $py34 /usr/bin/python3; then
      echo "Cannot create missing /usr/bin/python3 link to $py34" 2>&1
      exit 1
    fi
    echo "Created the missing /usr/bin/python3 -> python3.4 link" 2>&1
fi

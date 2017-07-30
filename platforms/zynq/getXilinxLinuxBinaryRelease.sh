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
if test "$1" = "" -o "$1" = "--help" -o "$1" = "-help" -o "$2" = ""; then
  cat <<EOF
This script retrieves and unpacks a Xilinx Zynq Linux binary release.
You must navigate to the web site at http://www.wiki.xilinx.com/Zynq+Releases and
choose the release, and determine the download file name, usually by clicking right on the link
and copying it.  An example of a download link is:
http://www.wiki.xilinx.com/file/view/2013.4-release.tar.xz/483333010/2013.4-release.tar.xz
You should also record from the site, the git release tag associated with the binary release.
This should be consistent with the repo tag used in ./createLinuxKernelHeaders.sh.
It may also want to be consistent with the release of the installed Xilinx EDK.

This script takes 3 arguments:
1. The OpenCPI zynq release name you are creating (e.g. 13_4).
2. The download URL for the release.
3. Your working directory for downloaded xilinx content.
EOF
  exit 1
fi
case $3 in (/*) gdir=$3 ;; (*) gdir=`pwd`/$3;; esac
name=xilinx-zynq-binary-release-for-$1
dir=$gdir/$name
echo Creating a subdirectory for the binary release in: $3/$name

if test -e $dir; then
  echo There is already a directory named $3/$name which will be removed.
fi
rm -r -f tmp $dir
mkdir -p tmp $dir
cd tmp
echo Downloading Xilinx Binary Zynq release from URL: $2
curl -O -L $2
if test `find . -type f | wc -l` != 1; then
  echo Expecting a single file from the download.
  cd ..; rm -r -f tmp
  exit 1
fi
REL=`echo *|sed s/\.tar\.*//`
echo Release file is: `ls`, unpacking it under $3/$name
T=`pwd`
cd $dir
tar -xf $T/*
echo Release contents is in: $3/$name
cd $T/..
rm -r -f tmp

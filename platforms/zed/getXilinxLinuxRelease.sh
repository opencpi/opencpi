#!/bin/sh
set -e
if test "$1" = "" -o "$1" = "--help" -o "$1" = "-help" -o "$2" = ""; then
  cat <<EOF
This script retrieves and unpacks a Xilinx Zynq Linux binary release.
You must navigate to the web site at http://www.wiki.xilinx.com/Zynq+Releases and
choose the release, and determine the download file name, usually by clicking right on the link
and copying it.  An example of a download link is:
http://www.wiki.xilinx.com/file/view/2013.4-release.tar.xz/483333010/2013.4-release.tar.xz
You should also record the git release tag associated with the release from the web site.

This script takes 2 arguments:
1. The xilinx release name (generally the download file without the -release suffix)
2. The download URL for the release.
EOF
  exit 1
fi
echo Creating a subdirectory for the $1 release: release-$1.
if test -e release-$1; then
  echo There is already a directory named release-$1 which you should remove to run this script.
  exit 1
fi
mkdir release-$1
cd release-$1
rm -r -f tmp
mkdir -p tmp SD-release
cd tmp
curl -O -L $2
if test `find . -type f | wc -l` != 1; then
  echo Expecting a single file from the download.
  cd ..; rm -r -f tmp
  exit 1
fi
REL=`echo *|sed s/\.tar\.*//`
echo Release file is $REL
cd ../SD-release
tar -xf ../tmp/$REL*
echo Release contents:
ls -l
cd ..
rm -r -f tmp
echo REMEMBER TO MAKE A SYMLINK TO THIS RELEASE IF IT IS THE DEFAULT
echo I.e., do:  ln -s release-$1 release

#!/bin/sh
set -e
if test "$1" = "" -o "$1" = "--help" -o "$1" = "-help"; then
  cat <<EOF
This script establishes a local clone of the Xilinx Linux kernel source tree, as well as a
clone of the Xilinx u-boot source tree.  It will not checkout the git trees for a tag/branch.
Tags can be listed with the ./showXilinxLinuxTags.sh script.
Thus the user has the option of specifying the desired tags to the follow-on script:
createXilinxKernelHeaders.sh.
This script has one argument, which is the directory in which the git clone will be created.
Usually it is "git".
EOF
  exit 1
fi
g=$1
set -e
if test ! -L $g; then
  mkdir -p $g
fi
cd $g
if test -d linux-xlnx; then
  echo The $g/linux-xlnx directory already exists.  It will be removed.
  rm -r -f linux-xlnx
fi
if test -d u-boot-xlnx; then
  echo The $g/u-boot-xlnx directory already exists.  It will be removed.
  rm -r -f u-boot-xlnx
fi
echo Cloning/downloading the Xilinx linux kernel tree from github.
git clone https://github.com/Xilinx/linux-xlnx.git
echo Cloning/downloading the Xilinx u-boot tree from github.
git clone https://github.com/Xilinx/u-boot-xlnx.git

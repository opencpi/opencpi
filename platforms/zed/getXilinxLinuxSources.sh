#!/bin/sh
set -e
if test "$1" = "" -o "$1" = "--help" -o "$1" = "-help"; then
  cat <<EOF
This script obtains a local clone of the Xilinx Linux kernel.  It will not checkout the tree
for a tag.  Tags can be listed with the ./showXilinxLinuxTags.sh script.
Thus the user has the option of specifying the desired tags to the follow-on script:
createXilinxKernelHeaders.sh.
This script must have an argument to run.
EOF
  exit 1
fi
if test `basename $(pwd)` != 'zed'; then
  echo This script should only be run from the platforms/zed directory.
  exit 1
fi
if test ! -L git; then
  mkdir -p git
fi
cd git
if test -d linux-xlnx; then
  echo The git/linux-xlnx directory already exists.  It will be removed.
  rm -r -f linux-xlnx
fi
if test -d u-boot-xlnx; then
  echo The git/u-boot-xlnx directory already exists.  It will be removed.
  rm -r -f u-boot-xlnx
fi
echo Cloning/downloading the Xilinx linux kernel tree from github.
git clone https://github.com/Xilinx/linux-xlnx.git
echo Cloning/downloading the Xilinx u-boot tree from github.
git clone https://github.com/Xilinx/u-boot-xlnx.git
exit 0

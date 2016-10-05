#!/bin/sh
# Example download URL:
#http://www.wiki.xilinx.com/file/view/2013.4-release.tar.xz/483333010/2013.4-release.tar.xz
if test "$#" != 3; then
  cat <<EOF
This script does everything to process a Xilinx release and populate a directory
to create a bootable OpenCPI SD card for the ZedBoard.
You must already know the the release name, the git repo tag, and the download URL.
If not, go to http://www.wiki.xilinx.com/Zynq+Releases.

Arg 1: The Xilinx release name. (e.g. 2013.4)
Arg 2: The Xilinx repo tag (e.g. xilinx-v2013.4)
Arg 3: The download URL for the Xilinx Release
E.g.:
http://www.wiki.xilinx.com/file/view/2013.4-release.tar.xz/483333010/2013.4-release.tar.xz
EOF
  exit 1
fi
./getXilinxLinuxRelease.sh $1 $3 && \
./getXilinxLinuxSources.sh - && \
./showXilinxLinuxTags.sh $1 && \
./createOpenCPIZedRelease.sh $1 $2 &&
(cd ../..; make; make rcc; make cleandriver; make driver) &&
OCPI_LIBRARY_PATH=$OCPI_LIBRARY_PATH:$OCPI_CDK_DIR/lib/hdl/assemblies ./makeSD.sh $1

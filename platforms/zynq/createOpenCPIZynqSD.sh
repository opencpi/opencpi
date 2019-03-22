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
set -e
if test "$1" = ""; then
  cat <<EOF
This script creates and fills a directory to be written to an SD card for OpenCPI on Zynq.
When the contents is written to an SD card, that card can be used as the boot disk on a
Zynq-based system.

It assumes:
1. You have created an OpenCPI Zynq release from Xilinx releases and kernel sources.
2. You have cross-built OpenCPI for the corresponding zynq software platform (version)
3. You have built the HDL assets for the particular zynq HDL platform.
4. You are running in the platforms/zynq directory or a specific software platform's directory.
5. You are running in the same directory where the release was created.

This script can be repeatedly run; it first clears out the directory it is filling.

This script takes three arguments to run:
1. the OpenCPI Zynq release name to use
2. the HDL platform name to use associated with the software platform you are enabling.
3. the Zynq software platform to use (which defaults to the name of the CWD)

Don't forget to copy zynq/default_mysetup.sh to mysetup.sh and customize mysetup.sh before
doing this.  Also, copy zynq/default_mynetsetup.sh to mynetsetup.sh and customize
mynetsetup.sh before doing this.

Also, artifacts for the SD card are found based on the OCPI_LIBRARY_PATH environment variable,
so make sure it includes the desired artifacts for running on the system you are targeting.

The result will be placed in a subdirectory of the release directory, called OpenCPI-SD, e.g.
opencpi-zynq-linux-release-<rel-name>/OpenCPI-SD.  You can further customize it if needed.
EOF
  exit 1
fi
REL=opencpi-zynq-linux-release-$1
[ -z "$OCPI_CDK_DIR" ] && echo OCPI_CDK_DIR environment not set up. && exit 1
source $OCPI_CDK_DIR/scripts/util.sh

export OCPI_TARGET_PLATFORM=${3:-$(basename $(pwd))}
# This one might be overridden if we want an SD from a particular build mode
# Someday provide the option to select the build mode
export OCPI_TARGET_DIR=$OCPI_TARGET_PLATFORM
export HDL_PLATFORM=${2:-zed}
echo Software platform is $OCPI_TARGET_PLATFORM, and hardware platform is $HDL_PLATFORM.
if test -z "$RPM_BUILD_ROOT"; then
  # We assume a built tree for the tool platform - check for exports etc.?
  # ensure OCPI_CDK_DIR and OCPI_TOOL_DIR
  source $OCPI_CDK_DIR/scripts/ocpitarget.sh $OCPI_TARGET_PLATFORM
  EXAMPLES_ROOTDIR=$(getProjectRegistryDir)/ocpi.assets
  cd $OCPI_TARGET_PLATFORM_DIR
else
  echo RPM Build detected - faking directory structure
  OCPI_CDK_DIR=${RPM_BUILD_ROOT}/opt/opencpi/cdk
  # Cannot just use CDK/lib and CDK/bin because the driver stuff isn't pushed there
  # EXAMPLES_ROOTDIR set externally
  # This is using a "path" variable assuming it has no colons in it!
  export OCPI_LIBRARY_PATH=${RPM_BUILD_ROOT}/opt/opencpi/projects/core/artifacts
fi
if test ! -d $REL; then
  echo The release directory, $REL, doesn\'t exist.
  exit 1
fi
if test "$OCPI_LIBRARY_PATH" = ""; then
  # Put all artifacts in the core project, as well as any pre-built bitstreams in the hdl
  # platforms' directory in case there are prebuilt bitstreams in the repo
  export OCPI_LIBRARY_PATH=$(getProjectRegistryDir)/ocpi.core/exports/artifacts:$HDL_PLATFORM_DIR
fi
BIN_DIR=${OCPI_CDK_DIR}/${OCPI_TARGET_DIR}/bin
KERNEL_LIB_DIR=$OCPI_CDK_DIR/${OCPI_TARGET_DIR}/lib
RUNTIME_LIB_DIR=${OCPI_CDK_DIR}/${OCPI_TARGET_DIR}/lib

cd $REL
sd=OpenCPI-SD-$HDL_PLATFORM
rm -r -f $sd
mkdir $sd
echo Populating the SD image directory from some of the binary release files.
case $HDL_PLATFORM in
    (zed)
	cp zed/boot.bin $sd
	;;
esac
echo Populating the SD image directory from patched kernel/device-tree/rootfs
# Use the device tree from the kernel build since it might be patched...
cp dts/zynq-$HDL_PLATFORM.dtb $sd/devicetree.dtb
# Use the kernel from the kernel build since it might be patched...
cp uImage $sd
# Use the new patched root fs.
cp uramdisk.image.gz $sd
# Copy the runtime package here
rm -r -f $sd/opencpi
mkdir $sd/opencpi
set -o pipefail
(cd $OCPI_CDK_DIR/..; ./packaging/prepare-package-list.sh deploy $OCPI_TARGET_PLATFORM 1 ) |
while read source dest; do
  [[ $source == */ ]] && continue # we don't do anything for individual directories
  if [ -n "$dest" ] ; then
    dest=$sd/opencpi/$dest
  else
    dest=$sd/opencpi
  fi
  links=
  [[ $$source != *@ ]] && links=-L
  cp -R $links $OCPI_CDK_DIR/../$source $dest
done || ( echo Preparation of file list failed. && exit 1)
echo Stripping all binaries to reduce space.
for f in `find $sd/opencpi/$OCPI_TARGET_PLATFORM/bin -type f`; do
  file -L $f| grep -q ' ELF ' && ${OCPI_TARGET_CROSS_COMPILE}strip $f || :
done
[ -n "$OCPI_LIBRARY_PATH" ] && {
  n=0
  echo Adding artifacts found in OCPI_LIBRARY_PATH for ${OCPI_TARGET_PLATFORM} and ${HDL_PLATFORM} HDL targets.
  mkdir $sd/opencpi/artifacts
  # Prefix artifact w/ "a" then number for bad simulators (AV-5233)
  for i in $(${OCPI_CDK_DIR}/${OCPI_TOOL_DIR}/bin/ocpirun \
			    -A ${OCPI_TARGET_PLATFORM},${HDL_PLATFORM} | \
		            xargs -rn1 readlink -e | sort -u ); do
    cp -p $i $sd/opencpi/artifacts/$(printf a%03d-%s $n $(basename $i))
    n=$(expr $n + 1)
  done
  echo Added $n artifacts to SD image.
}
# If the platform has a system.xml, make it the one
[ -f $sd/opencpi/$OCPI_TARGET_PLATFORM/system.xml ] && {
    rm -f $sd/opencpi/system.xml
    mv $sd/opencpi/$OCPI_TARGET_PLATFORM/system.xml $sd/opencpi
}
# Give the user a starting point for the setup scripts
cp -p $sd/opencpi/$OCPI_TARGET_PLATFORM/default_mynetsetup.sh $sd/opencpi/mynetsetup.sh
cp -p $sd/opencpi/$OCPI_TARGET_PLATFORM/default_mysetup.sh $sd/opencpi/mysetup.sh
mv $sd/opencpi/$OCPI_TARGET_PLATFORM/zynq_net_setup.sh $sd/opencpi
mv $sd/opencpi/$OCPI_TARGET_PLATFORM/zynq_setup.sh $sd/opencpi
cd ..
echo New OpenCPI Release SD:
du -k -s -h $REL/$sd/opencpi/artifacts
du -k -s -h $REL/$sd/opencpi/*/lib
du -k -s -h $REL/$sd/opencpi/*/bin
[ -d $REL/$sd/opencpi/*/artifacts ] && du -k -s -h $REL/$sd/opencpi/*/artifacts
du -k -s -h $REL/$sd/opencpi
du -k -s -h $REL/$sd

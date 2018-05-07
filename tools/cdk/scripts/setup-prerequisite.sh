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
# This sourced file supports installation of prerequisite software packages in a
# roughlyt standardized way.  The actual installation scripts that source this script for
# setup perform the actual compilation, but this script manages downloading, cloning, etc.
# and target platform setup, etc.
# Arguments to this source'd file are:
# $1 is the target platform (required, but can be "")
# $2 the name of the prerequisite
# $3 a description of the prerequisite
# $4 the download file
# $5 the download directory url, or pathname if no colon
# $6 the local directory resulting from unpacking the file, or "." if there is no directory
# $7 set to 1 to enable cross compilation (for runtime software on cross-compiled platforms)

scriptfile=${BASH_SOURCE[1]}
scriptdir=$(dirname $scriptfile)
set +o posix
platform=$1
package=$2
description=$3
file=$4
url=$5
directory=$6
cross=$7

if [ -z "$OCPI_CDK_DIR" ]; then
  echo "The environment (specifically OCPI_CDK_DIR) is not set up."
  echo "You probably need to do \"source <whereever-the-cdk-is>/opencpi-setup.sh\"."
  echo "If the source tree has never been used yet, you need to first run:"
  echo "   \"./scripts/init-opencpi.sh\" at the top level of the source directory."
  echo "This is done automatically by scripts like install-packages.sh, install-prequisites.sh or build-opencpi.sh"
  exit 1
elif [ ! -f $OCPI_CDK_DIR/scripts/ocpitarget.sh ]; then
  echo "The environment OCPI_CDK_DIR variable does not point to a valid location."  
  exit 1
fi
caller_dir=$(dirname ${BASH_SOURCE[${#BASH_SOURCE[*]}-1]})
# echo Caller of setup-install.sh is $caller_dir
set -e
source $OCPI_CDK_DIR/scripts/ocpitarget.sh "$platform"
platform=$OCPI_TARGET_PLATFORM
if test "$OCPI_TARGET_PLATFORM" != "$OCPI_TOOL_PLATFORM" -a "$cross" != "1"; then
 echo ====== We will not crossbuild $package for non-development environments thus are skipping it.
 exit 0
fi
if test "$OCPI_TARGET_PLATFORM" != "$OCPI_TOOL_PLATFORM" -a -n "$OCPI_CROSS_BUILD_BIN_DIR"; then
  if test ! -d "$OCPI_CROSS_BUILD_BIN_DIR"; then
    echo The cross-compilation tools for building $OCPI_TARGET_PLATFORM on $OCPI_TOOL_PLATFORM are missing.
    exit 1
  fi
fi	
source $OCPI_CDK_DIR/scripts/setup-prereq-dirs.sh
function install_done {
  RC=$?
  trap - EXIT
  if [ -z "$OcpiSetup" -a $RC = 0 ]; then
    echo "====== Finished building & installing the $description \"$package\" for platform \"$OCPI_TARGET_PLATFORM\""
    echo "====== The installation is in $OCPI_PREREQUISITES_INSTALL_DIR/$package"
    echo "====== The platform-specific parts are in $OCPI_PREREQUISITES_INSTALL_DIR/$package/$OCPI_TARGET_DIR"
  else
    echo ====== Build/install of $package prerequisite failed.
    exit 1
  fi
}
OcpiSetup=1
trap install_done EXIT
echo ====== Starting installation of the $description \"$package\" for platform \"$platform\".
echo ========= It will be downloaded/copied/cloned and built in $OCPI_PREREQUISITES_BUILD_DIR/$package
echo ========= It will be installed in $OCPI_PREREQUISITES_INSTALL_DIR/$package/$OCPI_TARGET_DIR
# Create and enter the directory where we will download and build
cd $OCPI_PREREQUISITES_BUILD_DIR
mkdir -p $package
cd $package
# Create the directory where the package will be installed
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/$package/$OCPI_TARGET_DIR
# If a download file is provided...
if [ -n "$url" ]; then
  if [[ "$url" == *.git ]]; then
    if [ -d $directory ]; then
      echo The git repo directory for $package, $directory, exists and is being '(re)'used.
      echo Remove `pwd`/$directory if you want to download it again.
      echo It will be updated now
      cd $directory
      git fetch
    else
      echo Downloading/cloning the distribution/repo for $package: $url
      git clone $url
      echo Download/clone complete.
      [ -d $directory ] || {
	  echo The directory $directory was not created when git repo cloned from $url.
	  exit 1
      }
      cd $directory
    fi
    git checkout $file
  else
    if [ -f "$file" ]; then
      echo The distribution file for $package, $file, exists and is being '(re)'used.
      echo Remove `pwd`/$file if you want to download it again.
    elif [ -f "$caller_dir/$file" ]; then
      echo "The distribution file for $package, $file, exists in the script's directory ($caller_dir) and is being (re)used."
      echo Remove $caller_dir/$file if you want to download it.
      file="$caller_dir/$file"
    else
      if [[ "$url" == [a-zA-Z]*:* ]]; then
        echo Downloading the distribution file: $file
        curl -O -L $url/$file
        echo Download complete.  Removing any existing build directories.
      else
        echo Copying the distribution file from $url/$file
        [[ $url == /* ]] || url=$scriptdir/$url
        cp $url/$file .
        echo Copy from $url/$file complete.  Removing any existing build directories.
      fi
      [ "$directory" != . ] && rm -r -f $directory
    fi
    if test -d $directory; then
      echo The distribution directory $directory exists, using it for this target:'  '$platform.
    else
      echo Unpacking download file $file into $directory.
      case $file in
      (*.tar.gz) tar xzf $file;;
      (*.tar) tar xf $file;;
      (*.tar.xz) tar -x --xz -f $file;;
      (*.zip) unzip $file;;
      (*) echo Unknown suffix in $file.  Cannot unpack it.; exit 1
      esac
    fi
    cd $directory
  fi
fi
if test -d ocpi-build-$OCPI_TARGET_DIR; then
   echo Removing existing build directory for $OCPI_TARGET_DIR, for rebuilding.
   rm -r -f ocpi-build-$OCPI_TARGET_DIR
fi
mkdir -p ocpi-build-$OCPI_TARGET_DIR
cd ocpi-build-$OCPI_TARGET_DIR
if test "$OCPI_TARGET_PLATFORM" != "$OCPI_TOOL_PLATFORM"; then
  if test -z "$OCPI_TARGET_CROSS_COMPILE"; then
      echo "Missing cross compilation variable (OCPI_TARGET_CROSS_COMPILE)."
      exit 1
  fi
  # This is for configure scripts that want to use --host
  if [[ "$OCPI_TARGET_CROSS_COMPILE" == *- ]] ; then
      export OCPI_CROSS_HOST=$(basename ${OCPI_TARGET_CROSS_COMPILE%-})
      PATH=$(dirname $OCPI_TARGET_CROSS_COMPILE):$PATH
  else
      PATH=$OCPI_TARGET_CROSS_COMPILE:$PATH
  fi
  # This is for raw scripts that just need to know where the tools are
  CC=${OCPI_TARGET_CROSS_COMPILE}gcc
  CXX=${OCPI_TARGET_CROSS_COMPILE}c++
  LD=${OCPI_TARGET_CROSS_COMPILE}c++
  AR=${OCPI_TARGET_CROSS_COMPILE}ar
else
  CC=gcc
  CXX=c++
  LD=c++
  AR=ar
fi
echo ====== Building package \"$package\" for platform \"$platform\" in `pwd`.

# Make a relative link from the install dir to the build dir
# args are the two args to ln (to from)
# replace the link if it exists
# if the "from" is already an existing directory, put the link in it
function relative_link {
  local base=$(basename $1) from=$2 to=$1
  [[ $to == /* ]] || to=`pwd`/$1
  if [ -d $2 ] ; then
    [ -L $2/$base ] && rm $2/$base
  else
    from=$(dirname $2)
    mkdir -p $from
  fi
  to=$(python -c "import os.path; print os.path.relpath('$(dirname $to)', '$from')")
  ln -s -f $to/$base $2/$base
}
OcpiSetup=


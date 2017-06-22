#!/bin/sh
# Arguments to this source'd file are:
# $1 is the target platform (required, but can be "")
# $2 the name of the prerequisite
# $3 the download file
# $4 the download directory url
# $5 the local directory resulting from unpacking the file
# $6 set to enable cross compilation

set +o posix
platform=$1
package=$2
file=$3
url=$4
directory=$5
cross=$6
# ensure exports
source ./scripts/core-init.sh
# setup CDK and OCPI_TOOL_*
OCPI_BOOTSTRAP=`pwd`/exports/scripts/ocpibootstrap.sh; source $OCPI_BOOTSTRAP
# setup OCPI_TARGET_*
source tools/cdk/scripts/ocpitarget.sh "$platform"
platform=$OCPI_TARGET_PLATFORM
if test "$OCPI_TARGET_PLATFORM" != "$OCPI_TOOL_PLATFORM" -a "$cross" != "1"; then
 echo We do not crossbuild $package for non-development environments so skipping building it.
 exit 0
fi

echo ====== Starting installation of package \"$package\" for platform \"$platform\".
set -e
if test -z "$OCPI_PREREQUISITES_INSTALL_DIR"; then
  if test -n "$OCPI_PREREQUISITES_DIR"; then
    export OCPI_PREREQUISITES_INSTALL_DIR=$OCPI_PREREQUISITES_DIR
  else
    export OCPI_PREREQUISITES_INSTALL_DIR=/opt/opencpi/prerequisites
  fi
  pdir="$(dirname $OCPI_PREREQUISITES_INSTALL_DIR)"
  if test ! -d $pdir; then
    echo "Error: $pdir does not exist and must be created first."
    echo "       With appropriate permissions, ideally not root-only."
    exit 1
  fi
fi
if test "$OCPI_PREREQUISITES_BUILD_DIR" = ""; then
  export OCPI_PREREQUISITES_BUILD_DIR=$OCPI_PREREQUISITES_INSTALL_DIR
fi
mkdir -p $OCPI_PREREQUISITES_BUILD_DIR
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR
cd $OCPI_PREREQUISITES_BUILD_DIR
mkdir -p $package
cd $package
# If a download file is provided...
if [ -n "$url" ]; then
  if [[ "$url" == *.git ]]; then
    if [ -d $directory ]; then
      echo The git repo directory for $package, $directory, exists and is being '(re)'used.
      echo Remove `pwd`/$directory if you want to download it again.
      cd $directory
    else
      echo Downloading/cloning the distribution/repo for $package: $url
      git clone $url
      echo Download/clone complete.
      [ -d $directory ] || {
	  echo The directory $directory was not created when git repo cloned from $url.
	  exit 1
      }
      cd $directory
      git checkout $file
    fi
  else
    if [ -f "$file" ]; then
      echo The distribution file for $package, $file, exists and is being '(re)'used.
      echo Remove `pwd`/$file if you want to download it again.
    else
      echo Downloading the distribution file: $file
      curl -O -L $url/$file
      echo Download complete.  Removing any existing build directories.
      rm -r -f $directory
    fi
    if test -d $directory; then
      echo The distribution directory $directory exists, using it for this target:'  '$platform.
    else
      echo Unpacking download file $file into $directory.
      case $file in
      (*.tar.gz) tar xzf $file;;
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
  if test -z "$OCPI_CROSS_BUILD_BIN_DIR" -o -z "$OCPI_CROSS_HOST"; then
      echo "Missing cross compilation variables (OCPI_CROSS_BUILD_BIN_DIR or OCPI_CROSS_HOST)."
      exit 1
  fi
  # This is for configure scripts that want to use --host
  PATH=$OCPI_CROSS_BUILD_BIN_DIR:$PATH
  # This is for raw scripts that just need to know where the tools are
  CC=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-gcc
  CXX=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-c++
  LD=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-c++
  AR=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-ar
else
  CC=gcc
  CXX=c++
  LD=c++
  AR=ar
fi
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/$package/$OCPI_TARGET_DIR
echo ====== Building package \"$package\" for platform \"$platform\".

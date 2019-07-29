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
# roughly standardized way.  The actual installation scripts that source this script for
# setup perform the actual compilation, but this script manages downloading, cloning, etc.
# and target platform setup, etc.  It also offers one helper function: relative_link
# It is documented in the platform development guide

# Arguments to this sourced file are:
# $1 is the target platform (required argument, but can be "" for the currently running one)
# $2 the name of the prerequisite
# $3 a description of the prerequisite
# $4 the download directory url, or directory pathname if no colon, or git repo if *.git
# $5 the download file or git repo tag or branch
# $6 the local directory resulting from unpacking the download, or "." if there is no directory
# $7 set to 1 to enable cross compilation (for runtime software on cross-compiled platforms)

# There are also environment variables to override the provided URLs.
# I.e. arg $4 is the canonical internet URL for the software, but an organization may
# want a local copy of all the download packages.  So we keep the install script the same,
# always listing the real internet location, but you can provide environment variables
# to override this, for all prerequisites - either with a URL or a pathname in the variables:
# OCPI_PREREQUISITES_LOCAL_SERVER OCPI_PREREQUISITES_LOCAL_PATHNAME
# If you set both, they both will be tried
# This provides an alternative local location (server or path/mount) to use in preference
# to the global URL.
# A second environment variable, OCPI_PREREQUISITES_DOWNLOAD_PATH specifies where to look for
# downloads.  It is a colon-separated sequence of these words:
# cache    - use a previously download file/tarball/git-repo if present
# local    - use the OCPI_PREREQUISITES_LOCAL_SERVER or PATHNAME if set
# internet - use the URL provided to this setup script.
# The default path is:    cache:local:internet
# To force downloads from the internet use:  internet
# To prevent downloads from the internet use:  cache:local

scriptfile=${BASH_SOURCE[1]}
scriptdir=$(dirname $scriptfile)
[[ $scriptdir == /* ]] || scriptdir=`pwd`/$scriptdir
set +o posix
platform=$1
package=$2
description=$3
url=$4
file=$5
directory=$6
cross=$7
# base is used when considering alternative local download sources
if [[ "$url" == *.git ]]; then
  base=$(basename $url)
else
  base=$file
fi
if [ -z "$OCPI_CDK_DIR" ]; then
  echo "The environment (specifically OCPI_CDK_DIR) is not set up."
  echo "You probably need to do \"source <wherever-the-cdk-is>/opencpi-setup.sh\"."
  echo "If the source tree has never been used yet, you need to first run:"
  echo "   \"./scripts/init-opencpi.sh\" at the top level of the source directory."
  echo "This is done automatically by scripts like install-packages.sh, install-prequisites.sh or build-opencpi.sh"
  exit 1
elif [ ! -f $OCPI_CDK_DIR/scripts/ocpitarget.sh ]; then
  echo "The environment OCPI_CDK_DIR variable does not point to a valid location."  
  exit 1
fi
#set -vx
[ -z "$OCPI_PREREQUISITES_DOWNLOAD_PATH" ] && OCPI_PREREQUISITES_DOWNLOAD_PATH=cache:internet
caller_dir=$(dirname ${BASH_SOURCE[${#BASH_SOURCE[*]}-1]})
[[ $caller_dir == /* ]] || caller_dir=`pwd`/$caller_dir
# echo Caller of setup-prerequisite.sh is $caller_dir
set -e
source $OCPI_CDK_DIR/scripts/ocpitarget.sh "$platform"
platform=$OCPI_TARGET_PLATFORM
if test "$OCPI_TARGET_PLATFORM" != "$OCPI_TOOL_PLATFORM" -a "$cross" != "1"; then
 echo ====== We will not crossbuild $package for non-development environments thus are skipping it.
 exit 0
fi
if test "$OCPI_TARGET_PLATFORM" != "$OCPI_TOOL_PLATFORM" -a -n "$OCPI_CROSS_COMPILE"; then
  if test ! -d "$(dirname $OCPI_CROSS_COMPILE)"; then
    echo The cross-compilation tools for building $OCPI_TARGET_PLATFORM on $OCPI_TOOL_PLATFORM are missing.
    exit 1
  fi
fi	
source $OCPI_CDK_DIR/scripts/setup-prereq-dirs.sh
function install_done {
  RC=$?
  trap - EXIT
  if [ -z "$OcpiSetup" -a $RC = 0 ]; then
    shopt -s nullglob
    # Here we remap libraries so we can use them dynamically in our own publicly exposed
    # library (ld.so.conf.d) with no namespace collisions in globally installed versions
    # for l in $OCPI_PREREQUISITES_INSTALL_DIR/$package/$OCPI_TARGET_DIR/lib/lib$package*.*; do
    #   set -vx
    #   local lsuff=${l##*/lib}
    #   local base=$(basename $l)
    #   local link=$(dirname $l)/libocpi_$lsuff
    #   [ -L $link -a $(readlink $link) = $base ] && continue;
    #   rm -f $link
    #   echo "Linking $link -> $base."
    #   ln -s $base $link
    # done
    echo "====== Finished building & installing the $description \"$package\" for platform \"$OCPI_TARGET_PLATFORM\""
    echo "====== The installation is in $OCPI_PREREQUISITES_INSTALL_DIR/$package"
    echo "====== The platform-specific parts are in $OCPI_PREREQUISITES_INSTALL_DIR/$package/$OCPI_TARGET_DIR"
  else
    echo ====== Build/install of $package prerequisite failed.
    exit 1
  fi
}
# args are global: (url, directory, file) except
# $1 is the download path type
# $2 is the local download url/path
# $3 is the alternative local download repo
function download_git {
  case $1 in
    cache)  # if it is already here, try to use it.
      [ -d $directory ] && {
        echo The git clone directory for $package, $directory, exists and is being '(re)'used.
        echo Remove the `pwd`/$directory directory if you want to download/clone it again.
        echo It will be reused as it was previously cloned and checked out.
        cd $directory
        # Note that normally everything happens in the build subdirectory so nothing should be
        # changed in the repo and this should be unnecessary. Specific install scripts should do
	# this if they actually build in the repo directory
        # git reset --hard
        # git clean -fdx
        # git status
        return 0
      };;
    local)
      local myurl=$2/$3
      echo "Trying to clone the git repo for $package locally from:  $myurl"
      rm -r -f $directory.bak
      [ -d $directory ] && mv $directory{,.bak}
      # Note that we force the directory name to be what we want even though
      # the myurl might be redirected in the local case.
      if git clone $myurl $directory; then
        echo The git clone succeeded from $myurl.
        if [ -d $directory ] ; then
          cd $directory
          git checkout $file && rm -r -f ../$directory.bak && return 0
	  cd ..
          echo The git checkout failed.
        else  
          echo The directory $directory was not created when the git repo was cloned from $myurl.
          echo This is considered a failure.
        fi
      else
        echo The git clone failed from $myurl
      fi
      rm -r -f $directory # in case of partial clone
      [ -d $directory.bak ] && mv $directory{.bak,}
      ;;
    internet)
      echo Downloading/cloning the distribution/repo for $package: $url
      rm -r -f $directory.bak
      [ -d $directory ] && mv $directory{,.bak}
      if git clone $url; then
        echo Download/clone complete.
        if [ -d $directory ]; then
          cd $directory
          git checkout $file && rm -r -f $directory.bak && return 0
          echo The git checkout failed.
        else
          echo The directory $directory was not created when git repo cloned from $url.
        fi
      else
        echo The git clone failed from $url.
      fi
      rm -r -f $directory # in case of partial clone
      [ -d $directory.bak ] && mv $directory{.bak,}
      ;;
    *) echo UNEXPECTED internal error && exit 1;;
  esac
  return 1
}
function unpack {
  [ -r $file ] || {
    echo After download/copy, the expected file $file does exist.
    return 1
  }
  [ "$directory" = . ] && return 0
  rm -r -f $directory
  echo Unpacking download file $file into $directory.
  case $file in
    (*.tar.gz) tar xzf $file;;
    (*.tar|*.tar.bz2) tar xf $file;;
    (*.tar.xz) tar -x --xz -f $file;;
    (*.zip) unzip $file;;
    (*) echo Unknown suffix in $file.  Cannot unpack it.; exit 1
  esac
  [ $? != 0 ] && {
    echo Failed when unpacking the download file: $file.
    return 1
  }
  [ -d $directory ] && cd $directory && return 0
  echo "Unpacking the download file ($file) did not create the expected directory: $directory"
  return 1
}
# enter a cached directory given that the download file is cached.
function enter {
  [ -d $directory ] && cd $directory && return 0
  echo "Even though the download file, $file, exists, the directory for it, $directory does not."
  return 1
}

# args are global: (url, directory, file) except:
# $1 is the download path type
# $2 is the actual URL
function download_url {
  case $1 in
    cache)
      [ -f "$file" ] && { # if it is where we normally download it, use it
        echo The distribution file for $package, $file, exists and is being '(re)'used.
        echo Remove `pwd`/$file if you want to download it again.
	enter
	return
      }
      [ -f "$caller_dir/$file" ] && { # consider one that has been manually downloaded
        echo "The distribution file for $package, $file, exists in the script's directory ($caller_dir) and is being (re)used."
        [ $url = . ] || echo Remove $caller_dir/$file if you want to download it next time.
	cp $caller_dir/$file .
	unpack
        return
      };;
    local)
      echo Trying to downloading the distribution file locally from:  $2/$3
      echo Download command is: curl -O -L $2/$3
      if curl -O -L $2/$file; then
        echo Download completed successfully from $2/$3
	if [ -r $3 ] ; then
	  [ "$3" != $file ] && mv -f $3 $file
          unpack
	  return
        else
          echo The file $3 was not created after the download.
	  echo This is considered a failure.
        fi
      else
        echo Download failed from $2.
      fi;;
    internet)
      echo Downloading the distribution file: $file
      echo Download command is: curl -O -L $url/$file
      curl -O -L $url/$file && {
        echo Download complete.  Removing any existing build directories.
        unpack
	return
      }
      echo Download from $url/$file failed.
      ;;
    *) echo UNEXPECTED internal error && exit 1;;
  esac
  return 1
}

# args are global: (url, directory, file) except:
# $1 is the download path type
# $2 is the local path to try
function download_path {
  case $1 in
    cache)
      download_url cache
      return;;
    local)
      [ $url = . -a -f "$caller_dir/$file" ] && {
        echo "The distribution file for $package, $file, exists in the script's directory ($caller_dir) and is being used."
	cp $caller_dir/$file .
	unpack
        return
      }
      echo Trying to copy the distribution file locally from:  $2/$3
      cp $2/$3 . && {
        echo Copy completed successfully from $2/$3.
	[ "$3" != $file ] && mv -f $3 $file
        unpack
	return
      }
      echo Copy failed from $2/$3.
      ;;
    internet)
      echo Copying the distribution file from $url/$file
      [[ $url == /* ]] || url=$scriptdir/$url
      cp $url/$file . && {
        echo Copy from $url/$file complete.  Removing any existing build directories.
        unpack
	return
      }
      echo Copying from $url/$file failed.
      ;;
    *) echo UNEXPECTED internal error && exit 1;;
  esac
  return 1
}
function download_local {
  if [[ "$url" == *.git ]]; then # try local locations for git clone
    download_git local $1 $2 && return 0
  elif [[ "$p" == [a-zA-Z]*://* ]]; then # a real URL, not a path
    download_url local $1 $2 && return 0
  else
    download_path local $1 $2 && return 0
  fi
  return 1
}
# Function to get the downloaded info (source file, tarball, or git repo)
function do_download {
  local download_path=(${OCPI_PREREQUISITES_DOWNLOAD_PATH/:/ })
  [ -z "$download_path" ] && download_path=(cached local internet)
  # Iterate down the path
  for d in ${download_path[@]}; do
    case $d in
      cache)  # if it is already here, use it.
        if [[ "$url" == *.git ]]; then # if we are dealing with a git repo
          download_git cache && return 0
	elif [[ "$url" == [a-zA-Z]*://* ]]; then # a real URL, not a path
          download_url cache && return 0
        else
	  download_path cache && return 0
        fi;;
      local)
        # Special case when the URL is in fact a pathname already
        [[ "$url" != *.git &&  "$url" != [a-zA-Z]*://* ]] && {
           download_path internet && return 0
           continue;           
	}
        [ -z "$OCPI_PREREQUISITES_LOCAL_SERVER" -a -z "$OCPI_PREREQUISITES_LOCAL_PATHNAME" ] && {
          echo No local server or pathname is supplied, so no local server or pathname used.
	  continue
        }
	for p in "$OCPI_PREREQUISITES_LOCAL_PATHNAME" "$OCPI_PREREQUISITES_LOCAL_SERVER"; do
          local lfile=$(eval echo "\$OCPI_PREREQUISITES_LOCAL_FILE_$package")
	  if [ -n "$lfile" ]; then
	    download_local $p $lfile && return 0
          else
            download_local $p $package-$base && return 0
            download_local $p $base && return 0
          fi
	done;;
      internet)
        if [[ "$url" == *.git ]]; then
          download_git internet && return 0
	elif [[ "$url" == [a-zA-Z]*:* ]]; then
          download_url internet && return 0
        else
          download_path internet && return 0
	fi;;
      *)
        echo OCPI_PREREQUISITES_DOWNLOAD_PATH is invalid: $OCPI_PREREQUISITES_DOWNLOAD_PATH
        echo It is a colon-separated list that may contain: cache, local, or internet
        exit 1;;
    esac
  done
  echo Could not obtain the download file \"$file\".  Looked in these places: ${download_path[@]}
  exit 1  
}

OcpiSetup=1
trap install_done EXIT
echo ====== Starting build and installation of the prerequisite $description \"$package\" for platform \"$platform\".
echo ========= It will be downloaded/copied/cloned and built in $OCPI_PREREQUISITES_BUILD_DIR/$package
echo ========= It will be installed in $OCPI_PREREQUISITES_INSTALL_DIR/$package/$OCPI_TARGET_DIR
# Create and enter the directory where we will download and build
cd $OCPI_PREREQUISITES_BUILD_DIR
mkdir -p $package
cd $package
# Create the directory where the package will be installed
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/$package/$OCPI_TARGET_DIR
do_download
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
  CC=${OCPI_TARGET_CROSS_COMPILE}$OcpiCC
  CXX=${OCPI_TARGET_CROSS_COMPILE}$OcpiCXX
  LD=${OCPI_TARGET_CROSS_COMPILE}$OcpiCXXLD
  AR=${OCPI_TARGET_CROSS_COMPILE}ar
else
  CC=$OcpiCC
  CXX=$OcpiCXX
  LD=$OcpiCXXLD
  AR=$OcpiAR
fi
echo ====== Building prerequisite package \"$package\" for platform \"$platform\" in `pwd`.

# Make a relative link from the install dir to the build dir
# args are the two args to ln (to-file from-dir)
# replace the link if it exists
function relative_link {
  local base=$(basename $1) to=$1 from=$2 link=$3
  [ -z "$link" ] && link=$base
  [[ $to == /* ]] || to=`pwd`/$to
  mkdir -p $from
  [ -L $from/$link ] && rm $from/$link
  to=$(python -c "import os.path; print os.path.relpath('$(dirname $to)', '$from')")
  ln -s -f $to/$base $from/$link
}
OcpiSetup=
# The convenience variables that take care of nearly all cases.
OcpiInstallDir=$OCPI_PREREQUISITES_INSTALL_DIR/$package
OcpiInstallExecDir=$OCPI_PREREQUISITES_INSTALL_DIR/$package/$OCPI_TARGET_DIR
OcpiCrossHost=$OCPI_CROSS_HOST
OcpiThisPrerequisiteDir=$scriptdir

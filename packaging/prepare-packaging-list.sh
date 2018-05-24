#!/bin/bash --noprofile
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
# This script prepares for one of the export combinations for some platforms
# It contains the rules for the combinations
#
# The choices are:
# all exports
# runtime exports
# devel exports (all minus runtime)
#
# Basically we generate a set of directories and files for export that are
# minimal for the export type.
# The inputs are the type of export (arg1), platforms (arg2) and all platforms (arg3)
#
# The result is a set of directories and files, one per line.
# each line is of the form
# <source-path> [<dest-path>]

[ "$1" = -v ] && verbose=1 && shift
type=$1
platforms=$2
cross=$3
allplatforms="$OCPI_ALL_RCC_PLATFORMS"
set -e

function found_in {
  local look=$1
  shift
  for i in $*; do [ $i = $look ] && return 0; done
  return 1
}

function is_platform {
  [ -d $1/lib -o -d $1/bin ]
}    

function skip_platform {
  local base=$(basename $1)
  is_platform $1 && ! found_in $(basename $1) $platforms
}

# emit the dir name if the tree is clean, otherwise emit each cached item here and recurse on
# directories.  Basically this is: git ls-files but don't descend when all contents are cached
function emit_project_dir {
    set -o pipefail
    # if find $1 -type f -a \! \( -path "*/target-*/*" -o \
    #                             -path "*/gen/*" -o \
    #                             -path "*/container-*/*" -o \
    #                             -path "*/config-*/*" -o \
    # 			        \( -path "*/lib/*" -a \! -path "*/rcc/platforms/*" \) \) |sort|
    if find $1 -type f | sort | diff - <(git ls-files $1 | sort ) > /dev/null ; then
      [ -n "$verbose" ] && echo Dir at $1 is the same >&2
      echo $1
    else
      [ -n "$verbose" ] && echo Dir at $1 is different, descending >&2
      for i in `ls -a $1`; do
	[ $i = . -o $i = .. ] && continue
	if [ -d $1/$i ]; then
	  [ `git ls-files $1/$i|wc -c` = 0 ] || emit_project_dir $1/$i  
        else
	  git ls-files $1/$i
	fi  
      done
   fi
}

[ $1 = test ] && {
  emit_project_dir $2  
  exit $?
}


shopt -s nullglob
for l in `find cdk -follow -type l`; do
  bad=1
  echo Dead exports link found: $l
done
for l in `find -H . -name "-*"`; do
  bad=1
  echo Found files starting with hyphen
done
[ -n "$bad" ] && exit 1
# FIXME: this list is redundant with "install-prerequisites.sh" and "places"
# This list could potentially be platform-specific
# and then there are platform-specific prereqs
prereqs="gmp lzma gtest patchelf inode64 ad9361 liquid"
case $type in
  all)
    for f in cdk/*; do
      [ $f != cdk/runtime ] && ! skip_platform $f && echo $f
    done
    for p in $prereqs; do
      for d in prerequisites/$p/*; do
        skip_platform $d || echo $d
      done
    done;;
  runtime)
    # Runtime is: cdk/runtime, and prereq dynamic librariesP
    for f in cdk/runtime/*; do
      skip_platform $f || echo $f cdk
    done
    for p in $prereqs; do
      for d in prerequisites/$p/*; do
        skip_platform $d ||
	  find $d -name "*.so" -o -name "*.so.*" -o -name "*.dylib"
      done
    done;;
  devel)
    [ -n "$cross" ] && prefix=/$platform/lib
    for f in cdk/*; do
      ( [ $f = cdk/runtime ] || skip_platform $f ) && continue
      # If its not platform specific, it won't be in the cross-platform devel
      [ -n "$cross" -a $f != cdk/$platforms ] && continue
      base=$(basename $f)
      diff <(cd cdk/runtime; [ -e $base ] && find -H $base) \
           <(cd cdk; find -H $base -name runtime -prune -o ! -type d -print) \
           | sed -n 's/^> *\(.*\)$/cdk\/\1/p'
    done
    for p in $prereqs; do
      for d in prerequisites/$p/*; do
        skip_platform $d && continue;
	if is_platform $d; then
	  [ -z "$cross" -a -d $d/bin ] && find $d/bin ! -type d
	  [ -d $d/include ] && find $d/include ! -type d
          [ -d $d/lib ] && find $d/lib -name "*.a"
        elif [ -z "$cross" ] && [[ $d == */include ]]; then
          echo $d
        fi
      done
    done
    # emit project stuff that are git repo items
    emit_project_dir project-registry
    emit_project_dir projects/core
    emit_project_dir projects/assets
    ;;
  *) echo Unknown export type;;
esac

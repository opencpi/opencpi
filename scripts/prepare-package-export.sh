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

type=$1
platforms=$2
allplatforms=$3

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

shopt -s nullglob
# FIXME: this is redundant with install/prerequisites and places
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
    for f in cdk/*; do
      ( [ $f = cdk/runtime ] || skip_platform $f ) && continue
      base=$(basename $f)
      diff <(cd cdk/runtime; [ -e $base ] && find -H $base) \
           <(cd cdk; find -H $base -name runtime -prune -o ! -type d -print) \
           | sed -n 's/^> *\(.*\)$/cdk\/\1/p'
    done
    for p in $prereqs; do
      for d in prerequisites/$p/*; do
        skip_platform $d && continue;
	if is_platform $d; then
	  [ -d $d/bin ] && find $d/bin ! -type d
	  [ -d $d/include ] && find $d/include ! -type d
          [ -d $d/lib ] && find $d/lib -name "*.a"
        elif [[ $d == */include ]]; then
          echo $d
        fi
      done
    done;;
    
  *) echo Unknown export type;;
esac

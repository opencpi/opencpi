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
# Populate the exports tree with the links for files that will be part of the CDK
#
# This script is done in the context of a particular target, as its first argument.
#
# This script must be tolerant of things not existing, and it is called repeatedly as
# more things get built
# This script does not care or drive how these things get built.
#
# This script is driven in part by the "places" file to find OpenCPI libraries and executables
# (the built stuff).

# The sorry state of POSIX/BSD/LINUX/MACOS command compatibility
if [ `uname -s` = Darwin ]; then
  extended="-E ."
  dylib=dylib
else
  extended="-regextype posix-extended"
  dylib=so
fi
if test "$*" = ""; then
  echo "Usage is: makeExportLinks.sh <target> <hyphen-if-not-exists-ok>"
  echo "This script takes two arguments:"
  echo "  <target> in the form of <platform><build-options>, e.g. centos7-d"
  echo "    It is commonly used with the value of the OCPI_TARGET_DIR environment variable."
  echo 'This script is typically used internally by "make exports"'
  echo 'It is designed to be run repeatedly, making links to whatever exists.'
  echo 'Thus it is run several times during the build process.'
  exit 1
fi
if [ "$1" = "-v" -o "$OCPI_EXPORTS_VERBOSE" = 1 ]; then
  verbose=yes
  [ "$1" = "-v" ] && shift
fi
[ -n "$1" -a -n "$verbose" ] && echo Exporting for platform: $1
target=$1
bootstrap=$2
[ -z "$1" ] && target=$OCPI_TOOL_DIR

# match_pattern: Find the files that match the pattern:
#  - use default bash glob, and also
#  - avoids looking at ./exports/
#  - consolidate files that are hard or soft linked into single (first in inode sort order) file
#  - following links so that patterns can match against the link path
function match_pattern {
  local arg=$1
  if [[ $arg == \|* ]]; then
    arg=$(echo "$arg" | sed 's=^|\(.*\)$=./\1=') # add ./ prefix for find command, replacing |
    arg=$(find $extended -regex "$arg")   # expand using find with extended regex
  else
    arg="$(echo $arg)" # normal shell glob behavior
  fi
  local matches=$(shopt -s nullglob; for i in $arg; do echo $i | grep -v '#$' | grep -v '^./exports/'; done)
  [ -z "$matches" ] && return 0
  ls -L -i -d $matches 2>/dev/null | sort -n -b -u | sed 's/^ *[0-9]* *//;s/^\.\///'
}

# Check the exclusion in $1 against the path in $2
# The exclusion might be shorter than the path
# No wild carding here (yet)
function match_filter {
  # echo match_filter $1 $2
  local -a edirs pdirs
  edirs=(${1//\// })
  pdirs=(${2//\// })
  for ((i=0; i<${#pdirs[*]}; i++)); do
    # echo MF:$i:${edirs[$i]}:${pdirs[$i]}:
    if [[ "${edirs[$i]}" == "" ]]; then
      return 0
    elif [[ "${edirs[$i]}" == target-* ]]; then
      if [[ "${pdirs[$i]}" != target-* ]]; then
	return 1
      fi
    elif [[ "${edirs[$i]}" != "${pdirs[$i]}" ]]; then
      return 1
    fi
  done
  return 0
}

function make_relative_link {
  # echo make_relative_link $1 $2
  # Figure out what the relative prefix should be
  up=$(echo $2 | sed 's-[^/]*$--' | sed 's-[^/]*/-../-g')
  link=${up}$1
  if [ -L $2 ]; then
    L=$(ls -l $2|sed 's/^.*-> *//')
    if [ "$L" = "$link" ]; then
      # echo Symbolic link already correct from $2 to $1.
      return 0
    else
      echo "Symbolic link wrong from $2 to $1 (via $link) (was $L), replacing it."
      rm $2
    fi
  elif [ -e $2 ]; then
    if [ -d $2 ]; then
      echo Link $2 already exists, as a directory.
    else
      echo Link $2 already exists, as a regular file.
    fi
    echo '   ' when trying to link to $1
    exit 1
  fi
  mkdir -p $(dirname $2)
  # echo ln -s $link $2
  ln -s $link $2
}

# link to source ($1) from link($2) if neither are filtered
# $3 is the type of object
# exclusions can be filtered by source or target
function make_filtered_link {
  # echo MAKE_FILTERED:$*
  local e;
  local -a edirs
  for e in $exclusions; do
    declare -a both=($(echo $e | tr : ' '))
    # echo EXBOTH=${both[0]}:${both[1]}:$3:$1:$2
    [ "${both[1]}" != "" -a "${both[1]}" != "$3" ] && continue
    # echo EXBOTH1=${both[0]}:${both[1]}:$3:$1:$2
    edirs=(${both[0]/\// })
    if [ ${edirs[0]} = exports ]; then
       if match_filter ${both[0]} $2; then [ -n "$verbose" ] && echo Filtered: $2 >&2 || : ; return; fi
    else
       if match_filter ${both[0]} $1; then [ -n "$verbose" ] && echo Filtered: $1 >&2 || : ; return; fi
    fi
  done
  # No exclusions matched.  Make the directory for the link
  make_relative_link $1 $2
}

# process an addition ($1), and $2 is non-blank for runtime
function do_addition {
  declare -a both=($(echo $1 | tr : ' '))
  [[ $1 == *\<target\>* && $target == - ]] && continue
  rawsrc=${both[0]//<target>/$target}
  set +f
  # old way letting shell default glob do the work:
  # for src in $rawsrc; do
  targets=$(match_pattern "$rawsrc")
  for src in $targets; do
    if [ -e $src ]; then
      dir=${both[1]//<target>/$target}
      base=$(basename $src)
      after=
      if [[ ${both[1]} =~ /$ || ${both[1]} == "" ]]; then
        after=$base
      else
        # export link has a file name, perhaps replace the suffix
        suff=$(echo $base | sed -n '/\./s/.*\(\.[^.]*\)$/\1/p')
        dir=${dir//<suffix>/$suff}
      fi
      make_filtered_link $src exports/$dir$after
      [ -n "$2" ] && make_filtered_link $src exports/runtime/$dir$after
    else
      [ -z "$bootstrap" ] && echo Warning: link source $src does not '(yet?)' exist.
    fi
  done
  set -f
}

set -e
mkdir -p exports
# We do not re-do links so that mod dates are preserved.
# Someday we could remove target links that are "legacy".
#[ -n "$1" -a -d exports/$1 ] && {
#  [ -n "$verbose" ] && echo Exports for platform $1 exist, replacing them.
#  rm -r -f exports/$1
#}
set -f
[ -n "$verbose" ] && echo Collecting exclusions
exclusions=$(test -f Project.exports && egrep '^[[:space:]]*\-' Project.exports | sed 's/^[ 	]*-[ 	]*\([^ 	#]*\)[ 	]*\([^ 	#]*\).*$/\1:\2/') || true
[ -n "$verbose" ] && echo Collecting additions
additions=$(test -f Project.exports && egrep '^[[:space:]]*\+' Project.exports | sed 's/^[ 	]*+[ 	]*\([^ 	#]*\)[ 	]*\([^ 	#]*\).*$/\1:\2/') || true
runtimes=$(test -f Project.exports && egrep '^[[:space:]]*\=' Project.exports | sed 's/^[ 	]*=[ 	]*\([^ 	#]*\)[ 	]*\([^ 	#]*\).*$/\1:\2/') || true
set +f
while read path opts; do
  case "$path" in
    \#*|""|end-of-runtime-for-tools) continue;;
    prerequisites)
      for p in $opts ; do
	shopt -s nullglob
        for l in build/autotools/target-$target/staging/lib/lib$p.*; do
          printf ""
          #  make_filtered_link $l exports/$target/lib/$(basename $l)
          # lsuff=${l##*/lib}
          # make_filtered_link $l exports/$target/lib/libocpi_$lsuff
        done
	shopt -u nullglob
      done
      continue;;
  esac
  directory= library=$(basename $path) dest=lib options=($opts) foreign= tools= driver= useobjs=
  library=${library//-/_}
  exclude= includes= libs= xincludes= runtime=
  while [ -n "$options" ] ; do
    case $options in
      -l) library=${options[1]}; unset options[1];;
      -d) directory=${options[1]}/; unset options[1];;
      -n) dest=noinst;;
      -f) foreign=1;;
      -D) defs="$defs -D${options[1]}"; unset options[1];;
      -t) tools=1;;
      -v) driver=1;;
      -s) useobjs=1;;
      -L) lib=${options[1]}; unset options[1]
	  case $lib in 
	      /*|@*|*/*);;
	      *) lib=libocpi_$lib;;
	  esac
	  libs+=" $lib";;
      -I) xincludes="$xincludes ${options[1]}"; unset options[1];;
      -x) exclude="$exclude -not -regex ${options[1]} -a"; unset options[1];;
      -T) tops="$tops ${options[1]}"; unset options[1];;
      -r) runtime=1;;
      -*) bad Invalid option: $options;;
      *)  bad Unexpected value in options: $options;;
    esac
    unset options[0]
    options=(${options[*]})
  done
  programs=`find -H $path $exclude -name "*_main.cxx"|sed 's=.*src/\(.*\)_main.c.*$=\1='`
  swig=`find -H $path $exclude -path "*/src/*.i"`
  api_incs=`find -H $path $exclude \( -path "*/include/*Api.h" -o -path "*/include/*Api.hh" \)`
  [ -n "$driver" ] && drivers+=" $(basename $path)"
  # echo LIB:$library PATH:$path PROGRAMS:$programs DIRECTORY:$directory
  for p in $programs; do
    dir=$directory
    for t in $tops; do
      if [ "$t" = $p ]; then
        dir=
        break
      fi    
    done
    file=build/autotools/target-$target/staging/bin/$dir$p
    [ -x $file -a "$dir" != internal/ ] && {
	make_filtered_link $file exports/$target/bin/$dir$p
        [ -z "$tools" -o -n "$runtime" ] &&
	    make_filtered_link $file exports/runtime/$target/bin/$dir$p
    }
  done
  shopt -s nullglob
  if [ "$dest" = lib ]; then
    for f in build/autotools/target-$target/staging/lib/libocpi_${library}{,_s,_d}.* ; do
      make_filtered_link $f exports/$target/lib/$(basename $f)
      [ -n "$driver" ] &&
        make_filtered_link $f exports/runtime/$target/lib/$(basename $f)
    done
  fi
  [ -n "$swig" ] && {
    base=$(basename $swig .i)
    for f in build/autotools/target-$target/staging/lib/{,_}$base.* ; do
        make_filtered_link $f exports/$target/lib/$(basename $f)
        # swig would be runtime on systems with python and users' ACI programs that used it
        [ -z "$tools" ] &&
          make_filtered_link $f exports/runtime/$target/lib/$(basename $f)
    done
  }
  shopt -u nullglob
  [ -n "$api_incs" ] && {
      for i in $api_incs; do
        make_filtered_link $i exports/include/aci/$(basename $i)
      done
  }
done < build/places
#if [ -d imports ]; then
#  make_filtered_link imports exports/imports main
#fi

# Add the ad-hoc export links
set -f
[ -n "$verbose" ] && echo Processing additions
for a in $additions; do
  do_addition $a
done
for a in $runtimes; do
  do_addition $a -
done
# After this are only exports done when targets exist
[ "$1" = - ] && exit 0
set +f
# Put the check file into the runtime platform dir
check=$OCPI_TOOL_PLATFORM_DIR/$OCPI_TOOL_PLATFORM-check.sh
[ -r "$check" ] && {
  to=$(python -c "import os.path; print os.path.relpath('"$check"', '.')")
  make_relative_link $to exports/runtime/$OCPI_TOOL_PLATFORM/$(basename $check)
  cat <<-EOF > exports/runtime/$OCPI_TOOL_PLATFORM/$OCPI_TOOL_PLATFORM-init.sh
	# This is the minimal setup required for runtime
	export OCPI_TOOL_PLATFORM=$OCPI_TOOL_PLATFORM
	export OCPI_TOOL_OS=$OCPI_TOOL_OS
	export OCPI_TOOL_DIR=$OCPI_TOOL_PLATFORM
EOF
}
# Put the minimal set of artifacts to support the built-in runtime tests
# And any apps that rely on software components in the core project
rm -r -f exports/runtime/$OCPI_TOOL_PLATFORM/artifacts
mkdir exports/runtime/$OCPI_TOOL_PLATFORM/artifacts
for a in projects/core/artifacts/*:*.*; do
  [ -f $a ] || continue
  link=`readlink -n $a`
  [[ $link == */target-*${target}/* ]] &&
    make_relative_link $a exports/runtime/$OCPI_TOOL_PLATFORM/artifacts/$(basename $a)
done
  
# Ensure driver list is exported
echo $drivers>exports/runtime/$1/lib/driver-list
echo $drivers>exports/$1/lib/driver-list
# Force precompilation of python files right here, but only if we are doing a target
dirs=
for d in `find exports -name "*.py"|sed 's=/[^/]*$=='|sort -u`; do
 python3 -m compileall -q $d
 python3 -O -m compileall -q $d
done

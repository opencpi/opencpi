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
# Options are:
# -v    verbose
# -b    bootstrap - don't care about missing files
# Positional arguments are: <rcc-platform> <rcc-platform-dir> <hdl-platform> <hdl-platform-dir>
# Platform args will be a dash if not supplied
# If there is an HDL platform, we are really focusing on exports for that platform
# In that case the rcc platform supplied is the one that is being associated with the hdl platform
# in this export.

# The sorry state of POSIX/BSD/LINUX/MACOS command compatibility
if [ `uname -s` = Darwin ]; then
  extended="-E ."
else
  extended="-regextype posix-extended"
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
[ "$1" = "-b" ] && bootstrap=1 && shift
[ -n "$1" -a -n "$verbose" ] && echo Exporting for RCC platform: $1 "(HDL platform $3)".
target=$1
rcc_platform=$1
rcc_platform_dir=$2
hdl_platform=$3
hdl_platform_dir=$4
target2=$target
platform=$rcc_platform
platform_dir=$rcc_platform_dir
[ "$hdl_platform" = - ] && hdl_platform=
[ -n "$hdl_platform" ] && {
  target2=$hdl_platform/$rcc_platform
  platform=$hdl_platform
  platform_dir=$hdl_platform_dir
}
[ -z "$target" ] && target=$OCPI_TOOL_DIR
export OCPI_CDK_DIR=`pwd`/bootstrap
# The only things we currently need from ocpitarget.sh is OcpiPlatformOs and OcpiPlatformPrerequisites
[ $rcc_platform != - -a -z "$hdl_platform" ] && source $OCPI_CDK_DIR/scripts/ocpitarget.sh $rcc_platform

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
  if [ ! -e $1 -a -z "$bootstrap" ]; then
    echo Warning: link source $1 does not '(yet?)' exist. >&2
    return
  fi
  # Figure out what the relative prefix should be
  local up
#  [[ $1 =~ ^/ ]] || up=$(echo $2 | sed 's-[^/]*$--' | sed 's-[^/]*/-../-g')
#  link=${up}$1
  link=$(python -c "import os.path; print os.path.relpath('$(dirname $1)','$(dirname $2)')")/
  link+=$(basename $1)
  # echo make_relative_link $1 $2 up:$up link:$link > /dev/tty
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
      echo Link $2 already exists, as a directory. >&2
      echo '   ' when trying to link to $1 >&2
      exit 1
    fi
    echo Link $2 already exists, as a regular file. >&2
    echo '   ' when trying to link to $1 >&2
    # Perhaps the tree has been de-linked (symlinks followed)
    # if contents are the same, reinstate the symlink
    cmp -s $1 $2 || diff -u $1 $2 || exit 1
    echo '   ' but contents are the same.  Link is recreated. >&2
    rm $2
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
    [ -z "${both[1]}" ] && bad UNEXPECTED EMPTY LINK TYPE
    [ "${both[1]}" != "-" -a "${both[1]}" != "$3" ] && continue
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
  [ "$target" = - ] && case $1 in
      *\<target\>*|*\<platform\>*|*\<platform_dir\>*) return;;
  esac
  rawsrc=${both[0]//<target>/$target2}
  rawsrc=${rawsrc//<platform>/$platform}
  rawsrc=${rawsrc/#<platform_dir>/$platform_dir}
  [ -n "$rcc_platform_dir" ] && rawsrc=${rawsrc/#<rcc_platform_dir>/$rcc_platform_dir}
  [ -n "$rcc_platform" ] && rawsrc=${rawsrc//<rcc_platform>/$rcc_platform}
  exp=${both[1]}
  [ -z "$exp" ] && bad unexpected empty second field
  # If not deployment(@) replace with just target else replace with deploy/target
  [ "$2" != "--" ] && exp=${exp//<target>/$target2} || exp=${exp//<target>/deploy/$target2}
  [ -n "$rcc_platform" ] && exp=${exp//<rcc_platform>/$rcc_platform}
  set +f
  targets=$(match_pattern "$rawsrc")
  for src in $targets; do
    # echo do_addition $1 $2 SRC:$src > /dev/tty
    if [ -e $src ]; then
      # figure out the directory for the export
      local dir=
      local srctmp=$src
      if [ -n "${both[2]}" ]; then  # a platform-specific export
        # dir=$target/
        srctmp=${src=#$platform_dir/=}
      fi
      if [[ $exp = - ]]; then
        : # [[ $srctmp == */* ]] && dir+=$(dirname $srctmp)/
      elif [[ $exp =~ /$ ]]; then
        dir+=$exp
      else
        dir+=$(dirname $exp)/
      fi
      # figure out the basename of the export
      local base=$(basename $src)
      local suff=$(echo $base | sed -n '/\./s/.*\(\.[^.]*\)$/\1/p')
      [[ $exp != - && ! $exp =~ /$ ]] && base=$(basename $exp)
      base=${base//<suffix>/$suff}
      # echo For $1 $2
      # echo dir=$dir base=$base
      make_filtered_link $src exports/$dir$base
      [ -n "$2" ] && [ "$2" = "-" ] && make_filtered_link $src exports/runtime/$dir$base
      # Calling make_filtered_link for @ (deployment)
      [ -n "$2" ] && [ "$2" = "--" ] && make_filtered_link $src exports/$dir$base
    else
      [ -z "$bootstrap" ] && echo Warning: link source $src does not '(yet?)' exist. >&2
    fi
  done
  set -f
}
function bad {
    echo Error: $* 1>&2
    exit 1
}
# This function reads an exports file for a certain type of entry and adds entries to the
# requested variable ($1)
# Args are:
# 1. variable name to set with the entries found
# 2. leading character to look for
# 3. file name to read
# 4. a flag indicating platform-specific entries
function readExport {
  local entries=($(egrep '^[[:space:]]*\'$2 $3 |
                   sed 's/^[ 	]*'$2'[ 	]*\([^ 	#]*\)[ 	]*\([^ 	#]*\).*$/\1:\2/'))
  # echo For "$1($3:$4) got ${#entries[@]} entries"
  # make sure there is a second field
  entries=(${entries[@]/%:/:-})
  # add a third field for platform-specifics
  [ -n "$4" ] && entries=(${entries[@]/%/:-})
  eval $1+=\" ${entries[@]}\"
  # eval echo \$1 is now:\${$1}:
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
[ -f Project.exports ] || bad No Project.exports file found for framework.
platform_exports=$platform_dir/$platform.exports
[ -f $platform_exports ] || platform_exports=
[ -n "$verbose" ] && echo Collecting exclusions
readExport exclusions - Project.exports
[ -n "$verbose" ] && echo Collecting additions and runtimes
[ -z "$hdl_platform" ] && readExport additions + Project.exports
[ -z "$hdl_platform" ] && readExport runtimes = Project.exports
[ -n "$platform_exports" ] && {
  echo Using extra exports file for platform $platform: $platform_exports
  readExport additions + $platform_exports -
  readExport runtimes = $platform_exports -
  readExport deployments @ $platform_exports -
  readExport exclusions - $platform_exports -
}
set +f
[ "$target" != - ] && {
[ -n "$verbose" ] && echo Processing framework source-code-based links
while read path opts; do
  case "$path" in
    \#*|""|end-of-runtime-for-tools|prerequisites) continue;;
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
  programs=`find -H $path $exclude -name "[a-zA-Z]*_main.cxx"|sed 's=.*src/\(.*\)_main.c.*$=\1='`
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
#    [ -x $file -a "$dir" != internal/ ] && {
    [ "$dir" != internal/ ] && {
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
        make_filtered_link $f exports/$target/lib/opencpi/$(basename $f)
        # swig would be runtime on systems with python and users' ACI programs that used it
        [ -z "$tools" ] &&
          make_filtered_link $f exports/runtime/$target/lib/opencpi/$(basename $f)
    done
  }
  shopt -u nullglob
  [ -n "$api_incs" ] && {
      for i in $api_incs; do
        make_filtered_link $i exports/include/aci/$(basename $i)
      done
  }
done < build/places
}
# Add the ad-hoc export links
set -f
[ -n "$verbose" ] && echo Processing cdk additions
for a in $additions; do
  do_addition $a
done
[ -n "$verbose" ] && echo Processing runtime additions
for a in $runtimes; do
  do_addition $a -
done
[ -n "$verbose" ] && echo Processing deployment additions
for a in $deployments; do
  do_addition $a --
done
# After this are only exports done when targets exist
[ -n "$hdl_platform" ] && exit 0
[ "$target" = - ] && exit 0
set +f
# Put the check file into the runtime platform dir
# FIXME: make sure if/whether this is really required and why
check=$rcc_platform_dir/${rcc_platform}-check.sh
[ -z "$hdl_platform" -a -r "$check" ] && {
  to=$(python -c "import os.path; print os.path.relpath('"$check"', '.')")
  make_relative_link $to exports/runtime/$target/$(basename $check)
  cat <<-EOF > exports/runtime/$target/${rcc_platform}-init.sh
	# This is the minimal setup required for runtime
	export OCPI_TOOL_PLATFORM=$rcc_platform
	export OCPI_TOOL_OS=$OcpiPlatformOs
	export OCPI_TOOL_DIR=$rcc_platform
	EOF
}
# Put the minimal set of artifacts to support the built-in runtime tests or
# any apps that rely on software components in the core project
rm -r -f exports/runtime/$target/artifacts exports/$target/artifacts
mkdir exports/runtime/$target/artifacts exports/$target/artifacts
for a in projects/core/artifacts/ocpi.core.*; do
  [ -f $a ] || continue
  link=`readlink -n $a`
  [[ $link == */target-*${target}/* ]] && {
    make_relative_link $a exports/runtime/$target/artifacts/$(basename $a)
    make_relative_link $a exports/$target/artifacts/$(basename $a)
  }
done

# Ensure driver list is exported
echo $drivers>exports/runtime/$1/lib/driver-list
echo $drivers>exports/$1/lib/driver-list

# Enable prerequisite libraries to be found/exported in our lib directory
function liblink {
  local base=$(basename $1)
  if [[ -L $l && $(readlink $1) != */* ]]; then
    cp -R -P $1 $2/$target/lib/$base
  else
    make_relative_link $1 $2/$target/lib/$base
  fi
}
# Enable prerequisites to be found/exported in directory of choosing
function anylink {
  local base=$(basename $2)
  if [[ -L $2 && $(readlink $2) != */* ]]; then
    cp -R -P $2 $3/$target/$1/$base
  else
    make_relative_link $2 $3/$target/$1/$base
  fi
}
shopt -s nullglob
for p in prerequisites/*; do
  for l in $p/$target/lib/*; do
    liblink $l exports
    if [[ $l == *.so || $l == *.so.* || $l == *.dylib ]]; then
       liblink $l exports/runtime
    fi
  done
done
# Some extra prereqs need to be part of our exports... exporting those here
for p in $OcpiPlatformPrerequisites; do
  # Prerequisites can be in the form: <prerequisite>:<platform>
  # Below we are removing the : and everything after it so we are left with only the prereq
  p=$(echo $p | cut -d: -f1)
  p="prerequisites/$p"
  for l in $p/$target/*; do
    if [[ "$l" = *conf ]]; then
      for f in $l/*; do
        # AV-4799 need to use different file internally
        if [ -e "releng/config_files/$(basename $f)" ]; then
          f="releng/config_files/$(basename $f)"
        fi
        anylink "" $f exports
        anylink "" $f exports/runtime
      done
    elif [[ "$l" = *bin ]]; then
      for f in $l/*; do
        anylink bin $f exports
        anylink bin $f exports/runtime
      done
    fi
  done
done
shopt -u nullglob

# If we are not building on the target platform do not pre-compile python AV-4850
if [ "$OCPI_TOOL_DIR" = "$target" ]; then
  # Force precompilation of python files right here, but only if we are doing a target
  py=python3
  command -v python3 > /dev/null || py=/opt/local/bin/python3
  dirs=
  for d in `find exports -name "*.py"|sed 's=/[^/]*$=='|sort -u`; do
    $py -m compileall -q $d
    $py -O -m compileall -q $d
  done
fi

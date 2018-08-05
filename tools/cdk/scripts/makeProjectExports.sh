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
# Populate the exports tree at the top level of this project with links to this
# project's assets, allowing them to be used by other projects.
#
# If a "target" argument is supplied (as opposed to specifying the first argument
# "-", it will allow target-specific assets to be exported if they exist.
# This will not affect other target-specific assets that have been exported by
# previous runs of this script.
#
# This script is tolerant of things not existing, and it is called repeatedly as
# more things get built
#
# There are four types of lines in the exports file:
# 1. Comment or blank lines.   Comments start with #
# 2. asset lines indicating assets to be exports (see below)
# 3. Addition lines starting with +, which export specific directories or files
# 4. Subtraction lines starting with -, which suppress exporting specific files
#    when ther are implies by asset lines
#
# Asset lines are similar to ocpidev "nouns", and indicate that assets should
# be made available to other projects.
# Asset lines are either singular/specific or plural
# When plural then indicate all assets of that type should exported.
# When singular/explicit, then a specific asset is exported.
# the asset lines that are supported are
# "all", which must means all assets that can be exported will be exported.
# "hdl platforms" - all possible hdl platforms
# "hdl platform <platform>" - a specific hdl platform
# "platforms" - all possible platforms
# "rcc platforms" - all possible rcc platforms
# "rcc platform <platform" - a specific rcc platform
# "hdl primitives" - all possible hdl platforms
# "hdl primitive <primitice>" - a specific hdl primitive
# "libraries" - all component libraries under "components"
# "library <library>" - a specific component library: components, under components, or a path to the library
# "spec" - a spec in the top level specs directory
# "specs" - all specs in the top level specs directory
# "hdl devices" - export the entire hdl/devices library

# The sorry state of POSIX/BSD/LINUX/MACOS command compatibility
extended=$(if [ `uname -s` = Darwin ]; then echo -E .; else echo . -regextype posix-extended; fi)
[ -z "$OCPI_CDK_DIR" -o ! -d "$OCPI_CDK_DIR" ] && echo "Error: OCPI_CDK_DIR environment setting not valid: '${OCPI_CDK_DIR}'" && exit 1
source $OCPI_CDK_DIR/scripts/util.sh

# match_pattern: Find the files that match the pattern:
#  - use default bash glob, and also
#  - avoids looking at ./exports/
#  - consolidate files that are hard or soft linked into single (first in inode sort order) file
#  - following links so that patterns can match against the link path
function warn_check { 
  local arg=$1
  if [ -n "$verbose" ]; then 
    echo $arg 
  fi
}
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
      echo Symbolic link wrong from $2 to $1 wrong \(was $L\), replacing it.
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
  local e;
  local -a edirs
  for e in $exclusions; do
    declare -a both=($(echo $e | tr : ' '))
    # echo EXBOTH=${both[0]}:${both[1]}:$3:$1:$2
    [ "${both[1]}" != "" -a "${both[1]}" != "$3" ] && continue
    # echo EXBOTH1=${both[0]}:${both[1]}:$3:$1:$2
    edirs=(${both[0]/\// })
    if [ ${edirs[0]} = exports ]; then
       if match_filter ${both[0]} $2; then return; fi
    else
       if match_filter ${both[0]} $1; then return; fi
    fi
  done
  # No exclusions matched.  Make the directory for the link
  make_relative_link $1 $2
}

function bad {
    echo Error: $* 1>&2
    exit 1
}
function needsnoun {
    echo Error: $* 1>&2
    exit 1
}
function nonoun {
    echo Error: $* 1>&2
    exit 1
}
function topdirs {
    # echo topdirs:$1:$2 > /dev/tty
    local dirs=`shopt -s nullglob; for d in $1/*; do [ -d $d -a $(basename $d) != lib ] && echo $(basename $d); done`
    local -a rdirs
    if [ -n "$2" ]; then
      for d in $dirs; do
	  if test -f $1/$d/Makefile && egrep -q "^[ 	]*include[ 	]*.*/include/$2.mk" $1/$d/Makefile; then
	    rdirs+=($d)
	  else
	    [ -n "$verbose" ] && echo Ignoring $d when looking for $2 1>&2
	  fi	    
      done
    else
	rdirs=($dirs)
    fi
    # echo topdirs returning:${rdirs[*]} > /dev/tty
    echo ${rdirs[@]}
}
function checkfiles {
    f=$(basename $1)
    d=$(dirname $1)
    shift
    for x in $*; do
	eval t=$d/$f/$x
	[ -f $t ] || echo file $t is missing
    done
}
if test "$*" = ""; then
  echo "Usage is: makeProjectExports.sh [-v] <target>"
  echo "This script takes one arguments:"
  echo "  <target> in the form of <os>-<version>-<machine>[/mm], e.g. linux-c6-x86_64"
  echo "    It is commonly used with the value of the OCPI_TARGET_DIR environment variable."
  echo 'This script is typically used internally by "make exports" in projects'
  echo 'It is designed to be run repeatedly, making links to whatever exists.'
  echo 'Thus it is run several times during the build process, usually after each target is built.'
  echo
  echo 'A -v option my be provided before other arguments to enable verbose progress messages.'
  exit 1
fi
if [ "$1" = "-v" -o "$OCPI_EXPORTS_VERBOSE" = 1 ]; then
  verbose=yes
  [ "$1" = "-v" ] && shift
  [ -n "$verbose" ] && echo Setting verbose mode.
fi
os=$(echo $1 | sed 's/^\([^-]*\).*$/\1/')
dylib=$(if [ "$os" = macos ]; then echo dylib; else echo so; fi)
set -e
set -f
exports=Project.exports
[ -n "$verbose" ] && echo Collecting exclusions
exclusions=$(test -f $exports && grep '^[ 	]*-' $exports | sed 's/^[ 	]*-[ 	]*\([^ 	#]*\)[ 	]*\([^ 	#]*\).*$/\1:\2/') || true
[ -n "$verbose" ] && echo Collecting additions
additions=$(test -f $exports && grep '^[ 	]*+' $exports | sed 's/^[ 	]*+[ 	]*\([^ 	#]*\)[ 	]*\([^ 	#]*\).*$/\1:\2/') || true
set +f
[ -n "$verbose" ] && echo Collecting assets
assets=$(test -f $exports && grep -v '^[ 	]*[-+#]' $exports | grep -v '^[ 	]*$' | \
  sed 's/^[ 	]*\([^#]*\).*$/\1/' | sed 's/  */:/g' || true)
[ -n "$verbose" ] && echo Assets: $assets
# parse/error-check asset "nouns" accumulating then into variables named by the type of asset.
# These are not yet supported:  hdl_assemblies
nouns=(hdl_primitives hdl_platforms hdl_libraries rcc_platforms libraries specs artifacts)
for n in ${nouns[*]}; do eval declare -a $n; done
allrequested=
for a in $assets; do
  declare -a args=($(echo $a | tr : ' '))
  case ${args[0]} in
      all) allrequested=1;;
      *) [ -n "$allrequested" ] && bad If \"all\" is specified, nothing else can be specified;;
  esac
done
[ -n "$allrequested" ] &&
    assets=$(echo ${nouns[*]} | tr _ :)
[ -n "$verbose" ] && echo Assets: $assets
for a in $assets; do
  declare -a args=($(echo $a | tr : ' '))
  model= needname= noun= name=
  # echo A:$a ARGS:${args[*]}
  while [ -n "$args" ]; do
      # echo args:${args[*]} noun:$noun needname:$needname name:$name
      arg=${args[0]}
      unset args[0]
      [ -n "$noun" ] && {
	  [ -z "$needname" ] && bad extraneous argument \"$arg\" invalid here
	  [ -n "$name" ] && bad extraneous argument \"$arg\" after name \"$name\" invalid here
	  name=$arg
      }
      case $arg in
	  # model qualifiers
	  hdl|HDL|rcc|RCC|ocl|OCL)
	      [ -n "$model" ] && bad \"$arg\" is invalid since model is already specified at \"$model\"
	      model=$arg;;
	  # nouns that need model qualifiers
	  primitive)
	      [ "$model" != hdl ] && bad primitive only supported for HDL model
	      case "${args[1]}" in
		  library|core)
		      noun=${model}_primitives; unset args[1];
		      needname=${model}/primitives
		      ;;
		  *)
		      bad only \"library\" or \"core\" is valid after \"primitive\", not ${args[0]};;
	      esac;;
	  platform)
	      [ "$model" != hdl -a "$model" != ocl ] &&
		  bad \"platform\" must be preceded by either \"rcc\" or \"hdl\"
	      noun=${model}_platforms; needname=$model/platforms;;
	  primitives|assemblies)
	      [ "$model" != hdl ] && bad \"$arg\" only supported for HDL model
	      all=`topdirs $model/$arg "hdl/hdl-(core|library|lib)"`
	      if [ -z "$all" ]; then
		  warn_check "Warning:  cannot export $model $arg since none exist" 
	      else
		  eval ${model}_$arg=\(${all[*]}\)
	      fi;;
	  platforms)
	      case "$model" in
		  hdl) allhdl=`topdirs hdl/platforms hdl/hdl-platform`;;
		  rcc) allrcc=`topdirs rcc/platforms`;;
		  "")
		      allhdl=`topdirs hdl/platforms hdl/hdl-platform`
		      allrcc=`topdirs rcc/platforms`;;
	      esac
	      if [ -z "$allrcc$allhdl" ]; then
		  warn_check "Warning:  cannot export ${model}${model+ }platforms since none exist."
	      else
		  [ -n "$allrcc" ] && {
		      for p in $allrcc; do
			  warn=`checkfiles rcc/platforms/$p '$f.mk'`
			  if [ -n "$warn" ]; then
			      warn_check "Warning:  cannot export RCC platform $p: $warn"
			  else
			      rcc_platforms+=($p)
			  fi
		      done
		  }
		  [ -n "$allhdl" ] && {
		      for p in $allhdl; do
			  warn=`checkfiles hdl/platforms/$p Makefile '$f.mk'`
			  if [ -n "$warn" ]; then
			      warn_check "Warning:  cannot export HDL platform $p: $warn"
			  else
			      hdl_platforms+=($p)
			  fi
		      done
		  }
	      fi;;
	  library|libraries)
	      dir=.
	      if [ -n "$model" ]; then
	        dir=$model/
		noun=${model}_libraries
              fi
	      if [ "$arg" = library ] ; then
		  if [ -z "$model" ] ; then
		      if [ "${args[1]}" = components ] ; then
		        needname=.
		      else
		        needname=components
		      fi
		  else
		      needname=$model
		  fi
	      else
		  if [ -z "$model" ] ; then
		      if [ -f components/Makefile ] &&
			     egrep -q '^[ 	]*include[ 	]*.*/include/(lib|library).mk' \
				   components/Makefile; then
			 libraries=(components)
		     else
			 libraries=(`topdirs components "(lib|library)"`)
		     fi
		  else
		      # echo $noun=\(`topdirs $model "(lib|library)"`\)
		      for l in `topdirs $model "(lib|library)"`; do eval $noun+=\($model/$l\); done
		  fi
	      fi;;
	  spec|specs)
	      [ -n "$model" ] && bad no model prefix is allowed before \"$arg\"
	      if [ "$arg" = specs ] ; then
		  if [ -d specs ]; then
		      specs+=(`shopt -s nullglob; for i in specs/*.xml specs/package-id; do [ -f $i ] && echo $(basename $i); done`)
		  else
		      [ -n "$allrequested" ] || warn_check "Warning:  cannot export specs since no specs exist in this project."
		  fi
	      else
		  noun=specs
		  needname=specs
	      fi;;
	  artifacts)
	      artifacts="$(shopt -s nullglob; for i in artifacts/*; do [ -f $i ] && echo $(basename $i) || :; done)"
	      noun=artifacts;;
      esac
      args=(${args[*]})
  done
  [ -n "$needname" ] && {
      [ -z "$name" ] && bad name is missing for \"$noun\"
      [ -d $needname/$name ] || bad there is no \"$noun\" named \"$name\"
      [ -n "$check" ] && {
   	 err=`checkfiles $needname/$name $check`
   	 [ -n "$err" ] && bad When processing $noun $name, $err
      }
      eval $noun+=\($name\)
      # eval echo $noun='${'$noun'[*]}'
  }
done
for i in ${nouns[*]}; do
    eval x=\${$i[*]}
    # echo $i is \( $x \)
done
###################################################################################
# Before we deal with asset exports there are some special case items to export
mkdir -p exports
[ -n "$2" -a "$2" != "-" ] && \
  if [[ "$2" != `cat exports/project-package-id 2>/dev/null` ]]; then
    echo "$2" > exports/project-package-id;
  fi
if [ -d imports ]; then
  make_filtered_link imports exports/imports main
fi
###################################################################################
# Now we start exporting assets by type
#
# Export hdl platforms
[ -n "$verbose" -a -n "$hdl_platforms" ] && echo Processing hdl platforms
for p in ${hdl_platforms[*]}; do
  d=hdl/platforms/$p
  [ -f $d/Makefile -a -f $d/$p.mk ] || bad HDL platform $p not exported due to missing files in $d
  [ -d $d/lib ] && make_filtered_link $d/lib exports/lib/platforms/$p platform
  # this $p.mk link is for bootstrapping before the platform is built to show it exists
  make_filtered_link $d/$p.mk exports/lib/platforms/mk/$p.mk platform
done
###################################################################################
# Export rcc platforms
[ -n "$verbose" -a -n "$rcc_platforms" ] && echo Processing rcc platforms
for p in ${rcc_platforms[*]}; do
  d=rcc/platforms/$p
  [ -f $d/$p.mk -o -f $d/$p-target.mk ] || bad RCC platform $p not exported due to missing files in $d
  make_filtered_link $d exports/lib/rcc/platforms/$p rcc-platform
done

###################################################################################
# Export component libraries at top level and under hdl
[ -n "$verbose" -a -n "${libraries[*]}" ] && echo Processing component libraries
for l in ${libraries[*]} ${hdl_libraries[*]} ${rcc_libraries[*]} ; do
  case $l in
      components) d=components; n=components;;
      */*) d=$l; n=$(basename $l);;
      *) d=components/$l; n=$l;;
  esac
  [  -d $d -a  -f $d/Makefile ] || bad Component library $l not exported due to missing files in $d
  egrep -q '^[ 	]*include[ 	]*.*/include/(lib|library).mk' $d/Makefile ||
      bad Component library $l failed due to missing/wrong '"include"' line in $d/Makefile
  make_filtered_link $d/lib exports/lib/$n component
done

###################################################################################
# Export component libraries at top level and under hdl
[ -n "$verbose" -a -n "${hdl_primitives[*]}" ] && echo Processing hdl primitives
# Add hdl primitives: they must be there before they are built
# since they depend on each other
for p in ${hdl_primitives[*]}; do
    d=hdl/primitives/$p
    [ -f $d/Makefile ] || bad HDL primitive $p not exported due to missing Makefile in $d
    if grep -q '^[ 	]*include[ 	]*.*/include/hdl/hdl-core.mk' $d/Makefile; then
	type=core
    elif egrep -q '^[ 	]*include[ 	]*.*/include/hdl/hdl-(lib|library).mk' $d/Makefile; then
	type=library
    else
      bad HDL primitive library $p due to missing or bad '"include"' line in $d/Makefile
    fi
    make_filtered_link hdl/primitives/lib/$p exports/lib/hdl/$p primitive
    [ $type = core ] &&
      make_filtered_link hdl/primitives/lib/${p}_bb exports/lib/hdl/${p}_bb primitive
done

###################################################################################
# Export specs at top level
[ -n "$verbose" -a -n "${specs}" ] && echo Processing top-level specs
for s in ${specs[*]}; do
    make_filtered_link specs/$s exports/specs/$s spec
done
###################################################################################
# Export artifacts at top level
[ -n "$verbose" -a -n "$artifacts" ] && echo Processing the artifacts
for s in ${artifacts[*]}; do
    make_filtered_link artifacts/$s exports/artifacts/$s artifact
done
###################################################################################
# Leftover assets not handled yet:
#


###################################################################################
# Export the ad-hoc export links introduced by lines starting with +
set -f
[ -n "$verbose" -a -n "$additions" ] && echo Processing additions
for a in $additions; do
  declare -a both=($(echo $a | tr : ' '))
  [[ $a == *\<target\>* && $1 == - ]] && continue
  rawsrc=${both[0]//<target>/$1}
  set +f
  # old way letting shell default glob do the work:
  # for src in $rawsrc; do
  targets=$(match_pattern "$rawsrc")
  for src in $targets; do
  if [ -e $src ]; then
    dir=exports/${both[1]//<target>/$1}
    base=$(basename $src)
    after=
    if [[ ${both[1]} =~ /$ || ${both[1]} == "" ]]; then
      after=$base
    else
      # export link has a file name, perhaps replace the suffix
      suff=$(echo $base | sed -n '/\./s/.*\(\.[^.]*\)$/\1/p')
      dir=${dir//<suffix>/$suff}
    fi
    make_relative_link $src $dir$after
  else
    if [ "$2" == "" ]; then
      echo Warning: link source $src does not '(yet?)' exist.
    fi
  fi
  done
  set -f
done
set +f
# This is speclinks for all libraries without recursing into python, then make, then python,
# and requiring imports to exist to find platforms we aren't using etc. etc. etc. 
# This script is really a "leaf" script that should not be recursing back into all the
# other make machinery.  If necessary before the python rewrite, we could have a
# shared implementation some other way that avoided all the recursion.
# Or change the python to do this and not use "make".

# export the specs for each of the libraries
python3 -c "import sys; sys.path.append(\"$OCPI_CDK_DIR/scripts/\");\
           import ocpiutil; ocpiutil.export_libraries()"

exit 0
notes:
assets:
 bitstream executables are exported
 device specs are exported as specs, as well as platform devices
 as well as all component library specs
core
 top level specs +specs/



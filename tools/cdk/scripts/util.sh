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
# Get the project registry directory. This is OCPI_PROJECT_REGISTRY_DIR,
# or OCPI_CDK_DIR/../project-registry, or /opt/opencpi/cdk.
# If in a development environment, use the fully functional python function
# for determining the registry location.
#   The python function is used because if currently in a project, it uses
#   the current project's imports link as the project-registry
# Otherwise, this is likely a runtime-only evironment, and the registry should
# be determined based solely on the environment and defaults.
function getProjectRegistryDir {
  if [ -n "$OCPI_CDK_DIR" -a -n "$(command -v python3 2> /dev/null)" -a -r $OCPI_CDK_DIR/scripts/ocpiutil.py ]; then
    python3 -c "\
import sys; sys.path.insert(0,\"$OCPI_CDK_DIR/scripts/\");
import ocpiassets; print (ocpiassets.Registry.get_registry_dir());"
  elif [ -n "$OCPI_PROJECT_REGISTRY_DIR" ]; then
    echo $OCPI_PROJECT_REGISTRY_DIR
  elif [ -n "$OCPI_CDK_DIR" ]; then
    # Return default registry relative to CDK
    echo $OCPI_CDK_DIR/../project-registry
  else
    # Return default global registry installation location
    echo /opt/opencpi/project-registry
  fi
}

# include all possible projects that can be searched. This includes
# OCPI_PROJECT_PATH, the contents of the project registry and OCPI_CDK_DIR.
function getProjectPathAndRegistered {
  registry_dir=$(getProjectRegistryDir)
  echo ${OCPI_PROJECT_PATH//:/ } \
           `test -d "$registry_dir" && find $registry_dir -mindepth 1 -maxdepth 1 -not -type f` $OCPI_CDK_DIR
}

# look for the name $1 in the directory $2 in the project path, and set $3 to the result
# return 0 on found, 1 on not found
function findInProjectPath {
  for p in $(getProjectPathAndRegistered); do
    [ -d $p/exports ] && p=$p/exports
    if [ "${OCPI_LOG_LEVEL:-0}" -gt 7 ]; then
      echo "OCPI(           ): looking for $p/$2/$1" # TODO / FIXME - add timestamp similar to rest of debug printouts
    fi
    if [ -e $p/$2/$1 ] ; then
      eval ${3}=$p/$2/$1
      return 0
    fi
  done
  return 1
}

# First arg is .mk file to use
# second arg is Make arg to invoke the right output
#    which can be an assignment or a target
# third arg is verbose
function setVarsFromMake {
  local quiet
  [ -z "$3" ] && quiet=1
  [ -z "$(command -v make 2> /dev/null)" ] && {
    echo The '"make"' command is not available. 2>&1
    return 1
  }
  local vars
  vars=$(set -o pipefail;\
         eval make -n -r -s -f $1 $2 ${quiet:+2>/dev/null} | \
	 grep '^[a-zA-Z_][a-zA-Z_]*=')
  [ $? != 0 ] && return 1
  eval $vars
}

function isPresent {
    local key=$1
    shift
    local vals=($*)
    for i in $*; do if [ "$key" = "$i" ]; then return 0; fi; done
    return 1
}

# Is $1 ok with $2 being "only" and $3 being "exclude"
function onlyExclude {
  local key=$1
  local only=$2
  local exclude=$3
  shift
  if ! isPresent $key $exclude && ( [ -z "$only" ] || isPresent $key $only ) then
     return 0
  fi
  return 1
}

# This is a copy of a function from makeExportLinks.sh, due to bootstrapping issues
# FIXME: allow makeExportLinks.sh to use this one
function makeRelativeLink {
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

# Capture initial time for seconds elapsed time
# one optional argument is variable to set in case things are nested
function setStartTime {
  local var=$1
  [ -z "$var" ] && var=_start_time_
  eval $var=$(date -u +%s)
}

# Print elapsed time on stdout in %T (HH:MM:SS) format
# optional argument is variable name of start time set with setStartTime
function getElapsedTime {
  local var=$1
  [ -z "$var" ] && var=_start_time_
  local elapsed=$(( $(date -u +%s) - $var))
  [ -z "$_end_time_" ] && {
    if [ "$(uname -s)" = Darwin ]; then
      _end_time_='-r '
    else
      _end_time_='-d @'
    fi
  }
  echo $(date $_end_time_$elapsed -u +%T) '('$elapsed seconds')'
}

# echo the platform we are running on
function ocpiGetToolPlatform {
  ocpiGetToolDir -
  return 0
}

# echo the tool dir, setting OCPI_TOOL_DIR as a side effect
function ocpiGetToolDir {
  [ -n "$OCPI_TOOL_DIR" ] || {
    GETPLATFORM=$OCPI_CDK_DIR/scripts/getPlatform.sh
    if test ! -f $OCPI_CDK_DIR/scripts/getPlatform.sh; then
      echo Error:  cannot find $OCPI_CDK_DIR/scripts/getPlatforms.sh 1>&2
      exit 1
    fi
    read v0 v1 v2 v3 v4 v5 <<< `${GETPLATFORM}`
    if test "$v5" == "" -o $? != 0; then
      echo Error:  Failed to determine runtime platform. 1>&2
      exit 1
    fi
    # We always set this as a side-effect
    export OCPI_TOOL_PLATFORM=$v4
    export OCPI_TOOL_PLATFORM_DIR=$v5
    export OCPI_TOOL_DIR=$v4
  }
  [ "$1" = - ] || echo $OCPI_TOOL_DIR
  return 0
}

function ocpiGetToolOS {
  ocpiGetToolDir -
  echo ${OCPI_TOOL_DIR/-*/}
  return 0
}

# do readlink -e, but more portably
# There are 100 ways to do this....
function ocpiReadLinkE {
  [ -f $1 -o -d $1 ] && python3 -c 'import os; print (os.path.realpath("'$1'"))'
}

function ocpiDirType {
  [ -d $1 -a -f $1/Makefile ] && {
      local type=`sed -n 's=^[ 	]*include[ 	]*.*OCPI_CDK_DIR.*/include/\(.*\)\.mk.*=\1=p' $1/Makefile | tail -1 2>/dev/null`
      local rc=$?
      # echo ocpiDirType of $1: rc: $rc type: $type > /dev/tty
      [ $rc = 0 ] && echo $type
  }
}

OcpiEcho=/bin/echo

if [ "$1" == __test__ ] ; then
  if eval findInProjectPath $2 $3 result ; then
    echo good result is $result
  else
    echo bad result
  fi
fi

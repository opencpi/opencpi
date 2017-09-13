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

# look for the name $1 in the directory $2 in the project path, and set $3 to the result
# return 0 on found, 1 on not found
function findInProjectPath {
  for p in ${OCPI_PROJECT_PATH//:/ } $OCPI_CDK_DIR ; do
    [ -d $p/exports ] && p=$p/exports
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
  [ -z $(command -v make 2> /dev/null) ] && {
    [ -n "$3" ] && echo The '"make"' command is not available. 2>&1
    return 1
  }
  eval $(eval make -n -r -s -f $1 $2 \
	 ${quiet:+2>/dev/null} | grep '^[a-zA-Z_][a-zA-Z_]*=')
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
    if [ $(uname -s) = Darwin ]; then
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
    GETPLATFORM=$OCPI_CDK_DIR/platforms/getPlatform.sh
    if test ! -f $OCPI_CDK_DIR/platforms/getPlatform.sh; then
      echo Error:  cannot find $OCPI_CDK_DIR/platforms/getPlatforms.sh 1>&2
      exit 1
    fi
    read v0 v1 v2 v3 v4 <<-EOF
	`${GETPLATFORM}`
	EOF
    if test "$v0" == "" -o $? != 0; then
      echo Error:  Failed to determine runtime platform. 1>&2
      exit 1
    fi
    # We always set this as a side-effect
    export OCPI_TOOL_PLATFORM=$v4
    # Determine OCPI_TOOL_MODE if it is not set already
    # It can be set to null to suppress these modes, and just use whatever has been
    # built without modes.
    if test "$OCPI_USE_TOOL_MODES" = "1"; then
      if test "$OCPI_TOOL_MODE" = ""; then
        # OCPI_TOOL_MODE not set at all, just look for one
        for i in sd so dd do; do
          if test -x "$OCPI_CDK_DIR/$v3/$i/ocpirun"; then
            export OCPI_TOOL_MODE=$i
            echo "Choosing tool mode "$i" since there are tool executables for it." 1>&2
            break
          fi
        done
      fi
      if [ -z "$OCPI_TOOL_MODE"]; then
        if test ! -x "$OCPI_CDK_DIR/bin/$v3/ocpirun"; then
          echo "Could not find any OpenCPI executables in $OCPI_CDK_DIR/$v3/*"
          if test "$OCPI_DEBUG" = 1; then do=d; else do=o; fi
          if test "$OCPI_DYNAMIC" = 1; then sd=d; else sd=s; fi
          export OCPI_TOOL_MODE=$sd$do
          echo "Hopefully you are building OpenCPI from scratch.  Tool mode will be \"$OCPI_TOOL_MODE\"". 1>&1
        fi
      fi
      export OCPI_TOOL_MODE=$v3/$OCPI_TOOL_MODE
    else
      export OCPI_TOOL_DIR=$v3
    fi
  }
  [ "$1" = - ] || echo $OCPI_TOOL_DIR
  return 0
}

function ocpiGetToolOS {
  ocpiGetToolDir -
  echo ${OCPI_TOOL_DIR/-*/}
  return 0
}
if [ "$1" == __test__ ] ; then
  if eval findInProjectPath $2 $3 result ; then
    echo good result is $result
  else
    echo bad result
  fi
fi

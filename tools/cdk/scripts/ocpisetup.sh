# This script is for example/application development.
# It is ALSO called during native environment setup when building the core tree.
# In this case the CDK is skeletal, but still usable
# This script should be sourced to set the OpenCPI CDK environment variables
# Since the (at least) the iVeia bash does not have extdebug/BASH_ARGV support, there is no
# way for this script to know where it lives.  Hence it requires its first arg to be that.
_MYNAME=ocpisetup.sh
if test $# == 0; then
  if test `basename $0` == $_MYNAME; then
    echo Error: ocpisetup.sh can only be run using the \".\" or \"source\" command. 1>&2
    exit 1
  fi
  if test "$BASH_SOURCE" == ""; then
    echo Error: ocpisetup.sh cannot determine where it is.  Supply its path as first arg. 1>&2
    exit 1
  fi
  _MYPATH=$BASH_SOURCE
elif test $# == 1; then
  _MYPATH=$1
fi
if test `basename $_MYPATH` != "$_MYNAME"; then
  if test $# == 1; then
    echo Error: ocpisetup.sh must be given its path as its first argument. 1>&2
  else
    echo Error: ocpisetup.sh can only be run using the \".\" or \"source\" command. 1>&2
  fi
  exit 1
elif test -f $_MYNAME; then
  echo Error: ocpisetup.sh can only be run from a different directory. 1>&2
  exit 1
fi
# Determine OCPI_CDK_DIR:
# If set and it is a directory, assume it is correct
# If not set, and if this script lives in a "scripts" directory,
# assume that "scripts" directory is at the top of the CDK.
# Otherwise, try the global default place.
# In all cases do a final sanity check.
# Note that this script will always be executed in a real CDK since
# even when executed in a cleaned core tree, it will be called after an initial
# export tree is created.
if [ "$OCPI_CDK_DIR" = "" -o ! -d "$OCPI_CDK_DIR" ]; then # and any other sanity checks?
  if [ $(basename $(dirname $_MYPATH)) = scripts ]; then
    # Be careful to maintain symbolic link names for UI clarity
    DIRDIR=$(dirname $(dirname $_MYPATH))
    if [[ $DIRDIR =~ /* ]]; then
      OCPI_CDK_DIR=$DIRDIR
    else
      OCPI_CDK_DIR=$(pwd)/$DIRDIR
    fi
  else
    OCPI_CDK_DIR=/opt/opencpi/cdk
  fi
  export OCPI_CDK_DIR=$(cd $OCPI_CDK_DIR; pwd)
  # FIXME: someday better "signature" would be more robust...
  if test ! -f $OCPI_CDK_DIR/platforms/getPlatform.sh; then
    echo Error:  ocpisetup.sh cannot find a valid place to set OCPI_CDK_DIR "(tried $OCPI_CDK_DIR)"
    exit 1
  fi
fi
if test "$OCPI_DYNAMIC" = ""; then
  if test "$OCPI_BUILD_SHARED_LIBRARIES" != ""; then
    export OCPI_DYNAMIC=$OCPI_BUILD_SHARED_LIBRARIES
  else
    export OCPI_DYNAMIC=0
  fi
fi
export OCPI_BUILD_SHARED_LIBRARIES=$OCPI_DYNAMIC

if test "$OCPI_TOOL_HOST" = ""; then
  GETPLATFORM=$OCPI_CDK_DIR/platforms/getPlatform.sh
  vars=($(${GETPLATFORM}))
  if test $? != 0; then
    echo Failed to determine runtime platform.
    return 1
  fi
  export OCPI_TOOL_OS=${vars[0]}
  export OCPI_TOOL_OS_VERSION=${vars[1]}
  export OCPI_TOOL_ARCH=${vars[2]}
  export OCPI_TOOL_HOST=${vars[3]}
  export OCPI_TOOL_PLATFORM=${vars[4]}
fi
# Determine OCPI_TOOL_MODE if it is not set already
# It can be set to null to suppress these modes, and just use whatever has been
# built without modes.
if test "$OCPI_USE_TOOL_MODES" != ""; then
  # OCPI_TOOL_MODE not set at all, just look for one
  for i in sd so dd do; do
    if test -x "$OCPI_CDK_DIR/$OCPI_TOOL_HOST/$i/ocpirun"; then
      export OCPI_TOOL_MODE=$i
      echo "Choosing tool mode "$i" since there are tool executables for it."
      break
    fi
  done
  if test "$OCPI_TOOL_MODE" = ""; then
    if test ! -x "$OCPI_CDK_DIR/bin/$OCPI_TOOL_HOST/ocpirun"; then
      echo "Could not find any OpenCPI executables in $OCPI_CDK_DIR/$OCPI_TOOL_HOST/*"
      if test "$OCPI_DEBUG" = 1; then do=d; else do=o; fi
      if test "$OCPI_BUILD_SHARED_LIBRARIES" = 1; then sd=d; else sd=s; fi
      export OCPI_TOOL_MODE=$sd$do
      echo "Hopefully you are building OpenCPI from scratch.  Tool mode will be \"$OCPI_TOOL_MODE\"".
    else
      export OCPI_TOOL_MODE=
    fi
  fi
fi
if test "$OCPI_TOOL_MODE" = ""; then
  export OCPI_TOOL_DIR=$OCPI_TOOL_HOST
else
  export OCPI_TOOL_DIR=$OCPI_TOOL_HOST/$OCPI_TOOL_MODE    	  
fi
# The tool mode and dir are set now, and are needed to set PATH below.
if test "$OCPI_TARGET_HOST" = ""; then
  export OCPI_TARGET_OS=$OCPI_TOOL_OS
  export OCPI_TARGET_OS_VERSION=$OCPI_TOOL_OS_VERSION
  export OCPI_TARGET_ARCH=$OCPI_TOOL_ARCH
  export OCPI_TARGET_HOST=$OCPI_TOOL_HOST
  export OCPI_TARGET_PLATFORM=$OCPI_TOOL_PLATFORM
fi
if test "$OCPI_TARGET_DIR" = ""; then
  if test "$OCPI_USE_TARGET_MODES" != ""; then
    if test "${OCPI_TARGET_MODE+UNSET}" = ""; then
      if test "$OCPI_DEBUG" = 1; then do=d; else do=o; fi
      if test "$OCPI_DYNAMIC" = 1; then sd=d; else sd=s; fi
      export OCPI_TARGET_MODE=$sd$do
      export OCPI_TARGET_DIR=${OCPI_TARGET_HOST}/${OCPI_TARGET_MODE}
    fi
  else
    export OCPI_TARGET_DIR=$OCPI_TARGET_HOST
  fi
fi

#default the target host to the tool host
export PATH=$OCPI_CDK_DIR/bin/$OCPI_TOOL_DIR:$OCPI_CDK_DIR/scripts:$PATH
export OCPI_LIBRARY_PATH=$OCPI_CDK_DIR/lib/components
echo OCPI_CDK_DIR is $OCPI_CDK_DIR and OCPI_TOOL_HOST is $OCPI_TOOL_HOST


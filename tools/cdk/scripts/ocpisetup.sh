# This script is for example/application development.
# It is ALSO called during environment setup when building the core tree.
# -- in this case OCPI_TOOL_HOST is already set.
# This script should be sourced to set the OpenCPI CDK environment variables
# Since the (at least) the iVeia bash does not have extdebug/BASH_ARGV support, there is no
# way for this script to know where it lives.  Hence it requires its first arg to be that.
_MYNAME=ocpisetup.sh
if test $# == 0; then
  if test `basename $0` == $_MYNAME; then
    echo Error: ocpisetup.sh can only be run using the \".\" or \"source\" command. 1>&2
    return 1     
  fi
  if test "$BASH_SOURCE" == ""; then
    echo Error: ocpisetup.sh cannot determine where it is.  Supply it\'s path as first arg. 1>&2
    return 1
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
  return 1
elif test -f $_MYNAME; then
  echo Error: ocpisetup.sh can only be run from a different directory. 1>&2
  return 1
fi
export OCPI_CDK_DIR=`cd $(dirname $_MYPATH); pwd`
if test "$OCPI_TOOL_HOST" = ""; then
  vars=($($OCPI_CDK_DIR/platforms/getPlatform.sh))
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
if test "${OCPI_TOOL_MODE+UNSET}" = ""; then
  # OCPI_TOOL_MODE not set at all, just look for one
  for i in sd so dd do; do
    if test -x "$OCPI_CDK_DIR/$OCPI_TOOL_HOST/$i/ocpirun"; then
      export OCPI_TOOL_MODE=$i
      echo "Choosing tool mode "$i" since there are tool executables for it."
      break
    fi
  done
  if test "$OCPI_TOOL_MODE" = ""; then
    if test ! -x "$OCPI_CDK_DIR/$OCPI_TOOL_HOST/ocpirun"; then
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
#default the target host to the tool host
export PATH=$OCPI_CDK_DIR/bin/$OCPI_TOOL_DIR:$OCPI_CDK_DIR/scripts:$PATH
export OCPI_LIBRARY_PATH=$OCPI_CDK_DIR/lib/components
echo OCPI_CDK_DIR is $OCPI_CDK_DIR and OCPI_TOOL_HOST is $OCPI_TOOL_HOST


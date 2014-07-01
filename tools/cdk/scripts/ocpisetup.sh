# This script is for example/application development.
# This script should be sourced to set the OpenCPI CDK environment variables
# Since the iVeia bash does not have extdebug/BASH_ARGV support, there is no
# way for this script to know where it lives.
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
export OCPI_CDK_DIR=`dirname $_MYPATH`
export OCPI_CDK_DIR=`cd $OCPI_CDK_DIR; pwd`
if test "$OCPI_TOOL_HOST" = ""; then
  vars=($(platforms/getPlatform.sh))
  if test $? != 0; then
    echo Failed to determine runtime platform.
    return 1
  fi
  export OCPI_TOOL_OS=${vars[0]}
  export OCPI_TOOL_OS_VERSION=${vars[1]}
  export OCPI_TOOL_ARCH=${vars[2]}
  export OCPI_TOOL_HOST=${vars[3]}
fi
if test "$OCPI_TARGET_HOST" = ""; then
  export OCPI_TARGET_OS=$OCPI_TOOL_OS
  export OCPI_TARGET_OS_VERSION=$OCPI_TOOL_OS_VERSION
  export OCPI_TARGET_ARCH=$OCPI_TOOL_ARCH
  export OCPI_TARGET_HOST=$OCPI_TOOL_HOST
fi
#default the target host to the tool host
export PATH=$OCPI_CDK_DIR/bin/$OCPI_TOOL_HOST:$OCPI_CDK_DIR/scripts:$PATH
export OCPI_LIBRARY_PATH=$OCPI_CDK_DIR/lib/components
echo OCPI_CDK_DIR is $OCPI_CDK_DIR and OCPI_TOOL_HOST is $OCPI_TOOL_HOST


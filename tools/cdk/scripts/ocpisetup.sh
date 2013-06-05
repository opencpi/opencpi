# This script should be sourced to set the OpenCPI CDK environment variables
# Since the iVeia bash does not have extdebug/BASH_ARGV support, there is no
# way for this script to know where it lives.
_MYNAME=ocpisetup.sh
if test $# == 0; then
  if test `basename $0` == $_MYNAME; then
    echo Error: ocpisetup.sh can only be run using the \".\" or \"source\" command.
    exit 1     
  fi
  if test "$BASH_SOURCE" == ""; then
    echo Error: ocpisetup.sh cannot determine where it is.  Supply it\'s path as first arg.
    return 1
  fi
  _MYPATH=$BASH_SOURCE
elif test $# == 1; then
  _MYPATH=$1
fi
if test `basename $_MYPATH` != "$_MYNAME"; then
  if test $# == 1; then
    echo Error: ocpisetup.sh must be given its path as its first argument.
  else
    echo Error: ocpisetup.sh can only be run using the \".\" or \"source\" command.
  fi
  exit 1
elif test -f $_MYNAME; then
  echo Error: ocpisetup.sh can only be run from a different directory.
  return 1
else
  export OCPI_CDK_DIR=`dirname $_MYPATH`
  export OCPI_CDK_DIR=`cd $OCPI_CDK_DIR; pwd`
#  export OCPI_RUNTIME_SYSTEM=`$OCPI_CDK_DIR/scripts/showRuntimeHost`
  export PATH=$OCPI_CDK_DIR/bin/$OCPI_TOOL_HOST:$OCPI_CDK_DIR/scripts:$PATH
#  if [[ $OCPI_RUNTIME_SYSTEM == darwin-* ]]; then
#    export DYLD_LIBRARY_PATH=$OCPI_CDK_DIR/lib/$OCPI_RUNTIME_SYSTEM:$DYLD_LIBRARY_PATH
#  else
#    export LD_LIBRARY_PATH=$OCPI_CDK_DIR/lib/$OCPI_RUNTIME_SYSTEM:$LD_LIBRARY_PATH
# fi
  export OCPI_LIBRARY_PATH=$OCPI_CDK_DIR/lib/components
  echo OCPI_CDK_DIR is $OCPI_CDK_DIR and OCPI_TOOL_HOST is $OCPI_TOOL_HOST
fi

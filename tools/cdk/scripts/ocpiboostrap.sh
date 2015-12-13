#!/bin/sh

# This script does a few things to help bootstrap the environment.
# It is run by other scripts based on where they are, i.e. it is assumed to be
# where they are...
# It does setup useful for runtime, but not development time.
# I.e. it does not set up or default any OCPI_TARGET variables.
_MYNAME=ocpibootstrap.sh
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

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
_MYDIR=$(dirname $_MYPATH)
case $_MYDIR in /*) ;; *) _MYDIR=`cd $_MYDIR; pwd`;; esac
OCPI_BOOTSTRAP=$_MYDIR/ocpibootstrap.sh; . $OCPI_BOOTSTRAP

if test "$OCPI_DYNAMIC" = ""; then
  if test "$OCPI_BUILD_SHARED_LIBRARIES" != ""; then
    export OCPI_DYNAMIC=$OCPI_BUILD_SHARED_LIBRARIES
  else
    export OCPI_DYNAMIC=0
  fi
fi
export OCPI_BUILD_SHARED_LIBRARIES=$OCPI_DYNAMIC

# The tool mode and dir are set now, and are needed to set PATH below.

if test "$OCPI_TARGET_PLATFORM" = ""; then
  if test "$OCPI_TARGET_HOST" != ""; then
    # For compatibility if  OCPI_TARGET_PLATFORM not set.
    for i in $OCPI_CDK_DIR/platforms/*; do
     if test -f $i/target -a "$(< $i/target)" = "$OCPI_TARGET_HOST"; then
       export OCPI_TARGET_PLATFORM=$(basename $i)
       break
     fi
    done
    if test "$OCPI_TARGET_PLATFORM" = ""; then
      echo The value of $OCPI_TARGET_HOST does not match any known platform.
      exit 1
    fi
  fi
fi
if test "$OCPI_TARGET_PLATFORM" != ""; then
  if test "$OCPI_TARGET_HOST" = ""; then
    f=$OCPI_CDK_DIR/platforms/$OCPI_TARGET_PLATFORM/target
    if test ! -f $f; then
      echo OCPI_TARGET_PLATFORM is $OCPI_TARGET_PLATFORM.  File $f is missing.
      exit 1
    fi
    t=$(< $f)
    export OCPI_TARGET_HOST=$t
    vars=(${OCPI_TARGET_HOST//-/ })
    export OCPI_TARGET_OS=${vars[0]}
    export OCPI_TARGET_OS_VERSION=${vars[1]}
    export OCPI_TARGET_ARCH=${vars[2]}
  fi
else
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

f=$OCPI_CDK_DIR/platforms/$OCPI_TARGET_PLATFORM/$OCPI_TARGET_PLATFORM-target.sh
if test ! -f $f; then
  echo Error: there is no file: \"$f\" to setup the build environment for the \"$OCPI_TARGET_PLATFORM\" platform.
  exit 1
fi
source $f
#default the target host to the tool host
export PATH=$OCPI_CDK_DIR/bin/$OCPI_TOOL_DIR:$PATH
if test "$OCPI_LIBRARY_PATH" = ""; then
  # Default library path for RCC workers only
  export OCPI_LIBRARY_PATH=$OCPI_CDK_DIR/lib/components:$OCPI_CDK_DIR/lib/platforms/$OCPI_TARGET_PLATFORM
fi
echo OCPI_CDK_DIR is $OCPI_CDK_DIR and OCPI_TOOL_HOST is $OCPI_TOOL_HOST


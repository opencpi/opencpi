# This script is for example/application development.
# It is ALSO called during native environment setup when building the core tree.
# In this latter case the CDK is skeletal, but still usable
# This script should be sourced to set the OpenCPI CDK environment variables
# This script does not prepare for execution (except our own tools).
# If the bash does not have extdebug/BASH_ARGV support, there is no
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
OCPI_BOOTSTRAP=$_MYDIR/ocpibootstrap.sh; source $OCPI_BOOTSTRAP

if test "$OCPI_DEBUG" = ""; then
  export OCPI_DEBUG=1
fi

if test "$OCPI_ASSERT" = ""; then
  export OCPI_ASSERT=1
fi

if test "$OCPI_DYNAMIC" = ""; then
  # OCPI_DYNAMIC is the right variable. OCPI_BUILD_SHARED_LIBRARIES is legacy
  if test "$OCPI_BUILD_SHARED_LIBRARIES" != ""; then
    export OCPI_DYNAMIC=$OCPI_BUILD_SHARED_LIBRARIES
  else
    export OCPI_DYNAMIC=0
  fi
fi
export OCPI_BUILD_SHARED_LIBRARIES=$OCPI_DYNAMIC

export PATH=$OCPI_CDK_DIR/bin/$OCPI_TOOL_DIR:$PATH
if test "$OCPI_LIBRARY_PATH" = ""; then
  # Default library path for core RCC workers and HDL assembliesa
  export OCPI_LIBRARY_PATH=$OCPI_CDK_DIR/lib/components:$OCPI_CDK_DIR/lib/hdl/assemblies
fi

# Initialize target variables, using OCPI_TARGET_PLATFORM if it is set
source $OCPI_CDK_DIR/scripts/ocpitarget.sh ""

echo OCPI_CDK_DIR is $OCPI_CDK_DIR and OCPI_TOOL_PLATFORM is $OCPI_TOOL_PLATFORM

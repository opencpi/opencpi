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

# This script is for example/application development.
# It is ALSO called during native environment setup when building the core tree.
# In this latter case the CDK is skeletal, but still usable
# This script should be sourced to set the OpenCPI CDK environment variables
# This script does not prepare for execution (except our own tools).
# If the bash does not have extdebug/BASH_ARGV support, there is no
# way for this script to know where it lives.  Hence it requires its first arg to be that.
cat <<EOF >&2
This ocpisetup.sh script is obsolete and deprecated.
Use the opencpi-setup.sh script, which is sourced where it lives in the top level CDK directory.
E.g.:  source <where-the-cdk-is>/opencpi-setup.sh
If using a clean/virgin unbuilt source tree, and in that directory,
use "./scripts/init-opencpi.sh" first (not sourced) or "make exports"
then use: "source cdk/opencpi-setup.sh <options>"
EOF
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
    exit 1
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

export PATH=$OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin:$PATH

# Initialize target variables, using OCPI_TARGET_PLATFORM if it is set
source $OCPI_CDK_DIR/scripts/ocpitarget.sh ""

echo OCPI_CDK_DIR is $OCPI_CDK_DIR and OCPI_TOOL_PLATFORM is $OCPI_TOOL_PLATFORM

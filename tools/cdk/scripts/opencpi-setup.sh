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

###############################################################################################
# This file is meant to be *sourced* to set up the environment for OpenCPI on development hosts
# in the typical way that such scripts are called from the user's .profile.
# It is not intended to be used in RPM installations since that is done globally.
#
# It not meant to be *executed* as a shell script.
# It is sourced by its name in the top level of the desired CDK installation
# (possibly /opt/opencpi/cdk)
# The CDK is where this is sourced from (its dirname) and so OCPI_CDK_DIR is set accordingly.

# It modifies the environment in these ways:
# 1. setting OCPI_CDK_DIR environment variable to an absolute pathname - dirname of this script
# 2. setting the OCPI_TOOL_* environment variables as a cache of this platform determination
# 3. adding the binaries directory for the running platform to the PATH environment variable
# 4. setting the OCPI_PREREQUISITES_DIR environment variable
#    either $OCPI_CDK_DIR/../prerequisites if present or /opt/opencpi/prerequisites
# 5. enable bash command line completion for OpenCPI commands with completion
#
# It internally sets shell variables starting with "ocpi_"
# It insists on bash.  Someday someone can write it for csh.
# It will not clobber the environment if OCPI_CDK_DIR is already set.
# You can give an optional "-v" argument to get it to be verbose.
ocpi_name=opencpi-setup.sh
ocpi_me=$BASH_SOURCE
[ -z "$BASH_VERSION" -o -z "$ocpi_me" ] && {
  echo Error:  You can only use the $ocpi_name script with the bash shell. >&2
  return 1
}
[ "$ocpi_me" == $0 ] && {
    echo  Error: This OpenCPI $ocpi_name file must be read using the \"source\" command. >&2
    echo '       'It appears that you are executing this script rather than sourcing it. >&2
    exit 1 # note exit and not return since we are not being sourced
}
[ -n "$OCPI_CDK_DIR" ] && {
  cat<<-EOF >&2
	Warning:  The OpenCPI $ocpi_name file must be sourced when OCPI_CDK_DIR is not set.
	          OCPI_CDK_DIR was already set to: $OCPI_CDK_DIR, so nothing was done.
	          Use "unset OCPI_CDK_DIR" before sourcing this script if you want to override/reset it.
	EOF
  return 1
}
ocpi_verbose=
[ "$1" = -v ] && ocpi_verbose=1
# Make the file name of this script absolute if it isn't already
[[ "$ocpi_me" = /* ]] || ocpi_me=`pwd`/$ocpi_me
ocpi_dir=`dirname $ocpi_me`
[ -d $ocpi_dir -a -x $ocpi_dir ] || {
    echo $ocpi_name:' ' Unexpected error:' ' directory $ocpi_dir not a directory or inaccessible. >&2
    return 1
}
export OCPI_CDK_DIR=$(cd $ocpi_dir && pwd)
ocpi_gp=$OCPI_CDK_DIR/scripts/getPlatform.sh
if [ ! -f $ocpi_gp -o ! -x $ocpi_gp ]; then
    echo $ocpi_name: cannot run the getPlatforms.sh script at $ocpi_gp. >&2
    return 1
fi
[ "$ocpi_verbose" = 1 ] && cat <<-EOF >&2
	This $ocpi_name script is located at:  $ocpi_me.
	OCPI_CDK_DIR set to be $OCPI_CDK_DIR.
	Determining the OpenCPI platform we are running on...
	EOF
read v0 v1 v2 v3 v4 v5 <<< `$ocpi_gp`
if [ "$v4" == "" -o $? != 0 ]; then
    echo $_name: failed to determine runtime platform. >&2
    return 1
fi
export OCPI_TOOL_OS=$v0
export OCPI_TOOL_OS_VERSION=$v1
export OCPI_TOOL_ARCH=$v2
export OCPI_TOOL_HOST=$v3
export OCPI_TOOL_PLATFORM=$v4
# Clean out any previous instances in the path
ocpi_bin="$OCPI_CDK_DIR/bin/$OCPI_TOOL_HOST"
export PATH=$ocpi_bin:$(echo "$PATH" | sed "s=$ocpi_bin:==g")
ocpi_comp=$OCPI_CDK_DIR/scripts/ocpidev_bash_complete
[ -f $ocpi_comp ] && source $ocpi_comp
[ "$ocpi_verbose" = 1 ] && cat <<-EOF >&2
	The OpenCPI platform we are running on is "$v4" (placed in OCPI_TOOL_PLATFORM).
	The OpenCPI target for this platform is \"$OCPI_TOOL_HOST\".
	PATH now set to $PATH
	Now determining where prerequisite software is installed.
	EOF
[ -z "$OCPI_PREREQUISITES_DIR" ] && {
  if [ -n "$OCPI_CDK_DIR" -a -d "$OCPI_CDK_DIR/../prerequisites" ]; then
    export OCPI_PREREQUISITES_DIR=$(cd $OCPI_CDK_DIR/../prerequisites; pwd)
  else
    export OCPI_PREREQUISITES_DIR=/opt/opencpi/prerequisites
  fi
  if [ ! -d $OCPI_PREREQUISITES_DIR ]; then
    echo "$ocpi_name:  $OCPI_PREREQUISITES_DIR does not.  The installation/build of OpenCPI is incomplete."
    return 1
  fi
}
[ "$ocpi_verbose" = 1 ] && {
  echo "Software prerequisites are located at $OCPI_PREREQUISITES_DIR" >&2
  echo "Below are all OCPI_* environment variables now set:" >&2
  env | grep OCPI >&2
}

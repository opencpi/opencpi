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
    cat <<-EOF >&2
	Error: This OpenCPI $ocpi_name file must be read using the \"source\" command.
	       It appears that you are executing this script rather than sourcing it.
	EOF
    exit 1 # note exit and not return since we are not being sourced
}
[ "$1" = --help -o "$1" = -h -o -z "$1" ] && {
  cat <<-EOF >&2
	This script modifies the OpenCPI environment variable settings and the PATH variable.
	Options to this $ocpi_name file when *sourced* are:
	 --dynamic or -d:   enable the currently running tools platform to use dynamic linking
	 --optimized or -O: enable the currently running tools platform to use optimized code
	 --help or -h:      print this message
	 --reset or -r:     reset any previous OpenCPI environment before setting up a new one
	 --clean or -c:     unset all OpenCPI environment variables and nothing more.
	 --list or -l:      list current settings - will not setup
	 --verbose or -v:   be verbose about what is happening
	 -                  use this option when using no other options
	Note that neither --dynamic nor --optimized affect what is built.  Just what is used.
EOF
  return 1
}
ocpi_dynamic= ocpi_optimized= ocpi_reset= ocpi_verbose= ocpi_clean= ocpi_list=
ocpi_options=($*)
while [ -n "$ocpi_options" ] ; do
  case $ocpi_options in
    -d|--dynamic) ocpi_dynamic=1;;
    -O|--optimized) ocpi_optimized=1;;
    -r|--reset) ocpi_reset=1;;
    -v|--verbose) ocpi_verbose=1;;
    -c|--clean) ocpi_clean=1;;
    -l|--list) ocpi_list=1;;
    -);; # perhaps the single required variable
    *)
      echo Unknown option \"$options\" when sourcing the $ocpi_name file. >&2
      return 1;;
  esac
  unset ocpi_options[0]
  ocpi_options=(${ocpi_options[*]})
done
[ -n "$ocpi_clean" ] && {
  [ -n "$OCPI_CDK_DIR" ] && {
    ocpi_cleaned=$(echo "$PATH" | sed "s=$OCPI_CDK_DIR/[^:/]*/bin[^:]*:==g")
    [ "$ocpi_cleaned" != "$PATH" ] && {
      [ -n "$ocpi_verbose" ] && echo Removing OpenCPI bin directory from PATH.
      PATH="$ocpi_cleaned"
    }
  }
  [ -n "$ocpi_verbose" ] && echo Unsetting all OpenCPI environment variables.
  for ocpi_v in $(env | egrep ^OCPI_ | sort | cut -f1 -d=); do
    unset $ocpi_v
  done
  return 0
}
[ -n "$ocpi_list" ] && {
  [ -n "$ocpi_verbose" ] && echo Listing OpenCPI environment and the PATH variable.
  env | grep OCPI >&2
  env | grep '^PATH='
  return 0
}

[ -n "$OCPI_CDK_DIR" ] && {
  [ -z "$ocpi_reset" -a -z "$ocpi_clean" ] && {
    cat<<-EOF >&2
	Warning:  The OpenCPI $ocpi_name file should be sourced when OCPI_CDK_DIR is not set.
	          OCPI_CDK_DIR was already set to: $OCPI_CDK_DIR, so nothing is changed.
	          Use the --reset argument to reset OpenCPI environment variables before setup
	          Use the --clean argument to unset all OpenCPI environment variables and return
	EOF
    return 1
  }
  [ -n "$ocpi_verbose" ] && echo Clearing all OpenCPI environment variables before setting anything >&2
  for ocpi_v in $(env | egrep ^OCPI | sort | cut -f1 -d=); do
    # echo Clearing $v
    unset $ocpi_v
  done
  

}
# Make the file name of this script absolute if it isn't already
[[ "$ocpi_me" = /* ]] || ocpi_me=`pwd`/$ocpi_me
ocpi_dir=`dirname $ocpi_me`
[ -d $ocpi_dir -a -x $ocpi_dir ] || {
    echo $ocpi_name:' ' Unexpected error:' ' directory $ocpi_dir not a directory or inaccessible. >&2
    return 1
}
ocpi_cdk_dir=$(cd $ocpi_dir && pwd)
ocpi_gp=$ocpi_cdk_dir/scripts/getPlatform.sh
if [ ! -f $ocpi_gp -o ! -x $ocpi_gp ]; then
    echo $ocpi_name: cannot run the internal getPlatforms.sh script at $ocpi_gp. >&2
    return 1
fi
[ "$ocpi_verbose" = 1 ] && cat <<-EOF >&2
	This $ocpi_name script is located at:
	  $ocpi_me
	OCPI_CDK_DIR is now set to be $OCPI_CDK_DIR.
	Determining the OpenCPI platform we are running on...
	EOF
export OCPI_CDK_DIR=$ocpi_cdk_dir
read v0 v1 v2 v3 v4 v5 <<< `$ocpi_gp`
if [ "$v4" == "" -o $? != 0 ]; then
    echo $ocpi_name: failed to determine runtime platform. >&2
    unset OCPI_CDK_DIR
    return 1
fi
export OCPI_TOOL_OS=$v0
export OCPI_TOOL_OS_VERSION=$v1
export OCPI_TOOL_ARCH=$v2
export OCPI_TOOL_PLATFORM=$v4
export OCPI_TOOL_PLATFORM_DIR=$v5
export OCPI_TOOL_DIR=$OCPI_TOOL_PLATFORM
[ -n "$ocpi_dynamic" -o -n "$ocpi_optimized" ] && OCPI_TOOL_DIR+=-
[ -n "$ocpi_dynamic" ] && OCPI_TOOL_DIR+=d
[ -n "$ocpi_optimized" ] && OCPI_TOOL_DIR+=o

# Clean out any previous instances in the path
ocpi_cleaned=$(echo "$PATH" | sed "s=$OCPI_CDK_DIR/[^:/]*/bin[^:]*:==g")
[ -n "$ocpi_verbose" -a "$PATH" != "$ocpi_cleaned" ] && echo Removing OpenCPI bin directory from PATH >&2
export PATH="$OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin:$ocpi_cleaned"
ocpi_comp=$OCPI_CDK_DIR/scripts/ocpidev_bash_complete
[ -f $ocpi_comp ] && source $ocpi_comp
[ "$ocpi_verbose" = 1 ] && cat <<-EOF >&2
	The OpenCPI platform we are running on is "$v4" (placed in OCPI_TOOL_PLATFORM).
	The OpenCPI target directory set for this environment is \"$OCPI_TOOL_DIR\".
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
return 0

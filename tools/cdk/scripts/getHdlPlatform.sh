#!/bin/bash --noprofile
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

##########################################################################################
# This script determines all hdl platforms and target variables with no parameters
# If a single parameter is passed in only its variables will be returned
# The three variables are: <Hdl-Platform> <Hdl-Platform-Directory> <Hdl-Rcc-Platform>
# If the hdl platform does not have a corresponding rcc platform - will be returned
# If it returns nothing (""), that is an error

# Given the directory of the platform we want to return
# If any second parameter is given it is assumed that we do not want to exit at the end
returnPlatform() {
  local d=$1 p=$(basename $1)
  local vars=($(egrep "^ *Hdl(RccPlatform|Part)_$p *:*= *" $d/$p.mk |
              sed 's/Hdl\([^ :=]*\) *:*= *\([^a-zA-Z0-9_]*\)/\1 \2/'|sort))
  [ ${#vars[@]} = 2 -o ${#vars[@]} = 4 ] || {
    echo "Error:  Platform file $d/$p.mk is invalid and cannot be used.${vars[*]}" >&2
    echo "Error:  HdlRccPlatform_<platform> variable is not valid." >&2
    exit 1
  }
  # If there is no corresponding rcc platform, return the platform name, directory, and a - (to signify no rcc platform)
  # If there is a corresponding rcc platform, return platform name, directory, and the rcc platform name
  [ ${#vars[@]} = 2 ] && echo $p $d - || echo $p $d ${vars[3]}

  # If the second parameter is not given exit the entire script
  # If the second parameter is given give a good return value and continue running the script
  [ -z $2 ] && exit 0 || :
}

if [ -n "$OCPI_CDK_DIR" -a -e "$OCPI_CDK_DIR/scripts/util.sh" ]; then
  source $OCPI_CDK_DIR/scripts/util.sh
  projects="`getProjectPathAndRegistered`"
elif [ -n "$OCPI_PROJECT_PATH" ]; then
  # If the CDK is not set or util.sh does not exist, fall back on OCPI_PROJECT_PATH
  echo "Unexpected internal error: OCPI_CDK_DIR IS NOT SET1" >&2 && exit 1
  projects="${OCPI_PROJECT_PATH//:/ }"
elif [ -d projects ]; then
  echo "Unexpected internal error: OCPI_CDK_DIR IS NOT SET2" >&2 && exit 1
  # Probably running in a clean source tree.  Find projects and absolutize pathnames
  projects="$(for p in projects/*; do echo `pwd`/$p; done)"
fi
if [ -z "$projects" ]; then
  echo "Unexpected error:  Cannot find any projects for HDL platforms." >&2
  exit 1
fi

# loop through all projects to find the platform
shopt -s nullglob
[ -z $1 ] && platforms=false
for j in $projects; do
  # Assume this is an exported project and check hdl/platforms...
  platforms_dir=$j/hdl/platforms
  if [ -n "$1" ]; then # looking for a specific platform (not the current one)
    d=$platforms_dir/$1
    [ -d $d -a -f $d/$1.mk ] && returnPlatform $d
  else # looking for all platforms
    if [ -d $platforms_dir ]; then
      possible_platforms=()
      for dir in $platforms_dir/*; do [ -d $dir ] && possible_platforms+=($dir); done
      for platform in ${possible_platforms[@]}; do
        tmp_platform=$(basename $platform)
        [ -e $platforms_dir/$tmp_platform/$tmp_platform.mk ] && returnPlatform $platform - && platforms="true"
      done
    fi
  fi
done # done with the project

if [ -n "$1" ]; then
  # If we did not find the hdl platform given see if a software platform was given instead.
  # The following is used to get all possible rcc platforms for a specific hdl platform
  setVarsFromMake $OCPI_CDK_DIR/include/hdl/hdl-targets.mk ShellHdlTargetsVars=1 $verbose
  hw_platforms=()
  for j in $projects; do
    platforms_dir=$j/hdl/platforms
    if [ -d $platforms_dir ]; then
      possible_platforms=()
      for dir in $platforms_dir/*; do [ -d $dir ] && possible_platforms+=($dir); done
      for platform in ${possible_platforms[@]}; do
        tmp_platform=$(basename $platform)
        [ -e $platforms_dir/$tmp_platform/$tmp_platform.mk ] && ret_platforms=($(returnPlatform $platform -))
        [ "${ret_platforms[2]}" = $1 ] && hw_platforms+=("${ret_platforms[1]}")
        # Below we are getting rcc platforms that correspond with the hdl platform which is set in <hdl_platform>.mk file
        var_name=HdlAllRccPlatforms_${tmp_platform}
        if [ -n "${!var_name}" ]; then
          if [[ "${!var_name}" =~ $1 ]]; then
            [ -e $platforms_dir/$tmp_platform/$tmp_platform.mk ] && ret_platforms=($(returnPlatform $platform -))
            hw_platforms+=("${ret_platforms[1]}")
          fi
        fi
      done
    fi
  done
  # Make array unique
  hw_platforms=($(echo "${hw_platforms[@]}" | tr ' ' '\n' | sort -u | tr '\n' ' '))
  for curr_platform in ${hw_platforms[@]}; do
    echo "$(basename $curr_platform) $curr_platform $1"
  done
  [ ${#hw_platforms[@]} -gt 0 ] && exit 0
  echo "Cannot find a platform named $1." >&2
else
  [ "$platforms" = "true" ] && exit 0 # We were able to find platforms exit with a 0 status
  echo "Cannot find any hdl platforms." >&2 # We were not able to find any platforms exit with a status of 1
fi
exit 1

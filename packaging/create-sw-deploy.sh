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

if [ -z "$1" -o -z "$2" ]; then
  echo "This script creates a directory of files needed for a software platform"
  echo "This script takes two arguments with an optional third argument: output path, platform, and cross."
  echo "The output path argument is the directory where the files are placed"
  echo "The platform argument is a string for the rcc platform that deployment package should be created for"
  echo "The cross argument is either empty or not empty. If it is not empty"
  echo "it signifies a platform is cross compiled. i.e. not centos 6 or 7"
  echo "Usage is: $0 <output_path> <platform> \"<cross>\""
  exit 1
fi
output_path="$1"
platform="$2"
cross="$3"
[ -z $OCPI_CDK_DIR ] && echo "Need to set \$OCPI_CDK_DIR before running this script" && exit 1

# Make output directory
mkdir -p $output_path

# Make sure the platform exists
[ -e $OCPI_CDK_DIR/runtime/$platform ] \
  || echo "WARNING: there is no $OCPI_CDK_DIR/runtime/$platform Have you exported $platform yet?"
platform_dir=$OCPI_CDK_DIR/runtime/$platform
opencpi_output_path=$output_path/$platform/opencpi
if [ -n "$cross" ]; then
  echo Adding files for sw platform: $platform
  mkdir -p $opencpi_output_path
  file_list=($($OCPI_CDK_DIR/../packaging/prepare-package-list.sh deploy $platform))
  set -e
  for file in ${file_list[@]}; do
    edited_file=${file#"cdk/"}
    edited_file=${edited_file#"runtime/"}
    if [[ $edited_file = */ ]]; then
      mkdir -p $opencpi_output_path/$edited_file
    else
      [ -d $file ] && mkdir -p $opencpi_output_path/$edited_file
      cp -LR $file $opencpi_output_path/$(dirname "$edited_file")
      # Normally we are simply doing a deep copy here, but there is an exception:
      # We need to preserve symlinks that are in the same directory (i.e. for alternative names)
      # So we do a post-pass to fix this.
      for l in $(find $file -type l); do
        if [[ $(readlink $l) != */* ]]; then
          subdir=
          [ -d $file ] && subdir=/$(echo $l | sed s=$(dirname $file)==)
          cp -PR $l $opencpi_output_path/$subdir
        fi
      done
    fi
  done
else
  echo "We do not create a deployment package for $platform"
fi

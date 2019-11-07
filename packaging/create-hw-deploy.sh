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

shopt -s extglob
if [ -z "$1" -o -z "$2" ]; then
  echo "This script adds hardware platform files to the software platform files outputted by create-sw-deploy.sh"
  echo "This script takes two arguments: the output path and the platform."
  echo "The output_path argument should be the same one provided to create-sw-deploy.sh"
  echo "The platform argument should be a string for the hdl platform you want build"
  echo "Usage is: $0 <output_path> \"<platform>\""
  exit 1
fi

output_path="$1"
hdl_platform="$2"
hdl_rcc_platform="$3"
if [ ! -d $output_path ]; then
  echo "Parameter given for output_path does not exist. Please make sure to run create-sw-deploy.sh before this script"
  exit 1
fi
if [ -z "$OCPI_CDK_DIR" ]; then
  echo "Need to set \$OCPI_CDK_DIR before running this script"
  exit 1
fi

# Go over hardware platforms and put exported files in correct spots
# Some exported files include:
#   udev rules
#   files needed for deployment
#   pdfs
#   system.xml
echo "Adding files for hw platform: $hdl_platform"
# Get the rcc platform that corresponds with the hdl_platform
# and put it in hdl_rcc_platform
[ "$hdl_rcc_platform" = - ] && hdl_rcc_platform=no_sw
opencpi_output_path=$output_path/$hdl_platform/sdcard-$hdl_rcc_platform/opencpi
mkdir -p $opencpi_output_path
# Bring in rcc platform
if [ "$hdl_rcc_platform" != "no_sw" ]; then
  for item in $output_path/$hdl_rcc_platform/opencpi/*; do
    if [[ $item = *deploy* ]]; then
      cp -R $item/$hdl_rcc_platform/* $opencpi_output_path/..
    else
      cp -R $item $opencpi_output_path
      if [[ $item = **opencpi/$hdl_rcc_platform** ]]; then
        mv $opencpi_output_path/$hdl_rcc_platform/system.xml $opencpi_output_path
        mv $opencpi_output_path/$hdl_rcc_platform/*_*setup.sh $opencpi_output_path
	( shopt -s nullglob &&
	  for f in $opencpi_output_path/$hdl_rcc_platform/*.conf; do
            mv $f $opencpi_output_path
	  done)
      fi
    fi
  done
fi
file_list=($($OCPI_CDK_DIR/../packaging/prepare-package-list.sh deploy $hdl_platform))
for file in ${file_list[@]}; do
  edited_file=${file#"cdk/"}
  edited_file=${edited_file#"runtime/"}
  if [[ $edited_file = */ ]]; then
    [[ $file != *deploy* ]] && mkdir -p $opencpi_output_path/$edited_file
  else
    [[ $file = *deploy* ]] \
      && cp -LR $file/$hdl_rcc_platform/!($hdl_platform-deploy) $opencpi_output_path/.. \
      || (cp -LR $file $opencpi_output_path/$(dirname $edited_file) &&
           [ -d $file ] && mkdir -p $opencpi_output_path/$edited_file)
  fi
done
[ -e $opencpi_output_path/$hdl_platform/$hdl_rcc_platform/system.xml ] &&
mv $opencpi_output_path/$hdl_platform/$hdl_rcc_platform/system.xml $opencpi_output_path
# Combine hardware platform udev-rules and one obtained from prepare-package.sh
mkdir -p $output_path/$hdl_platform/host-udev-rules
[ -e $opencpi_output_path/udev-rules ] \
  && mv $opencpi_output_path/udev-rules/* $opencpi_output_path/../../host-udev-rules \
  && rmdir $opencpi_output_path/udev-rules
[ -d $opencpi_output_path/$hdl_platform ] \
    && ([ -d $opencpi_output_path/$hdl_platform/$hdl_rcc_platform/udev-rules ] &&
	    mv $opencpi_output_path/$hdl_platform/$hdl_rcc_platform/udev-rules/* \
	       $opencpi_output_path/../../host-udev-rules &&
	    rmdir $opencpi_output_path/$hdl_platform/$hdl_rcc_platform/udev-rules;
	rmdir $opencpi_output_path/$hdl_platform/$hdl_rcc_platform
	rm -rf $opencpi_output_path/$hdl_platform) \
# Exit with 0 status so the script that calls this script does not complain
exit 0
# TODO Build pdfs for current platform AV-4538 AV-4817
#[ -e $output_path/$hdl_platform/pdfs ] && rm -rf $output_path/$hdl_platform/pdfs
#$OCPI_CDK_DIR/../doc/generator/genDocumentation.sh --repopath $OCPI_CDK_DIR/../ --outputpath $output_path/$hdl_platform/pdfs --dirsearch "$hdl_platform_dir"
## Removing unnecessary files
#find $output_path/$hdl_platform/pdfs/ ! -name "*.pdf" -type f -delete
## Removing empty directories caused by above
#find $output_path/$hdl_platform/pdfs/ -type d -empty -delete

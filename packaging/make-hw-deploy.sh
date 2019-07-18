#!/bin/bash -e
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
[ -z "$1" ] && echo "Do not run this script by hand; it is a utility script for make rpm and make deploy" && exit 1
verbose=--quiet
[ "$1" = -v ] && verbose=-vv && shift
platform=$1 && shift
cross=0
[ -n "$1" ] && cross=1
shift
hdl_rcc_platform=$1 && shift
set -e

my_loc=$(readlink -e "$(dirname $0)")
tmpdir=$(mktemp -d)
OCPI_CDK_DIR=$my_loc/../cdk
# If there is no rcc platform set hdl_rcc_platform to no_sw
[ "$hdl_rcc_platform" = "-" ] && hdl_rcc_platform=no_sw
[ "$hdl_rcc_platform" != "no_sw" ] && OCPI_CDK_DIR=$OCPI_CDK_DIR $my_loc/create-sw-deploy.sh $tmpdir "$hdl_rcc_platform" "$cross"
OCPI_CDK_DIR=$OCPI_CDK_DIR $my_loc/create-hw-deploy.sh $tmpdir "$platform" "$hdl_rcc_platform"
mkdir -p cdk/$platform
cp -R $tmpdir/$platform/. ./cdk/$platform/$platform-deploy
rm -rf $tmpdir

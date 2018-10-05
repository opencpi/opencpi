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
package=$1 && shift
base=$1 && shift
name=$1 && shift
release=$1 && shift
version=$1 && shift
hash=$1 && shift
[ -n "$1" ] && specific_rcc=$1
shift
[ -n "$1" ] && rpm_build=0 && shift
set -e
command -v rpmbuild >/dev/null 2>&1 || {
  echo "Error: Cannot build an RPM: rpmbuild (rpm-build package) is not available."
  exit 1
}

my_loc=$(readlink -e "$(dirname $0)")
tmpdir=$(mktemp -d)
OCPI_CDK_DIR=$my_loc/../cdk
[ -z "$specific_rcc" ] && read -r hdl_platform hdl_platform_dir hdl_rcc_platform <<<$($OCPI_CDK_DIR/../tools/cdk/scripts/getHdlPlatform.sh $platform) || hdl_rcc_platform=$specific_rcc
# If there is no rcc platform set hdl_rcc_platform to no_sw
[ "$hdl_rcc_platform" = "-" ] && hdl_rcc_platform=no_sw
[ "$hdl_rcc_platform" != "no_sw" ] && OCPI_CDK_DIR=$OCPI_CDK_DIR $my_loc/create-sw-deploy.sh $tmpdir "$hdl_rcc_platform" "$cross"
OCPI_CDK_DIR=$OCPI_CDK_DIR $my_loc/create-hw-deploy.sh $tmpdir "$platform" "$specific_rcc"
# If rpm_build is set we only want to produce the deployment package for
# source and do not want to build rpms
[ -n "$rpm_build" ] && cp -R $tmpdir/$platform/. ./cdk/$platform/$platform-deploy && rm -rf $tmpdir && exit 0
tmp_rpm_build_dir=$my_loc
cd $my_loc/..
platform_dir=$tmpdir/$platform
rpm_build_dir="$tmp_rpm_build_dir/target-$platform"
mkdir -p $rpm_build_dir
find $rpm_build_dir -maxdepth 1 -mindepth 1 ! -name "*.rpm" -exec rm -rf {} + || :
mkdir -p $rpm_build_dir/SOURCES || :
cp -r $platform_dir/ $rpm_build_dir/SOURCES/
rm -rf $rpm_build_dir/opencpi-hw-platform-${platform}*rpm
set -o pipefail
eval ${SPOOF_HOSTNAME}
# If rcc platform is not empty append it to the name
[ -n $hdl_rcc_platform ] && name=$name-$hdl_rcc_platform
rpmbuild -bb packaging/hw.spec \
  --define="RPM_BASENAME       $base"\
  --define="RPM_NAME           $name"\
  --define="RPM_RELEASE        $release"\
  --define="RPM_VERSION        $version" \
  --define="RPM_HASH           $hash" \
  --define="RPM_PLATFORM       $platform" \
  --define="RPM_CROSS          $cross" \
  --define="_topdir            $rpm_build_dir"\
  --define="_build_name_fmt    %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm"\
  2>&1 | tee build-rpm.log

cp -v $rpm_build_dir/RPMS/$name* $rpm_build_dir
rm -rf $tmpdir
rm $my_loc/../build-rpm.log

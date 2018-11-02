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
[ -z "$1" ] && echo "Do not run this script by hand; it is a utility script for make rpm and make deploy" && exit 1
[ "$1" = -v ] && verbose=$1 && shift
platform=$1 && shift
[ -n "$1" ] && cross=1
shift
package=$1 && shift
base=$1 && shift
name=$1 && shift
release=$1 && shift
version=$1 && shift
hash=$1 && shift
# rpm_build is given when this script is called from deploy in top level make
[ -n "$1" ] && rpm_build=0
shift
set -e
# The correct value for cross and name are not coming out of make
# set correctly here instead
[ "$OCPI_TOOL_PLATFORM" = $platform ] && unset cross && name=$base
# If a specific rcc platform is given specific_sw, platform, and name must be set
if [[ $platform = *":"* ]]; then
    specific_sw_platform=$(echo $platform | cut -d: -f2)
    platform=$(echo $platform | cut -d: -f1)
    # Name will also have : that needs to be removed
    name=$(echo $name | cut -d: -f1)
fi
set +e
rcc_plat="$(./tools/cdk/scripts/getPlatform.sh $platform 2>/dev/null)"
hdl_plat="$(./tools/cdk/scripts/getHdlPlatform.sh $platform 2>/dev/null)"
set -e
# If rcc_plat is not null then platform is a rcc platform
if [ -n "$rcc_plat" ]; then
  if [ -n "$rpm_build" ]; then
    echo "You tried to deploy software platform: $platform but you can only deploy hardware platforms."
  else
  ./packaging/make-sw-rpms.sh $verbose $platform "$cross" $package \
    $base $name $release $version $hash
  fi
# If hdl_plat is not null then platform is a hdl platform
elif [ -n "$hdl_plat" ]; then
  # Since the platform is a hdl platform we need to change sw to hw in name
  name=$(echo $name | sed -e 's/opencpi-sw-platform/opencpi-hw-platform/g')
  # Build rpms for platform.  If rpm_build is set then we will not build rpms
  # instead contents will be deployed to $OCPI_CDK_DIR/deploy/<hw-platform>
  ./packaging/make-hw-rpms.sh $verbose $platform "$cross" $package \
    $base $name $release $version $hash "$specific_sw_platform" $rpm_build;
else
  echo "Platform: $platform was found"
fi

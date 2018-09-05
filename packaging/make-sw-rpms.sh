#!/bin/bash --norc
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
# This script creates software RPMs for the specified platform ($1) and package ($2)
# Since the runtime and devel packages are in one spec file and created together
# this really means if the package ($2) is "driver" it creates the driver RPM, otherwise
# it creates both the runtime and devel RPMs

# It is called by the top level Makefile with arguments for naming package files.

# It first determines the "Requires" aspects for the RPM, and then calls rpmbuild
# The "Requires" for the driver RPM are in that spec file (for now).

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
set -e
command -v rpmbuild >/dev/null 2>&1 || {
  echo "Error: Cannot build an RPM: rpmbuild (rpm-build package) is not available."
  exit 1
}
# Set target-specific environment variables, for this script and for the spec files
source $OCPI_CDK_DIR/scripts/ocpitarget.sh $platform
target=packaging/target-$platform
mkdir -p $target
if [ "$package" = driver ]; then
  msg="driver package (opencpi-driver)"
  spec=driver.spec
else
  msg="runtime (opencpi) and development (opencpi-devel) packages"
  spec=cdk.spec
  # derive RPM Requires: from package script for the platform, for runtime and devel
  p=$OCPI_TARGET_PLATFORM_DIR/${platform}-packages.sh
  touch $target/runtime-requires $target/devel-requires
  [ -f $p ] && {
    # first line is runtime
    $p list | head -1 | xargs -n 1 | sed 's/^/Requires:/' > $target/runtime-requires
    # second line is devel
    $p list | head -2 | tail -1 | xargs -n 1 | sed 's/^/Requires:/' > $target/devel-requires
    # last line is devel too (after epel)
    $p list | tail -1 | xargs -n 1 | sed 's/^[a-zA-Z/]/Requires:&/' >> $target/devel-requires
  }
fi
echo "Creating RPM file(s) in $target for $msg for the $platform platform."
mkdir -p $target
source $OCPI_CDK_DIR/../build/prerequisites/myhostname/myhostname.sh
# Run rpmbuild, passing in all the generic package naming information and running it
# with the host name spoofing to avoid embedding internal server names in the rpm file.
# Note that later versions of rpmbuild actually have a specific feature to do this easily
# using --define _buildhost, but this happened after centos7.
# We redirect the output directories to avoid anything going into ~/rpmbuild, and
# to put the rpm files directory into the packaging/target-<platform> directory
env $MYHOSTNAME_SPOOF rpmbuild $verbose -bb\
  --define="RPM_BASENAME       $base"\
  --define="RPM_NAME           $name"\
  --define="RPM_RELEASE        $release"\
  --define="RPM_VERSION        $version" \
  --define="RPM_HASH           $hash" \
  --define="RPM_PLATFORM       $platform" \
  --define="RPM_OPENCPI        $PWD" \
  --define="RPM_CROSS          $cross" \
  --define="RPM_CROSS_COMPILE  ${OCPI_TARGET_CROSS_COMPILE:--}" \
  --define="_topdir            $PWD/$target"\
  --define="_rpmdir            $PWD/$target"\
  --define="_build_name_fmt    %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm"\
  packaging/$spec
echo "Created RPM file(s) in $target:" && ls -ltr $target/*.rpm

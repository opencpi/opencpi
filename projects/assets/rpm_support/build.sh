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


# This file is named "build.sh" to match all the prerequisite repositories.

if [ "$1" = "rpm" ]; then
  if [ -z "${SPOOF_HOSTNAME}" ]; then
    echo "You cannot call this script directly. It MUST be called by the build process from the core."
    exit 99
  fi
  mkdir -p ~/rpmbuild/SOURCES || :
  rm -rf opencpi-project-assets*rpm || :
  rm -rf opencpi-assets-${RPM_VERSION}.tar.xz || :
  export XZ_OPT="-e9 -v -T0"
#   export XZ_OPT="-1 -v -T0"
  cd ..
  echo Compressing assets repo for distribution:
  time tar -Jc --exclude-vcs --exclude="rpm_support" -f ../opencpi-assets-${RPM_VERSION}.tar.xz .
  mv ../opencpi-assets-${RPM_VERSION}.tar.xz ~/rpmbuild/SOURCES
  set -o pipefail
  eval ${SPOOF_HOSTNAME}
  tar -cf ~/rpmbuild/SOURCES/assets_source.tar rpm_support/*
  rpmbuild -bb rpm_support/ocpiassets.spec \
    --define="COMMIT_HASH ${COMMIT_HASH}" \
    --define="COMMIT_TAG ${COMMIT_TAG_FINAL}" \
    --define="RELEASE_TAG ${RELEASE_TAG}" \
    --define="RPM_RELEASE ${RPM_RELEASE}" \
    --define="RPM_VERSION ${RPM_VERSION}" \
    2>&1 | tee build-rpm.log
  cp -v ~/rpmbuild/RPMS/*/opencpi-project-assets* .
else
    echo "This is for RPM packaging and NOT building the assets! For that, use the standard Makefile."
    exit 99
fi

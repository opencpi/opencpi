#!/bin/sh
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

set -e
OCPI_PATCHELF_VERSION=0.9
OCPI_PATCHELF_COMMIT=44b7f9583ffe0ee09c4da8bd996ef9a6a0017e1a

. ../prereq_utils.sh

# NOTE: Currently, 0.9 is sufficient (AV-1040), so we don't need
# a direct commit. If we need to move back to that, then we must
# update OCPI_PATCHELF_COMMIT_SHORT below as well!

OCPI_PATCHELF_COMMIT=${OCPI_PATCHELF_VERSION}
TARBALL=ocpi-prereq-patchelf-${OCPI_PATCHELF_VERSION}.tar

prereq_init_git patchelf-${OCPI_PATCHELF_COMMIT} https://github.com/NixOS/patchelf.git ${OCPI_PATCHELF_COMMIT}

OCPI_PATCHELF_COMMIT_SHORT=.$(cd patchelf-${OCPI_PATCHELF_COMMIT} && git rev-parse --short HEAD)
OCPI_PATCHELF_COMMIT_SHORT='%{nil}'

if [ "$1" = "rpm" ]; then
    if [ -e ocpi_patchelf.spec ]; then
        mkdir -p ~/rpmbuild/SOURCES || :
        rm -rf ${TARBALL} || :
        rm -rf ~/rpmbuild/RPMS || :
        tar cf ${TARBALL} patchelf-${OCPI_PATCHELF_COMMIT}
        cp *.patch ${TARBALL} ~/rpmbuild/SOURCES
        skip_host || rpmbuild -ba \
                 --define="OCPI_TARGET_HOST ${OCPI_TARGET_HOST}" \
                 --define="OCPI_PATCHELF_VERSION ${OCPI_PATCHELF_VERSION}" \
                 --define="OCPI_PATCHELF_COMMIT ${OCPI_PATCHELF_COMMIT}" \
                 --define="OCPI_PATCHELF_COMMIT_SHORT ${OCPI_PATCHELF_COMMIT_SHORT}" \
                 ocpi_patchelf.spec
        cp -v ~/rpmbuild/RPMS/*/ocpi-prereq-patchelf* . || :
    else
        echo "Missing RPM spec file in" `pwd`
        exit 1
    fi
else
    echo This script only builds RPMs. Try \"$0 rpm\".
    exit 99
fi

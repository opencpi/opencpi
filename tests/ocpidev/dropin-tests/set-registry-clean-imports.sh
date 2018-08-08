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

# AV-4141
# This dropin test catches the bug found in AV-4141
# This confirms that a manually set set registry will only
# only be removed by 'make clean' if it matches the current
# global default. Otherwise it should remain untouched.
set -ex

proj="set-reg-clean"
reg="$proj-project-registry"
rm -r -f $proj $reg

ocpidev create registry $reg
ocpidev create project $proj

pushd $proj

ocpidev set registry ../$reg
make cleanimports
test "$(readlink imports)" == "../$reg"

ocpidev set registry
source $OCPI_CDK_DIR/scripts/util.sh # needed for getProjectRegistryDir
test "$(ocpiReadLinkE imports)" == "$(getProjectRegistryDir)"
make cleanimports
test -z "$(ls imports 2>/dev/null)"

ocpidev set registry ../$reg
OCPI_PROJECT_REGISTRY_DIR=../$reg make cleanimports
test -z "$(ls imports 2>/dev/null)"

popd
ocpidev delete -f project $proj
test -z "$(ls $proj 2>/dev/null)"

ocpidev delete -f registry $reg
test -z "$(ls $reg 2>/dev/null)"

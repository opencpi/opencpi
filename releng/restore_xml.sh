#!/bin/bash -e
# AV-960: Restore XML artifacts removed by save_xml.sh

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

if [ -z "${RPM_BUILD_ROOT}" ]; then
  echo Do not call this script by hand. It is used by the RPM build process.
  false
fi

for fname in $(find "${RPM_BUILD_ROOT}" -name "*.saved_xml" -print); do
  echo Restoring XML info from ${fname/${RPM_BUILD_ROOT}/}...
  ${RPM_BUILD_ROOT}/opt/opencpi/cdk/bin/${OCPI_TOOL_DIR}/ocpixml add ${fname/.saved_xml/} ${fname}
  ${RPM_BUILD_ROOT}/opt/opencpi/cdk/bin/${OCPI_TOOL_DIR}/ocpixml get ${fname/.saved_xml/} | grep uuid
  rm ${fname}
done

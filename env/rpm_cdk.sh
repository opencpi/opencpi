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

export OCPI_CDK_DIR=/opt/opencpi/cdk
export OCPI_TOOL_HOST=@OCPI_TOOL_HOST@
if [ -z "$OCPI_PROJECT_REGISTRY_DIR" ]; then
  OCPI_PROJECT_REGISTRY_DIR=$OCPI_CDK_DIR/../project-registry
fi
export OCPI_LIBRARY_PATH=$OCPI_PROJECT_REGISTRY_DIR/ocpi.core/exports/lib/components/rcc
# Import any user configuration files
for i in /opt/opencpi/cdk/env.d/*.sh ; do
  if [ -r "$i" ]; then
    . "$i"
  fi
done

# Sourced script to start the process of a customized environment.

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

if test "$OCPI_CDK_DIR" != ""; then
  echo "Warning!!!!!!: "you are setting up the OpenCPI build environment when it is already set.
  echo "Warning!!!!!!: "this is not guaranteed to work.  You should probably use a new shell.
  echo You can also \"source scripts/clean-env.sh\" to start over.
fi
# Initialize access to the core tree's export directory
source ./scripts/core-init.sh
# Run the bootstrap to establish the CDK and the OCPI_TOOL_* variables
# This usually generates any errors before the custom script gets control
export OCPI_BOOTSTRAP=$(pwd)/exports/scripts/ocpibootstrap.sh; source $OCPI_BOOTSTRAP

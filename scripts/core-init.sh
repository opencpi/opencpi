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

# This sourced script is for clean environments, only for use in the core source tree,
# although if CDK is available we let it go with a warning
if test "$OCPI_CDK_DIR" == "" -a ! -d exports; then
  # We're being run in an uninitialized environment.
  if test ! -x ./scripts/makeExportLinks.sh; then
    echo Error: it appears that this script is not being run at the top level of OpenCPI.
    exit 1
  fi
  # Ensure a skeletal CDK
  ./scripts/makeExportLinks.sh - x x
fi

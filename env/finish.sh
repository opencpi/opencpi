# For now this script needs to know where it is, and on some circa 2002 bash versions,
# it can't, hence the extra argument.  This sets up the CDK VARS

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

cat <<EOF >&2
This finish.sh script is obsolete and deprecated.
Use the opencpi-setup.sh script, which is sourced in the top level CDK directory.
If using a clean/virgin unbuilt source tree, use "make exports" first, then use
"source cdk/opencpi-setup.sh <options>"
EOF
source exports/scripts/ocpisetup.sh exports/scripts/ocpisetup.sh
echo ""; echo " *** OpenCPI Environment settings"; echo ""
env | grep OCPI_ | sort

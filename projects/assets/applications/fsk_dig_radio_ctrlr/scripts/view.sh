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

# usage: view.sh <app_output_filename>

if [ -z "$1" ]; then
  echo "ERROR: must specify output file, usage: view.sh <app_output_filename>"
  exit 1
fi

CMD=eog
command -v $CMD > /dev/null
if [ "$?" != "0" ]; then
  echo "ERROR: this script requires the $CMD command, which was not found, exiting now"
  exit 1
fi

$CMD $1

exit 0

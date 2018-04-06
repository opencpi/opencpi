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

MEASUREMENT_FILE=measured.out
OCPI_LIBRARY_PATH=../:$OCPI_LIBRARY_PATH ocpirun -t 1 app.xml > $MEASUREMENT_FILE 2>&1
OCPIRUN_EXIT=$?
if [ "$OCPIRUN_EXIT" != "0" ]; then
cat $MEASUREMENT_FILE
exit 1
fi
./scripts/test_measured_mem_offsets.sh $MEASUREMENT_FILE

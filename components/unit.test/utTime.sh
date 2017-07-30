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


$OCPI_BASE_DIR/runtime/local/util/$OCPI_OUT_DIR/timeCvt --format=CSV --in=timeData.raw --out=timeData.csv
cat timeData.csv | grep ":unit_test:" | grep ",1,0" | grep "Worker Run" > runtime.csv
if [ ! -f "./expectedTime.csv" ] 
 then
    cp ./runtime.csv ./expectedTime.csv;
fi
$OCPI_BASE_DIR/runtime/local/util/$OCPI_OUT_DIR/timeCompare --deviation=$1 --tf=runtime.csv --ef=expectedTime.csv


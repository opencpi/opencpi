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

if test ! -d ../../artifacts; then
  echo "ERROR: OpenCPI assets project has not been built and exported"
  exit 1
fi

if test ! -d ../../../core/artifacts; then
  printf "ERROR: OpenCPI core project has either not been built and exported, "
  echo "or is in a locate other than the expected ../../../core/"
  exit 1
fi

if test ! -f ./target-centos7/fsk_dig_radio_ctrlr; then
  echo "ERROR: application has not been built"
  exit 1
fi

OCPI_LIBRARY_PATH=../../artifacts/:../../../core/artifacts/ \
./target-centos7/fsk_dig_radio_ctrlr fsk_dig_radio_ctrlr_filerw.xml
if [ "$?" != 0 ]; then
  exit 1
fi

OUT_FILE=odata/fsk_dig_radio_ctrlr_filerw.bin
if test ! -f $OUT_FILE; then
  echo "ERROR: expected output file ($OUT_FILE) does not exist"
    exit 1
fi

./scripts/view.sh $OUT_FILE
if [ "$?" != 0 ]; then
  exit 1
fi

exit 0

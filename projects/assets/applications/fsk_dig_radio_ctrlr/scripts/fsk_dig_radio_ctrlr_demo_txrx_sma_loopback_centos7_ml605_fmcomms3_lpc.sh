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

if [ "$(basename $PWD)" != "fsk_dig_radio_ctrlr" ]; then
  echo "ERROR: this script must be run from the fsk_dig_radio_ctrlr directory"
  exit 1
fi

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

#OCPI_LOG_LEVEL=10 \
OCPI_HDL_SIMULATORS= \
OCPI_LIBRARY_PATH=../../artifacts/:../../../core/artifacts/ \
  ./target-centos7/fsk_dig_radio_ctrlr \
  --rx-data-stream-ID SMA_RX1A \
  --rx-baud-rate 38400 \
  --rx-tuning-freq-MHz 2400 \
  --rx-gain-mode auto \
  --tolerance-rx-tuning-freq-MHz 0.1 \
  --tx-data-stream-ID SMA_TX1A \
  --tx-baud-rate 38400 \
  --tx-tuning-freq-MHz 2400 \
  --tx-gain-mode manual \
  --tx-gain-dB -30 \
  --tolerance-tx-tuning-freq-MHz 0.1 \
  --tolerance-tx-gain-dB 1 \
  fsk_dig_radio_ctrlr_fmcomms3_txrx.xml

if [ "$?" != "0" ]; then
  exit 1
fi

OUT_FILE=odata/fsk_dig_radio_ctrlr_fmcomms3_txrx.bin
if test ! -f $OUT_FILE; then
  echo "ERROR: expected output file ($OUT_FILE) does not exist"
    exit 1
fi

./scripts/view.sh $OUT_FILE
if [ "$?" != 0 ]; then
  exit 1
fi

exit 0

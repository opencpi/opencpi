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

TEST_COULD_NOT_COMPLETE=100

do_test_plot() {

  OUT_FILE=odata/fsk_dig_radio_ctrlr_fmcomms3_txrx.bin
  if test ! -f $OUT_FILE; then
    printf "TEST COULD NOT BE COMPLETED: "
    echo "ERROR: expected output file ($OUT_FILE) does not exist"
      exit $TEST_COULD_NOT_COMPLETE
  fi

  ./scripts/view.sh $OUT_FILE
  if [ "$?" != 0 ]; then
    printf "TEST COULD NOT BE COMPLETED: "
    echo "ERROR: could not view output file"
    exit $TEST_COULD_NOT_COMPLETE
  fi
}

echo_func_name() {
  echo
  echo "------------------------------------------------------"
  echo $1
  echo "------------------------------------------------------"
  echo
}

# this test suffers from the problem:
# Calibration TIMEOUT (0x16, 0x10)
# ERROR: exception caught: Worker dig_radio_ctrlr produced error during execution: config lock request was unsuccessful, set OCPI_LOG_LEVEL to 8 (or higher) for more info
# this only tests that app completes and exits w/ 0 exit status (no
# verification of Os.jpeg)
# 2.083334e6 / 16 / 39 ~= 3339
#test_min_rx_tx_baud_rate() {
#  echo_func_name ${FUNCNAME[0]}
#
#  #OCPI_LOG_LEVEL=10 \
#  OCPI_HDL_SIMULATORS= \
#  OCPI_LIBRARY_PATH=../../artifacts/:../../../core/artifacts/ \
#    ./target-centos7/fsk_dig_radio_ctrlr \
#    --rx-data-stream-ID SMA_RX1A \
#    --rx-baud-rate 3339 \
#    --rx-tuning-freq-MHz 2450 \
#    --rx-gain-mode manual \
#    --rx-gain-dB 24 \
#    --tolerance-rx-tuning-freq-MHz 0.1 \
#    --tolerance-rx-gain-dB 1 \
#    --tx-data-stream-ID SMA_TX1A \
#    --tx-baud-rate 3339 \
#    --tx-tuning-freq-MHz 2450 \
#    --tx-gain-mode manual \
#    --tx-gain-dB -34 \
#    --tolerance-tx-tuning-freq-MHz 0.1 \
#    --tolerance-tx-gain-dB 1 \
#    fsk_dig_radio_ctrlr_fmcomms2_txrx.xml
#
#  if [ "$?" != "0" ]; then
#    echo FAILED
#    exit 1
#  fi
#}

# this only tests that app completes and exits w/ 0 exit status (no
# verification of Os.jpeg)
# 8e6 is known reliable No-OS AD9361 samp rate
# 8e6 / 16 / 39 ~= 12820.5128205129
test_min_reliable_rx_tx_baud_rate() {
  echo_func_name ${FUNCNAME[0]}

  #OCPI_LOG_LEVEL=10 \
  OCPI_HDL_SIMULATORS= \
  OCPI_LIBRARY_PATH=../../artifacts/:../../../core/artifacts/ \
    ./target-centos7/fsk_dig_radio_ctrlr \
    --rx-data-stream-ID SMA_RX1A \
    --rx-baud-rate 12820.5128205129 \
    --rx-tuning-freq-MHz 2450 \
    --rx-gain-mode manual \
    --rx-gain-dB 24 \
    --tolerance-rx-tuning-freq-MHz 0.1 \
    --tolerance-rx-gain-dB 1 \
    --tx-data-stream-ID SMA_TX1A \
    --tx-baud-rate 12820.5128205129 \
    --tx-tuning-freq-MHz 2450 \
    --tx-gain-mode manual \
    --tx-gain-dB -34 \
    --tolerance-tx-tuning-freq-MHz 0.1 \
    --tolerance-tx-gain-dB 1 \
    fsk_dig_radio_ctrlr_fmcomms2_txrx.xml

  if [ "$?" != "0" ]; then
    echo FAILED
    exit 1
  fi
}

# this test suffers from the problem:
# Calibration TIMEOUT (0x16, 0x10)
# ERROR: exception caught: Worker dig_radio_ctrlr produced error during execution: config lock request was unsuccessful, set OCPI_LOG_LEVEL to 8 (or higher) for more info
# this only tests that app completes and exits w/ 0 exit status (no
# verification of Os.jpeg)
# 2.083334e6 / 16 / 39 ~= 3339
# 2400 + (2.083334e6 / 16)/2 ~= 2400.0651041876
#test_min_rx_tx_tuning_freq_MHz() {
#  echo_func_name ${FUNCNAME[0]}
#
#  #OCPI_LOG_LEVEL=10 \
#  OCPI_HDL_SIMULATORS= \
#  OCPI_LIBRARY_PATH=../../artifacts/:../../../core/artifacts/ \
#    ./target-centos7/fsk_dig_radio_ctrlr \
#    --rx-data-stream-ID SMA_RX1A \
#    --rx-baud-rate 3339 \
#    --rx-tuning-freq-MHz 2400.0651041876 \
#    --rx-gain-mode manual \
#    --rx-gain-dB 24 \
#    --tolerance-rx-tuning-freq-MHz 0.1 \
#    --tolerance-rx-gain-dB 1 \
#    --tx-data-stream-ID SMA_TX1A \
#    --tx-baud-rate 3339 \
#    --tx-tuning-freq-MHz 2400.0651041876 \
#    --tx-gain-mode manual \
#    --tx-gain-dB -34 \
#    --tolerance-tx-tuning-freq-MHz 0.1 \
#    --tolerance-tx-gain-dB 1 \
#    fsk_dig_radio_ctrlr_fmcomms2_txrx.xml
#
#  if [ "$?" != "0" ]; then
#    echo FAILED
#    exit 1
#  fi
#}

# this only tests that app completes and exits w/ 0 exit status (no
# verification of Os.jpeg)
# 2400 + (8e6 / 16)/2 ~= 2400.25
test_min_reliable_rx_tx_tuning_freq_MHz() {
  echo_func_name ${FUNCNAME[0]}

  #OCPI_LOG_LEVEL=10 \
  OCPI_HDL_SIMULATORS= \
  OCPI_LIBRARY_PATH=../../artifacts/:../../../core/artifacts/ \
    ./target-centos7/fsk_dig_radio_ctrlr \
    --rx-data-stream-ID SMA_RX1A \
    --rx-baud-rate 12820.5128205129 \
    --rx-tuning-freq-MHz 2400.25 \
    --rx-gain-mode manual \
    --rx-gain-dB 24 \
    --tolerance-rx-tuning-freq-MHz 0.1 \
    --tolerance-rx-gain-dB 1 \
    --tx-data-stream-ID SMA_TX1A \
    --tx-baud-rate 12820.5128205129 \
    --tx-tuning-freq-MHz 2400.25 \
    --tx-gain-mode manual \
    --tx-gain-dB -34 \
    --tolerance-tx-tuning-freq-MHz 0.1 \
    --tolerance-tx-gain-dB 1 \
    fsk_dig_radio_ctrlr_fmcomms2_txrx.xml

  if [ "$?" != "0" ]; then
    echo FAILED
    exit 1
  fi
}

# this only tests that app completes and exits w/ 0 exit status (no
# verification of Os.jpeg)
test_min_rx_gain_dB() {
  echo_func_name ${FUNCNAME[0]}

  #OCPI_LOG_LEVEL=10 \
  OCPI_HDL_SIMULATORS= \
  OCPI_LIBRARY_PATH=../../artifacts/:../../../core/artifacts/ \
    ./target-centos7/fsk_dig_radio_ctrlr \
    --rx-data-stream-ID SMA_RX1A \
    --rx-baud-rate 57600 \
    --rx-tuning-freq-MHz 2450 \
    --rx-gain-mode manual \
    --rx-gain-dB -3 \
    --tolerance-rx-tuning-freq-MHz 0.1 \
    --tolerance-rx-gain-dB 1 \
    --tx-data-stream-ID SMA_TX1A \
    --tx-baud-rate 57600 \
    --tx-tuning-freq-MHz 2450 \
    --tx-gain-mode manual \
    --tx-gain-dB 0 \
    --tolerance-tx-tuning-freq-MHz 0.1 \
    --tolerance-tx-gain-dB 1 \
    fsk_dig_radio_ctrlr_fmcomms2_txrx.xml

  if [ "$?" != "0" ]; then
    echo FAILED
    exit 1
  fi
}

# this only tests that app completes and exits w/ 0 exit status (no
# verification of Os.jpeg)
test_min_tx_gain_dB() {
  echo_func_name ${FUNCNAME[0]}

  #OCPI_LOG_LEVEL=10 \
  OCPI_HDL_SIMULATORS= \
  OCPI_LIBRARY_PATH=../../artifacts/:../../../core/artifacts/ \
    ./target-centos7/fsk_dig_radio_ctrlr \
    --rx-data-stream-ID SMA_RX1A \
    --rx-baud-rate 57600 \
    --rx-tuning-freq-MHz 2450 \
    --rx-gain-mode manual \
    --rx-gain-dB 54 \
    --tolerance-rx-tuning-freq-MHz 0.1 \
    --tolerance-rx-gain-dB 1 \
    --tx-data-stream-ID SMA_TX1A \
    --tx-baud-rate 57600 \
    --tx-tuning-freq-MHz 2450 \
    --tx-gain-mode manual \
    --tx-gain-dB -89.75 \
    --tolerance-tx-tuning-freq-MHz 0.1 \
    --tolerance-tx-gain-dB 1 \
    fsk_dig_radio_ctrlr_fmcomms2_txrx.xml

  if [ "$?" != "0" ]; then
    echo FAILED
    exit 1
  fi
}

# this only tests that app completes and exits w/ 0 exit status (no
# verification of Os.jpeg), note that due to AD9361 configuration constraints we
# 61.44e6 / 16/ 39 ~= 64102
test_max_rx_tx_baud_rate() {
  echo_func_name ${FUNCNAME[0]}

  #OCPI_LOG_LEVEL=10 \
  OCPI_HDL_SIMULATORS= \
  OCPI_LIBRARY_PATH=../../artifacts/:../../../core/artifacts/ \
    ./target-centos7/fsk_dig_radio_ctrlr \
    --rx-data-stream-ID SMA_RX1A \
    --rx-baud-rate 64102 \
    --rx-tuning-freq-MHz 2450 \
    --rx-gain-mode manual \
    --rx-gain-dB 24 \
    --tolerance-rx-tuning-freq-MHz 0.1 \
    --tolerance-rx-gain-dB 1 \
    --tx-data-stream-ID SMA_TX1A \
    --tx-baud-rate 64102 \
    --tx-tuning-freq-MHz 2450 \
    --tx-gain-mode manual \
    --tx-gain-dB -34 \
    --tolerance-tx-tuning-freq-MHz 0.1 \
    --tolerance-tx-gain-dB 1 \
    fsk_dig_radio_ctrlr_fmcomms2_txrx.xml

  if [ "$?" != "0" ]; then
    echo FAILED
    exit 1
  fi
}

# this test suffers from the problem:
# Calibration TIMEOUT (0x16, 0x10)
# ERROR: exception caught: Worker dig_radio_ctrlr produced error during execution: config lock request was unsuccessful, set OCPI_LOG_LEVEL to 8 (or higher) for more info
# this only tests that app completes and exits w/ 0 exit status (no
# verification of Os.jpeg)
# 2.083334e6 / 16 / 39 ~= 3339
# 2500 - (2.083334e6 / 16)/2 ~= 2499.99666132371
#test_max_rx_tx_tuning_freq_MHz() {
#  echo_func_name ${FUNCNAME[0]}
#
#  #OCPI_LOG_LEVEL=10 \
#  OCPI_HDL_SIMULATORS= \
#  OCPI_LIBRARY_PATH=../../artifacts/:../../../core/artifacts/ \
#    ./target-centos7/fsk_dig_radio_ctrlr \
#    --rx-data-stream-ID SMA_RX1A \
#    --rx-baud-rate 3339 \
#    --rx-tuning-freq-MHz 2499.99666132371 \
#    --rx-gain-mode manual \
#    --rx-gain-dB 24 \
#    --tolerance-rx-tuning-freq-MHz 0.1 \
#    --tolerance-rx-gain-dB 1 \
#    --tx-data-stream-ID SMA_TX1A \
#    --tx-baud-rate 3339 \
#    --tx-tuning-freq-MHz 2499.99666132371 \
#    --tx-gain-mode manual \
#    --tx-gain-dB -34 \
#    --tolerance-tx-tuning-freq-MHz 0.1 \
#    --tolerance-tx-gain-dB 1 \
#    fsk_dig_radio_ctrlr_fmcomms2_txrx.xml
#
#  if [ "$?" != "0" ]; then
#    echo FAILED
#    exit 1
#  fi
#}

# this only tests that app completes and exits w/ 0 exit status (no
# verification of Os.jpeg)
# 8e6 is known reliable No-OS AD9361 samp rate
# 2500 - (8 / 16)/2 ~= 2499.75
test_max_reliable_rx_tx_tuning_freq_MHz() {
  echo_func_name ${FUNCNAME[0]}

  #OCPI_LOG_LEVEL=10 \
  OCPI_HDL_SIMULATORS= \
  OCPI_LIBRARY_PATH=../../artifacts/:../../../core/artifacts/ \
    ./target-centos7/fsk_dig_radio_ctrlr \
    --rx-data-stream-ID SMA_RX1A \
    --rx-baud-rate 12820.5128205129 \
    --rx-tuning-freq-MHz 2499.75 \
    --rx-gain-mode manual \
    --rx-gain-dB 24 \
    --tolerance-rx-tuning-freq-MHz 0.001 \
    --tolerance-rx-gain-dB 1 \
    --tx-data-stream-ID SMA_TX1A \
    --tx-baud-rate 12820.5128205129 \
    --tx-tuning-freq-MHz 2499.75 \
    --tx-gain-mode manual \
    --tx-gain-dB -34 \
    --tolerance-tx-tuning-freq-MHz 0.001 \
    --tolerance-tx-gain-dB 1 \
    fsk_dig_radio_ctrlr_fmcomms2_txrx.xml

  if [ "$?" != "0" ]; then
    echo FAILED
    exit 1
  fi
}

# this only tests that app completes and exits w/ 0 exit status (no
# verification of Os.jpeg)
test_max_rx_gain_dB() {
  echo_func_name ${FUNCNAME[0]}

  #OCPI_LOG_LEVEL=10 \
  OCPI_HDL_SIMULATORS= \
  OCPI_LIBRARY_PATH=../../artifacts/:../../../core/artifacts/ \
    ./target-centos7/fsk_dig_radio_ctrlr \
    --rx-data-stream-ID SMA_RX1A \
    --rx-baud-rate 57600 \
    --rx-tuning-freq-MHz 2450 \
    --rx-gain-mode manual \
    --rx-gain-dB 71 \
    --tolerance-rx-tuning-freq-MHz 0.1 \
    --tolerance-rx-gain-dB 1 \
    --tx-data-stream-ID SMA_TX1A \
    --tx-baud-rate 57600 \
    --tx-tuning-freq-MHz 2450 \
    --tx-gain-mode manual \
    --tx-gain-dB -60 \
    --tolerance-tx-tuning-freq-MHz 0.1 \
    --tolerance-tx-gain-dB 1 \
    fsk_dig_radio_ctrlr_fmcomms2_txrx.xml

  if [ "$?" != "0" ]; then
    echo FAILED
    exit 1
  fi
}

# this only tests that app completes and exits w/ 0 exit status (no
# verification of Os.jpeg)
test_max_tx_gain_dB() {
  echo_func_name ${FUNCNAME[0]}

  #OCPI_LOG_LEVEL=10 \
  OCPI_HDL_SIMULATORS= \
  OCPI_LIBRARY_PATH=../../artifacts/:../../../core/artifacts/ \
    ./target-centos7/fsk_dig_radio_ctrlr \
    --rx-data-stream-ID SMA_RX1A \
    --rx-baud-rate 57600 \
    --rx-tuning-freq-MHz 2450 \
    --rx-gain-mode manual \
    --rx-gain-dB 12 \
    --tolerance-rx-tuning-freq-MHz 0.1 \
    --tolerance-rx-gain-dB 1 \
    --tx-data-stream-ID SMA_TX1A \
    --tx-baud-rate 57600 \
    --tx-tuning-freq-MHz 2450 \
    --tx-gain-mode manual \
    --tx-gain-dB 0 \
    --tolerance-tx-tuning-freq-MHz 0.1 \
    --tolerance-tx-gain-dB 1 \
    fsk_dig_radio_ctrlr_fmcomms2_txrx.xml

  if [ "$?" != "0" ]; then
    echo FAILED
    exit 1
  fi
}

# lock fails as expected, but was printing out wrong bandwidth limits, hard to
# automate this as of now...
display_bug_0() {
  echo_func_name ${FUNCNAME[0]}

  #OCPI_LOG_LEVEL=10 \
  OCPI_HDL_SIMULATORS= \
  OCPI_LIBRARY_PATH=../../artifacts/:../../../core/artifacts/ \
    ./target-centos7/fsk_dig_radio_ctrlr \
    --rx-data-stream-ID SMA_RX1A \
    --rx-baud-rate 12820.5128205129 \
    --rx-tuning-freq-MHz 2499.98717948719 \
    --rx-gain-mode manual \
    --rx-gain-dB 24 \
    --tolerance-rx-tuning-freq-MHz 0.1 \
    --tolerance-rx-gain-dB 1 \
    --tx-data-stream-ID SMA_TX1A \
    --tx-baud-rate 12820.5128205129 \
    --tx-tuning-freq-MHz 2499.98717948719 \
    --tx-gain-mode manual \
    --tx-gain-dB -34 \
    --tolerance-tx-tuning-freq-MHz 0.1 \
    --tolerance-tx-gain-dB 1 \
    fsk_dig_radio_ctrlr_fmcomms2_txrx.xml
}

exemplifies_need_for_soft_lock() {
  echo_func_name ${FUNCNAME[0]}

  #OCPI_LOG_LEVEL=10 \
  OCPI_HDL_SIMULATORS= \
  OCPI_LIBRARY_PATH=../../artifacts/:../../../core/artifacts/ \
    ./target-centos7/fsk_dig_radio_ctrlr \
    --rx-data-stream-ID SMA_RX1A \
    --rx-baud-rate 12820.5128205129 \
    --rx-tuning-freq-MHz 2499.98717948719 \
    --rx-gain-mode manual \
    --rx-gain-dB 24 \
    --tolerance-rx-tuning-freq-MHz 0.001 \
    --tolerance-rx-gain-dB 1 \
    --tx-data-stream-ID SMA_TX1A \
    --tx-baud-rate 12820.5128205129 \
    --tx-tuning-freq-MHz 2499.98717948719 \
    --tx-gain-mode manual \
    --tx-gain-dB -34 \
    --tolerance-tx-tuning-freq-MHz 0.001 \
    --tolerance-tx-gain-dB 1 \
    fsk_dig_radio_ctrlr_fmcomms2_txrx.xml
}

if test ! -d ../../artifacts; then
  printf "TEST COULD NOT BE COMPLETED: "
  echo "ERROR: OpenCPI assets project has not been built and exported"
  exit $TEST_COULD_NOT_COMPLETE
fi

if test ! -d ../../../core/artifacts; then
  printf "TEST COULD NOT BE COMPLETED: "
  printf "ERROR: OpenCPI core project has either not been built and exported, "
  echo "or is in a location other than the expected ../../../core/"
  exit $TEST_COULD_NOT_COMPLETE
fi

if test ! -f ./target-centos7/fsk_dig_radio_ctrlr; then
  printf "TEST COULD NOT BE COMPLETED: "
  echo "ERROR: application has not been built"
  exit $TEST_COULD_NOT_COMPLETE
fi

#test_min_rx_tx_baud_rate
#if [ "$?" != "0" ]; then
#  echo FAILED
#  exit 1
#fi

test_min_reliable_rx_tx_baud_rate
if [ "$?" != "0" ]; then
  echo FAILED
  exit 1
fi

#test_min_rx_tx_tuning_freq_MHz
#if [ "$?" != "0" ]; then
#  echo FAILED
#  exit 1
#fi

test_min_reliable_rx_tx_tuning_freq_MHz
if [ "$?" != "0" ]; then
  echo FAILED
  exit 1
fi

test_min_rx_gain_dB
if [ "$?" != "0" ]; then
  echo FAILED
  exit 1
fi

test_min_tx_gain_dB
if [ "$?" != "0" ]; then
  echo FAILED
  exit 1
fi

test_max_rx_tx_baud_rate
if [ "$?" != "0" ]; then
  echo FAILED
  exit 1
fi

#test_max_rx_tx_tuning_freq_MHz
#if [ "$?" != "0" ]; then
#  echo FAILED
#  exit 1
#fi

test_max_reliable_rx_tx_tuning_freq_MHz
if [ "$?" != "0" ]; then
  echo FAILED
  exit 1
fi

test_max_rx_gain_dB
if [ "$?" != "0" ]; then
  echo FAILED
  exit 1
fi

test_max_tx_gain_dB
if [ "$?" != "0" ]; then
  echo FAILED
  exit 1
fi

#display_bug_0

echo PASSED

exit 0

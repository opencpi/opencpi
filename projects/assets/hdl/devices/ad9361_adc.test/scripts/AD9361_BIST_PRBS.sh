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

if [ -z "$OCPI_TOOL_DIR" ]; then
  echo OCPI_TOOL_DIR env variable must be specified before running BIST_PRBS_rates.sh
  exit 1
fi
if [ ! -d target-$OCPI_TOOL_DIR ]; then
  echo "missing binary directory: (target-$OCPI_TOOL_DIR does not exist)"
  exit 1
fi

# sometimes app fail's because ad9361_config_proxy.rcc worker's no-os library
# (from ADI) times out when setting the tx frequency, causing the app to
# immediately exit
MAX_NUM_APP_RETRIES=3 # set to 0 to disable retries

. ./scripts/test_utils.sh

### ------------------------------------------------------------------------ ###

APP_XML=$1

dogrep() {
  # echo out import property readbacks so they get put into the log and can
  # be compared against the golden file
  grep "ad9361_config_proxy.bb_pll_is_locked " $LOGFILENAME | tail -n +2 | sed "s/[0-9]*://g"
  grep "ad9361_config_proxy.rx_pll_is_locked " $LOGFILENAME | tail -n +2 | sed "s/[0-9]*://g"
  grep "ad9361_config_proxy.tx_pll_is_locked " $LOGFILENAME | tail -n +2 | sed "s/[0-9]*://g"
  grep "ad9361_config_proxy.rx_fir_en_dis " $LOGFILENAME    | tail -n +2 | sed "s/[0-9]*://g"
  grep "ad9361_config_proxy.rx_sampling_freq " $LOGFILENAME | tail -n +2 | sed "s/[0-9]*://g"
  grep "ad9361_config_proxy.bist_prbs " $LOGFILENAME        | tail -n +2 | sed "s/[0-9]*://g"
  grep "ad9361_config_proxy.ad9361_init " $LOGFILENAME      |              sed "s/[0-9]*://g"
  grep "ad9361_config_proxy.DATA_CLK_Delay" $LOGFILENAME    | tail -n +2 | sed "s/[0-9]*://g"
  grep "ad9361_config_proxy.Rx_Data_Delay" $LOGFILENAME     | tail -n +2 | sed "s/[0-9]*://g"
  grep "ad9361_config_proxy.FB_CLK_Delay" $LOGFILENAME      | tail -n +2 | sed "s/[0-9]*://g"
  grep "ad9361_config_proxy.Tx_Data_Delay" $LOGFILENAME     | tail -n +2 | sed "s/[0-9]*://g"
  grep "ad9361_data_sub.DATA_CLK_Delay" $LOGFILENAME        |              sed "s/[0-9]*://g"
  grep "ad9361_data_sub.RX_Data_Delay" $LOGFILENAME         |              sed "s/[0-9]*://g"
  grep "ad9361_data_sub.FB_CLK_Delay" $LOGFILENAME          |              sed "s/[0-9]*://g"
  grep "ad9361_data_sub.TX_Data_Delay" $LOGFILENAME         |              sed "s/[0-9]*://g"
  grep "qadc.overrun " $LOGFILENAME                         | tail -n +2 | sed "s/[0-9]*://g"
  grep "ad9361_adc_sub.r1_samps_dropped" $LOGFILENAME       | tail -n +2 | sed "s/[0-9]*://g"
  grep "ad9361_adc_sub.r2_samps_dropped" $LOGFILENAME       | tail -n +2 | sed "s/[0-9]*://g"
  grep "file_write.bytesWritten" $LOGFILENAME               | tail -n +2 | sed "s/[0-9]*://g"
}

dober() {
  ./target-$OCPI_TOOL_DIR/calculate_AD9361_BIST_PRBS_RX_BER $FILENAME | grep BER
}

#firenables=( $DISABLE $ENABLE )
firenables=( $DISABLE )
twortwots=( 0 1 ) # force using 2R2T timing diagram regardless of number of enabled channels

FOUND_PLATFORMS=$(./target-$OCPI_TOOL_DIR/get_comma_separated_ocpi_platforms)
AT_LEAST_ONE_ML605_AVAILABLE=$(./target-$OCPI_TOOL_DIR/get_at_least_one_platform_is_available ml605)
if [ "$FOUND_PLATFORMS" == "" ]; then
  echo ERROR: no platforms found! check ocpirun -C
  echo "TEST FAILED"
  exit 1
elif [ "$FOUND_PLATFORMS" == "zed" ]; then
    # DATA_CLK_P rate rounded via floor = floor(1/5.712 ns) ~= 175.070028 MHz
    # for LVDS, max samp rate = DATA_CLK_P rate / 4         ~=  43.767507 Msps complex
  samprates=( $AD9361_MIN_ADC_RATE 25e6 30e6 35e6 40e6 43767507 )
elif [ "$FOUND_PLATFORMS" == "zed_ise" ]; then
  # DATA_CLK_P rate rounded via floor = floor(1/4.294 ns) ~= 232.883092 MHz
  # for LVDS, max samp rate = DATA_CLK_P rate / 4         ~=  58.220773 Msps complex
  samprates=( $AD9361_MIN_ADC_RATE 25e6 30e6 35e6 40e6 45e6 50e6 55e6 58220773 )
elif [ "$AT_LEAST_ONE_ML605_AVAILABLE" == "true" ]; then
  samprates=( $AD9361_MIN_ADC_RATE 25e6 30e6 35e6 40e6 45e6 50e6 55e6 $AD9361_MAX_ADC_RATE )
else
  printf "platform found which is not supported: "
  echo $FOUND_PLATFORMS
  echo "TEST FAILED"
  exit 1
fi

if [ ! -d odata ]; then
  mkdir odata
fi

touch odata/retries.log

for twortwot in "${twortwots[@]}"
do
  for firenable in "${firenables[@]}"
  do
    for samprate in "${samprates[@]}"
    do
      APP_RUNTIME_SEC=1
      echo "FIR enabled      : $firenable"
      if [ "$twortwot" == "0" ]; then
        echo "Data port config : 1R1T"
      else
        echo "Data port config : 2R2T"
      fi
      echo "sample rate      : $samprate sps"
      echo "runtime          : $APP_RUNTIME_SEC sec"

      PREFIX=/var/volatile/app_"$samprate"sps_fir"$firenable"_"$twortwot"_"$APP_RUNTIME_SEC"sec

      TEST_ID="PRBS"
      FILENAME="$PREFIX"_prbs.out
      LOGFILENAME="$PREFIX"_prbs.log
      P="-pad9361_config_proxy=rx_fir_en_dis=$firenable \
         -pad9361_config_proxy=rx_sampling_freq=$samprate \
         -pad9361_config_proxy=bist_prbs=$BIST_INJ_RX \
         -pfile_write=filename=$FILENAME" \
        runtest
      dogrep
      dober
      mvfiles
      echo --------------------------------------------------------------------------------

    done
  done

  # this test just confirms we can run for 10 sec (the majority of that 10 sec
  # with the fifo.hdl worker acting as a data sink) without overflowing the adc
  # worker('s clock domain-crossing fifo)

  APP_RUNTIME_SEC=10

  echo "FIR enabled      : 0"
  if [ "$twortwot" == "0" ]; then
    echo "Data port config : 1R1T"
  else
    echo "Data port config : 2R2T"
  fi

  if [ "$FOUND_PLATFORMS" == "" ]; then
    echo ERROR: no platforms found! check ocpirun -C
    echo "TEST FAILED"
    exit 1
  elif [ "$FOUND_PLATFORMS" == "zed" ]; then
    # DATA_CLK_P rate rounded via floor = floor(1/5.712 ns) ~= 175.070028 MHz
    # for LVDS, max samp rate = DATA_CLK_P rate / 4         ~=  43.767507 Msps complex
    echo "sample rate      : 43767507 sps"
  elif [ "$FOUND_PLATFORMS" == "zed_ise" ]; then
    # DATA_CLK_P rate rounded via floor = floor(1/4.294 ns) ~= 232.883092 MHz
    # for LVDS, max samp rate = DATA_CLK_P rate / 4         ~=  58.220773 Msps complex
    echo "sample rate      : 58220773 sps"
  elif [ "$AT_LEAST_ONE_ML605_AVAILABLE" == "true" ]; then
    echo "sample rate      : 61.44 sps"
  else
    printf "platform found which is not supported: "
    echo $FOUND_PLATFORMS
    echo "TEST FAILED"
    exit 1
  fi

  echo "runtime          : $APP_RUNTIME_SEC sec"

  PREFIX=/var/volatile/app_61.44e6sps_fir0_"$twortwot"_"$APP_RUNTIME_SEC"sec

  TEST_ID="additional test for overrun avoidance"
  FILENAME="$PREFIX"_prbs.out
  LOGFILENAME="$PREFIX"_prbs.log
  P="-pad9361_config_proxy=rx_fir_en_dis=$firenable \
     -pad9361_config_proxy=rx_sampling_freq=$samprate \
     -pad9361_config_proxy=bist_prbs=$BIST_INJ_RX \
     -pfile_write=filename=$FILENAME" \
    runtest
  dogrep
  mvfiles
  echo --------------------------------------------------------------------------------

done


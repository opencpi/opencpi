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
fi

. ./scripts/test_utils.sh

### ------------------------------------------------------------------------ ###

APP_XML=ad9361_test_1r1t_lvds_app.xml

APP_RUNTIME_SEC=1

TWO_R_TWO_T=1

DISABLE_LOG=0
MAX_NUM_SAMPLES=999999
REG_SYNC_FFFF=4294967295

run_delay_tests_1r1t_fmcomms3() {

  touch /var/volatile/toberemoved.out
  rm /var/volatile/*out

  for clkdelay in "${clkdelays[@]}"
  do
  for datadelay in "${datadelays[@]}"
  do

#    -pad9361_config_proxy=rx_fir_config_write="rx $FIR_RX1,rx_gain 0,rx_dec 2,rx_coef {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},rx_coef_size 16" \ # TODO / FIXME figure out what's wrong with this
  ocpirun -d -v -t $APP_RUNTIME_SEC \
    -pad9361_config_proxy=ad9361_init="reference_clk_rate $FMCOMMS3_REF_CLK_RATE,frequency_division_duplex_mode_enable 1,xo_disable_use_ext_refclk_enable 0,two_t_two_r_timing_enable $TWO_R_TWO_T,pp_tx_swap_enable 0,pp_rx_swap_enable 0,rx_data_clock_delay 0,rx_data_delay 5,tx_fb_clock_delay $clkdelay,tx_data_delay $datadelay" \
    -pad9361_config_proxy=en_state_machine_mode=$ENSM_MODE_FDD \
    -pad9361_config_proxy=rx_sampling_freq_multiplier=1 \
    $@ \
    $APP_XML > /dev/null 2>&1

  #./target-$OCPI_TOOL_DIR/calculate_AD9361_BIST_PRBS_RX_BER $FILENAME $DISABLE_LOG $MAX_NUM_SAMPLES $REG_SYNC_FFFF | grep BER | tr -d "estimated_BER : " | tr -d "%" | tr -d "\n"
  ./target-$OCPI_TOOL_DIR/calculate_AD9361_BIST_PRBS_RX_BER $FILENAME $DISABLE_LOG $MAX_NUM_SAMPLES $REG_SYNC_FFFF | grep -e BER -e sync #| tr -d "estimated_BER : " | tr -d "%" | tr -d "\n" #| tr -d "ERROR caused premature exit: reg sync " #| tr -d "(file read was /var/volatile/app*out)"
  printf "\t"

  done
  echo
  done
}

#firenables=( $DISABLE $ENABLE ) # TODO / FIXME figure out what's wrong with this
firenables=( $DISABLE)
samprates=( $AD9361_MIN_ADC_RATE 25e6 40e6 $AD9361_MAX_ADC_RATE )

clkdelays=( 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 )
datadelays=( 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 )

for firenable in "${firenables[@]}"
do
  for samprate in "${samprates[@]}"
  do
    echo "FIR enabled is $firenable : "
    echo "$samprate sps : "

    FILENAME=/var/volatile/app_"$samprate"sps_fir"$firenable"_prbs.out
    run_delay_tests_1r1t_fmcomms3 \
      -pad9361_config_proxy=rx_fir_en_dis=$firenable \
      -pad9361_config_proxy=tx_sampling_freq=$samprate \
      -pad9361_config_proxy=bist_loopback=1 \
      -pdata_src=mode=lfsr \
      -pfile_write=filename=$FILENAME

  done
done


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

FMCOMMS3_REF_CLK_RATE=40e6

# these are from ad9361_config_proxy-spec, which is derived from ADI's no-os headers
#ENSM_MODE_FDD=3
ENSM_MODE_FDD_INDEPENDENT_CTRL=7

if [ -z $1 ]; then
  echo "first argument must be app xml filename"
  exit 1
fi

APP_XML=$1

APP_RUNTIME_SEC=1
if [ ! -z $2 ]; then
  APP_RUNTIME_SEC=$2
fi

TWO_R_TWO_T=1
if [ ! -z $3 ]; then
  TWO_R_TWO_T=$3
fi

ENSM_MODE=$ENSM_MODE_FDD_INDEPENDENT_CTRL
if [ ! -z $4 ]; then
  ENSM_MODE=$4
fi

RUNFILE=.run_app_FMCOMMS3.sh.lastrun
if [ -f $RUNFILE ]; then
  rm -rf $RUNFILE
fi

RX_CHANNEL_SWAP_ENABLE=0
DELAY_RX_DATA=0
if [ "$RX_DATA_CLOCK_DELAY" = "" ]; then
  export RX_DATA_CLOCK_DELAY=2
fi
if [ "$RX_DATA_DELAY" = "" ]; then
  export RX_DATA_DELAY=0
fi

TX_CHANNEL_SWAP_ENABLE=0

echo "ocpirun -U -d -v -t $APP_RUNTIME_SEC \
  -pad9361_config_proxy=ad9361_init=\"reference_clk_rate $FMCOMMS3_REF_CLK_RATE,frequency_division_duplex_mode_enable 1,xo_disable_use_ext_refclk_enable 0,two_t_two_r_timing_enable $TWO_R_TWO_T,pp_tx_swap_enable 0,pp_rx_swap_enable 0,tx_channel_swap_enable $TX_CHANNEL_SWAP_ENABLE,rx_channel_swap_enable $RX_CHANNEL_SWAP_ENABLE,delay_rx_data $DELAY_RX_DATA,rx_data_clock_delay $RX_DATA_CLOCK_DELAY,rx_data_delay $RX_DATA_DELAY,tx_fb_clock_delay 7,tx_data_delay 0\" \
  -pad9361_config_proxy=en_state_machine_mode=$ENSM_MODE \
  $P $APP_XML" > $RUNFILE


chmod +x $RUNFILE
./$RUNFILE
XX=$?
if [ "$XX" != "0" ]; then
  exit $XX
fi


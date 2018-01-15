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

if [ -z "$OCPI_TOOL_HOST" ]; then
  echo OCPI_TOOL_HOST env variable must be specified before running BIST_PRBS_rates.sh
  exit 1
fi
if [ ! -d target-$OCPI_TOOL_HOST ]; then
  echo "OCPI_TOOL_HOST env variable set incorrectly (target-$OCPI_TOOL_HOST does not exist)"
  exit 1
fi

. ./scripts/test_utils.sh

### ------------------------------------------------------------------------ ###

#set -x # uncomment for debugging purposes

APP_RUNTIME_SEC=1

APP_XML=ad9361_adc_test_1r1t_lvds_app.xml
if [ ! -z "$1" ]; then
  APP_XML=$1
fi

if [ ! -d odata ]; then
  mkdir odata
fi
if [ -d odata/delays ]; then
  rm -rf odata/delays
fi
mkdir odata/delays

run_delay_tests_1r1t_fmcomms3() {

  loop2=( 0 1 )
  clkdelays=( 0 )
  datadelays=( 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 )

  echo " rx_data_clock_delay   rx_data_delay->"
  echo "  |"
  echo "  v"
  echo " 	0	1	2	3	4	5	6	7	8	9	10	11	12	13	14	15"
  for loop in "${loop2[@]}"
  do
  for clkdelay in "${clkdelays[@]}"
  do
  printf "$clkdelay	"
  for datadelay in "${datadelays[@]}"
  do

    LOGFILENAME=odata/delays/ocpirun_clkdel"$clkdelay"_datadel"$datadelay".log
    if [ -f $LOGFILENAME ]; then
      rm -rf $LOGFILENAME
    fi

    if [ -f $FILENAME ]; then
      rm -rf $FILENAME
    fi

    OCPI_LIBRARY_PATH=$OCPI_LIBRARY_PATH:./assemblies/:$OCPI_PROJECT_PATH P="-pfile_write=filename=$FILENAME" RX_DATA_CLOCK_DELAY=$clkdelay RX_DATA_DELAY=$datadelay ./scripts/run_app_FMCOMMS3.sh $APP_XML $APP_RUNTIME_SEC $twortwot > $LOGFILENAME 2>&1

    if [ ! -f $FILENAME ]; then
      printf "error"
    else
      if [ ! -d odata/delays ]; then
        mkdir odata/delays
      fi
      ./target-$OCPI_TOOL_HOST/calculate_AD9361_BIST_PRBS_RX_BER $FILENAME | grep BER | tr -d "estimated_BER : " | tr -d "%" | tr -d "\n" | tee odata/delays/ber_clkdel"$clkdelay"_datadel"$datadelay".log
    fi
    printf "\t"

  done
  echo
  done
  clkdelays=( 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 )
  datadelays=( 0 )
  done
}

#firenables=( $DISABLE $ENABLE ) # TODO / FIXME figure out what's wrong with this
firenables=( $DISABLE)
twortwots=( 0 1 ) # force using 2R2T timing diagram regardless of number of enabled channels
samprates=( $AD9361_MIN_ADC_RATE 25e6 40e6 $AD9361_MAX_ADC_RATE )

for twortwot in "${twortwots[@]}"
do
  for firenable in "${firenables[@]}"
  do
    for samprate in "${samprates[@]}"
    do
      echo "FIR enabled      : $firenable"
      if [ "$twortwot" == "0" ]; then
        echo "Data port config : 1R1T"
      else
        echo "Data port config : 2R2T"
      fi
      echo "sample rate      : $samprate sps"

      FILENAME=/var/volatile/app_"$samprate"sps_fir"$firenable"_prbs.out
      run_delay_tests_1r1t_fmcomms3 \
        -pad9361_config_proxy=rx_fir_en_dis=$firenable \
        -pad9361_config_proxy=tx_sampling_freq=$samprate \
        -pad9361_config_proxy=bist_prbs=$BIST_INJ_RX \
        -pfile_write=filename=$FILENAME

    done
  done
done


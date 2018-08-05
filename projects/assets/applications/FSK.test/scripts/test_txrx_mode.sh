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

#This script tests the FSK app for the following permutations of sample rate and RF frequency:
sample_rate_min=36
sample_rate_increment=1
frequency_min=233
frequency_max=2988
frequency_increment=1
#Analog parameters which remain fixed
rx_rf_gain_dB=6
rx_bb_cutoff_frequency_MHz=5
rx_bb_gain_dB=42
tx_rf_gain_dB=4
tx_bb_cutoff_frequency_MHz=5
tx_bb_gain_dB=-4
#Number of allowed failures per test
number_of_allowed_failures=5
#Ensure increment is not 0
sample_rate_increment_eq_zero=$(awk -vn1="$sample_rate_increment" -vn2="0" 'BEGIN{print (n1<=n2)?1:0 }')
frequency_increment_eq_zero=$(awk -vn1="$frequency_increment" -vn2="0" 'BEGIN{print (n1<=n2)?1:0 }')
if [ $sample_rate_increment_eq_zero -eq 1 ] || [ $frequency_increment_eq_zero -eq 1 ] ; then
    echo "Increments must be greater than 0"
    exit
fi
if [ $# -eq 0 ] ; then
    echo "No arguments supplied. Syntax is sh `basename $0` <platform>"
else
    echo "Testing $1 platform..."
    if [ $1 == "zed" ] ; then
	app_xml=app_fsk_txrx_zipper.xml
        sample_rate_max=32
	rf_cutoff_frequency_MHz=-1
    elif [ $1 == "matchstiq_z1" ] ; then
	app_xml=app_fsk_txrx_matchstiq_z1.xml
        sample_rate_max=36 # TODO / FIXME - change this to 40?
	rf_cutoff_frequency_MHz=400
    elif [ $1 == "ml605" ] ; then
	app_xml=app_fsk_txrx_zipper.xml
        sample_rate_max=30
	rf_cutoff_frequency_MHz=-1
    elif [ $1 == "alst4" ] ; then
	app_xml=app_fsk_txrx_zipper.xml
        sample_rate_max=19
	rf_cutoff_frequency_MHz=-1
    else
	echo "$1 is not a valid platform"
	exit -1
    fi
    sample_rate=$sample_rate_min
    #max_sample_rate_exceeded=`echo $sample_rate'>'$sample_rate_max | bc -l`
    max_sample_rate_exceeded=$(awk -vn1="$sample_rate" -vn2="$sample_rate_max" 'BEGIN{print (n1>n2)?1:0 }')
    frequency=$frequency_min
    #max_frequency_exceeded=`$frequency'>'$frequency_max | bc -l`
    max_frequency_exceeded=$(awk -vn1="$frequency" -vn2="$frequency_max" 'BEGIN{print (n1>n2)?1:0 }')
    failures=0
    total_tests=0
    total_failures=0
    #Remove header from input file
    `dd bs=1 skip=242 count=8912 if=idata/Os.jpeg of=odata/goldenOs.jpeg &> /dev/null`
    #for sample_rate in $(eval echo "{$sample_rate_min..$sample_rate_max..$sample_rate_increment}")
    #do
    while [ $max_sample_rate_exceeded -eq 0 ] && [ $failures -lt $number_of_allowed_failures ]; do
	echo "Testing $sample_rate MS/s"
	#1,000,000*2^16=65536000000
	#phs_inc=$((65536000000 / (sample_rate * 1000000)))
	phs_inc=`dc $sample_rate 1000000 mul p`
	phs_inc=`dc 65536000000 $phs_inc div p`
	#t=$((20 - (sample_rate / 2)))
	t=`dc $sample_rate 2 div p`
	t=`dc 20 $t sub p`
	#for frequency in $(eval echo "{$frequency_min..$frequency_max..$frequency_increment}")
	#do
	while [ $max_frequency_exceeded -eq 0 ] && [ $failures -lt $number_of_allowed_failures ]; do
	    failures=0
	    success=0
	    tx_frequency=`dc $frequency 1 add p`
	    echo "Testing frequency TX: $tx_frequency RX: $frequency"
	    while [ $failures -lt $number_of_allowed_failures ] && [ $success -eq 0 ]
	    do
		((total_tests++))
		#echo "Success = $success"
		ocpirun \
		    -t $t \
		    -pcomplex_mixer=phs_inc=$phs_inc \
		    -prx=sample_rate_MHz=$sample_rate \
		    -prx=frequency_MHz=$frequency \
		    -prx=rf_cutoff_frequency_MHz=$rf_cutoff_frequency_MHz \
		    -prx=rf_gain_dB=$rx_rf_gain_dB \
		    -prx=bb_cutoff_frequency_MHz=$rx_bb_cutoff_frequency_MHz \
		    -prx=bb_gain_dB=$rx_bb_gain_dB \
		    -ptx=sample_rate_MHz=$sample_rate \
		    -ptx=frequency_MHz=$tx_frequency \
		    -ptx=rf_gain_dB=$tx_rf_gain_dB \
		    -ptx=bb_cutoff_frequency_MHz=$tx_bb_cutoff_frequency_MHz \
		    -ptx=bb_gain_dB=$tx_bb_gain_dB \
		    -prf_tx=loopback=0x00 \
		    -prf_tx=tx_pkdbw=0x00 \
		    -prf_tx=tx_pa_en=0x0B \
		    $app_xml
		if [[ $? != 0 ]]; then
		    ((failures++))
		    ((total_failures++))
		    echo "Sample Rate: $sample_rate, Frequency: $frequency failed: ocpirun error. Trying again. Failure $failures"
		    continue
		fi
		`dd bs=1 if=odata/out_app_fsk_txrx.bin of=odata/truncated_out_app_fsk_txrx.bin count=8912 &> /dev/null`
		cmp odata/goldenOs.jpeg odata/truncated_out_app_fsk_txrx.bin
		if [[ $? != 0 ]]; then
		    ((failures++))
		    ((total_failures++))
		    echo "Sample Rate: $sample_rate, Frequency: $frequency failed: output file incorrect. Trying again. Failure $failures"
		    rm odata/truncated_out_app_fsk_txrx.bin
		    continue
		fi
		rm odata/truncated_out_app_fsk_txrx.bin
		#ls -l odata
		#echo "Success"
		success=1
		#frequency=`echo $frequency+$frequency_increment | bc -l`
		frequency=`dc $frequency $frequency_increment add p`
		#max_frequency_exceeded=`echo $frequency'>'$frequency_max | bc -l`
		max_frequency_exceeded=$(awk -vn1="$frequency" -vn2="$frequency_max" 'BEGIN{print (n1>n2)?1:0 }')
	    done
	    if [ $failures -eq $number_of_allowed_failures ]; then
		echo "Error: Test condition Sample Rate: $sample_rate, Frequency: $frequency failed $number_of_allowed_failures times. Aborting test"
		exit
	    fi
	done
	#sample_rate=`echo $sample_rate+$sample_rate_increment | bc -l`
	sample_rate=`dc $sample_rate $sample_rate_increment add p`
	#max_sample_rate_exceeded=`echo $sample_rate'>'$sample_rate_max | bc -l`
	max_sample_rate_exceeded=$(awk -vn1="$sample_rate" -vn2="$sample_rate_max" 'BEGIN{print (n1>n2)?1:0 }')
	frequency=$frequency_min
	max_frequency_exceeded=$(awk -vn1="$frequency" -vn2="$frequency_max" 'BEGIN{print (n1>n2)?1:0 }')
    done
    echo "Total Failures: $total_failures"
    echo "Total Tests: $total_tests"
fi

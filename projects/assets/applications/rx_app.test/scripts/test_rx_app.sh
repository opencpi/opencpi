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

#This script tests the rx app
#It requires network access to a signal generator
#RX app usage
# "    rf_tune_freq       # RF tuning frequency in MHz\n"
# "    data_bw            # Bandwidth of the data being written to file in MS/s\n"
# "    rf_bw              # RF bandwidth in MHz\n"
# "    rf_gain            # RF gain in dB\n"
# "    bb_bw              # Baseband bandwidth in MHz\n"
# "    bb_gain            # Baseband gain in dB (5 - 60)\n"
# "    if_tune_freq       # IF tuning frequency in MHz. 0 disables IF tuning)\n"
# "    runtime            # Runtime of app in seconds (1 - 5)\n"
# "    enable_timestamps  # Enable timestamps (1 or 0)\n"

# Environment variables can further configure how the script runs
# ALLOW_FAILURES allows failure to happen. Each RF frequency is tested once and recorded in freq.log. 0 for passed. 1 for failed.
# PLOT plots failures vs. RF frequency in freq.log

#The script tests for the following permutations of RF frequency:
rf_tune_freq_min=233
rf_tune_freq_max=2988
rf_tune_freq_incr=1

#Constant parameters
data_bw=2.5
rf_bw=400
rf_gain=10
bb_bw=1.25
bb_gain=51
if_tune_freq=1
runtime=1
enable_timestamps=0
number_of_allowed_failures=2

#Ensure increment is not 0
rf_tune_freq_incr_eq_zero=`echo $rf_tune_freq_incr==0 | bc -l`
if [ $rf_tune_freq_incr_eq_zero -eq 1 ] ; then
    echo "rf_tune_freq_inc cannot be 0"
    exit
fi
if [ $# -ne 2 ] ; then
   echo "Wrong number of arguments supplied. Syntax is sh `basename $0` <sdr ip address> <sig-gen ip address>"
else
    directory=$(cd `dirname $0` && pwd)
    sdr_ip=$1
    #Test sig gen by setting power
    echo "Setting signal generator power level"
    sig_gen_ip=$2
    sig_gen_power_level=-60
    perl  $directory/configure_sig_gen.pl $sig_gen_ip "pow $sig_gen_power_level dBm"
    if [ $? -ne 0 ] ; then
	exit
    fi
    sdr_user=root
    sdr_pass=root
    rf_tune_freq=$rf_tune_freq_min
    max_rf_tune_freq_exceeded=`echo $rf_tune_freq'>'$rf_tune_freq_max | bc -l`
    #echo $max_rf_tune_freq_exceeded
    log_file=$directory/../freq.`date +"%Y%m%d_%H%M%S"`.log
    echo "Frequency Test Log for test_rx_app.sh" > $log_file
    echo `date` >> $log_file
    if test -n "${ALLOW_FAILURES}"; then
	allow_failures=1
    else
	allow_failures=0
    fi
    total_tests=0
    total_failures=0
    failures=0
    while [ $max_rf_tune_freq_exceeded -eq 0 ] ; do #&& [ $failures -lt $number_of_allowed_failures ]; do
	echo "Testing rf_tune_frequency: " $rf_tune_freq
	if test -n "${USE_EPIQ_APP}"; then
	    output_dir="/tmp/"
	    output_file="test.out"
	    rf_tune_freq_hz=`echo $rf_tune_freq*1000000 | bc -l`
	    data_bw_hz=`echo $data_bw*1000000 | bc -l`
	    bb_bw_hz=`echo $bb_bw*1000000 | bc -l`
	    complete_run_command="/root/test_apps/rx_samples $output_dir$output_file $rf_tune_freq_hz 1048576 1 0 6 30 $bb_bw_hz $data_bw_hz 2048 0"
	    sig_gen_rf_tune_freq=`echo $rf_tune_freq-.25 | bc -l`
	else
	    library_path="OCPI_LIBRARY_PATH=$OCPI_CDK_DIR/$OCPI_TOOL_PLATFORM/artifacts:/mnt/ocpi_assets/artifacts:"
	    run_command="./target-xilinx13_3/rx_app $rf_tune_freq $data_bw $rf_bw $rf_gain $bb_bw $bb_gain $if_tune_freq $runtime $enable_timestamps matchstiq_z1"
	    complete_run_command="cd /mnt/ocpi_assets/applications/rx_app && $library_path $run_command"
	    sig_gen_rf_tune_freq=`echo $rf_tune_freq+1.25 | bc -l`
	fi
	perl $directory/configure_sig_gen.pl $sig_gen_ip "freq $sig_gen_rf_tune_freq MHz"
	perl $directory/configure_sig_gen.pl $sig_gen_ip "OUTP ON"
	((total_tests++))
	#echo $complete_run_command
	$OCPI_CDK_DIR/scripts/testrunremote.sh $sdr_ip $sdr_user $sdr_pass "$complete_run_command" > /dev/null
	perl $directory/configure_sig_gen.pl $sig_gen_ip "OUTP OFF"
	if test -n "${USE_EPIQ_APP}"; then
	    sshpass -p "$sdr_pass" scp $sdr_user@$sdr_ip:$output_dir$output_file $directory
	    python $directory/verifyRxAppFft.py $directory/$output_file complex 65536 2500000
	else
	    python $directory/verifyRxAppFft.py $directory/../../rx_app/odata/rx_app_shortened.out complex 65536 2500000
	fi
	result=$?
	if [ $result -ne 0 ] ; then
	    ((failures++))
	    ((total_failures++))
	    if [ $failures -eq $number_of_allowed_failures ]; then
		if [ $allow_failures -eq 1 ] ; then
		    echo $rf_tune_freq","$failures >> $log_file
		    failures=0
		else
		    echo "Error: Test condition rf_tune_freq: $rf_tune_freq MHz failed $number_of_allowed_failures times. Aborting test"
		    echo $rf_tune_freq","$failures >> $log_file
		    break
		fi
	    else
		continue
	    fi
	else
	    echo $rf_tune_freq","$failures >> $log_file
	    failures=0
	fi
	rf_tune_freq=`echo $rf_tune_freq+$rf_tune_freq_incr | bc -l`
	max_rf_tune_freq_exceeded=`echo $rf_tune_freq'>'$rf_tune_freq_max | bc -l`
    done
    echo "Total Failures: $total_failures"
    echo "Total Tests: $total_tests"
    if test -n "${PLOT}"; then
	python $directory/plotFailures.py $log_file &
    fi
fi

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

#This script tests the FSK app in bbloopback mode:
#Number of allowed failures per test
number_of_allowed_failures=3
if [ $# -lt 2 ] || [ $# -gt 2 ]; then
  echo "Incorrect number of arguments supplied. Syntax is sh `basename $0` <platform> <number of times to execute>"
else
  echo "Testing $1 platform..."
  if [ $1 == "zed" ] ; then
    app_xml=app_fsk_txrx_zed_no_rcc.xml
  elif [ $1 == "matchstiq_z1" ] ; then
    app_xml=app_fsk_txrx_matchstiq_z1.xml
  elif [ $1 == "alst4" ] ; then
    app_xml=app_fsk_txrx_zipper.xml
  else
    echo "$1 is not a valid platform"
    exit -1
  fi
  total_tests=0
  total_failures=0
  #Remove header from input file
  `dd bs=1 skip=242 count=8912 if=idata/Os.jpeg of=odata/goldenOs.jpeg &> /dev/null` # YES THIS IS 8912 not 8192
  failures=0
  while [ $total_tests -lt $2 ]
  do
    ((total_tests++))
    echo "Test $total_tests of $2"
    if [ -f odata/out_app_fsk_bbloopback.bin ]; then
      # this ensures that, later in this script when cmp is called, the cmp
      # compares the output file of the fsk_app execution that just occured, as
      # opposed to that of an execution which occured before this script was run
      rm odata/out_app_fsk_bbloopback.bin
    fi
    if [ $1 == "alst4" ] ; then
      echo "3" | `./target-$OCPI_TOOL_HOST/FSK bbloopback &> /dev/null`
    else
      echo "3" | `./target-linux-x13_3-arm/FSK bbloopback &> /dev/null`
    fi
    if [ -f odata/out_app_fsk_bbloopback.bin ]; then
      `dd bs=1 if=odata/out_app_fsk_bbloopback.bin of=odata/truncated_out_app_fsk_txrx.bin count=8912 &> /dev/null` # YES THIS IS 8912 not 8192
      cmp odata/goldenOs.jpeg odata/truncated_out_app_fsk_txrx.bin
      if [[ $? != 0 ]]; then
        ((failures++))
        ((total_failures++))
        echo "Failed: output file incorrect. Trying again. Failure $failures"
        if [ -f odata/truncated_out_app_fsk_txrx.bin ]; then
          rm odata/truncated_out_app_fsk_txrx.bin
        fi
      continue
      fi
    else
      echo "Failed: app run did not produce odata/out_app_fsk_bbloopback.bin"
      ((failures++))
      ((total_failures++))
    fi
    if [ -f odata/truncated_out_app_fsk_txrx.bin ]; then
        rm odata/truncated_out_app_fsk_txrx.bin
    fi
    if [ $failures -eq $number_of_allowed_failures ]; then
        echo "Error: Failed $number_of_allowed_failures times. Aborting test"
        exit
    fi
  done
  echo "Total Failures: $total_failures"
  echo "Total Tests: $total_tests"
fi

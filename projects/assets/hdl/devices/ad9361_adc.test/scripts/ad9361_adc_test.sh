#!/bin/bash
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

touch toberemoved.log
rm *log > /dev/null 2>&1

if [ -d odata ]; then
  rm -rf odata
fi
mkdir odata

APP_XML=ad9361_adc_test_1r1t_lvds_app.xml
if [ ! -z "$1" ]; then
  APP_XML=$1
fi

if [ ! -f $APP_XML ]; then
  echo "app xml not found: $APP_XML"
  echo "(pwd is: $PWD)"
  exit 1
fi

echo "Running PRBS Built-In-Self-Test across range of sample rates for 1R1T LVDS mode"
OCPI_LIBRARY_PATH=$OCPI_LIBRARY_PATH:./assemblies/:$OCPI_PROJECT_PATH ./scripts/AD9361_BIST_PRBS.sh $APP_XML 2>&1 | tee odata/AD9361_BIST_PRBS.log
if [ "$?" !=  "0" ]; then
  cat odata/AD9361_BIST_PRBS.log
  echo "TEST FAILED"
  exit 1
fi

diff odata/AD9361_BIST_PRBS.log scripts/AD9361_BIST_PRBS.golden
X=$?

if [ "$X" ==  "0" ]; then
  echo "TEST PASSED"
else
  echo "TEST FAILED"
  exit 1
fi

#echo "Running additional reports: (PRBS Built-In-Self-Test across range of clock and data delays)"
#./scripts/AD9361_BIST_PRBS_delays.sh $APP_XML > odata/AD9361_BIST_PRBS_delays.log
exit 0

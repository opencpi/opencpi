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

APP_XML=ad9361_test_1r1t_lvds_app.xml

if [ ! -f $APP_XML ]; then
  echo "app xml not found: $APP_XML"
  echo "(pwd is: $PWD)"
  exit 1
fi

DO_PRBS=1
if [ ! -z "$1" ]; then
  if [ "$1" == "disableprbs" ]; then # ONLY DO THIS TO SAVE TIME IF YOU KNOW
                                     # PRBS IS ALREADY WORKING
    DO_PRBS=0
  fi
fi

if [ "$DO_PRBS" == "1" ]; then
  echo "Running PRBS Built-In-Self-Test across range of sample rates for LVDS mode"
  OCPI_LIBRARY_PATH=$OCPI_LIBRARY_PATH:./assemblies/:$OCPI_PROJECT_PATH ./scripts/AD9361_BIST_PRBS.sh $APP_XML > odata/AD9361_BIST_PRBS.log 2>&1
  if [ "$?" !=  "0" ]; then
    cat odata/AD9361_BIST_PRBS.log
    echo "TEST FAILED"
    exit 1
  fi

  diff odata/AD9361_BIST_PRBS.log scripts/AD9361_BIST_PRBS.golden
  X=$?

  if [ "$X" !=  "0" ]; then
    echo "TEST FAILED"
    exit 1
  fi
fi

echo "Running loopback Built-In-Self-Test across range of sample rates for LVDS mode"
OCPI_LIBRARY_PATH=$OCPI_LIBRARY_PATH:./assemblies/:$OCPI_PROJECT_PATH ./scripts/AD9361_BIST_loopback.sh $APP_XML > odata/AD9361_BIST_loopback.log 2>&1
if [ "$?" !=  "0" ]; then
  cat odata/AD9361_BIST_loopback.log
  echo "TEST FAILED"
  exit 1
fi

diff odata/AD9361_BIST_loopback.log scripts/AD9361_BIST_loopback.golden
X=$?

if [ "$X" ==  "0" ]; then
  echo "TEST PASSED"
else
  echo "TEST FAILED"
  exit 1
fi



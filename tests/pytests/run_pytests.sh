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

# remove any leftover results from last run
make clean
ocpidev -d ../av-test build --hdl-platform isim
MIN_COVERAGE=80 #%
rm -f .coverage
# Run each test and collect coverage info
set -e
if [ -z "$(type -p coverage3 2> /dev/null)" ]; then
  pyrun_command="python3"
else
  pyrun_command="coverage3 run --append"
fi
for i in *_test.py; do
  echo "Running: $pyrun_command $i"
  $pyrun_command $i
  # Run find the corresponding file in the CDK and run the doctest
  doctest=`echo $OCPI_CDK_DIR/scripts/$i | sed -e 's/_test\.py$/\.py/g'`
  if [ -e "$doctest" ]; then
    echo "Running: $pyrun_command $doctest -v"
    OCPI_CDK_DIR=$(pwd) $pyrun_command $doctest -v
  fi
done
if [ "$pyrun_command" == "python" ]; then
  echo "Skipping coverage report because the coverage command does not exist"
else
  # TODO: add a minimum coverage threshold
  coverage3 report --omit "*_test.py" ||\
    sh -c "echo FAIL: coverage less than \"$MIN_COVERAGE\"% ; exit 1"
fi


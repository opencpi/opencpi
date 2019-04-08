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
[ -z "$HDL_PLATFORM" ] && {
  sims=(`ocpirun -C --only-platforms | grep '.*-.*sim' | sed s/^.*-//`)
  if [ -n "$sims" ]; then
     echo Available simulators are: ${sims[*]}, using $sims.
    HDL_PLATFORM=$sims
  else
    echo No simulators are available, not using any.
  fi
}
set -e
# clearly this test cannot run concurrently with av-test!
make -C ../av-test cleaneverything
ocpidev -d ../av-test build ${HDL_PLATFORM:+--hdl-platform $HDL_PLATFORM}
MIN_COVERAGE=80 #%
rm -f .coverage
# Run each test and collect coverage info
if [ -z "$(type -p coverage3 2> /dev/null)" ]; then
  pyrun_command="python3"
else
  pyrun_command="coverage3 run --append"
fi
for i in *_test.py; do
  echo "Running: $pyrun_command $i"
  $pyrun_command $i
done
# Run doctest from the CDK 
find $OCPI_CDK_DIR/$OCPI_TOOL_PLATFORM/lib/_opencpi/util ! -name "__init__.py" -name '*.py' |\
  xargs python3 -m doctest -v 

if [ "$pyrun_command" == "python3" ]; then
  echo "Skipping coverage report because the coverage command does not exist"
else
  # TODO: add a minimum coverage threshold
  coverage3 report --omit "*_test.py" ||\
    sh -c "echo FAIL: coverage less than \"$MIN_COVERAGE\"% ; exit 1"
fi

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

# Be careful with deletions
[ $(basename $0) = run_pytests.sh -a $(basename `pwd`) = pytests ] && {
    echo Removing directories from previous tests
    for d in ./*; do [ -d $d ] && rm -r -f $d; done
}
MIN_COVERAGE=80 #%
rm -f .coverage
# Run each test and collect coverage info
set -e
if [ -z "$(type -p coverage3 2> /dev/null)" ]; then
  pyrun_command="python3"
else
  pyrun_command="coverage3 run"
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
if [ "$pyrun_command" == "python3" ]; then
  echo "Skipping coverage report because the coverage command does not exist"
else
  # TODO: Classic mode for coverage (needed by CentOS6) does not support
  # the '--fail-under=$MIN_COVERAGE' option. Add this option conditionally
  # depending on version, or parse the output for the coverage value.
  coverage3 run --omit "*_test.py" ||\
    sh -c "echo FAIL: coverage less than \"$MIN_COVERAGE\"% ; exit 1"
fi


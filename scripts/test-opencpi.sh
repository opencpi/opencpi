#!/bin/sh
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

##########################################################################################
# Run all the go-no-go tests we have
source $OCPI_CDK_DIR/scripts/ocpitarget.sh $1
bin=$OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin
echo ======================= Running Unit Tests in $bin &&
$bin/ocpitests &&
echo ======================= Running Datatype/protocol Tests &&
$bin/ocpidds -t 10000 > /dev/null &&
echo ======================= Running Python Tests in tests/pytests &&
(cd tests/pytests && git clean -dfx . && ./run_pytests.sh) &&
echo ======================= Running Container Tests &&
$OCPI_CDK_DIR/scripts/run_tests.sh &&
echo ======================= Running unit tests in project/core &&
make -C $OCPI_CDK_DIR/../projects/core runtest &&
echo ======================= Running Application tests in project/assets &&
make -C $OCPI_CDK_DIR/../projects/assets/applications run &&
echo ======================= Running Application tests in project/assets &&
make -C $OCPI_CDK_DIR/../projects/inactive/applications run &&
[ "$OCPI_TOOL_OS" != macos ] && {
  echo ======================= Loading the OpenCPI Linux Kernel driver. &&
    $OCPI_CDK_DIR/scripts/ocpidriver load
}
echo All tests passed.
exit 0

#    stepsForParallel[stepName] = transformAVTestsStep(os)
#    stepsForParallel[stepName] = transformRCCExampleStep(os)
#    stepsForParallel[stepName] = transformOcpiDevTestsStep(os)

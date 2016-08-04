#!/bin/sh

# #####
#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #

# quick way to run all tests
OCPI_RPM=$(rpm -q angryviper-devel 2>/dev/null | grep -v "not installed")
if test -z "${OCPI_RPM}"; then
  # run this in the binary executables directory by doing: ../src/run_tests.sh
  export OCPI_RCC_TARGET=$OCPI_TARGET_HOST
  export OCPI_SMB_SIZE=3000000
  export OCPI_LIBRARY_PATH=$OCPI_CDK_DIR/lib/components
else
  #RPM-based install needs to copy to a directory that the user can read/write
  export OCPI_RCC_TARGET=$(/opt/opencpi/cdk/platforms/getPlatform.sh | awk '{print $4}')
  export OCPI_SMB_SIZE=3000000
  export OCPI_LIBRARY_PATH=/opt/opencpi/cdk/components/lib/rcc/
  # export OCPI_LOG_LEVEL=11
  if test -z "${VG}"; then
    export DIR=$(mktemp -d --tmpdir ocpi_ctests.XXXXX)
  else
    export DIR=/tmp/ctests_run
    rm -rf ${DIR} || :
    mkdir ${DIR}
  fi
  cp --target-directory=${DIR} /opt/opencpi/cdk/bin/${OCPI_RCC_TARGET}/ctests/*
  echo Copied all tests to ${DIR}
  cd ${DIR}
fi

if test "$OCPI_TARGET_OS" = macos; then
  export OCPI_RCC_SUFFIX=dylib
else
  export OCPI_RCC_SUFFIX=so
fi
# tmp=/tmp/ocpictest$$
failed=
set -o pipefail
out="2> /dev/null"
if test "$OUT" != ""; then out="$OUT"; fi
function doit {
  tmp=$1_run.log
  $VG ./$1 $out | tee $tmp | (egrep 'FAILED|PASSED|Error:';exit 0)
  rc=$?
  if egrep -q 'FAILED|Error:' $tmp; then
     echo $1 test failed explicitly, with FAILED or Error message.
     if test "$failed" = ""; then failed=$1; fi
  elif test $rc != 0; then
     echo $1 test failed implicitly, no FAILED message, but non-zero exit.
     if test "$failed" = ""; then failed=$1; fi
  elif ! grep -q PASSED $tmp; then
     echo $1 test failed implicitly, no PASSED message, but zero exit.
     if test "$failed" = ""; then failed=$1; fi
  fi
}
if test $# = 1 ; then
  doit $1
else
for i in `ls -d test* | grep -v '_main' | grep -v '\.'` ; do
  echo Running $i...
  doit $i
done
fi
if test "$failed" = ""; then
  echo All container tests passed.
  if test -n "${OCPI_RPM}"; then
    if test -z "${VG}"; then
      rm -rf ${DIR}
    else
      echo Left files in ${DIR} for examination because valgrind was detected.
    fi
  fi
  exit 0
else
  echo Some container tests failed.  The first to fail was: $failed.
  if test -n "${OCPI_RPM}"; then
    echo Left files in ${DIR} for examination.
  fi
  exit 1
fi

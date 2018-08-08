#!/bin/bash --noprofile
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

# quick way to run all tests
# First ensure CDK and TOOL_xxx vars
#OCPI_BOOTSTRAP=$OCPI_CDK_DIR/scripts/ocpibootstrap.sh; . $OCPI_BOOTSTRAP
export OCPI_SMB_SIZE=3000000

if [ -d $OCPI_CDK_DIR/../project-registry ]; then
  source $OCPI_CDK_DIR/scripts/util.sh
  # Add core AND the default installed projects/core project so that
  # core's artifacts can be found on a remote system via the default
  # installed location
  core1=$(getProjectRegistryDir)/ocpi.core
  [ -d $core1/exports ] && core1+=/exports
  core2=$OCPI_CDK_DIR/../projects/core
  [ -d $core2/exports ] && core2+=/exports
  export OCPI_LIBRARY_PATH=$core1/artifacts:$core2/artifacts
fi
# add the runtime artifacts as a last resort.
export OCPI_LIBRARY_PATH=${OCPI_LIBRARY_PATH:+$OCPI_LIBRARY_PATH:}$OCPI_CDK_DIR/$OCPI_TOOL_DIR/artifacts


# export OCPI_LOG_LEVEL=11
# Allow caller to force location (e.g. Jenkins)
if [ -z "${DIR}" ]; then
  export DIR=$(mktemp -d -t ocpi_ctests.XXXXX)
fi
echo "========= Outputs from these tests will be in: $DIR"
# Where ever this script lives, or what the pwd is, go to where the tests are.
cd $OCPI_CDK_DIR/${OCPI_TARGET_DIR:-$OCPI_TOOL_DIR}/bin/ctests

failed=
set -o pipefail
out="2> /dev/null"
if test "$OUT" != ""; then out="$OUT"; fi
function doit {
  tmp=$DIR/$1_run.log
  $VG ./$1 $out | tee $tmp | (egrep 'FAILED|PASSED|Error:';exit 0)
  rc=$?
  if egrep -q 'FAILED|Error:' $tmp; then
     echo "$1 test failed explicitly, with FAILED or Error message."
     if test "$failed" = ""; then failed=$1; fi
  elif test $rc != 0; then
     echo "$1 test failed implicitly, no FAILED message, but non-zero exit."
     if test "$failed" = ""; then failed=$1; fi
  elif ! grep -q PASSED $tmp; then
     echo "$1 test failed implicitly, no PASSED message, but zero exit."
     if test "$failed" = ""; then failed=$1; fi
  fi
}
if test $# = 1 ; then
  doit $1
else
tests=`for i in test*; do [ -f $i -a -x $i ] && echo $i; done`
[ -z "$tests" ] && {
  echo Error:  ctests appear to be missing from `pwd`
  exit 1
}
for i in $tests; do
  echo "Running $i..."
  doit $i
done
fi
if test "$failed" = ""; then
  echo "All container tests passed."
  if test -z "${VG}"; then
    rm -rf ${DIR}
    echo "Deleted output directory."
  else
    echo "Left files in ${DIR} for examination because valgrind was detected."
  fi
  exit 0
else
  echo "Some container tests failed.  The first to fail was: $failed."
  echo "Left files in ${DIR} for examination."
  exit 1
fi

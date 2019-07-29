#!/bin/sh --noprofile
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

# Note that this script runs on remote/embedded systems and thus may not have access to the full
# development host environment.
# This script is SOURCED at the start of a run script for a platform
#
# Its arguments are: <options> - <ports>
# Options are: run, verify, view, and remote
# Ports are the names of output ports

#echo PARMS: $*
source $OCPI_CDK_DIR/scripts/util.sh
ocpiGetToolPlatform
tput=tput
[ -z "$(command -v tput)" ] && tput=true
spec=$1; shift
component=${spec##*.}
platform=$1; shift
while [ $1 != - ]; do
    [ $1 = run ] && run=run
    [ $1 = verify ] && verify=verify
    [ $1 = view ] && view=view
    [ $1 = remote ] && remote=remote
    shift
done
shift # get rid of the dash
# now the args are all ports
ports=($*)
[ -z "$remote" -a -x runremote.sh -a -n "$run" -a -z "$verify" -a -z "$view" ] && {
    # We are: only running, and running locally, and the platform is remote,
    # run all the cases remotely
    ./runremote.sh TestVerbose=$TestVerbose TestTimeout=$TestTimeout Cases="\"$Cases\"" \
                   ./run.sh run remote
    exit $?
}
# docase <model> <worker> <case> <subcase> <timeout> <duration> <ports>

function docase {
  [ -z "$Cases" ] || {
     local ok
     set -f
     for c in $Cases; do
       [[ ($c == *.* && $3.$4 == $c) || ($c != *.* && $3 == $c) ]] && ok=1
     done
     set +f
     [ -z "$ok" ] && return 0
  }
  [ -n "$header" ] || {
    header=1
    $OcpiEcho -n "Performing test cases for $spec on platform "
    $tput setaf 4 2>/dev/null
    $OcpiEcho -n $platform
    $tput sgr0 2>/dev/null
    echo ".  Functions are: $run $verify $view"
  } 1>&2
  r=0
  [ -z "$run" ] || {
    local output outputs timearg
  # all ports are passed to the script, but to test optional ports they're also passed here
    if [ "$#" -gt 6 ]; then
      for o in ${@:7}; do
        output=" -pfile_write"
        [ ${#ports[@]} != 1 ] && output="$output""_from_$o"
        outputs="$outputs$output=fileName=$3.$4.$2.$1.$o.out"
      done
    fi

    echo '  'Executing case "$3.$4" using worker $2.$1 on platform $platform... 1>&2
    if [ $5 != 0 ]; then
      timearg=--timeout=$5
    elif [ $6 != 0 ]; then
      timearg=--seconds=$6
    elif [ -n "$TestTimeout" ]; then
      timearg=--timeout=$TestTimeout
    fi
    lockrcc=
    # If we are testing in a remote environment keep infrastructure workers local
    [ "$OCPI_ENABLE_REMOTE_DISCOVERY" = 1 -o -n "$OCPI_SERVER_ADDRESS" -o \
      -n "$OCPI_SERVER_ADDRESSES" -o -n "$OCPI_SERVER_ADDRESSES_FILE" ] &&
        lockrcc="-s '?ocpi.core.file_read=model!=\"rcc\"||platform==host_platform' \
                 -s '?ocpi.core.file_write=model!=\"rcc\"||platform==host_platform'"
    cmd=('OCPI_LIBRARY_PATH=../../../lib/rcc:../../../lib/ocl:../../gen/assemblies:$OCPI_CDK_DIR/$OCPI_TOOL_DIR/artifacts' \
             '$OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin/'ocpirun -d -v -H -m$component=$1 -w$component=$2 \
                 $lockrcc -P$component=$platform \
                 --sim-dir=$3.$4.$2.$1.simulation $timearg \
                 --dump-file=$3.$4.$2.$1.props $outputs ../../gen/applications/$3.$4.xml)
    [ -z "$remote" ] && rm -f -r $3.$4.$2.$1.*
    set -o pipefail
    # This breaks under sudo! AV-4234
    if [ "$TestVerbose" = 1 ]; then
        out=/dev/stdout
    else
        out=/dev/null
    fi
    setStartTime
    if [ -z "$remote" -a -x runremote.sh ]; then
        # We are local, running interleaved run/verify and platform is remote
        # Remote execution is simply ocpirun
        ./runremote.sh \
            "TestVerbose=$TestVerbose TestTimeout=$TestTimeout Cases=$3.$4 ./run.sh run remote" \
            2>&1 | tee $3.$4.$2.$1.remote_log > $out
    elif [ -z "$remote" ]; then
        (echo ${cmd[@]}; eval time env ${cmd[@]}) 2>&1 | tee $3.$4.$2.$1.log > $out
    elif [ "$TestVerbose" = 1 ]; then
        (echo ${cmd[@]}; eval time env ${cmd[@]}) 2>&1 | tee $3.$4.$2.$1.log
    else
        (echo ${cmd[@]}; eval time env ${cmd[@]}) > $3.$4.$2.$1.log 2>&1
    fi
    r=$?
    set +o pipefail
    if [ $r = 0 ]; then
      $tput setaf 2 2>/dev/null
       echo '    'Execution succeeded, time was $(getElapsedTime).
      $tput sgr0 2>/dev/null
    else
      $tput setaf 1 2>/dev/null
      if (( r > 128 )); then
        # Fail immediately if execution stopped on a signal
        let s=r-128
        echo '    'Execution FAILED due to signal $s\; log is in run/$platform/$3.$4.$2.$1.log 1>&2
        $tput sgr0 2>/dev/null
        [ $s = 2 ] && exit $r
      else
        echo '    'Execution FAILED\($r\) - see log in run/$platform/$3.$4.$2.$1.log 1>&2
        $tput sgr0 2>/dev/null
      fi
      [ "$TestAccumulateErrors" != 1 ] && exit $r
      failed=1
      return 0
    fi
  }
  [ -z "$view" -a -z "$verify" ] ||
    if [ "$r" = 0 ]; then
      if [ -f $3.$4.$2.$1.props ]; then
        ../../gen/applications/verify_$3.sh $2.$1 $4 $view $verify
        r=$?
        [ -n "$verify" -a $r = 0 -a "$KeepSimulations" != 1 ] && rm -r -f $3.$4.$2.$1.simulation
        if (( r > 128 )); then
          let s=r-128
          echo Verification exited with signal $s. 1>&2
          [ $s = 2 ] && exit $r
        fi
        [ $r = 0 ] && return 0
        failed=1
        [ "$TestAccumulateErrors" = 1 ] && return 0
        exit 1
      else
        $tput setaf 1 2>/dev/null
        echo '    'Verification for $3.$4:  FAILED.  No execution using $2.$1 on platform $platform. 1>&2
        $tput sgr0 2>/dev/null
        failed=1
        [ "$TestAccumulateErrors" = 1 ] && return 0
        exit 1
      fi
    else
      echo Execution failed so verify or view not performed.
    fi
}

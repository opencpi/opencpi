#!/bin/sh --noprofile
# Note that this script runs on remote/embedded systems and thus may not have access to the full
# development host environment.
# This script is SOURCED at the start of a run script for a platform
#
# Its arguments are: <options> - <ports>
# Options are: run, verify, view, and remote
# Ports are the names of output ports

#echo PARMS: $*
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
    # If we are: only runnning, and running locally, and the platform is remote, run remotely
    ./runremote.sh ./run.sh run remote
    exit $?
}
# docase <name> <platform> <model> <worker> <case> <subcase> <implprops>
function docase {
  [ -z "$Cases" ] || {
     local ok
     for c in $Cases; do
       [[ ($c == *.* && $c == $5.$6) || ($c != *.* && $c == $5) ]] && ok=1
     done
     [ -z "$ok" ] && return 0
  }
  [ -n "$header" ] || {
    header=1
    echo Performing test cases for $spec on platform $platform 1>&2
    echo Functions performed in one pass are: $run $verify $view 1>&2
  }
  r=0
  [ -z "$run" ] || {
    local output outputs
    for o in ${ports[@]}; do
      output=" -pfile_write"
      [ ${#ports[@]} != 1 ] && output+="_$o"
      outputs+="$output=fileName=$3.$4.$2.$1.$o.out"
    done
    echo Running $component test case: "$3.$4" on platform $platform using worker $2.$1... 1>&2
    cmd=('OCPI_LIBRARY_PATH=../../../lib/rcc:../../gen/assemblies:$OCPI_CDK_DIR/lib/components/rcc' \
         ocpirun -l8 -d -v -m$component=$1 -w$component=$2 -P$component=$platform \
	         --sim-dir=$3.$4.$2.$1.simulation \
		 --dump-file=$3.$4.$2.$1.props $outputs ../../gen/applications/$3.$4.xml)
    rm -f -r $3.$4.$2.$1.*
    if [ -z "$remote" -a -x runremote.sh ]; then
      ./runremote.sh "(echo ${cmd[@]}; time ${cmd[@]})" > $3.$4.$2.$1.log 2>&1
    else
      (echo ${cmd[@]}; eval time env ${cmd[@]}) > $3.$4.$2.$1.log 2>&1 
    fi
    r=$?
    [ $r != 0 ] && {
	tput setaf 1
	echo Execution FAILED - see log in run/$platform/$3.$4.$2.$1.log 1>&2
	tput sgr0
	failed=1
    }
  }
  [ -z "$view" -a -z "$verify" ] || 
    if [ "$r" = 0 ]; then
      ../../gen/applications/verify_$3.sh $2.$1 $4 $view $verify
      [ -n "$verify" -a $? = 0 -a -z "$KeepSimulations" ] && rm -r -f $3.$4.$2.$1.simulation
    else
      echo Execution failed so verify or view not performed.
    fi
}
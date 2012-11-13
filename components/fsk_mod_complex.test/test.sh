#!/bin/bash 
rt=$( ../include/uut.sh $1 );
rc=$?

if [[ $rc != 0 ]]; then
    echo $rt;
    exit $rc;
fi
./unitTest --utname=fsk_mod_complex --compp="<property name='deviation' value='100'/>" --real=false --model=$1
rc=$?
if [[ $rc != 0 ]]; then
    exit $rc;
fi
./utTime.sh 10
rc=$?
exit $rc



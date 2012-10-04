#!/bin/bash 
rt=$( ../include/uut.sh $1 );
rc=$?

if [[ $rc != 0 ]]; then
    echo $rt;
    exit $rc;
fi
./unitTest --utname=fm_demod_complex --compp="<property name='deviation' value='100'/>" --real=true --model=$1
rc=$?
exit $rc;



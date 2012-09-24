#!/bin/bash 
rt=$( ../include/uut.sh $1 );
rc=$?

if [[ $rc != 0 ]]; then
    echo $rt;
    exit $rc;
fi
./unitTest --utname=noise_gen_complex --compp="<property name='deviation' value='100'/>"  --utp="<property name='mask' value='15'/>" --real=false --model=$1

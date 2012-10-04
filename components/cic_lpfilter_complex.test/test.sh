#!/bin/bash 
rt=$( ../include/uut.sh $1 );
rc=$?

if [[ $rc != 0 ]]; then
    echo $rt;
    exit $rc;
fi
./unitTest --utname=cic_lpfilter_complex --compp="<property name='deviation' value='100'/>" --utp="<property name='M' value='10'/>" --real=false --model=$1
rc=$?
exit $rc;

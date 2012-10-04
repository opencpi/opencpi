#!/bin/bash 
rt=$( ../include/uut.sh $1 );
rc=$?

if [[ $rc != 0 ]]; then
    echo $rt;
    exit $rc;
fi
./unitTest --utname=cic_hpfilter_complex --compp="<property name='deviation' value='100'/>" --utp="<property name='M' value='2'/>" --real=false --model=$1
rc=$?
exit $rc;

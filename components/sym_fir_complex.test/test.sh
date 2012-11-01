#!/bin/bash 
rt=$( ../include/uut.sh $1 );
rc=$?

if [[ $rc != 0 ]]; then
    echo $rt;
    exit $rc;
fi
./unitTest --utname=sym_fir_complex --compp="<property name='deviation' value='100'/>" --utp="<property name='taps' valuefile='fir_complex_coefs.xml'/>" --real=false --model=$1
rc=$?
if [[ $rc != 0 ]]; then
    exit $rc;
fi
./utTime.sh 10
rc=$?
exit $rc;


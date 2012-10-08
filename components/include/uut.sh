#!/bin/bash -f
if [ -z $1 ]; then
    echo 'You must select a model for the unit under test, valid models are {rcc,hdl,ocl}';
    exit -1;
fi
echo UUT model selection = $1
exit 0;
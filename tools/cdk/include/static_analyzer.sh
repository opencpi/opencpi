#!/bin/bash -f
analyzer=/opt/opencpi/prerequisites/loc/hfcca14.py
if [ -z $analyzer ]; then
    echo 'You must install the static code anayzer to use this target, see $(OCPI_BASE_DIR)/intall_codeanalyzer.sh'
    exit -1;
fi
ofile=$1
oft=$1.tmp
echo 'Analyzing the software components'
rm -rf $ofile
python $analyzer >> $oft;
cat $OCPI_BASE_DIR/tools/cdk/include/Metrics_Header.txt >> $ofile;
cat $oft | sed -n '/LOC    Avg.NLOC AvgCCN Avg.ttoken  function_cnt    file/,$ p' >> $ofile;
rm -rf $oft








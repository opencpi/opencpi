#!/bin/bash -f 
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

analyzer=/opt/opencpi/prerequisites/loc/hfcca14.py
if [ -z "$analyzer" ]; then
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
rm -rf $oft;
echo "  " >> $ofile;
echo "  " >> $ofile;
echo "This section profiles the static component section size allocations" >> $ofile;
echo "  " >> $ofile;
pushd $OCPI_BASE_DIR/components/lib/rcc/linux-x86_64/ ;
ls >> ../../../$oft
`size @../../../$oft >> ../../../$ofile;` 
rm -rf ../../..$oft
popd












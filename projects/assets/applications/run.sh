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

set -e -x
OPTS=$1
BIAS=$2
BIAS0=$3
DURATION=$4
CMP=" && cmp test.input test.output"
$VG ocpirun -v -d $OPTS $BIAS $FR $FW bias
$VG ocpirun -v -d -pbias=biasValue=0 $OPTS $BIAS $FR $FW bias $CMP
$VG ocpirun -v -d $OPTS $FR $FW copy $CMP
$VG ocpirun -v -d $OPTS $BIAS $FR file-bias-capture
$VG ocpirun -v -d $OPTS hello
$VG ocpirun -v -d $OPTS $BIAS $FW pattern-bias-file
$VG ocpirun -v -d $OPTS $BIAS $FW pattern
$VG ocpirun -v -d $OPTS $FR $FW proxybias
$VG ocpirun -v -d -pproxy=proxybias=0 $OPTS $BIAS $FR $FW proxybias $CMP
# This delay is necessary until capture.hdl has the stop-on-eof feature
# The default is appropropriate for hardware, but not sim
$VG ocpirun -v -d $OPTS $BIAS -t ${DURATION:-3} tb_bias
$VG ocpirun -v -d $OPTS $BIAS $FR tb_bias_file
$VG ocpirun -v -d $OPTS $BIAS $FR $FW testbias
$VG ocpirun -v -d -pbias=biasValue=0 $OPTS $BIAS $FR $FW testbias $CMP
$VG ocpirun -v -d $OPTS $BIAS0 $FR $FW testbias2
$VG ocpirun -v -d -pbias0=biasValue=0 -pbias1=biasValue=0 $OPTS $BIAS0 $FR $FW testbias2 $CMP

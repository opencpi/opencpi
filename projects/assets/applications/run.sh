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

##########################################################################################
# This file is for running on embedded systems where there is no make, python, etc.
set -e -x
OPTS=$1
BIAS=$2
BIAS0=$3
DURATION=$4
CMP=" && cmp test.input test.output"
export OCPI_LIBRARY_PATH=$OCPI_CDK_DIR/../projects/assets/artifacts:$OCPI_CDK_DIR/../projects/core/artifacts:/mnt
echo OCPI_LIBRARY_PATH===$OCPI_LIBRARY_PATH
$VG ocpirun -v -d $OPTS $FR $FW copy $CMP
$VG ocpirun -v -d $OPTS hello
$VG ocpirun -v -d $OPTS $FR $FW proxybias
$VG ocpirun -v -d -pproxy=proxybias=0 $OPTS $BIAS $FR $FW proxybias $CMP
$VG ocpirun -v -d $OPTS $BIAS0 $FR $FW testbias2
$VG ocpirun -v -d -pbias0=biasValue=0 -pbias1=biasValue=0 $OPTS $BIAS0 $FR $FW testbias2 $CMP
function doit {
$VG ocpirun -v -d $OPTS $BIAS $FR $FW bias
$VG ocpirun -v -d -pbias=biasValue=0 $OPTS $BIAS $FR $FW bias $CMP
# This delay is necessary until capture.hdl has the stop-on-eof feature
# The default is appropriate for hardware, but not sim
$VG ocpirun -v -d $OPTS $BIAS $FR $FW testbias
$VG ocpirun -v -d -pbias=biasValue=0 $OPTS $BIAS $FR $FW testbias $CMP
}
OPTS=-m=rcc
doit
OPTS=-mbias=hdl
doit

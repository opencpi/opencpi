#!/bin/bash --noprofile
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


NUM_BYTES=$1
FILENAME=$2

echo REG_BIT_WIDTH_p $OCPI_TEST_REG_BIT_WIDTH_p
echo num_samples $OCPI_TEST_num_samples
echo mode $OCPI_TEST_mode
echo enable $OCPI_TEST_enable
echo fixed_value $OCPI_TEST_fixed_value
echo LFSR_bit_reverse $OCPI_TEST_LFSR_bit_reverse

echo number of samples in output file: $(xxd -b -c 4 $FILENAME | wc -l)
CMD="xxd -b -c 4 -l $NUM_BYTES $FILENAME"
echo "output from: $CMD"
echo "         I LSB    I MSB    Q LSB    Q MSB"
exec $CMD


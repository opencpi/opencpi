#!/usr/bin/env python3
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

"""
FIR Real: Validate output data for Symmetric FIR Real (2xNUM_TAPS_p taps)

Validation args:
- output data filename
- input data file (not used)

To test the FIR Real filter, a binary data file is generated containing an impulse.
The output of the filter is thus an impulse response, showing the symmetric tap values.

"""

import sys
import os.path
import opencpi.colors as color
import bin2int


if len(sys.argv) < 2:
    print("Usage expected: output filename, input filename\n")
    sys.exit(1)

#Generate text file of symmetric tap values for comparison
num_taps=2*int(os.environ.get("OCPI_TEST_NUM_TAPS_p"))
taps=list(map(int, os.environ.get("OCPI_TEST_taps").split(","))) #tap values are comma-separated
output_file=sys.argv[1]

#The following section tests the framework and is not generally needed for enduser testing
#start framework test
goldTapsFilename="../../gen/properties/{0}.{1}.{2}".format(os.environ['OCPI_TESTCASE'], os.environ['OCPI_TESTSUBCASE'], "taps")
fp=open(goldTapsFilename)
goldtaps=list(map(int,fp.readlines()))
fp.close()

if goldtaps!=taps:
    print ('taps does not match goldtaps" in ' + goldTapsFilename)
    sys.exit(1)
#end framework test

#Check to confirm number of taps is as expected
if num_taps != 2*len(taps):
    print ('Actual number of taps does not match specified NUM_TAPS_p')
    sys.exit(1)

#Check to make sure not all taps are zero
if( sum(list(map(abs,taps))) == 0):
    print ('taps are all zero')
    sys.exit(1)

#Convert output binary file to text file
filter_group_delay = int(num_taps/2)+4
bin2int.bin2int_real(output_file, output_file.rstrip('out')+'tmp', num_taps, filter_group_delay)

#Compare symmetric taps file to output
taps.extend(reversed(taps)); #extend the taps to be symmetric
data2cmp = [line.strip() for line in open(output_file.rstrip('out')+'tmp')]
for x in range(len(taps)):
    if abs(abs(int(taps[x])) - abs(int(data2cmp[x]))) > 1:
        print ('data2cmp[' + str(x) + '] = ' + str(data2cmp[x]) + ' while expected = ' + str(taps[x]))
        sys.exit(1)

print ('Data matched expected results.')

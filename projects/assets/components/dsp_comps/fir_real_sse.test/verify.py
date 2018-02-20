#!/usr/bin/env python
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

class color:
    PURPLE = '\033[95m'
    CYAN = '\033[96m'
    DARKCYAN = '\033[36m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    END = '\033[0m'


print "\n","*"*80
print "*** Python: FIR Real ***"

print "*** Validation of Real FIR output (binary data file) ***"

if len(sys.argv) < 2:
    print("Usage expected: output filename, input filename\n")
    sys.exit(1)

#Generate text file of symmetric tap values for comparison
num_taps=2*int(os.environ.get("OCPI_TEST_NUM_TAPS_p"))
taps=map(int, os.environ.get("OCPI_TEST_taps").split(",")) #tap values are comma-separated
output_file=sys.argv[1]

#The following section tests the framework and is not generally needed for enduser testing
#start framework test
goldTapsFilename="../../gen/properties/{0}.{1}.{2}".format(os.environ['OCPI_TESTCASE'], os.environ['OCPI_TESTSUBCASE'], "taps")
fp=open(goldTapsFilename)
goldtaps=map(int,fp.readlines())
fp.close()

if goldtaps!=taps:
    print color.RED + color.BOLD + 'FAILED: taps does not match goldtaps in ' + goldTapsFilename
    print color.RED + color.BOLD + '*** Error: End Validation ***\n' + color.END
    sys.exit(1)
#end framework test

#Check to confirm number of taps is as expected
if num_taps != 2*len(taps):
    print color.RED + color.BOLD + 'FAILED: Actual number of taps does not match specified NUM_TAPS_p'
    print color.RED + color.BOLD + '*** Error: End Validation ***\n' + color.END
    sys.exit(1)

#Check to make sure not all taps are zero
if( sum(map(abs,taps)) == 0):
    print color.RED + color.BOLD + 'FAILED: taps are all zero'
    print color.RED + color.BOLD + '*** Error: End Validation ***\n' + color.END
    sys.exit(1)

#Convert output binary file to text file
build_str = 'python ../../bin2int.py ' + output_file + ' ' + output_file.rstrip('out')+'tmp' + ' ' + str(num_taps) + ' ' + str(int(num_taps)/2+4)
print '\n'+build_str
os.system(build_str)

#Compare symmetric taps file to output
taps.extend(reversed(taps)); #extend the taps to be symmetric
data2cmp = [line.strip() for line in open(output_file.rstrip('out')+'tmp')]
for x in range(len(taps)):
    if abs(abs(int(taps[x])) - abs(int(data2cmp[x]))) > 1:
        print color.RED + color.BOLD + 'FAILED' + color.END, x, taps[x], data2cmp[x]
        print color.RED + color.BOLD + '*** Error: End Validation ***\n' + color.END
        sys.exit(1)

print 'Data matched expected results.'
print color.GREEN + color.BOLD + 'PASSED' + color.END
print '*** End Validation ***\n'

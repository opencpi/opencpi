#!/usr/bin/env python2
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
AGC Real: Validate output data for AGC Real (binary data file).

Validate args:
- amount to validate (number of real signed 16-bit samples)
- output target file
- input file generated during the previous generate step

To test the AGC Real component, a binary data file is generated containing
real signed 16-bit samples with a tone at Fs/16 where the first 1/4 of the
file is at 20% max amplitude, the second 1/4 of the file at 90% max amplitude,
the third 1/4 of the file at 20% max amplitude, and the last 1/4 of the file at
30% max amplitude. The output file is also a binary file containing real
signed 16-bit samples where the amplitude has been smoothed by the AGC circuit
to the REF amplitude property setting and the tone is still present and of
sufficient power. The input file produced during the generate phase is also fed
to the validation phase where a python implementation of the agc is compared to
the UUT output.

"""
import sys
import opencpi.colors as color
import numpy as np


print "\n","*"*80
print "*** Python: AGC Real ***"

print "*** Validate output against expected data ***"
if len(sys.argv) < 2:
    print ("Exit: Need to know how many input samples")
    sys.exit(1)
elif len(sys.argv) < 3:
    print("Exit: Enter an output filename")
    sys.exit(1)
elif len(sys.argv) < 4:
    print("Exit: Enter an input filename")
    sys.exit(1)

num_samples = int(sys.argv[1])

# Read all of input data file as real int16
print 'Input file to validate: ', sys.argv[3]
ifx = open(sys.argv[3], 'rb')
din = np.fromfile(ifx, dtype=np.int16, count=-1)
ifx.close()

# Read all of output data file as real int16
print 'Output file to validate: ', sys.argv[2]
ofx = open(sys.argv[2], 'rb')
dout = np.fromfile(ofx, dtype=np.int16, count=-1)
ofx.close()

# Ensure dout is not all zeros
if all(dout == 0):
    print color.RED + color.BOLD + 'FAILED, values are all zero' + color.END
    sys.exit(1)
# Ensure that dout is the expected amount of data
if len(dout) != num_samples:
    print color.RED + color.BOLD + 'FAILED, input file length is unexpected' + color.END
    print color.RED + color.BOLD + 'Length dout = ', len(dout), 'while expected length is = ' + color.END, num_samples
    sys.exit(1)

print 'Real Input Avg  = ', np.mean(din)
print 'Real Output Avg = ', np.mean(dout)

# Perform the AGC function on the input data
Navg   = 16
maxint = pow(2,Navg-1)-1
Ref    = 0.3*maxint/np.sqrt(2)
Mu     = 1/(4*(maxint-Ref))

# initial values
det_buf = np.array(np.zeros(Navg), dtype=np.float32)          # buffer to measure output signal
gain    = np.array(np.ones(num_samples), dtype=np.float32)    # loop gain
err     = np.array(np.zeros(num_samples), dtype=np.float32)   # loop error
ydet    = np.array(np.zeros(num_samples), dtype=np.float32)   # output detected
y       = np.array(np.zeros(num_samples+1), dtype=np.float32) # output

for i in xrange(Navg-1,num_samples): # lagging by Navg samples
    # detecting output level
    det_buf = y[i+1-Navg:i+1] # buffering
    ydet[i] = sum(abs(det_buf))/Navg

    # compare to reference
    err[i] = Ref - ydet[i]

    # correct the gain to VGA
    gain[i] = gain[i-1] + err[i]*Mu
    if abs(gain[i]) > maxint: # limit to max
        gain[i] = (gain[i]>=maxint)*maxint + (gain[i]< maxint)*(-maxint)

    # VGA, variable gain amplifier
    y[i+1] = np.rint(gain[i] * din[i])

# compare python AGC (y) to UUT output (dout)
for i in xrange(Navg-1+384,num_samples/4):
    if abs(y[i+1] - dout[i+2]) > 2:
        print color.RED + color.BOLD + 'FAILED Real' + color.END, i, y[i+1], dout[i+2], y[i+1]-dout[i+2]
        print color.RED + color.BOLD + '*** Error: End Validation ***\n' + color.END
        sys.exit(1)
for i in xrange(num_samples/4+896,num_samples/2):
    if abs(y[i+1] - dout[i+2]) > 2:
        print color.RED + color.BOLD + 'FAILED Real' + color.END, i, y[i+1], dout[i+2], y[i+1]-dout[i+2]
        print color.RED + color.BOLD + '*** Error: End Validation ***\n' + color.END
        sys.exit(1)
for i in xrange(num_samples/2+384,num_samples*3/4):
    if abs(y[i+1] - dout[i+2]) > 2:
        print color.RED + color.BOLD + 'FAILED Real' + color.END, i, y[i+1], dout[i+2], y[i+1]-dout[i+2]
        print color.RED + color.BOLD + '*** Error: End Validation ***\n' + color.END
        sys.exit(1)
for i in xrange(num_samples*3/4+256,num_samples-2):
    if abs(y[i+1] - dout[i+2]) > 2:
        print color.RED + color.BOLD + 'FAILED Real' + color.END, i, y[i+1], dout[i+2], y[i+1]-dout[i+2]
        print color.RED + color.BOLD + '*** Error: End Validation ***\n' + color.END
        sys.exit(1)

print 'Data matched expected results.'
print color.GREEN + color.BOLD + 'PASSED' + color.END
print '*** End validation ***\n'

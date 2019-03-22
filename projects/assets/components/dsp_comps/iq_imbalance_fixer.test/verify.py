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
IQ Imbalance Fixer: Verify output data

Verify args:
1. Number of complex signed 16-bit samples to validate
2. Output file used for validation
3. Input file used for comparison

To test the IQ Imbalance Fixer, a binary data file is generated containing complex
signed 16-bit samples with tones at 5Hz, 13Hz, and 27Hz. A phase offset of 10
degrees is applied to the Q channel, and then different gain amounts between the
I and Q rails, which result in a spectral image in the range of -Fs/2 to DC.

To validate the test, the output file is examined using FFT analysis to determine
that the spectral image has been removed and the tones are still present and of
sufficient power in the range DC to Fs/2.
"""
import os.path
import shutil
import struct
import sys
import opencpi.colors as color
import numpy as np

print "\n","*"*80
print "*** Python: IQ Imbalance Fixer ***"

print "*** Validation of IQ Imbalance Fixer output (binary data file) ***"
if len(sys.argv) != 4:
    print("Invalid arguments:  usage is: verify.py <num-samples> <output-file> <input-file>")
    sys.exit(1)

num_samples = int(sys.argv[1])
ofilename = sys.argv[2]
ifilename = sys.argv[3]

dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))

#Read input and output data files as complex int16
ifile = open(ifilename, 'rb')
din = np.fromfile(ifile, dtype=dt_iq_pair, count=-1)
ifile.close()
ofile = open(ofilename, 'rb')
dout = np.fromfile(ofile, dtype=dt_iq_pair, count=-1)
ofile.close()

enable = os.environ.get("OCPI_TEST_enable")

if(enable=="true"): # => NORMAL MODE
    #Throw away the first half of the output file to remove the start-up transients
    dout_normal = dout[num_samples/2:num_samples]

    #Ensure dout is not all zeros
    if all(dout_normal == 0):
        print color.RED + color.BOLD + 'FAILED, values are all zero' + color.END
        sys.exit(1)

    #Ensure that dout is the expected amount of data
    if len(dout_normal) != num_samples/2:
        print color.RED + color.BOLD + 'FAILED, output file length is unexpected' + color.END
        print color.RED + color.BOLD + 'Length dout = ', len(dout_normal)/2, 'while expected length is = ' + color.END, num_samples
        sys.exit(1)

    #share values used during generation of the input file
    #convert to complex data type to perform fft and power measurements
    Tone05 = 5
    Tone13 = 13
    Tone27 = 27
    Fs = 100

    complex_idata = np.array(np.zeros(num_samples), dtype=np.complex)
    complex_odata = np.array(np.zeros(num_samples/2), dtype=np.complex)
    for i in xrange(0,num_samples):
        complex_idata[i] = complex(din['real_idx'][i], din['imag_idx'][i])
    for i in xrange(0,num_samples/2):
        complex_odata[i] = complex(dout_normal['real_idx'][i], dout_normal['imag_idx'][i])

    IFFT = 1.0/num_samples * abs(np.fft.fft(complex_idata,num_samples))
    OFFT = 1.0/(num_samples/2) * abs(np.fft.fft(complex_odata,num_samples/2))
    eps = pow(10, -10) #Error factor to avoid divide by zero in log10

    #input: three tones in range DC to +Fs/2
    IPowerT1  = 20*np.log10(IFFT[float(Tone05)/(float(Fs)/2.0)*float(len(IFFT)/2.0)]+eps)
    IPowerT2  = 20*np.log10(IFFT[float(Tone13)/(float(Fs)/2.0)*float(len(IFFT)/2.0)]+eps)
    IPowerT3  = 20*np.log10(IFFT[float(Tone27)/(float(Fs)/2.0)*float(len(IFFT)/2.0)]+eps)
    #input: three spectral image tones in range -Fs/2 to DC
    IPowerNT1 = 20*np.log10(IFFT[float(-Tone05)/(float(Fs)/2.0)*float(len(IFFT)/2.0)]+eps)
    IPowerNT2 = 20*np.log10(IFFT[float(-Tone13)/(float(Fs)/2.0)*float(len(IFFT)/2.0)]+eps)
    IPowerNT3 = 20*np.log10(IFFT[float(-Tone27)/(float(Fs)/2.0)*float(len(IFFT)/2.0)]+eps)
    print 'Input Tone 1 power level = ', IPowerT1, ' dBm'
    print 'Input Tone 2 power level = ', IPowerT2, ' dBm'
    print 'Input Tone 3 power level = ', IPowerT3, ' dBm'
    print 'Input image Tone 1 power level = ', IPowerNT1, ' dBm'
    print 'Input image Tone 2 power level = ', IPowerNT2, ' dBm'
    print 'Input image Tone 3 power level = ', IPowerNT3, ' dBm'

    #output: determine the max power level in the range -Fs/2 to DC
    neg_freqs=OFFT[num_samples/4:num_samples/2]
    max_neg_freq=20*np.log10(neg_freqs[np.argmax(neg_freqs)])
    #output: determine the max power level in the range DC to +Fs/2
    pos_freqs=OFFT[0:num_samples/4]
    max_pos_freq=20*np.log10(pos_freqs[np.argmax(pos_freqs)])
    #output: three tones in range DC to +Fs/2
    OPowerT1  = 20*np.log10(OFFT[float(Tone05)/(float(Fs)/2.0)*float(len(OFFT)/2.0)]+eps)
    OPowerT2  = 20*np.log10(OFFT[float(Tone13)/(float(Fs)/2.0)*float(len(OFFT)/2.0)]+eps)
    OPowerT3  = 20*np.log10(OFFT[float(Tone27)/(float(Fs)/2.0)*float(len(OFFT)/2.0)]+eps)
    #three suppressed spectral image tones in range -Fs/2 to DC
    OPowerNT1 = 20*np.log10(OFFT[float(-Tone05)/(float(Fs)/2.0)*float(len(OFFT)/2.0)]+eps)
    OPowerNT2 = 20*np.log10(OFFT[float(-Tone13)/(float(Fs)/2.0)*float(len(OFFT)/2.0)]+eps)
    OPowerNT3 = 20*np.log10(OFFT[float(-Tone27)/(float(Fs)/2.0)*float(len(OFFT)/2.0)]+eps)
    print 'Maximum Frequency from -Fs/2 to 0 = ', max_neg_freq, ' dBm'
    print 'Maximum Frequency from 0 to +Fs/2 = ', max_pos_freq, ' dBm'
    print 'Output Tone 1 power level = ', OPowerT1, ' dBm'
    print 'Output Tone 2 power level = ', OPowerT2, ' dBm'
    print 'Output Tone 3 power level = ', OPowerT3, ' dBm'
    print 'Output image Tone 1 power level = ', OPowerNT1, ' dBm'
    print 'Output image Tone 2 power level = ', OPowerNT2, ' dBm'
    print 'Output image Tone 3 power level = ', OPowerNT3, ' dBm'

    #perform calculations comparing output power to input power for spectral images and tones
    if abs(OPowerT1 - IPowerT1) > 6.9:
        print color.RED + color.BOLD + 'FAILED, Output Tone 1 level = ', OPowerT1, ' dBm' + color.END
        sys.exit(1)
    if abs(OPowerT2 - IPowerT2) > 6.4:
        print color.RED + color.BOLD + 'FAILED, Output Tone 2 level = ', OPowerT2, ' dBm' + color.END
        sys.exit(1)
    if abs(OPowerT3 - IPowerT3) > 6.6:
        print color.RED + color.BOLD + 'FAILED, Output Tone 3 level = ', OPowerT3, ' dBm' + color.END
        sys.exit(1)
    if IPowerNT1 - OPowerNT1 < 67.9:
        print color.RED + color.BOLD + 'FAILED, Output image Tone 1 level = ', OPowerNT1, ' dBm' + color.END
        sys.exit(1)
    if IPowerNT2 - OPowerNT2 < 79.5:
        print color.RED + color.BOLD + 'FAILED, Output image Tone 2 level = ', OPowerNT2, ' dBm' + color.END
        sys.exit(1)
    if IPowerNT3 - OPowerNT3 < 73.3:
        print color.RED + color.BOLD + 'FAILED, Output image Tone 3 level = ', OPowerNT3, ' dBm' + color.END
        sys.exit(1)
    #compare max tone in range DC to +Fs/2 to max value of noise floor in range -Fs/2 to DC
    if max_pos_freq - max_neg_freq < 64.4:
        print color.RED + color.BOLD + 'FAILED, Noise floor from -Fs/2 to 0 too high' + color.END
        sys.exit(1)

    print 'Data matched expected results.'
    print color.GREEN + color.BOLD + 'PASSED' + color.END
    print '*** End validation ***\n'
else: # => BYPASS MODE
    #There is a 4 sample latency in processing, so the first 4 samples of the output are 0. Correcting for that here
    din_bypass = din[0:num_samples-4]
    dout_bypass = dout[4:num_samples]
    #Test that odata is the expected amount
    if (din_bypass != dout_bypass).all():
        print color.RED + color.BOLD + "FAILED: Input and output file do not match" + color.END
        sys.exit(1)
    else:
        print color.GREEN + color.BOLD + "PASSED: Input and output file match" + color.END

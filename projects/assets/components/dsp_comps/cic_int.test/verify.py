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
CIC Interpolator: Verify output data

Verify args:
1. amplitude of output file
2. name of output data file
3. name of input data file

Tests:
#1: Not all zeros
#2: Is the expected amount
#3: Matches the expected output
"""
import datetime
import sys
import os.path
import opencpi.colors as color
import numpy as np


if len(sys.argv) != 4:
    print("Invalid arguments:  usage is: verify.py <amplitude> <output-file> <input-file>")
    sys.exit(1)
print "    VALIDATE (I/Q 16b binary data file):"

# from OCS or OWD
N=int(os.environ.get("OCPI_TEST_N"))
M=int(os.environ.get("OCPI_TEST_M"))
R=int(os.environ.get("OCPI_TEST_R"))

# from arguments to generate.py (-test.xml)
AMPLITUDE = int(sys.argv[1])

# from -test.xml (properties that are declared to be test='true')
Ft = int(os.environ.get("OCPI_TEST_TARGET_FREQ"))      # target frequency

print '    UUT:(N=%d, M=%d, R=%d) Test Data:(%d)' % (N,M,R,Ft)

# Declare complex data type
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))

# Read output and input data as complex int16 samples
ofilename = open(sys.argv[2], 'rb')
odata = np.fromfile(ofilename, dtype=dt_iq_pair, count=-1)
ofilename.close()
ifilename = open(sys.argv[3], 'rb')
idata = np.fromfile(ifilename, dtype=dt_iq_pair, count=-1)
ifilename.close()

# Deinterleave numpy.array into I & Q arrays
real = odata['real_idx']
imag = odata['imag_idx']

# Test #1 - Check that output data is not all zeros
if all(odata == 0):
    print '    ' + color.RED + color.BOLD + 'FAIL, values are all zero' + color.END
    sys.exit(1)
else:
    print '    PASS - File is not all zeros'

# Test #2 - Check that output data is the expected amount
# Calculate the expected input file size (adjusted by interpolation factor and bytes lost at startup)
if M == 1:
    bytes_lost_at_startup = int (N*R+N+M)*4
elif M > 1 :
    bytes_lost_at_startup = int (N*R+N+M-1)*4
ifile_nbytes = int(os.stat(sys.argv[3]).st_size)
#print 'DBG:  Input filesize:', sys.argv[3],"/",ifile_nbytes,"bytes","/",ifile_nbytes / 4,"samples"
ofile_nbytes = int(os.stat(sys.argv[2]).st_size)
#print 'DBG: Output filesize:\t', sys.argv[2],"/",ofile_nbytes,"bytes","/",ofile_nbytes / 4,"samples"
if ((ifile_nbytes * R - bytes_lost_at_startup) != ofile_nbytes):
    print '    FAIL - Output file is not the correct size.  Expected: ', ifile_nbytes * R - bytes_lost_at_startup, ", Got: ", ofile_nbytes;
    sys.exit(1)
else:
    print '    PASS - Output file is the correct size'

# Test #3 - Check that output data values: Unity Gain Response or Tone
if (Ft == 0): # Unity Gain Response (DC input results in DC output)
    # Calculate the expected output DC value, by scaling the input DC value
    # based on the configuration of the worker under test
    AMPLITUDE = int(AMPLITUDE * ((R*M)**N) / 2**np.ceil((N*(np.log2(R*M))))) #overwrite
    #print 'DBG: Test for Unity Gain Response: expected amplitude=%d' % AMPLITUDE
    # Skip startup data, then compare the steady-state output for expected DC value
    skip_index = (N+M)*2
    #print 'DBG: skip_index and I/Q values:', skip_index, real[skip_index], imag[skip_index]
    if all(real[skip_index::] != AMPLITUDE) and all(imag[skip_index::] != AMPLITUDE):
        print '    FAIL - I or Q values do not match the expected constant value of %d' % AMPLITUDE
        sys.exit(1)
    else:
        #nsamples_matched = len(real[skip_index::] / 2)
        print '    PASS - I & Q values match the expected constant value of %d' % AMPLITUDE
else: # Compare Expected vs Measured tone
    Fs = float(1024000 / R)     # sample frequency used in generate.py
    Fs_int = Fs * R             # sample frequency after decimation
    T1 = 50.0                   # frequency of target signal
    nsamples = len(real)
    if (R >= 2048):
        # When there is a large amount of data to process, due to a large interpolator factor,
        # limit the number of samples to process, to largest power of 2.
        # This greatly reduces the FFT processing duration,
        # with a small sacrifice of not processing all of the data.
        nsamples = int(2**(np.floor(np.log2(len(real)))))
    data=np.zeros((nsamples), dtype=complex)
    data.real = real[:nsamples]
    data.imag = imag[:nsamples]
    # Perform FFT
    w = np.fft.fft(data)
    freqs = np.fft.fftfreq(len(w), 1.0/Fs_int)
    # Locate max Tone
    idx_of_max_tone = np.argmax(np.abs(w))
    freq_of_max_tone = freqs[idx_of_max_tone]
    measured_freq_hz = round(freq_of_max_tone, 1)
    #print "DBG: Expected=%.1f vs Measured=%.1f :" % (T1, measured_freq_hz)
    if (T1 != measured_freq_hz): # Expected vs Measured
        print '    FAIL - Expected Tone not detected @ %.1f Hz' % measured_freq_hz
        sys.exit(1)
    else:
        print '    PASS - Expected Tone detected @ %.1f Hz' % T1

print '    Data matched expected results.'
print '    ' + color.GREEN + color.BOLD + 'PASSED' + color.END

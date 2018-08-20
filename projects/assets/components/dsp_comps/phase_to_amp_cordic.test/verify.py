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
Phase to Amplitude CORDIC: Verify output data

Verify args:
1. name of output file
2. name of input file

To test the Phase to Amplitude CORDIC, two operating modes are tested:
A) When enable=0, the worker configured for 'BYPASS' Mode and simply
passes the 16b real input to the outputs lower 16b [15:0] data lines.
B) When enable=1, a constant (DC) input results in a constant output frequency.

Validation Tests:
#1: Not all zeros
#2: Is the expected amount
#3: Matches the expected tone
"""
import sys
import os.path
import numpy as np

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

if len(sys.argv) != 3:
    print("Invalid arguments:  usage is: verify.py <output-file> <input-file>")
    sys.exit(1)
print "    VALIDATE (I/Q 16b binary data file):"

# from OCS or OWD
enable = os.environ.get("OCPI_TEST_enable")
magnitude = int(os.environ.get("OCPI_TEST_magnitude"))
DATA_WIDTH = int(os.environ.get("OCPI_TEST_DATA_WIDTH"))
DATA_EXT = int(os.environ.get("OCPI_TEST_DATA_EXT"))
STAGES = int(os.environ.get("OCPI_TEST_STAGES"))

# from -test.xml (properties that are declared to be test='true')
CONSTANT_VALUE = float(os.environ.get("OCPI_TEST_CONSTANT_VALUE"))
NUM_SAMPLES = int(os.environ.get("OCPI_TEST_NUM_SAMPLES"))

# from arguments to generate.py (-test.xml)
# NONE

dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))

# Read all output data as complex int16 samples
ofilename = open(sys.argv[1], 'rb')
odata = np.fromfile(ofilename, dtype=dt_iq_pair, count=-1)
ofilename.close()
# Read all input data as real int16 samples
ifilename = open(sys.argv[2], 'rb')
idata = np.fromfile(ifilename, dtype=np.int16, count=-1)
ifilename.close()

# Test #1 - Check that output data is not all zeros
if all(odata == 0):
    print '    ' + color.RED + color.BOLD + 'FAIL, values are all zero' + color.END
    sys.exit(1)
else:
    print '    PASS - File is not all zeros'

# Test #2 - Check that output data is the expected amount
if len(odata) != int(NUM_SAMPLES-STAGES-2):
    print '    ' + color.RED + color.BOLD + 'FAIL, output file length is unexpected' + color.END
    print '    ' + color.RED + color.BOLD + 'Length ofilename = ', len(odata), 'while expected length is = ' + color.END, len(idata)
    sys.exit(1)
else:
    print '    PASS - Input and output file lengths match'

# Test #3 - Check that output data values
if (enable == "true"): # => NORMAL MODE
    Fs = 1 #simply arbitrary
    real = odata['real_idx']
    imag = odata['imag_idx']
    # Construct complex array of vectors for performing FFT
    # Zero-pad "data" with the number of input samples - output samples
    # so that the expected tone will be at the correct frequency.
    data=np.zeros(NUM_SAMPLES-STAGES-2, dtype=complex)
    data.real[0:NUM_SAMPLES-STAGES-2] = real
    data.imag[0:NUM_SAMPLES-STAGES-2] = imag
    # CRITICAL NOTE:
    # The FFT performed on the output data requires a large amount 
    # of samples to provided the necessary resolution for verification.
    # Perform FFT
    w = np.fft.fft(data)
    freqs = np.fft.fftfreq(len(w), 1.0/Fs)
    #print "DBG: ", (freqs.min(), freqs.max())
    # Locate max Tone
    idx = np.argmax(np.abs(w))
    measured_freq = freqs[idx]
    
    expected_freq = CONSTANT_VALUE * Fs/float(2**DATA_WIDTH)
    # Max possible difference is based on the bit width of the CORDIC
    max_delta = 1/float(2**(DATA_WIDTH))
    calc_delta = abs(expected_freq - measured_freq)

    #print "DBG: ", DATA_WIDTH, DATA_EXT, STAGES, CONSTANT_VALUE, NUM_SAMPLES
    #print "DBG: expected_freq\t=", expected_freq
    #print "DBG: measured_freq\t=", measured_freq
    #print "DBG: max_delta\t\t=", max_delta
    #print "DBG: calc_delta\t\t=", calc_delta

    # Check that the difference between the calculated and expected frequencies is greater 
    # than the max possible difference (max_delta).
    if (calc_delta > max_delta):
        print '    FAIL - Expected:Max:Delta ', expected_freq, measured_freq, calc_delta, max_delta
        sys.exit(1)
    else:
        print '    PASS - Max freq is within the expected range'

else: # => BYPASS MODE
    if (idata[0:NUM_SAMPLES-STAGES-2] != odata['real_idx']).all():
        print '    FAIL: Input and output data files do not match'
        sys.exit(1)
    else:
        print '    PASS - Input and output data files match'

print '    Data matched expected results.'
print '    ' + color.GREEN + color.BOLD + 'PASSED' + color.END

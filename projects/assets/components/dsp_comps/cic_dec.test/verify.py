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
CIC Decimator: Verify output data

Verify args:
1. amplitude used to generate input data file
2. name of output data file
3. name of input data file

Tests:
#1: Not all zeros
#2: Is the expected amount
#3: Matches the expected output
"""
import sys
import os.path
import numpy as np
import datetime

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

def myround(x, base=8):
    return base * round(float(x) / base)

if len(sys.argv) != 4:
    print("Invalid arguments:  usage is: verify.py <amplitude> <output-file> <input-file>")
    sys.exit(1)
print "    VALIDATE (I/Q 16b binary data file):"

# from OCS or OWD
N=int(os.environ.get("OCPI_TEST_N"))
M=int(os.environ.get("OCPI_TEST_M"))
R=int(os.environ.get("OCPI_TEST_R"))

# from -test.xml (properties that are declared to be test='true')
Ft = int(os.environ.get("OCPI_TEST_TARGET_FREQ"))      # target frequency

print '    UUT:(N=%d, M=%d, R=%d) Test Data:(%d)' % (N,M,R,Ft)

# from arguments to generate.py (-test.xml)
AMPLITUDE = int(sys.argv[1])

# Sample frequency used in generate.py
Fs = float(1024000)
# Sample frequency after decimation.
Fs_dec = Fs / float (R)

# Declare complex data type
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))

# Read input and output data as complex int16 samples
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
ifile_nbytes = int(os.stat(sys.argv[3]).st_size)
#print 'DBG:  Input filesize/R:', sys.argv[3],"/",ifile_nbytes,"bytes","/",ifile_nbytes / 4,"samples"
ofile_nbytes = int(os.stat(sys.argv[2]).st_size)
#print 'DBG: Output filesize:\t', sys.argv[2],"/",ofile_nbytes,"bytes","/",ofile_nbytes / 4,"samples"
# Compare input file size (adjusted by decimation factor) and output file size
# Need special rounding to adjust for 'odd' valued decimation factors
if (myround((ifile_nbytes / R), 8) != ofile_nbytes):
    print '    FAIL - Output file is not the correct size'
    sys.exit(1)
else:
    print '    PASS - Output file is the correct size'
    
# Test #3 - Check that output data values: Unity Gain Response or Tone
if (Ft == 0): # Unity Gain Response (DC input results in DC output)
    # Calculate the expected output DC value, by scaling the input DC value 
    # based on the configuration of the worker under test
    AMPLITUDE = int(AMPLITUDE * ((R*M)**N) / 2**np.ceil((N*(np.log2(R*M))))) #overwrite
    #print 'DBG: Test for Unity Gain Response: expected amplitude =', AMPLITUDE
    # Skip startup data, then compare the steady-state output for expected DC value
    skip_index = (N+M)*2
    #print 'DBG: skip_index and I/Q values:', skip_index, real[skip_index], imag[skip_index]
    if all(real[skip_index::] != AMPLITUDE) and all(imag[skip_index::] != AMPLITUDE):
        print '    FAIL - I or Q values do not match the expected constant value of', AMPLITUDE
        sys.exit(1)
    else:
        #nsamples_matched = len(real[skip_index::] / 2)
        print '    PASS - I & Q values match the expected constant value of', AMPLITUDE
else: # Measure and compare power of tone(s)
    T1 = 50.0
    T2 = 100.0
    T3 = Fs / R # In the first null of the CIC filter
    start_time = datetime.datetime.now()
    print '    Start time =', start_time

    ifile_nsamples = ifile_nbytes / 4
    ofile_nsamples = ofile_nbytes / 4

    #print 'DBG: ifile_nbytes=%d ifile_nsamples=%d' % (ifile_nbytes, ifile_nsamples)
    #print 'DBG: ofile_nbytes=%d ofile_nsamples=%d' % (ofile_nbytes, ofile_nsamples)

    # Construct complex array of vectors for performing FFT
    complex_idata = np.array(np.zeros(ifile_nsamples/2), dtype=np.complex)
    for i in xrange(0,ifile_nsamples/2):
        complex_idata[i] = complex(idata['real_idx'][i], idata['imag_idx'][i])
    complex_odata = np.array(np.zeros(ofile_nsamples), dtype=np.complex)
    for i in xrange(0,ofile_nsamples):
        complex_odata[i] = complex(odata['real_idx'][i], odata['imag_idx'][i])
    IFFT = 1.0/(ifile_nsamples/2) * abs(np.fft.fft(complex_idata,ifile_nsamples/2))
    OFFT = 1.0/(ofile_nsamples) * abs(np.fft.fft(complex_odata,ofile_nsamples))
    eps = pow(10, -10) #Error factor to avoid divide by zero in log10
    #print 'DBG: len(IFFT)=%d \nDBG: len(OFFT)=%d' % (len(IFFT), len(OFFT))

    # Report time needed to perform FFTs 
    end_time = datetime.datetime.today()
    print '    End time =', end_time
    print '    Elapsed time:', end_time - start_time

    #print 'DBG: float(T1)/(float(Fs)/2.0)*float(len(IFFT)/2.0)=', float(T1)/(float(Fs)/2.0)*float(len(IFFT)/2.0)
    #print 'DBG: float(T2)/(float(Fs)/2.0)*float(len(IFFT)/2.0)=', float(T2)/(float(Fs)/2.0)*float(len(IFFT)/2.0)
    #print 'DBG: float(T3)/(float(Fs)/2.0)*float(len(IFFT)/2.0)=', float(T3)/(float(Fs)/2.0)*float(len(IFFT)/2.0)
    #input: three tones in range DC to +Fs/2
    IPowerT1 = 20*np.log10(IFFT[float(T1)/(float(Fs)/2.0)*float(len(IFFT)/2.0)]+eps)
    IPowerT2 = 20*np.log10(IFFT[float(T2)/(float(Fs)/2.0)*float(len(IFFT)/2.0)]+eps)
    IPowerT3 = 20*np.log10(IFFT[float(T3)/(float(Fs)/2.0)*float(len(IFFT)/2.0)]+eps)
    print "    Input Tone 1 power level =", IPowerT1, "dBm"
    print "    Input Tone 2 power level =", IPowerT2, "dBm"
    print "    Input Tone 3 power level =", IPowerT3, "dBm"

    #print 'DBG: float(T1)/(float(Fs_dec)/2.0)*float(len(OFFT)/2.0)=', float(T1)/(float(Fs_dec)/2.0)*float(len(OFFT)/2.0)
    #print 'DBG: float(T2)/(float(Fs_dec)/2.0)*float(len(OFFT)/2.0)=', float(T2)/(float(Fs_dec)/2.0)*float(len(OFFT)/2.0)
    #print 'DBG: float(T3)/(float(Fs_dec)/2.0)*float(len(OFFT)/2.0)=', float(T3)/(float(Fs_dec)/2.0)*float(len(OFFT)/2.0)
    #output: two tones in range DC to +Fs/2
    OPowerT1 = 20*np.log10(OFFT[float(T1)/(float(Fs_dec)/2.0)*float(len(OFFT)/2.0)]+eps)
    OPowerT2 = 20*np.log10(OFFT[float(T2)/(float(Fs_dec)/2.0)*float(len(OFFT)/2.0)]+eps)
    #output: one filtered tone in range -Fs/2 to DC
    OPowerT3 = 20*np.log10(OFFT[float(-T3)/(float(Fs_dec)/2.0)*float(len(OFFT)/2.0)]+eps)
    print "    Output Tone 1 power level =", OPowerT1, "dBm"
    print "    Output Tone 2 power level =", OPowerT2, "dBm"
    print "    Output Tone 3 power level =", OPowerT3, "dBm"

    # Perform calculations comparing output power to input power
    # Note: Comparison values were determined empirically across entire test suite
    diffT1 = abs(OPowerT1 - IPowerT1)
    if (M == 2 and R == 8192):
        if (diffT1 < 37.0) : 
            print "    FAIL, Output Tone 1 power level = ", OPowerT1, "dBm", diffT1
            sys.exit(1)
    elif (M == 2 and R == 8191):
        if (diffT1 < 46.0) : 
            print "    FAIL, Output Tone 1 power level = ", OPowerT1, "dBm", diffT1
            sys.exit(1)
    elif (R == 8191):
        if (diffT1 < 15.0): 
            print "    FAIL, Output Tone 1 power level = ", OPowerT1, "dBm", diffT1
            sys.exit(1)
    elif diffT1 > 8.0 : 
        print "    FAIL, Output Tone 1 power level = ", OPowerT1, "dBm", diffT1
        sys.exit(1)

    diffT2 = abs(OPowerT2 - IPowerT2)
    if (R >= 8191):
        if (diffT2 < 37.0) : 
            print "    FAIL, Output Tone 1 power level =", OPowerT2, "dBm", diffT2
            sys.exit(1)
    elif diffT2 > 8.0 : 
        print "    FAIL, Output Tone 2 power level =", OPowerT2, "dBm", diffT2
        sys.exit(1)

    diffT3 = abs(OPowerT3 - IPowerT3)
    if diffT3 < 57.0 :
        print "    FAIL, Output Tone 3 power level =", OPowerT3, "dBm", diffT3
        sys.exit(1)

print '    Data matched expected results.'
print '    ' + color.GREEN + color.BOLD + 'PASSED' + color.END

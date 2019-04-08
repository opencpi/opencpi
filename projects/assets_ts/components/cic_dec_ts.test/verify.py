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

def myround(x, base=8):
    return base * round(float(x) / base)

def getmsg(m):
    length = m[0]
    opcode = m[1]
    data   = None
    if length > 0:
        data = m[2:length/4+2]
    return (opcode, length, data)

if len(sys.argv) != 4:
    print("Invalid arguments:  usage is: verify.py <amplitude> <output-file> <input-file>")
    sys.exit(1)

SAMPLES_OPCODE  = 0
TIME_OPCODE     = 1
INTERVAL_OPCODE = 2
FLUSH_OPCODE    = 3
SYNC_OPCODE     = 4
DONE_OPCODE     = 5

# from OCS or OWD
N=int(os.environ.get("OCPI_TEST_N"))
M=int(os.environ.get("OCPI_TEST_M"))
R=int(os.environ.get("OCPI_TEST_R"))

# from -test.xml (properties that are declared to be test='true')
Ft = int(os.environ.get("OCPI_TEST_TARGET_FREQ"))      # target frequency

# from arguments to generate.py (-test.xml)
AMPLITUDE = int(sys.argv[1])

# Sample frequency used in generate.py
Fs = float(1024000)
# Sample frequency after decimation.
Fs_dec = Fs / float (R)

# Declare complex data type
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))

#Read input and output data files as uint32
ifile = open(sys.argv[3], 'rb')
din = np.fromfile(ifile, dtype=np.uint32, count=-1)
ifile.close()
ofile = open(sys.argv[2], 'rb')
dout = np.fromfile(ofile, dtype=np.uint32, count=-1)
ofile.close()

#Parse messages within input data
index = 0
msg_count = 0
msg = []
samples = []
din_non_sample_msg = []
while index < len(din):
    msg.append(getmsg(din[index:]))
    if msg[msg_count][2] is None:
        index = index + 2
    else:
        index = index + len(msg[msg_count][2]) + 2
    msg_count += 1
for i in range(0,len(msg)):
    if(msg[i][0]==0):
        samples.extend(msg[i][2])
    else:
        din_non_sample_msg.append(msg[i])
idata = np.array(samples, dtype=dt_iq_pair)

#Parse messages within output data
index = 0
msg_count = 0
msg = []
samples = []
dout_non_sample_msg = []
sync_count=0
#MessagesInFile structure
message_opcode = 0
message_length = 1
message_data   = 2
while index < len(dout):
    msg.append(getmsg(dout[index:]))
    if msg[msg_count][message_data] is None:
        index = index + 2
    else:
        index = index + len(msg[msg_count][2]) + 2
    msg_count += 1
for i in range(0,len(msg)):
    if(msg[i][0]==SAMPLES_OPCODE):
        samples.extend(msg[i][message_data])
    else:
        dout_non_sample_msg.append(msg[i])
        if(msg[i][message_opcode]==SYNC_OPCODE and msg[i-1][message_opcode]==SAMPLES_OPCODE):
            sync_count += 1

odata = np.array(samples, dtype=dt_iq_pair)

# Deinterleave numpy.array into I & Q arrays
real = odata['real_idx']
imag = odata['imag_idx']

# Test #1 - Check that output data is not all zeros
if all(odata == 0):
    print '    FAIL, values are all zero'
    sys.exit(1)
else:
    print '    PASS - File is not all zeros'

# Test #2 - Check that samples output data is the expected amount
bytes_per_sample=4 #2 byte I, 2 byte Q
bytes_dropped = sync_count*N*bytes_per_sample #sync opcodes after samples opcodes result in N samples dropped 
samples_dropped = bytes_dropped / bytes_per_sample
ifile_nbytes = len(idata)*4 
#print 'DBG:  Input filesize/R:', sys.argv[3],"/",ifile_nbytes,"bytes","/",ifile_nbytes / 4,"samples"
ofile_nbytes = len(odata)*4
#print 'DBG: Output filesize:\t', sys.argv[2],"/",ofile_nbytes,"bytes","/",ofile_nbytes / 4,"samples"
# Compare input file size (adjusted by decimation factor) and output file size
# Need special rounding to adjust for 'odd' valued decimation factors
#print bytes_dropped
#print myround((ifile_nbytes / R), 8)
#print ofile_nbytes
if (myround((ifile_nbytes / R), 8) - bytes_dropped != ofile_nbytes):
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
    OFFT = 1.0/(ofile_nsamples+samples_dropped) * abs(np.fft.fft(complex_odata,ofile_nsamples+samples_dropped))
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

# Test #4 - Parse non-sample data
if len(dout_non_sample_msg) != len(din_non_sample_msg):
    print '    FAIL - Input and output files have different number of non-samples messages'
    sys.exit(1)

for i in range(len(dout_non_sample_msg)):
    if(dout_non_sample_msg[i][0]==INTERVAL_OPCODE):
        #Convert to 64 bit number
        input_interval  = (din_non_sample_msg[i][2][0] << 32) + din_non_sample_msg[i][2][1]
        output_interval = (dout_non_sample_msg[i][2][0] << 32) + dout_non_sample_msg[i][2][1]
        if output_interval != input_interval // R:
            print '    FAIL - Input and output interval messages did not match as expected'
            print "    Output: ", dout_non_sample_msg[i][2], " Input/R: ", din_non_sample_msg[i][2]//R
            sys.exit(1)
    else:
        if dout_non_sample_msg[i][message_opcode] != din_non_sample_msg[i][message_opcode] or \
           dout_non_sample_msg[i][message_length] != din_non_sample_msg[i][message_length] or \
            not np.array_equal(dout_non_sample_msg[i][message_data],din_non_sample_msg[i][message_data]):
            print dout_non_sample_msg
            print din_non_sample_msg
            print '    FAIL - Input and output non-samples messages did not match as expected'
            sys.exit(1)

print '    Data matched expected results.'
print '    PASSED'

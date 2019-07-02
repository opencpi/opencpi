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
Generate test data & Validate output

Generate args:
1. -g = generate test file
2. target file
3. center frequency of generated sinusoid in Hz
4. sample frequency of generated sinusoid in Hz
5. amplitude of generated sinusoid
6. number of sinusoid cycles generated in target file

Validate args:
1. -v = validate output file
2. input data file used for comparison
3. output data file used for validation
4. number of samples used in validation
5. sample frequency of output data file
6. mixer enabled or bypassed

To test the worker, a binary data file is generated containing complex
signed 16-bit samples with a tone at a configurable center frequency and sample
frequency. The data should pass through the worker unchanged.

"""
# import struct
# import shutil
import sys
import os
import os.path
# import random
import opencpi.colors as color
import numpy as np


def generate(argv):
    """
    Ex: python test_sinewave.py -g {odata_filename} {TARGET_FREQ} {SAMPLE_FREQ} {AMPLITUDE} {NUM_CYCLES}
    Generate test data:
    Tone at TARGET_FREQ, sampled at SAMPLE_FREQ
    """
    print("GENERATE (I/Q 16b binary data file):")
    if len(argv) < 3:
        print("Exit: Enter output filename")
        return
    elif len(argv) < 4:
        print("Exit: Enter target frequency (int:1)")
        return
    elif len(argv) < 5:
        print("Exit: Enter sample frequency (typ:16)")
        return
    elif len(argv) < 6:
        print("Exit: Enter amplitude (max 16bit signed = 32767)")
        return
    elif len(argv) < 7:
        print("Exit: Enter number of repetitions (min:1)")
        return

    # Local Variables
    print("Argument 1:",argv[1])
    filename = argv[2]

    # Create cosine & sine waveforms
    Fs = float(argv[4])     # sample frequency
    Ts = 1.0/float(Fs);     # sampling interval
    t = np.arange(0,1,Ts)   # time vector
    Ft = int(argv[3])       # target frequency
    AMPLITUDE = int(argv[5])
    NUM_CYCLES = int(argv[6])

    # Generate tone
    print("Test data: Tone @ frequency = ", Ft)
    c = np.cos(2*np.pi*Ft*t)
    s = np.sin(2*np.pi*Ft*t)

    # Initialize empty array, sized to store interleaved I/Q 16bit samples
    z = np.array(np.zeros(len(c)), dtype=dt_iq_pair)

    # Scale data (1 cycle) by AMPLITUDE and interleave into a single array
    z['real_idx'] = np.int16(AMPLITUDE*c)
    z['imag_idx'] = np.int16(AMPLITUDE*s)

    # Write desired number of cycles to file
    f = open(filename, 'wb')
    for i in range(0,int(NUM_CYCLES)):
        for x in range(len(z)):
            f.write(z[x])
    f.close()

    # Summary
    print('Filename:', filename)
    print('# of Bytes:', len(z)*NUM_CYCLES*4)
    print('# of I/Q (16b) samples:', (len(z)*NUM_CYCLES))
    print('Sample Frequency:', Fs)
    print('Target Frequency:', Ft)
    print('Amplitude:', AMPLITUDE)
    print('Number of Cycles:', NUM_CYCLES)
    print('*** END Generate ***')
    print("*"*80+"\n")

def validate(argv):
    """
    Ex: python test_sinewave.py -v {idata_filename} {odata_filename} {number_of_samples} {sample_rate} {enable}
    Validate:
    TEST #1: Verify I & Q values are not all zeros
    TEST #2: Output File Size
    TEST #3: Peak at DC or Input = Output file
    """
    print("VALIDATE:")
    if len(argv) < 3:
        print("Exit: Enter input filename")
        return
    if len(argv) < 4:
        print("Exit: Enter output filename")
        return
    if len(argv) < 5:
        print("Exit: Enter number of samples for FFT")
        return
    if len(argv) < 6:
        print("Exit: Enter sample rate of data")
        return
    if len(argv) < 7:
        print("Exit: Enter true for enable or false for disabled")
        return

    num_samples = int(argv[4])
    sample_rate = float(argv[5])
    enable = int(argv[6])

    #Open input file and grab samples as complex int16
    input_file = open(argv[2], 'rb')
    input_file_samples = np.fromfile(input_file, dtype=dt_iq_pair, count=-1)
    input_file.close()

    #Open output file and grab samples as complex int16
    print('File to validate ', argv[3])
    output_file = open(argv[3], 'rb')
    output_file_samples = np.fromfile(output_file, dtype=dt_iq_pair, count=-1)
    output_file.close()
    #Ensure dout is not all zeros
    if all(output_file_samples == 0):
        print(color.RED + color.BOLD + 'FAILED, values are all zero' + color.END)
        return
    else:
        print('Pass: File is not all zeros')
    #Ensure that dout is the expected amount of data
    if len(output_file_samples) != len(input_file_samples):
        print(color.RED + color.BOLD + 'FAILED, input file length is unexpected' + color.END)
        print(color.RED + color.BOLD + 'Length dout = ', len(output_file_samples), 'while expected length is = ' + color.END, len(input_file_samples))
        return
    else:
        print('Pass: Input and output file lengths match')
        if(enable == 1):
            complex_data = np.array(np.zeros(num_samples), dtype=np.complex)
            for i in range(0,num_samples-1,2):
                complex_data[i] = complex(output_file_samples['real_idx'][i], output_file_samples['imag_idx'][i])
            FFT = abs(np.fft.fft(complex_data,num_samples))
            Max_FFT_freq=np.argmax(FFT)*sample_rate/num_samples
            if Max_FFT_freq != 2.0:
                print('Fail: Max of FFT occurs at index: ',Max_FFT_freq , ' Hz (Should be 0)')
                return
            else:
                print('Pass: Max of FFT occurs at index: ',Max_FFT_freq , ' Hz')
        else:
            if (input_file_samples != output_file_samples).all():
                print('Fail: Input and output file do not match')
                return
            else:
                print('Pass: Input and output file match')

    print('Data matched expected results.')
    print(color.GREEN + color.BOLD + 'PASSED' + color.END)
    print('*** End validation ***\n')

def main():
    print("\n","*"*80)
    print("*** Python: Backpressure IQ protocol, ZLM Support ***")
    if len(sys.argv) < 2:
        print("Exit: Enter option (-g/-v):")
        return

    #I/Q pair in a 32-bit vector (31:0) is Q(0) Q(1) I(0) I(1) in bytes 0123 little-Endian
    #Thus Q is indexed at byte 0 and I is indexed at byte 2
    global dt_iq_pair
    dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,2), 'imag_idx':(np.int16,0)}))

    if sys.argv[1] == '-g':
        generate(sys.argv)
    elif sys.argv[1] == '-v':
        validate(sys.argv)
    else:
        print("Exit: Enter option (-g/-v):")
        return

if __name__ == '__main__':
    main()

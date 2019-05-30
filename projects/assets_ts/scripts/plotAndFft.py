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


import numpy as np
try:
    import matplotlib.pyplot as plt
except (ImportError, RuntimeError):
    print("******* Error: python matplotlib is unavailable or not installed")
    sys.exit(1)
import sys
import os.path
import itertools

def getmsg(m):
    length = m[0]
    opcode = m[1]
    data   = None
    if length > 0:
        data = m[2:length/4+2]
    return (opcode, length, data)

def main():
    dataType = "real"
    numSamples = -1
    messagesInFile = "false"
    if len(sys.argv) < 2:
        print("Exit: Enter input filename")
        sys.exit(1)
    elif len(sys.argv) < 3:
        print("Exit: Enter data type: real or complex")
        sys.exit(1)
    elif len(sys.argv) < 4:
        print("Exit: Enter number of samples")
        sys.exit(1)
    elif len(sys.argv) < 5:
        print("Exit: Enter sample rate of input")
        sys.exit(1)

    fig = plt.figure(1)
    f = open(sys.argv[1], 'r')
    dataType = sys.argv[2].lower()
    numSamples = int(sys.argv[3])
    sampleRate = float(sys.argv[4])
    if len(sys.argv) > 5:
        if sys.argv[5].lower() == "true":
            messagesInFile = "true"
    print "file is : " + f.name
    print "data is : " + dataType
    print "num samples is: " + str(numSamples)
    print "sample rate is: " + str(sampleRate)
    print "messages in file is: " + messagesInFile

    if dataType == "complex":
        print "Input is complex data"
        #I/Q pair in a 32-bit vector (31:0) is Q(0) Q(1) I(0) I(1) in bytes 0123 little-Endian
        #Thus Q is indexed at byte 0 and I is indexed at byte 2
        dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,2), 'imag_idx':(np.int16,0)}))

        #Optionally parse non-samples messages out of the data before plotting
        if messagesInFile == "false":
            data = np.fromfile(f, dtype=dt_iq_pair)
        else:
            din = np.fromfile(f, dtype=np.uint32, count=-1)
            index = 0
            msg_count = 0
            msg = []
            samples = []
            non_sample_msg = []
            while index < len(din):
                msg.append(getmsg(din[index:]))
                if msg[msg_count][2] is None:
                    index = index + 2
                else:
                    index = index + len(msg[msg_count][2]) + 2
                msg_count += 1
            for i in xrange(0,len(msg)):
                if(msg[i][0]==0):
                    samples.extend(msg[i][2])
                else:
                    non_sample_msg.append(msg[i])
            data = np.array(samples, dtype=dt_iq_pair)

        #Pull out I and Q and make lists for each
        iList = data['real_idx']
        qList = data['imag_idx']
        iqList = list();
        iqList_mag = list();
        for a,data in enumerate(iList):
            iqList.append(complex(int(iList[int(a)]),int(qList[int(a)])))

        #Create time domain plot
        fig = plt.figure(1)
        ax1 = fig.add_subplot(2, 1, 1)
        ax2 = fig.add_subplot(2, 1, 2)
        ax1.plot(range(len(qList[0:numSamples])),iList[0:numSamples], c='r', label='I')
        ax2.plot(range(len(qList[0:numSamples])),qList[0:numSamples], c='r', label='Q')

        #Beautify plot
        ax1.set_title('Time Domain Plot\n' + os.path.basename(f.name))
        ax1.set_xlabel('Sample Index')
        ax2.set_xlabel('Sample Index')
        ax1.set_ylabel('Amplitude')
        ax2.set_ylabel('Amplitude')
        leg1 = ax1.legend()
        leg2 = ax2.legend()

        # sample spacing
        T = 1.0 / sampleRate
        x = np.linspace(0.0, numSamples*T, numSamples)

        #Generate FFT bins
        xf = np.fft.fftfreq(numSamples, T)
        xf = np.fft.fftshift(xf)

        #Perform FFT
        yf = np.fft.fft(iqList[0:numSamples])
        yplot = np.fft.fftshift(yf)
        yf_plot=1.0/numSamples * np.abs(yplot)

        #Create FFT plot
        fft_fig = plt.figure(2)
        ax3 = fft_fig.add_subplot(1, 1, 1)
        epsilon = pow(10, -10) #Error factor to avoid divide by zero in log10
        ax3.plot(xf,20*np.log10(yf_plot+epsilon))

        #Beautify plot
        ax3.set_title(str(numSamples) + '-Point Complex FFT\n' + os.path.basename(f.name))
        ax3.set_xlabel('Frequency (Hz)')
        ax3.set_ylabel('Power (dBm)')
        #ax3.set_ylim([-50, 150])

        #Show plot
        plt.grid()
        plt.show()

        print("End!")

    else:
        print "Input is real data"

        #Optionally parse non-samples messages out of the data before plotting
        if messagesInFile == "false":
            data = np.fromfile(f, dtype=np.int16)
        else:
            din = np.fromfile(f, dtype=np.uint32, count=-1)
            index = 0
            msg_count = 0
            msg = []
            samples = []
            non_sample_msg = []
            while index < len(din):
                msg.append(getmsg(din[index:]))
                if msg[msg_count][2] is None:
                    index = index + 2
                else:
                    index = index + len(msg[msg_count][2]) + 2
                msg_count += 1
            for i in xrange(0,len(msg)):
                if(msg[i][0]==0):
                    samples.extend(msg[i][2])
                else:
                    non_sample_msg.append(msg[i])
            data = np.array(samples).view(dtype=np.int16)

        #Create time domain plot
        fig = plt.figure(1)
        ax1 = fig.add_subplot(1, 1, 1)
        ax1.plot(data[0:numSamples], c='r', label='rData')

        #Beautify plot
        ax1.set_title('Time Domain Plot\n' + os.path.basename(f.name))
        ax1.set_xlabel('Sample Index')
        ax1.set_ylabel('Amplitude')
        leg1 = ax1.legend()

        # sample spacing
        T = 1.0 / sampleRate
        x = np.linspace(0.0, numSamples*T, numSamples)

        #Generate FFT bins
        xf = np.fft.fftfreq(numSamples, T)
        xf = np.fft.fftshift(xf)

        #Perform FFT
        yf = np.fft.fft(data[0:numSamples])
        yplot = np.fft.fftshift(yf)
        yf_plot=1.0/numSamples * np.abs(yplot)

        #Create FFT plot
        fft_fig = plt.figure(2)
        ax2 = fft_fig.add_subplot(1, 1, 1)
        epsilon = pow(10, -10) #Error factor to avoid divide by zero in log10
        ax2.plot(xf[xf.size/2:xf.size],20*np.log10(yf_plot[yf.size/2:yf.size]+epsilon))

        #Beautify plot
        ax2.set_title(str(numSamples) + '-Point Real FFT\n' + os.path.basename(f.name))
        ax2.set_xlabel('Frequency (Hz)')
        ax2.set_ylabel('Power (dBm)')
        #ax2.set_ylim([-50, 150])

        #Show plot
        plt.grid()
        plt.show()

        print("End!")

if __name__ == '__main__':
    main()

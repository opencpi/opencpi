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


import numpy as np
import matplotlib.pyplot as plt
import sys
import os.path
import itertools

def main():
    dataType = "real"
    numSamples=-1
    if len(sys.argv)<2:
        print("Exit: Enter input filename")
        return
    elif len(sys.argv)<3:
        print("Exit: Enter data type: real or complex")
        return 
    elif len(sys.argv)<4:
        print("Exit: Enter number of samples")
        return
    elif len(sys.argv)<5:
        print("Exit: Enter sample rate of input")
        return
    elif len(sys.argv)<6:
        print("Exit: Enter messageSize property of timestamper, 0 for no timestamps")
        return
    elif len(sys.argv)>7 :
        print("Exit: Wrong number of arguments")
        return

    fig=plt.figure(1)
    f=open(sys.argv[1], 'r')
    data=np.fromfile(f,dtype=np.int16)
    f.close()
    dataType=sys.argv[2].lower()
    numSamples=int(sys.argv[3])
    if numSamples>len(data):
        print "Error: Number of samples requested is less than file length"
        return
    sampleRate=int(sys.argv[4])
    messageSize=int(sys.argv[5])
    message_size_in_words=messageSize/4
    print "file is : "+f.name
    print "data is : "+dataType
    print "num samples is: "+str(numSamples)
    print "sample rate is: "+str(sampleRate)

    if dataType=="complex":
        print "Input is complex data"
        #Pull out I and Q and make lists for each
        iqList=np.array([],dtype=complex);
        timestampList=list();
        iqList_mag=list();
        a=0;
        while a<numSamples:
            #print "a =: ",a
            if(message_size_in_words>0):
                if(a%(message_size_in_words*2+4)==0):
                    timestampList.append(((data[a+1].astype(np.uint16)<<16)+data[a].astype(np.uint16))+(1.0*((data[a+3].astype(np.uint16)<<16)+data[a+2].astype(np.uint16))/0xFFFFFFFF))
                    print "Timestamp at index:","{0:09}".format(a),":","{:10.7f}".format(timestampList[-1]),"Seconds:","{0:#x}".format(((data[a+1].astype(np.uint16)<<16)+data[a].astype(np.uint16)).astype(np.uint32)),"Fraction:","{0:#x}".format(((data[a+3].astype(np.uint16)<<16)+data[a+2].astype(np.uint16)).astype(np.uint32)),("Delta:{:10.7f}".format(timestampList[-1]-timestampList[-2]),"Expected:,{:10.7f}".format(1.0/sampleRate*message_size_in_words)) if (len(timestampList)>1) else " "
                    a+=4
                else:
                    iqList=np.append(iqList,complex(data[a],data[a+1]))
                    a+=2
            else:
                iqList=np.append(iqList,complex(data[a],data[a+1]))
                a+=2

        #Create time domain plot
        fig=plt.figure(1)
        ax1=fig.add_subplot(2,1,1)
        ax2=fig.add_subplot(2,1,2)
        ax1.plot(range(len(iqList)),iqList.real,c='r',label='Q')
        ax2.plot(range(len(iqList)),iqList.imag,c='r',label='I')

        #Beautify plot
        ax1.set_title('Time Domain Plot')
        ax1.set_xlabel('Sample Index')
        ax2.set_xlabel('Sample Index')
        ax1.set_ylabel('Amplitude')
        ax2.set_ylabel('Amplitude')
        leg1=ax1.legend()
        leg2=ax2.legend()

        # sample spacing
        T=1.0/sampleRate

        #Generate FFT bins
        xf=np.fft.fftfreq(iqList.size,T)
        xf=np.fft.fftshift(xf)

        #Perform FFT
        yf=np.fft.fft(iqList)
        yplot=np.fft.fftshift(yf)
        yf_plot=1.0/iqList.size*np.abs(yplot)

        #Create FFT plot
        fft_fig=plt.figure(2)
        ax3=fft_fig.add_subplot(1,1,1)
        epsilon= pow(10,-10) #Error factor to avoid divide by zero in log10
        ax3.plot(xf,20*np.log10(yf_plot+epsilon))

        #Beautify plot
        ax3.set_title(str(numSamples)+'-Point Complex FFT')
        ax3.set_xlabel('Frequency (Hz)')
        ax3.set_ylabel('Power (dBm)')
        ax3.set_ylim([-50,150])

        #Show plot
        plt.grid()
        plt.show()

        print("End!")
    else:
        print "Non-complex data not supported"

if __name__ == '__main__':
    main()

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
PR CORDIC: Validate output data for Polar-to-Rectangular CORDIC

Validation args:
- output data filename
- input data file

To test the PR CORDIC, a binary data file is generated containing a constant-phase
ramp input in the upper 16 bits, and a constant magnitude value in the lower 16 bits.
The output of the circuit is a complex sinusoid (tone given a constant-phase input)
with maximum amplitude as defined by the magnitude input.

"""
import numpy as np
import sys
import os.path

try:
  import matplotlib.pyplot as plt
  PlotLib = True
except (ImportError, RuntimeError):
  PlotLib = False

def dispGetPeakErr(val,n):

    maximum=0

    for k in range(0,n):
        if abs(val[k])>maximum:
            maximum=abs(val[k])

    return(maximum)

# Get difference between actual and expected
def getDiff(diff,a_val,e_val,n):

    for k in range(0,n):
         diff[k]=a_val[k]-e_val[k]

    return(diff)

def AvgPeakError(x,y,n):

    sumDiffsX=0
    sumDiffsY=0
    for k in range(0,n):
        sumDiffsX+=abs(x[k])
        sumDiffsY+=abs(y[k])
    x_err=sumDiffsX/n
    y_err=sumDiffsY/n
    print "  AvgX Peak Error = %.3f" % (x_err)
    print "  AvgY Peak Error = %.3f" % (y_err)

    if (x_err and y_err) <= 1:
        return 1
    else:
        return -1

def CountErrors(diffs,n):

    error5bit=0
    error4bit=0
    error3bit=0
    error2bit=0
    error1bit=0
    error0bit=0

    for k in range(0,n):
        if  -1 <= diffs[k] <= 1:
            error1bit+=1;
        elif -2 <= diffs[k] <= 2:
            error2bit+=1;
        elif -4 <= diffs[k] <= 4:
            error3bit+=1;
        elif -8 <= diffs[k] <= 8:
            error4bit+=1;
        else:
            error5bit+=1;

    print "  error +-1= %d\t %% Err = %.3f" % (error1bit, (error1bit/n)*100)
    print "  error +-2= %d\t %% Err = %.3f" % (error2bit, (error2bit/n)*100)
    print "  error +-4 = %d\t %% Err = %.3f" % (error3bit, (error3bit/n)*100)
    print "  error +-8 = %d\t %% Err = %.3f" % (error4bit, (error4bit/n)*100)
    print "  error +-16 = %d\t %% Err = %.3f" % (error5bit, (error5bit/n)*100)
    totalError = (error1bit/n) + (error2bit/n) \
    + (error3bit/n) + (error4bit/n) + (error5bit/n)
    print "==== Sanity check, total %% Err = %.3f" % (totalError*100)

def getDATA(FILE):

    ifd=open(FILE,"r")
    idata=np.fromfile(ifd,dtype=np.int16)
    ifd.close()

    upper16=idata[1::2]
    lower16=idata[0::2]

    return(upper16,lower16)

def main():

    print 'Number of arguments:', len(sys.argv), 'arguments.'
    print 'Argument List:', str(sys.argv)

    if len(sys.argv) < 4:
        print("Usage expected: OCPI_TEST_NUM_SAMPLES=129023 OCPI_TEST_DATA_WIDTH=16 OCPI_TEST_STAGES=18 ./analyze_pr_cordic.py plot, output file, input file")
        sys.exit(1)

    PLOT = int(sys.argv[1])
    OFILE = sys.argv[2]
    IFILE = sys.argv[3]
    DATA_WIDTH = int(os.environ.get("OCPI_TEST_DATA_WIDTH"))
    NUM_OUTPUT_SAMPLES = int(os.environ.get("OCPI_TEST_NUM_SAMPLES")) - int(os.environ.get("OCPI_TEST_STAGES")) - 1

    iUpper16, iLower16 = getDATA(IFILE)
    oUpper16, oLower16 = getDATA(OFILE)

    x_expected = [None]*NUM_OUTPUT_SAMPLES
    y_expected = [None]*NUM_OUTPUT_SAMPLES

    for k in range(0,NUM_OUTPUT_SAMPLES):
        x_expected[k] = round(iUpper16[k] * np.cos ((np.pi*iLower16[k])/(2**(DATA_WIDTH-1))))
        y_expected[k] = round(iUpper16[k] * np.sin ((np.pi*iLower16[k])/(2**(DATA_WIDTH-1))))

    # Buffers for holding the difference
    diffsY=[None]*NUM_OUTPUT_SAMPLES
    diffsX=[None]*NUM_OUTPUT_SAMPLES

    diffsX= getDiff(diffsX,oUpper16,x_expected,NUM_OUTPUT_SAMPLES)
    diffsY= getDiff(diffsY,oLower16,y_expected,NUM_OUTPUT_SAMPLES)

    check_AvgPeakError=0
    check_AvgPeakError=AvgPeakError(diffsX,diffsY,NUM_OUTPUT_SAMPLES)

    x_peak_err=dispGetPeakErr(diffsX,NUM_OUTPUT_SAMPLES)
    y_peak_err=dispGetPeakErr(diffsY,NUM_OUTPUT_SAMPLES)
    print "Peak Errors"
    print "  X Peak Error = %d" % (x_peak_err)
    print "  Y Peak Error = %d" % (y_peak_err)

    print "X Errors"
    CountErrors(diffsX,NUM_OUTPUT_SAMPLES)
    print "Y Errors"
    CountErrors(diffsY,NUM_OUTPUT_SAMPLES)

    # only pass if both of these test cases pass
    pass_fail=0
    if (check_AvgPeakError == 1) and (x_peak_err <= 1) and (y_peak_err <= 1):
        pass_fail=0
    else:
        pass_fail=-1

    #Plot data
    if PlotLib and PLOT == 1:

        plt.figure(1)
        plt.plot(iUpper16)
        plt.title("Actual Input Data -- Upper 16-bits")
        plt.grid()

        plt.figure(2)
        plt.plot(iLower16)
        plt.title("Actual Input Data -- Lower 16-bits")
        plt.grid()

        plt.figure(3)
        plt.plot(oUpper16)
        plt.title("Actual Output Data -- Upper 16-bits")
        plt.grid()

        plt.figure(4)
        plt.plot(oLower16)
        plt.title("Actual Output Data -- Lower 16-bits")
        plt.grid()

        plt.figure(5)
        plt.plot(x_expected)
        plt.title("Expected output Data -- Upper 16-bits")
        plt.grid()

        plt.figure(6)
        plt.plot(y_expected)
        plt.title("Expected output Data -- Lower 16-bits")
        plt.grid()

        plt.figure(7)
        plt.plot(diffsY)
        plt.title("Difference of Y -- Lower 16-bits")
        plt.grid()

        plt.figure(8)
        plt.plot(diffsX)
        plt.title("Difference of X -- Upper 16-bits")
        plt.grid()

        plt.figure(9)
        plt.plot(oLower16, color='r')
        plt.plot(y_expected, color='b')
        plt.title("Y: Red=Actual, Blue=Expected Data -- Lower 16-bits")
        plt.grid()

        plt.figure(10)
        plt.plot(oUpper16, color='r')
        plt.plot(x_expected, color='b')
        plt.title("X: Red=Actual, Blue=Expected Data -- Lower 16-bits")
        plt.grid()

        plt.show()

    sys.exit(pass_fail)

if __name__ == '__main__':
    main()

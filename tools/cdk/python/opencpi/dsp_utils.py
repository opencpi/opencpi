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
CIC Utils

Helper functions and variables common to all CIC implementations
"""
import os.path
import math
import numpy as np

def cic_dec_model(idata, N, M, R, DATA_WIDTH):
    """
    Python CIC Decimator model
    Expect idata in the format numpy.int16
    Variable names track cic_dec_gen.vhd
    """
    ACC_WIDTH = int(math.ceil(N*math.log2(R*M)))+DATA_WIDTH
    integ     = np.zeros(N+1,dtype=np.int64)
    out_integ = np.zeros(len(idata),dtype=np.int64) #Store sequence of integrator output

    # Integrator
    for a in range(len(idata)):
        integ[0] = idata[a]
        for b in range(1,N+1):
            integ[b] = integ[b] + integ[b-1]
            if integ[b] > pow(2,ACC_WIDTH): #Overflow
                integ[b] = integ[b]-pow(2,ACC_WIDTH)
            #Underflow??
        out_integ[a] = integ[N]
    #print(out_integ)

    # Decimator
    # Starting point of decimation is rate minus prop delay of integrator stage
    out_dec = out_integ[R-N::R]
    #print(out_dec)

    # Comb
    comb       = np.zeros(N+1,dtype=np.int64)
    comb_dly   = np.zeros((N+1,M+1),dtype=np.int64)
    comb_r     = np.zeros(N+1,dtype=np.int64)         #copy of comb
    comb_dly_r = np.zeros((N+1,M+1),dtype=np.int64)   #copy of comb_r
    out_comb   = np.zeros(len(out_dec),dtype=np.int64)

    for a in range(len(out_dec)):
        comb_r      = comb.copy()
        comb_dly_r  = comb_dly.copy()
        comb[0] = out_dec[a]
        for b in range(1,N+1):
            for c in range(1,M+1):
                comb_dly[b][c] = comb_dly_r[b][c-1]
            comb_dly[b][0] = comb_r[b-1]
            comb[b] = comb_r[b-1] - comb_dly[b][M]
        out_comb[a] = comb[N]
        #print(comb)
    #print(out_comb)

    odata = np.int16(out_comb >> (ACC_WIDTH - DATA_WIDTH))

    return odata

"""
This doesn't seem to work...

CIC Decimator: Verify output data

2 Test Cases implemented: Impulse and constant value

Methodology derived from Two Easy Ways To Test Multistage CIC Decimation Filters
https://www.dsprelated.com/showarticle/1171.php
"""
"""
#Verify test cases
#I and Q have been verified to be equal, so verification will only be performed on I
real=real[N::] # We're outputting startup data, so skip first N samples
print(real)
idx=1          # index to check
if TEST_CASE == 0: # Impulse
    print("    Checking Impulse Response")
    if np.count_nonzero(real) != N:
        print("Incorrect number of non-zero values. Should be: ", N, " but is: ",np.count_nonzero(real))
        sys.exit(1)
    yout1 = np.int16(math.factorial(R+N-1)/(math.factorial(R)*math.factorial(N-1))-N)
    if yout1 != real[idx]:
        print("Incorrect output value at position ",idx,". Should be: ", yout1, " but is: ",real[idx])
        sys.exit(1)
elif TEST_CASE == 1: # Step
    print("    Checking Step Response")
    l = [x - real[N] for x in real] #Subtract saturation value to determine how many values lead up to it
    if np.count_nonzero(l) != N:
        print("Incorrect number of values leading up to saturation. Should be: ", N, " but is: ",np.count_nonzero(l))
        sys.exit(1)
    yout1 = np.int16(math.factorial(R+N)/(math.factorial(R)*math.factorial(N))-N)
    if yout1 != real[idx]:
        print("Incorrect output value at position ",idx,". Should be: ", yout1, " but is: ",real[idx])
        sys.exit(1)

"""

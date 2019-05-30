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

import os.path

# from OCS or OWD
N=int(os.environ.get("OCPI_TEST_N"))
M=int(os.environ.get("OCPI_TEST_M"))
R=int(os.environ.get("OCPI_TEST_R"))
ACC_WIDTH=int(os.environ.get("OCPI_TEST_ACC_WIDTH"))
DATA_WIDTH=int(os.environ.get("OCPI_TEST_DATA_WIDTH"))

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

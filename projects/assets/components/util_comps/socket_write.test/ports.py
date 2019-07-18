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

import re

# This is a quick utility program for setting port numbers in XML (and sometimes portmap file)

flags = [0] * 256

# Test 7
#flags[0] = 12359
#flags[9] = 12360
#flags[11] = 12361
#flags[205] = 12362

# Test 8
#flags[0] = 12363
#flags[9] = 12364
#flags[11] = 12365
#flags[205] = 12366

# Test 9
#for x in xrange(0,256):
  #print "{} odata/myoutput_{:03d}.out".format(12400+x, x)
  #flags[x] = 12400+x

# Test 11
#for x in xrange(0,256):
  #print "{} odata/myoutput_{}.out".format(12660+x, x)
  #flags[x] = 12660+x

# Test 10
for x in xrange(0,256):
  print "{} odata/myoutput_{:03d}.out".format(12920+x, x)
  flags[x] = 12920+x

strn = ','.join([str(x) for x in flags])

print """    <Property Name="outSocket" Value='ports {
     """,
print re.sub("(([0-9]+,){16})", "\\1\n      ", strn),
print "}'/>"

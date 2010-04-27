#! /bin/sh
# Copyright (c) 2009 Mercury Federal Systems.
# 
# This file is part of OpenCPI.
# 
# OpenCPI is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# OpenCPI is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

# ----------------------------------------------------------------------
# Test script to run the CPI::OS unit tests.
#
# Revision History:
#
#     06/03/2009 - Frank Pilhofer
#                  Initial version.
#
# ----------------------------------------------------------------------
#

TESTS="Event FileSystem Mutex ProcessManager Semaphore Socket Thread Timer"

result=0

for test in $TESTS ; do
    EXE=${CPIDIR}/adapt/os/cpios/${OUTDIR}/${test}
    if test ! -x $EXE ; then
	echo "$EXE not found"
	result=1
	continue
    fi
    # Returns non-zero if any tests failed
    $EXE
    if test $? -ne 0 ; then
	result=1
    fi
done

exit $result

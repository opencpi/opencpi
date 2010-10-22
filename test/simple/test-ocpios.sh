
# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
#
#    Mercury Federal Systems, Incorporated
#    1901 South Bell Street
#    Suite 402
#    Arlington, Virginia 22202
#    United States of America
#    Telephone 703-413-0781
#    FAX 703-413-0784
#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #

#! /bin/sh


# ----------------------------------------------------------------------
# Test script to run the OCPI::OS unit tests.
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
    EXE=${OCPIDIR}/adapt/os/ocpios/${OUTDIR}/${test}
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

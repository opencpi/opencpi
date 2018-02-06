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

include $(OCPI_CDK_DIR)/include/ocpisetup.mk

all: 
	rm -f unitTest; ln -s ../unit.test/unitTest .
	rm -f utTime.sh; ln -s ../unit.test/utTime.sh .
	rm -f genExpected; ln -s ../genExpected.test/genExpected .


# OCPI_LIBRARY_PATH is like LD_LIBRARY_PATH, but for OpenCPI components.
run: all
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/rcc ./test.sh 

clean::
	rm -f -r *.*~ *~ *.raw deltaDataOut.dat testDataOut.dat genExpected unitTest utTime.sh *.csv

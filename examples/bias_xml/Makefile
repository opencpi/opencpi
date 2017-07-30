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

APP=biasFile
include ../setup.mk

good.output test.input: mkTestFile.cxx
	g++ -Wall -o $(DIR)/a.out mkTestFile.cxx
	$(DIR)/a.out $(FILE_SIZE) > test.input 2> good.output

run: all test.input good.output
	$(VG) $(PROG) $(ARGS) > /dev/null
	cmp test.output good.output

runxml: all test.input
	$(VG) ocpirun -d -v $(ARGS) bias.xml > /dev/null
	cmp test.output good.output

runhdl1: all test.input
	$(PROGS) $(ARGS)
	cmp test.output good.output

runhdl2: all test.input
	$(PROG) $(ARGS)
	cmp test.output good.output

clean::
	rm -f -r test.output test.input good.output

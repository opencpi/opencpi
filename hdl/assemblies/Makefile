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

Assemblies=\
	testbias_file \
	testbias \
	tb_bias \
	tb_bias_param \
	biascapture \
	tb_bias_vhdl \
	testpsd \
	testddc \
	testbias2 \
	testbias5 \
	testbias_ver \
	empty \
	time_test \
	time_test_file \

ifndef OCPI_MINIMAL_BUILD
Assemblies += \
	patternbias \
	test_width \
	testbias_file_read \
	testbias_file_write \
	ptestonly \
	adapt \

endif
include $(OCPI_CDK_DIR)/include/hdl/hdl-assemblies.mk

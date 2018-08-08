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

# This is the dsp_comps library

Tests = \
	Baudtracking_simple.test \
	dc_offset_filter.test \
	iq_imbalance_fixer.test \
	complex_mixer.test \
	fir_real_sse.test \
	fir_complex_sse.test \
	phase_to_amp_cordic.test \
	rp_cordic.test \
	pr_cordic.test \
	cic_dec.test \
	cic_int.test \

# All workers created here will be built automatically
# If you want to limit the workers that actually get built, set the Workers= variable
Workers = \
	Baudtracking_simple.rcc \
	real_digitizer.rcc \
	dc_offset_filter.hdl \
	iq_imbalance_fixer.hdl \
	complex_mixer.rcc \
	complex_mixer.hdl \
	fir_real_sse.hdl fir_complex_sse.hdl \
	phase_to_amp_cordic.hdl \
	rp_cordic.hdl \
	pr_cordic.hdl \
	cic_dec.hdl \
	cic_int.hdl \

include $(OCPI_CDK_DIR)/include/library.mk

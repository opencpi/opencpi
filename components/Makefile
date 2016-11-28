###########################################################################
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
###########################################################################

$(if $(realpath $(OCPI_CDK_DIR)),,$(error The OCPI_CDK_DIR environment variable is not set correctly (currently $(OCPI_CDK_DIR)).))

Package=ocpi

Tests =
#	unit.test \
#	genExpected.test

# These are the components that would ship in an RCC-only version of the CDK.
#  a. What we would consider a "bare minimum" of a runtime environment (file read, file write, etc)
#  b. The ones needed by a default run of "make run" in the "examples" subdir
#  c. Examples that the end user can steal from (e.g. testmulti)
#  Do NOT add any HDL components to the OCPI_CORE_BUILD.
Workers = \
	bias.rcc \
	bias_cc.rcc \
	bias_param.rcc \
	capture.rcc \
	file_read.rcc \
	file_read_msg.rcc \
	file_write.rcc \
	file_write_msg.rcc \
	hello_world.rcc \
	hello_world_cc.rcc \
	nothing.rcc \
	pattern.rcc \
	proxy.rcc \
	ptest.rcc \
	ptest_cc.rcc \
	testmulti.rcc \
	testmulti_cc.rcc \
	testzc.rcc

ifndef OCPI_CORE_BUILD # this variable restricts the build to the above "core" RCC components
# HDL equivalent to "Core" components above should be built first
Workers += \
	bias.hdl \
	bias_16.hdl \
	bias_param.hdl \
	bias_ver.hdl \
	bias_vhdl.hdl \
	capture.hdl \
	file_read.hdl \
	file_write.hdl \
	nothing.hdl \
	pattern.hdl \
	proxy_hdl.rcc \
	ptest.hdl \
	sma.hdl \
	time_gen.hdl

# These components are old / untested and not typically shipped (AV-790,AV-877)
ifndef RPM_BUILD_ROOT
Workers += \
	adc_unpack.rcc \
	blur.rcc \
	canny.rcc \
	canny_partial.rcc \
	cic_hpfilter_complex.rcc \
	cic_lpfilter_complex.rcc \
	convert_s16_f32.rcc \
	corner_eigen_vals_vecs.rcc\
	ddc.hdl \
	dds_complex.rcc \
	delay.hdl \
	delay.rcc \
	dilate.rcc \
	dsample_complex.rcc \
	erode.rcc \
	fm_demod_complex.rcc \
	framegate.hdl \
	framegate.rcc \
	fsk_mod_complex.rcc \
	gaussian_blur.rcc \
	good_features_to_track.rcc \
	loopback.rcc \
	min_eigen_val.rcc \
	mixer_complex.rcc \
	morph_mean.rcc \
	noise_gen_complex.rcc \
	optical_flow_pyr_lk.rcc \
	psd.hdl \
	replicate.rcc \
	scharr.rcc \
	sobel.rcc \
	sobel_32f.rcc \
	split1x2.rcc \
	splitter2x2.hdl \
	splitter2x2.rcc \
	sym_fir_complex.rcc \
	sym_fir_real.rcc

Tests += \
	cic_lpfilter_complex.test \
	cic_hpfilter_complex.test \
	dds_complex.test \
	fsk_mod_complex.test \
	fm_demod_complex.test \
	mixer_complex.test \
	loopback.test \
	sym_fir_real.test \
	sym_fir_complex.test \
	noise_gen_complex.test
endif # not RPM_BUILD_ROOT

ifndef OCPI_MINIMAL_BUILD
Workers += \
	bias_spcm.hdl \
	bias_wide.hdl \
	comparator_complex.rcc \
	comparator_real.rcc \
	copy.rcc \
	$(Tests)

ifeq ($(OCPI_HAVE_OPENCL),1)
Workers += \
	fft1d.ocl \
	mmul.ocl \
	power_spectrum_1d.ocl \
	testmulti.ocl \
	vadd.ocl \
	vsadd.ocl
endif # OCPI_HAVE_OPENCL
endif # OCPI_MINIMAL_BUILD
endif # OCPI_CORE_BUILD

include $(OCPI_CDK_DIR)/include/library.mk

loc:
	$(OCPI_CDK_DIR)/include/static_analyzer.sh ./components_sw_metrics.txt

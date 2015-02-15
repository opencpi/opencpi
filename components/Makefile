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

$(if $(realpath $(OCPI_CDK_DIR)),,$(error The OCPI_CDK_DIR environment variable is not set correctly.))

Package=ocpi

Tests = \
	unit.test \
	dds_complex.test \
	fsk_mod_complex.test \
	fm_demod_complex.test \
	mixer_complex.test \
	loopback.test \
	sym_fir_real.test \
	sym_fir_complex.test \
	noise_gen_complex.test \
	cic_lpfilter_complex.test \
	cic_hpfilter_complex.test \
	genExpected.test 


Implementations = \
        bias_16.hdl \
	bias_spcm.hdl \
	adc_unpack.rcc \
	bias.hdl \
	bias_ver.hdl \
	bias_param.hdl \
	bias_vhdl.hdl \
	blur.rcc \
	canny.rcc \
	canny_partial.rcc \
	cic_lpfilter_complex.rcc \
	cic_hpfilter_complex.rcc \
	comparator_complex.rcc \
	comparator_real.rcc \
	convert_s16_f32.rcc \
	corner_eigen_vals_vecs.rcc\
	copy.rcc \
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
	scharr.rcc \
	sma.hdl \
	sobel.rcc \
	sobel_32f.rcc \
	split1x2.rcc \
	splitter2x2.hdl \
	splitter2x2.rcc \
	sym_fir_complex.rcc \
	sym_fir_real.rcc \
	testmulti.rcc \
	testzc.rcc \
	psd.hdl \
	ddc.hdl \
	fft1d.ocl \
	mmul.ocl \
	vadd.ocl \
	vsadd.ocl \
	power_spectrum_1d.ocl \
	testmulti.ocl \
        file_read.rcc \
        file_read_msg.rcc \
        file_write.rcc \
        file_write_msg.rcc \
	bias.rcc \
	pattern.hdl pattern.rcc \
	capture.hdl capture.rcc \
	ptest.rcc \
	ptest_cc.rcc \
	ptest.hdl \
        file_read.hdl \
	file_write.hdl \
        replicate.rcc \
	bias_cc.rcc \
        bias_param.rcc \
	hello_world.rcc \
	hello_world_cc.rcc \
	proxy.rcc \
	$(Tests)

#	fir_prog.hdl \

#	wbr.hdl 

#HdlTargets=all

ifndef RccTargets
RccTargets=$(OCPI_TARGET_HOST)
endif

include $(OCPI_CDK_DIR)/include/lib.mk

loc:
	$(OCPI_CDK_DIR)/include/static_analyzer.sh ./components_sw_metrics.txt



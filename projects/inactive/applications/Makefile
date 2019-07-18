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

$(if $(realpath $(OCPI_CDK_DIR)),,\
  $(error The OCPI_CDK_DIR environment variable is not set correctly.))
# To restrict the applications that are built or run, you can set the Applications
# variable to the specific list of which ones you want to build and run, e.g.:
Applications=\
	bestfit_test \
	canny \
	dds_ports \
	feature_detection \
	image_filtering \
	optical_flow_xml \
	bias2_xml \
	bias_xml \
	cat_xml \
	copy_xml \
	corbaURL_xml \
	ext2file_xml\
	hello_xml \
	hello \
	vsadd \
	ptest \
	copy \
	file-bias-capture.xml \
	pattern-bias-file.xml \
	pattern.xml \
	tb_bias_file.xml \
	tb_bias.xml

#	fsk_modem_xml \ # Fails to build due to missing "CmdOption.h"

# Otherwise all applications will be built and run
include $(OCPI_CDK_DIR)/include/applications.mk

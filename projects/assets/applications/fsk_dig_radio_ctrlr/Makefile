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
# This is the application Makefile for the "fsk_dig_radio_ctrlr" application
# If there is a fsk_dig_radio_ctrlr.cc (or fsk_dig_radio_ctrlr.cxx) file, it will be assumed to be a C++ main program to build and run
# If there is a fsk_dig_radio_ctrlr.xml file, it will be assumed to be an XML app that can be run with ocpirun.
# The RunArgs variable can be set to a standard set of arguments to use when executing either.

OcpiAppNoRun=1

include $(OCPI_CDK_DIR)/include/application.mk

clean::
	rm -rf odata/* scripts/*.pyc html/ latex/ 7 xx yy _impactbatch.log

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

include $(OCPI_CDK_DIR)/include/util.mk

ifndef OcpiRunArgs
OcpiRunArgs= -t 2
endif

# To restrict the applications that are built or run, you can set the Applications
# variable to the specific list of which ones you want to build and run, e.g.:
Applications=bias.xml \
             cic_int_dc_offset_iq_imbalance_mixer_cic_dec \
             copy.xml \
             file-bias-capture.xml \
             hello.xml \
             nothing.xml \
             pattern-bias-file.xml \
             pattern.xml \
             proxybias.xml \
             ptest.xml \
             tb_bias_file.xml \
             tb_bias.xml \
             testbias2.xml \
             testbias5.xml \
             iqstream_max_calculator_test \
             testbias.xml 

# These are not yet runnable via 'make run'
#Applications+=FSK \
              dc_offset_filter \
              dc_offset_iq_imbalance \
              dc_offset_iq_imbalance_cic_dec \
              dc_offset_iq_imbalance_mixer_cic_dec \
              empty_rxtx_zed_zipper_card \
              iqstream_max_calculator_test \
              rx_app \

# Otherwise all applications will be built and run
include $(OCPI_CDK_DIR)/include/applications.mk

# Some custom ocpirun commands for testing XML applications
CMP= && cmp test.input test.output
PF=set -o pipefail &&
runbias: export OCPI_LIBRARY_PATH=$(OcpiGetDefaultLibraryPath)
runbias:
	$(PF) $(OcpiRunBefore) ocpirun $(OcpiRunArgs) -v -d bias 2>&1 | tee xml_bias_run.log $(OcpiRunAfter)
	$(PF) $(OcpiRunBefore) ocpirun $(OcpiRunArgs) -v -d -pbias=biasValue=0 bias 2>&1 | tee xml_bias_withvalue_run.log $(CMP) $(OcpiRunAfter)
	$(PF) $(OcpiRunBefore) ocpirun $(OcpiRunArgs) -v -d pattern-bias-file 2>&1 | tee xml_pattern-bias-file_run.log $(OcpiRunAfter)
	$(PF) $(OcpiRunBefore) ocpirun $(OcpiRunArgs) -v -d -mcapture=rcc file-bias-capture 2>&1 | tee xml_file-bias-capture_run.log $(OcpiRunAfter)
	$(PF) $(OcpiRunBefore) ocpirun $(OcpiRunArgs) -v -d pattern 2>&1 | tee xml_pattern_run.log $(OcpiRunAfter)
	$(PF) $(OcpiRunBefore) ocpirun $(OcpiRunArgs) -v -d -mcapture=rcc tb_bias 2>&1 | tee xml_tb_bias_run.log $(OcpiRunAfter)
	$(PF) $(OcpiRunBefore) ocpirun $(OcpiRunArgs) -v -d tb_bias_file 2>&1 | tee xml_tb_bias_file_run.log $(OcpiRunAfter)
	$(PF) $(OcpiRunBefore) ocpirun $(OcpiRunArgs) -v -d testbias 2>&1 | tee xml_testbias_run.log $(OcpiRunAfter)
	$(PF) $(OcpiRunBefore) ocpirun $(OcpiRunArgs) -v -d -pbias=biasValue=0 testbias 2>&1 | tee xml_testbias_withvalue_run.log $(CMP) $(OcpiRunAfter)

runtests: export OCPI_LIBRARY_PATH=$(OcpiGetDefaultLibraryPath)
runtests: runbias
	$(PF) $(OcpiRunBefore) ocpirun $(OcpiRunArgs) -v -d copy 2>&1 | tee xml_copy_run.log $(CMP) $(OcpiRunAfter)
	$(PF) $(OcpiRunBefore) ocpirun $(OcpiRunArgs) -v -d hello 2>&1 | tee xml_hello_run.log $(OcpiRunAfter)
	$(PF) $(OcpiRunBefore) ocpirun $(OcpiRunArgs) -v -d testbias2 2>&1 | tee xml_testbias2_run.log $(OcpiRunAfter)
	$(PF) $(OcpiRunBefore) ocpirun $(OcpiRunArgs) -v -d -pbias0=biasValue=0 -pbias1=biasValue=0 testbias2 2>&1 | tee xml_testbias2_withvalue_run.log $(CMP) $(OcpiRunAfter)
	$(PF) $(OcpiRunBefore) ocpirun $(OcpiRunArgs) -v -d ptest 2>&1 | tee xml_ptest_run.log

clean::
	$(AT)rm -r -f *.log
	$(AT)rm -r -f simulations/
	$(AT)rm -f test.out test.output test.outputwrong time_test.out *_run.log

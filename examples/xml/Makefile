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

include $(OCPI_CDK_DIR)/include/util.mk

all:
include ../setup.mk
OcpiRun=$(OCPI_CDK_DIR)/bin/$(OCPI_TOOL_DIR)/ocpirun

CMP= && cmp test.input test.output

PF=set -o pipefail &&
runbias:
	env
	$(PF) $(VG) $(OcpiRun) -v -d $(OPTS) $(FR) $(FW) bias 2>&1 | tee xml_bias_run.log
	$(PF) $(VG) $(OcpiRun) -v -d -pbias=biasValue=0 $(OPTS) $(FR) $(FW) bias 2>&1 | tee xml_bias_withvalue_run.log $(CMP)
	$(PF) $(VG) $(OcpiRun) -v -d $(OPTS) $(FW) pattern-bias-file 2>&1 | tee xml_pattern-bias-file_run.log
	$(PF) $(VG) $(OcpiRun) -v -d $(OPTS) $(FR)  -mcapture=rcc file-bias-capture 2>&1 | tee xml_file-bias-capture_run.log
	$(PF) $(VG) $(OcpiRun) -v -d $(OPTS) $(FW) pattern 2>&1 | tee xml_pattern_run.log
	$(PF) $(VG) $(OcpiRun) -v -d $(OPTS)  -mcapture=rcc tb_bias 2>&1 | tee xml_tb_bias_run.log
	$(PF) $(VG) $(OcpiRun) -v -d $(OPTS) $(FR) tb_bias_file 2>&1 | tee xml_tb_bias_file_run.log
	$(PF) $(VG) $(OcpiRun) -v -d $(OPTS) $(FR) $(FW) testbias 2>&1 | tee xml_testbias_run.log
	$(PF) $(VG) $(OcpiRun) -v -d -pbias=biasValue=0 $(OPTS) $(FR) $(FW) testbias 2>&1 | tee xml_testbias_withvalue_run.log $(CMP)

run: runbias
	$(PF) $(VG) $(OcpiRun) -v -d $(OPTS) $(FR) $(FW) copy 2>&1 | tee xml_copy_run.log $(CMP)
	$(PF) $(VG) $(OcpiRun) -v -d $(OPTS) hello 2>&1 | tee xml_hello_run.log
	$(PF) $(VG) $(OcpiRun) -v -d $(OPTS) $(FR) $(FW) testbias2 2>&1 | tee xml_testbias2_run.log
	$(PF) $(VG) $(OcpiRun) -v -d -pbias0=biasValue=0 -pbias1=biasValue=0 $(OPTS) $(FR) $(FW) testbias2 2>&1 | tee xml_testbias2_withvalue_run.log $(CMP)
	$(PF) $(VG) $(OcpiRun) -v -d ptest 2>&1 | tee xml_ptest_run.log

test:

clean::
	rm -f test.out test.output test.outputwrong time_test.out *_run.log

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

# This is the Makefile for worker complex_mixer_cores.hdl

include $(OCPI_CDK_DIR)/include/hdl/xilinx.mk

ExcludeTargets=isim altera

# For vivado we use vivado stubs with netlists
# For xsim we use simulation netlists
SourceFiles=\
  $(if $(filter vivado,$(HdlToolSet)),\
      ./vivado_ip/complex_multiplier_stub.vhd ./vivado_ip/dds_compiler_stub.vhd,\
    $(if $(filter xsim,$(HdlToolSet)),\
      ./vivado_ip/complex_multiplier_sim_net.vhd ./vivado_ip/dds_compiler_sim_net.vhd))

# For vivado we use vivado stubs with netlists
# For xsim we use simulation netlists
Cores=\
  $(if $(filter vivado,$(HdlToolSet)),\
      ./vivado_ip/complex_multiplier.edf ./vivado_ip/dds_compiler.edf)

include $(OCPI_CDK_DIR)/include/worker.mk

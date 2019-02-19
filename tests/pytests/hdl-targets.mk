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

# This file is a snapshot/fake version of cdk/include/hdl/hdl-targets.mk for testing
# purposes.

# Include this here so that hdl-targets.mk (this file) can be included on its own when
# things like HdlAllPlatforms is required.
all:
$(info HdlToolSet_zynq="vivado ";\
       HdlToolName_vivado="Vivado";\
       HdlPart_zed_zipper="xc7z020-1-clg484 ";\
       HdlFamily_xsim="xsim ";\
       HdlTargets_zynq="xc7z020 ";\
       HdlFamily_ep4sgx230k-c2-f40="stratix4 ";\
       HdlTargets_spartan6="xc6slx45 ";\
       HdlToolSet_modelsim="modelsim ";\
       HdlToolName_modelsim="Modelsim";\
       HdlPart_zed="xc7z020-1-clg484 ";\
       HdlTopTargets="xilinx altera modelsim ";\
       HdlSimTools="isim icarus verilator ghdl xsim modelsim ";\
       HdlAllFamilies="isim virtex5 virtex6 spartan3adsp spartan6 zynq_ise zynq xsim stratix4 stratix5 modelsim ";\
       HdlPart_zed_ise="xc7z020_ise_alias-1-clg484 ";\
       HdlTargets_stratix5="ep5sgsmd8k2 ";\
       HdlTargets_stratix4="ep4sgx230k ep4sgx530k ep4sgx360 ";\
       HdlTargets_virtex6="xc6vlx240t ";\
       HdlTargets_virtex5="xc5vtx240t xc5vlx50t xc5vsx95t xc5vlx330t xc5vlx110t ";\
       HdlFamily_xc7z020_ise_alias-1-clg484="zynq_ise ";\
       HdlTargets_altera="stratix4 stratix5 ";\
       HdlPart_alst4x="ep4sgx530k-c2-h40 ";\
       HdlAllTargets="isim virtex5 virtex6 spartan3adsp spartan6 zynq_ise zynq xsim stratix4 stratix5 modelsim xilinx altera ";\
       HdlTargets_xilinx="isim virtex5 virtex6 spartan3adsp spartan6 zynq_ise zynq xsim ";\
       HdlFamily_isim="isim ";\
       HdlPart_isim="isim ";\
       HdlToolSet_virtex6="xst ";\
       HdlToolSet_virtex5="xst ";\
       HdlAllPlatforms="zed zed_zipper alst4x alst4 modelsim isim xsim zed_ise ml605 ";\
       HdlBuiltPlatforms="modelsim isim xsim";\
       HdlFamily_xc6vlx240t-1-ff1156="virtex6 ";\
       HdlToolSet_zynq_ise="xst ";\
       HdlToolName_xst="ISE";\
       HdlFamily_ep4sgx530k-c2-h40="stratix4 ";\
       HdlTargets="isim virtex5 virtex6 spartan3adsp spartan6 zynq_ise zynq xsim stratix4 stratix5  modelsim ";\
       HdlToolSet_isim="isim ";\
       HdlToolName_isim="ISE";\
       HdlPart_alst4="ep4sgx230k-c2-f40 ";\
       HdlPart_modelsim="modelsim ";\
       HdlPart_xsim="xsim ";\
       HdlTargets_spartan3adsp="xc3sd3400a ";\
       HdlToolSet_spartan3adsp="xst ";\
       HdlToolSet_xsim="xsim ";\
       HdlToolName_xsim="Vivado";\
       HdlPart_ml605="xc6vlx240t-1-ff1156 ";\
       HdlFamily_modelsim="modelsim ";\
       HdlFamily_xc7z020-1-clg484="zynq ";\
       HdlToolSet_spartan6="xst ";\
       HdlTargets_zynq_ise="xc7z020_ise_alias ";\
       HdlToolSet_stratix5="quartus ";\
       HdlToolSet_stratix4="quartus ";\
       HdlToolName_quartus="Quartus";\
       HdlPlatformDir_zed="fake_plat";\
       HdlPlatformDir_zed_zipper="fake_plat";\
       HdlPlatformDir_alst4x="fake_plat";\
       HdlPlatformDir_alst4="fake_plat";\
       HdlPlatformDir_modelsim="fake_plat";\
       HdlPlatformDir_isim="fake_plat";\
       HdlPlatformDir_xsim="fake_plat";\
       HdlPlatformDir_zed_ise="fake_plat";\
       HdlPlatformDir_ml605="fake_plat";)

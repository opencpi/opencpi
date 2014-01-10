ifndef _HDL_TARGETS_
_HDL_TARGETS_=here
# This file is the database of hdl targets and associated tools
# It is a "leaf file" that is used in several places.

# This is the list of top level targets.
# All other targets are some level underneath these
# The levels are: top, family, part, speed

#Testing: HdlTopTargets=xilinx altera verilator test1
HdlTopTargets:=xilinx altera icarus verilator modelsim # icarus altera # verilator # altera

# The first part in a family is the one used for core building
# Usually it should be the largest
HdlTargets_xilinx:=isim virtex5 virtex6 spartan3adsp xsim spartan6 zynq
HdlTargets_virtex5:=xc5vtx240t xc5vlx50t xc5vsx95t xc5vlx330t xc5vlx110t
HdlTargets_virtex6:=xc6vlx240t
HdlTargets_spartan6:=xc6slx45
HdlTargets_spartan3adsp:=xc3sd3400a
HdlTargets_zynq:=xc7z020

HdlTargets_altera:=stratix4 stratix5 # altera-sim
# The "k", when present indicates the transceiver count (k = 36)
# But in many places it is left off..
HdlTargets_stratix4:=ep4sgx230k ep4sgx530k ep4sgx360
HdlTargets_stratix5:=ep5sgsmd8k2
#Testing: HdlTargets_test1=test2

HdlSimTools=isim icarus verilator ghdl xsim modelsim

# Tools are associated with the family or above
HdlToolSet_ghdl:=ghdl
HdlToolSet_isim:=isim
HdlToolSet_xsim:=xsim
HdlToolSet_modelsim:=modelsim
HdlToolSet_spartan3adsp:=xst
HdlToolSet_virtex5:=xst
HdlToolSet_virtex6:=xst
HdlToolSet_spartan6:=xst
HdlToolSet_zynq:=xst
HdlToolSet_verilator:=verilator
HdlToolSet_icarus:=icarus
HdlToolSet_stratix4:=quartus
HdlToolSet_stratix5:=quartus

# Platforms
HdlAllPlatforms:=ml555 schist ml605 ocpiIsim alst4 alst4x wilda5 isim_pf xsim_pf n210 modelsim_pf iva1lpe zed wilda5
# Parts as chip-speed-package
HdlPart_ml555:=xc5vlx50t-1-ff1136
HdlPart_schist:=xc5vsx95t-2-ff1136
HdlPart_ml605:=xc6vlx240t-1-ff1156
HdlPart_alst4:=ep4sgx230k-c2-f40
HdlPart_alst4x:=ep4sgx530k-c2-h40
HdlPart_n210:=xc3sd3400a-5-fg676
HdlPart_iva1lpe:=xc6slx45-2-csg324
HdlPart_isim_pf:=isim
HdlPart_xsim_pf:=xsim
HdlPart_modelsim_pf:=modelsim
HdlPart_wilda5:=ep5sgsmd8k2-c3-f40
HdlPart_zed:=xc7z020-1-clg484
endif

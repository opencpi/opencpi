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
HdlTargets_xilinx:=isim virtex5 virtex6 spartan3adsp xsim
HdlTargets_virtex5:=xc5vtx240t xc5vlx50t xc5vsx95t xc5vlx330t xc5vlx110t
HdlTargets_virtex6:=xc6vlx240t xc6slx45t
HdlTargets_spartan3adsp:=xc3sd3400a

HdlTargets_altera:=stratix4 stratix5 # altera-sim
HdlTargets_stratix4:=ep4sgx230k ep4sgx360
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
HdlToolSet_verilator:=verilator
HdlToolSet_icarus:=icarus
HdlToolSet_stratix4:=quartus
HdlToolSet_stratix5:=quartus

# Platforms
HdlAllPlatforms:=ml555 schist ml605 ocpiIsim alst4 isim_pf xsim_pf n210
# Parts as chip-speed-package
HdlPart_ml555:=xc5vlx50t-1-ff1136
HdlPart_schist:=xc5vsx95t-2-ff1136
HdlPart_ml605:=xc6vlx240t-1-ff1156
HdlPart_alst4:=ep4sgx230k-c2-f40
HdlPart_n210:=xc3sd3400a-5-fg676
HdlPart_isim_pf:=isim
HdlPart_xsim_pf:=xsim
HdlPart_modelsim_pf:=modelsim
endif

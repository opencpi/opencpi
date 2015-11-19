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

# Make the initial definition as a simply-expanded variable
HdlAllPlatforms:=

override OCPI_HDL_PLATFORM_PATH:=$(call Unique,\
  $(OCPI_HDL_PLATFORM_PATH) \
  $(foreach p,$(subst :, ,$(OCPI_PROJECT_PATH)) $(OCPI_CDK_DIR),$(call OcpiExists,$p/lib/platforms)))

define doPlatformsDir
  HdlSavePlatforms:=$$(HdlAllPlatforms)
  include $1/$(notdir $1).mk
  HdlNewPlatforms:=$$(filter-out $$(HdlSavePlatforms),$$(HdlAllPlatforms))
  $$(foreach p,$$(filter-out $$(HdlSavePlatforms),$$(HdlNewPlatforms)),\
    $$(eval HdlPlatformDir_$$p:=$1/$$p)\
    $$(eval HdlAllPlatforms+=$$p))

endef
  
$(foreach d,$(subst :, ,$(OCPI_HDL_PLATFORM_PATH)),\
  $(foreach p,$(notdir $d),\
    $(if $(filter platforms,$p),$(eval $(call doPlatformsDir,$d)),\
       $(if $(wildcard $d),,$(error in OCPI_HDL_PLATFORM_PATH $d does not exist))\
       $(foreach l,$(if $(wildcard $d/lib),$d/lib,$d),\
	 $(if $(wildcard $l/hdl/$p.xml),,$(error no $p.xml file found in $l))\
         $(eval -include $l/$p.mk)\
         $(eval HdlAllPlatforms+=$p) \
         $(eval HdlPlatformDir_$p:=$l)))))

$(call OcpiDbgVar,HdlAllPlatforms)
endif

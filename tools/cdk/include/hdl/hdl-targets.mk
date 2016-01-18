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
  $(foreach p,$(OcpiGetProjectPath) $(OCPI_CDK_DIR),$(call OcpiExists,$p/lib/platforms)))

# Add a platform to the database.
# Arg 1: The directory where the *.mk file is
# Arg 2: The name of the platform
# Arg 3: The actual platform directory for using the platform (which may not exist).
HdlAddPlatform=\
  $(call OcpiDbg,HdlAddPlatform($1,$2,$3))\
  $(eval include $1/$2.mk)\
  $(eval HdlAllPlatforms+=$2)\
  $(eval HdlPlatformDir_$2:=$3)

# Call this with a directory that is a platform's directory, either source (with "lib" subdir 
# if built) or exported. For the individual platform directories we need to deal with
# the prebuilt, postbuilt, and exported scenarios.  Hence the complexity.
# Both the *.xml and *.mk are generally needed, but the *.mk is more critical here,
# so we key on that.
# If we are pointing at a non-exported platform directory, we prefer its local export subdir 
# ("lib"), if the hdl/*.mk is present.
# (under hdl since it is in fact a worker in a library)
HdlDoPlatform=\
  $(foreach p,$(notdir $1),\
    $(foreach d,$(if $(wildcard $1/lib/$p.mk),$1/lib,$1),\
      $(if $(wildcard $d/hdl/$p.xml)$(wildcard $d/$p.xml),,$(error no $p.xml file found for platform under: $1))\
      $(if $(wildcard $d/$p.mk),,$(error no $p.mk file found under $1. $p not built?))\
      $(call HdlAddPlatform,$d,$p,$d)))

# Handle a directory named "platforms", exported or not
HdlDoPlatformsDir=\
  $(if $(wildcard $1/mk),\
    $(foreach d,$(wildcard $1/mk/*.mk),\
      $(foreach p,$(basename $(notdir $d)),\
        $(call HdlAddPlatform,$1/mk,$p,$1/$p))),\
    \
    $(foreach d,$(wildcard $1/*),\
      $(foreach p,$(notdir $d),\
        $(if $(wildcard $d/$p.xml)$(wildcard $d/lib/hdl/$p.xml)$(wildcard $d/hdl/$p.xml),\
          $(call HdlDoPlatform,$d)))))
  
$(call OcpiDbgVar,HdlAllPlatforms)
$(foreach d,$(subst :, ,$(OCPI_HDL_PLATFORM_PATH)),\
  $(if $(wildcard $d),,$(error in OCPI_HDL_PLATFORM_PATH "$d" does not exist))\
  $(if $(filter platforms,$(notdir $d)),\
    $(call HdlDoPlatformsDir,$d),\
    $(call HdlDoPlatform,$d)))

$(call OcpiDbgVar,HdlAllPlatforms)
endif

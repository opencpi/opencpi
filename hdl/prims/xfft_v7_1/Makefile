OCPI_DIR=../../../ocpi
CoreBlackBoxFile=$(Core)_bb.v
Targets=xc5vsx95t-2-ff1136 xc5vlx50t-ff1136-1
DontBuildCores=yes # suppress normal rules to build the core from source files
include $(OCPI_DIR)/include/hdl/hdl-core.mk
# This only works for a single target at this time
$(CoreResults):
	$(AT)echo Exporting pre-built coregen NGC file $(Core).ngc for target \"$(Target)\"
	cd $(Target); ln -s ../$(Core)-$(Target).ngc $(Core).ngc

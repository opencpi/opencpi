# We use the ADI library and the auto-generated properties file
# Someday this will just be:  RccLibraries=ad9361
include $(OCPI_CDK_DIR)/include/util.mk
$(call OcpiCheckPrereq,ad9361,$(RccTargets),lib/libad9361.*)
AD9361=$(OCPI_PREREQUISITES_INSTALL_DIR)/ad9361
AD9361_LIB=$(AD9361)/$(OCPI_TARGET_DIR)/lib/libad9361.a

RccIncludeDirs=$(AD9361)/include
AEPLibraries=$(AD9361_LIB)
# Our "platform" functions that the ADI library calls
SourceFiles=ad9361_platform.cc

include $(OCPI_CDK_DIR)/include/worker.mk

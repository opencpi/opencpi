# We use the ADI library and the auto-generated properties file
# Someday this will just be:  RccLibraries=ad9361
AD9361=$(OCPI_PREREQUISITES_INSTALL_DIR)/ad9361
ifeq ($(wildcard $(AD9361)/include/ad9361-properties.xml),)
  $(error The AD9361 package is not installed at $(OCPI_PREREQUISITES_INSTALL_DIR)/ad9361)
endif
AD9361_LIB=$(wildcard $(AD9361)/$(OCPI_TARGET_DIR)/lib/libad9361.*)
ifeq ($(AD9361_LIB),)
  $(error The AD9361 package is not built for: $(OCPI_TARGET_DIR))
endif

XmlIncludeDirs=$(AD9361)/include
RccIncludeDirs=$(AD9361)/include
AEPLibraries=$(AD9361_LIB)
# Our "platform" functions that the ADI library calls
SourceFiles=ad9361_platform.cc

include $(OCPI_CDK_DIR)/include/worker.mk

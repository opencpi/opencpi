include $(OCPI_DIR)/include/util.mk
# Makefile fragment for HDL workers

Model=hdl
SourceSuffix=.v
LibraryAccessTarget=$(if $(findstring xc5,$(1))$(findstring virtex5,$(1)),virtex5,virtex6)
LibraryFileTarget=$(if $(filter virtex5,$(1))$(findstring xc5,$(1)),hdllib.ref,$(LibName).sdbl)
LibraryCleanTargets=$(if $(filter virtex5,$(1))$(findstring xc5,$(1)),hdllib.ref vlg??,*.sdb?)
ifeq ($(origin SourceFiles),undefined)
SourceFiles=$(wildcard *.[vV])
endif
ifndef LibName
LibName=work
else
ifndef InstallDir
InstallDir=$(OCPI_DIR)/lib/hdl/$(LibName)
$(InstallDir):
	$(AT)mkdir $@
endif
endif
include $(OCPI_DIR)/include/hdl/xst.mk


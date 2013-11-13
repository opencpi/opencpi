$(if $(OCPI_CDK_DIR),,$(error The OCPI_CDK_DIR environment variable must be set for this Makefile.))

Package=ocpi

Implementations = \
	wsi_32_to_16.hdl \
	wsi_16_to_32.hdl

export HdlLibraries= bsv

include $(OCPI_CDK_DIR)/include/lib.mk



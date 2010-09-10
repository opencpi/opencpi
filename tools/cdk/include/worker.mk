# Generic worker makefile
include $(OCPI_DIR)/include/util.mk
ifeq ($(Model),)
$(error This directory named $(CwdName) does not end in any of: $(Models))
endif
$(info Model is $(Model))
include $(OCPI_DIR)/include/$(Model)/$(Model)-worker.mk

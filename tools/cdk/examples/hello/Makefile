#$(info start $(MAKECMDGOALS))
APP=hello
include ../setup.mk

#$(info beforex $(MAKECMDGOALS))
#.PHONY: run
run: all
	OCPI_LIBRARY_PATH=lib $(VG) $(PROG) $(OUT)

#$(info aftery $(MAKECMDGOALS))

# These make the worker library for this application
Implementations = hello.rcc
include $(OCPI_INC_DIR)/lib.mk

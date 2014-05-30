#$(info start $(MAKECMDGOALS))
APP=copy
include ../setup.mk

run: all
	OCPI_LIBRARY_PATH=lib $(VG) $(PROG) < $(PROG) > test.output
	ls -l $(PROG) test.output
	cmp $(PROG) test.output
# These make the worker library for this application
Implementations = copy.rcc
include $(OCPI_INC_DIR)/lib.mk

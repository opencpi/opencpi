APP=fsk_modem
include ../setup.mk

OCPI_INC_DIR+=-I$(OCPI_BASE_DIR)/core/util/misc/include

run: all
	OCPI_LIBRARY_PATH=$(OCPI_BASE_DIR)/components/lib/rcc $(VG) $(PROG) $(OUT)



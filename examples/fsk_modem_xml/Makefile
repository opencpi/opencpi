APP=fsk_modem
include ../setup.mk

OCPI_INC_DIR+=-I../../runtime/util/misc/include

run: all
	$(VG) $(PROG) $(OUT)

clean::
	rm -f dataOut.dat


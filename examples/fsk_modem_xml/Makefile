APP=fsk_modem
IncludeDirs+=\
  ../../runtime/util/misc/include \
  ../../runtime/util/property/include \
  ../../runtime/util/ezxml/include \
  ../../os/interfaces/include

include ../setup.mk


run: all
	$(VG) $(PROG) -v $(OUT)

clean::
	rm -f dataOut.dat


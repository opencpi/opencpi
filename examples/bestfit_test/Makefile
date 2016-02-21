APP=bestfit
include ../opencv.mk
ifndef EXIT

run: $(PROG) all
	$(OPENCV_ENV) \
	export OCPI_LIBRARY_PATH=lib; \
	($(VG) $(PROG) 9999 .001 10 && \
	$(VG) $(PROG) 24000 .002 1000 && \
	$(VG) $(PROG) 40000 .001 100 )  > /dev/null

# These make the worker library for this application
Implementations = impl1.rcc impl2.rcc impl3.rcc
include $(OCPI_CDK_DIR)/include/lib.mk
endif

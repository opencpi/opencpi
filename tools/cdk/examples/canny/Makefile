APP=canny
include ../opencv.mk
ifndef EXIT

run: $(PROG) all
	$(OPENCV_ENV) \
	export OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/rcc; \
	export OCPI_SMB_SIZE=120000000; \
	$(VG) $(PROG) boston_small.jpg > /dev/null	

endif

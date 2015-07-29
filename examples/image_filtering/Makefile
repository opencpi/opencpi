APP=image_filtering
include ../opencv.mk
ifndef EXIT
OUT= > /dev/null
run: $(PROG)
	$(OPENCV_ENV) \
	export OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components; \
	export OCPI_SMB_SIZE=100000000; \
	$(VG) $(PROG)  boston_small.jpg ocpi.erode $(OUT)

endif

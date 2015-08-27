APP=optical_flow
include ../opencv.mk
ifndef EXIT
OUT= > /dev/null	
run: $(PROG)
	$(OPENCV_ENV) \
	export OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components; \
	export OCPI_SMB_SIZE=600000000; \
	$(VG) $(PROG) uavA.jpg uavB.jpg  $(OUT)

endif

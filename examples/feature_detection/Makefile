APP=feature_detection
include ../opencv.mk
ifndef EXIT
run: $(PROG)
	$(OPENCV_ENV) \
	export OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components; \
	export OCPI_SMB_SIZE=100000000; \
	$(VG) $(PROG)  boston_small.jpg  > /dev/null	

clean::
	rm -f -r feature_detection *.dSYM *.*~ *~ output_image.jpg *.o *.d

endif

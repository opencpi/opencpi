ifndef OCPI_OPENCV_HOME
$(info Set OCPI_OPENCV_HOME to the OpenCV install location to use this example (e.g. /usr/local))
$(info OpenCV not available.)
all:
run:
clean:
else

ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard ../../export),../..)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

OPENCV_LIBS=opencv_core opencv_highgui opencv_ml opencv_objdetect
OPENCV_LIB_DIR=$(OCPI_OPENCV_HOME)/lib

LDFLAGS=$(OCPI_LD_FLAGS) \
        -Xlinker -rpath -Xlinker $(OPENCV_LIB_DIR) -L$(OPENCV_LIB_DIR) $(OPENCV_LIBS:%=-l%)

INCS += -I$(OCPI_INC_DIR) -I$(OCPI_OPENCV_HOME)/include/opencv

all: image_filtering

image_filtering: image_filtering.cxx
	g++ -g -o $@ $(INCS) $^ $(LDFLAGS)

run: image_filtering
	export OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components; \
	export OCPI_SMB_SIZE=100000000; \
	./image_filtering  boston_small.jpg erode > /dev/null	

clean::
	rm -f -r image_filtering *.dSYM *.*~ *~ output_image.jpg *.o *.d

endif
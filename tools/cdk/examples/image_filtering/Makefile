ifndef OCPI_HAVE_OPENCV
OCPI_HAVE_OPENCV=0
endif

ifndef OCPI_CDK_DIR
OCPI_CDK_DIR=../..
endif


ifeq ($(OCPI_HAVE_OPENCV),0)
f1::
	echo This demo application requires OpenCV \
	If OpenCV is already installed, set OCPI_HAVE_OPENCV=1 and OCPI_OPENCV_HOME
else
ifndef OCPI_OPENCV_HOME
OCPI_OPENCV_HOME=/usr/local
endif
endif


LIBDIR=$(OCPI_CDK_DIR)/lib/$(OcpiHostTarget)
LIBS=interfaces rcc_container rdma_drivers opencv_core opencv_highgui opencv_ml util
LIB_PATH=export $(OcpiLibraryPathEnv)=$(LIBDIR):$(OCPI_OPENCV_HOME)/lib; export OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/../components/lib; 
INCS += -I$(OCPI_OPENCV_HOME)/include/opencv
LIBRARY_PATH 	+= -L$(OCPI_OPENCV_HOME)/lib -L$(OCPI_BASE_DIR)/lib

all: image_filtering

image_filtering: image_filtering.cxx
	$(LIB_PATH) g++ -g -o $@ $(INCS) -I$(OCPI_CDK_DIR)/include $^ -L$(LIBDIR) $(LIBS:%=-l%)

run: image_filtering
	$(LIB_PATH) \
	export OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components; \
	export OCPI_SMB_SIZE=40000000; \
	./image_filtering  boston_small.jpg erode > /dev/null	

clean::
	rm -f image_filtering *.*~ *~ output_image.jpg *.o *.d


# These make the worker library for this application
#Implementations = hello.rcc
include $(OCPI_CDK_DIR)/include/lib.mk

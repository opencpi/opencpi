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


LIBDIR=$(OCPI_CDK_DIR)/lib/$(HostTarget)
LIBS=interfaces rcc_container rdma_drivers opencv_core opencv_highgui opencv_ml
ifeq ($(OSTYPE),darwin)
LIB_PATH=export DYLD_LIBRARY_PATH=$(LIBDIR);
else
LIB_PATH=export LD_LIBRARY_PATH=$(LIBDIR);
endif

INCS += -I$(OCPI_OPENCV_HOME)/include/opencv -I../image_utils
LIBRARY_PATH 	+= -L$(OCPI_OPENCV_HOME)/lib -L$(OCPI_BASE_DIR)/lib

all: feature_detection

../image_utils/image_utils.o: ../image_utils/DemoWorkerFacade.cxx ../image_utils/DemoWorkerFacade.h
	cd ../image_utils; make

feature_detection: feature_detection.cxx ../image_utils/image_utils.o
	$(LIB_PATH) g++ -g -o $@ $(INCS) -I$(OCPI_CDK_DIR)/include $^ -L$(LIBDIR) $(LIBS:%=-l%)

run: feature_detection
	$(LIB_PATH) export OCPI_LIBRARY_PATH=lib/rcc; ./feature_detection > /dev/null	

clean::
	rm -f feature_detection *.*~ *~ output_image.jpg *.o *.d


# These make the worker library for this application
#Implementations = hello.rcc
include $(OCPI_CDK_DIR)/include/lib.mk

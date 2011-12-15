ifndef OCPI_OPENCV_HOME
$(info Set OCPI_OPENCV_HOME to the OpenCV install location to use this example (e.g. /usr/local))
$(info OpenCV not available.)
all:
run:
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

all: bestfit

bestfit: bestfit.cxx
	g++ -g -Wall -o $@ $(INCS) $^ $(LDFLAGS)

run: bestfit all
	export OCPI_LIBRARY_PATH=lib; \
	(./bestfit 9999 .001 10; \
	./bestfit 24000 .002 1000; \
	./bestfit 40000 .001 100 )  > /dev/null

clean::
	rm -f -r lib bestfit *.dSYM *.*~ *~ output_image.jpg *.o *.d


# These make the worker library for this application
Implementations = impl1.rcc impl2.rcc impl3.rcc
include $(OCPI_CDK_DIR)/include/lib.mk
endif

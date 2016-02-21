# helpers for opencv makefiles
ifndef OCPI_OPENCV_HOME
ifeq ($(filter clean%,$(MAKECMDGOALS)),)
$(info Set OCPI_OPENCV_HOME to the OpenCV install location to use this example (e.g. /usr/local))
$(info OpenCV not available.)
endif
all:
run:
clean:
EXIT=sdf
else
include ../setup.mk

OPENCV_LIBS=opencv_core opencv_highgui opencv_ml opencv_objdetect
OPENCV_LIB_DIR=$(OCPI_OPENCV_HOME)
OCPI_LD_FLAGS+= \
        -Xlinker -rpath -Xlinker $(OPENCV_LIB_DIR)/lib -L$(OPENCV_LIB_DIR)/lib $(OPENCV_LIBS:%=-l%)

INCS += -I$(OCPI_OPENCV_HOME)/include/opencv -I$(OCPI_OPENCV_HOME)/include

OPENCV_ENV= export $(OcpiLibraryPathEnv)=$(OPENCV_LIB_DIR)/lib;

endif

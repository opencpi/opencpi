# helpers for opencv makefiles
ifndef OCPI_OPENCV_HOME
$(info Set OCPI_OPENCV_HOME to the OpenCV install location to use this example (e.g. /usr/local))
$(info OpenCV not available.)
all:
run:
clean:
EXIT=sdf
else
ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard ../../export),../..)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

OPENCV_LIBS=opencv_core opencv_highgui opencv_ml opencv_objdetect
OPENCV_LIB_DIR=$(OCPI_OPENCV_HOME)
LDFLAGS=$(OCPI_LD_FLAGS) \
        -Xlinker -rpath -Xlinker $(OPENCV_LIB_DIR)/lib -L$(OPENCV_LIB_DIR)/lib $(OPENCV_LIBS:%=-l%)

INCS += -I$(OCPI_INC_DIR) -I$(OCPI_OPENCV_HOME)/include/opencv -I$(OCPI_OPENCV_HOME)/include
PROG=$(OCPI_TARGET_DIR)/$(APP)

all: $(PROG)

$(PROG): $(APP).cxx | $(OCPI_TARGET_DIR)
	g++ -g -Wall -o $@ $(INCS) $^ $(LDFLAGS)

OPENCV_ENV= export $(OcpiLibraryPathEnv)=$(OPENCV_LIB_DIR)/lib;
clean::
	rm -r -f lib target-* *.*~ timeData.raw output_image.jpg
endif

include $(OCPI_CDK_DIR)/include/util.mk
APP:=$(CwdName)
$(call OcpiIncludeProject)

ifneq ($(filter clean%,$(MAKECMDGOALS)),)
  include $(OCPI_CDK_DIR)/include/ocpisetup.mk
endif
APPCC:=$(wildcard $(APP).cxx)$(wildcard $(APP).cc)
ifdef APPCC
  PROG=$(DIR)/$(APP)
  DIR=target-$(OCPI_TARGET_DIR)
else ifneq ($(wildcard $(APP).xml),)
  PROG=$(APP).xml
else
  $(info No application found looking for: $(APP).xml $(APP).cc $(APP).cxx)
endif
OUT= > /dev/null

INCS = -I$(OCPI_INC_DIR)

all: $(PROG)

$(DIR):
	mkdir -p $(DIR)

ifdef APPCC
$(PROG): $(APPCC) | $(DIR)
	$(AT)echo Building $@...
	$(AT)$(CXX) -g -Wall $(OCPI_EXPORT_DYNAMIC) -o $@ $(INCS) $^ $(OCPI_LD_FLAGS)
endif

run: all
	$(AT)if [ -f $(APP).cc -o -f $(APP).cxx ]; then \
	  OCPI_LIBRARY_PATH=lib $(VG) $(PROG) $(RunArgs) $(OUT) ; \
        elif [ -f $(APP).xml ]; then \
	  ocpirun $(RunArgs) $(APP).xml; \
	fi

clean::
	$(AT)rm -r -f lib target-* *.*~ timeData.raw output_image.jpg test.output


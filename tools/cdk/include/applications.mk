include $(OCPI_CDK_DIR)/include/util.mk
$(call OcpiIncludeProject)
ifeq ($(origin Applications),undefined)
  Applications=$(call OcpiFindSubdirs,application)
  $(call OcpiDbgVar,Applications)
endif

.PHONY: all test clean

all:
	$(AT)set -e; for l in $(Applications); do $(MAKE) -C $$l; done

test:
	$(AT)set -e; for l in $(Applications); do $(MAKE) -C $$l test; done

clean:
	$(AT)set -e; for l in $(Applications); do $(MAKE) -C $$l clean; done

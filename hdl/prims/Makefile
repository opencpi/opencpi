OCPI_DIR=../../ocpi
include $(OCPI_DIR)/include/util.mk
Libs=bsv ocpi xfft_v7_1
all: $(Libs)

clean:
	$(AT)$(foreach l,$(Libs),$(MAKE) -C $(l) OCPI_DIR=$(call AdjustRelative,$(OCPI_DIR)) clean;)

install:
	$(AT)set -e;$(foreach l,$(Libs),$(MAKE) -C $(l) OCPI_DIR=$(call AdjustRelative,$(OCPI_DIR)) install;)

.PHONY: $(Libs)

$(Libs):
	$(AT)$(MAKE) -C $@ OCPI_DIR=$(call AdjustRelative,$(OCPI_DIR))


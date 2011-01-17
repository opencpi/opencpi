# Make the hdl assets
.PHONY: primitives devices platforms components applications
ifndef OCPI_CDK_DIR
export OCPI_CDK_DIR=$(shell pwd)/../ocpi
$(info Setting OCPI_CDK_DIR to $(OCPI_CDK_DIR))
endif
applications: platforms components
	$(AT)$(MAKE) -C applications

components: primitives
	$(AT)$(MAKE) -C ../components hdl

devices: primitives
	$(AT)$(MAKE) -C devices

platforms: primitives devices
	$(AT)$(MAKE) -C platforms

primitives:
	$(AT)$(MAKE) -C primitives

clean:
	$(AT)for i in primitives devices platforms applications; do \
		$(MAKE) -C $$i clean; \
		done
	$(AT)$(MAKE) -C ../components cleanhdl

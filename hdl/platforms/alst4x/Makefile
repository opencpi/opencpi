# The alst4x platform, mostly a clone of the alst4 platform
Libraries=stratix4
Cores=pcie_4243_hip_s4gx_gen2_x4_128
ExportFiles=alst4x.qsf alst4x.sdc jtagSupport_alst4x loadFlash_alst4x testbias_alst4x_base.bitz
Configurations=\
  base $(basename $(notdir $(subst /alst4,/alst4x,$(wildcard ../alst4/alst4_zipper*.xml))))
# FIXME: we need a better way to "clone" a platform
# Unfortunately, the xml file must exist prior to dependencies, so we just do them all immediately
ifeq ($(filter clean%,$(MAKECMDGOALS)),)
  $(shell \
    test -e alst4x.xml || sed "s/'alst4'/'alst4x'/" ../alst4/alst4.xml > alst4x.xml; \
    test -e alst4x.vhd || sed "s/alst4_worker/alst4x_worker/" ../alst4/alst4.vhd > alst4x.vhd; \
    test -e alst4x.sdc || ln -s ../alst4/alst4.sdc alst4x.sdc; \
    test -e alst4x.qsf || ln -s ../alst4/alst4.qsf alst4x.qsf; \
    test -e jtagSupport_alst4x || ln -s ../alst4/jtagSupport_alst4 jtagSupport_alst4x; \
    mkdir -p gen; \
    for c in $(filter-out base,$(Configurations)); do \
      test -e gen/$$c.xml || ln -s ../../alst4/$${c//alst4x/alst4}.xml gen/$$c.xml; \
    done)
endif

include $(OCPI_CDK_DIR)/include/hdl/hdl-platform.mk

# This is STILL needed even though we make this file above, since gnumake caches the contents
# of the current working directory at startup. 
alst4x.xml: ../alst4/alst4.xml; sed "s/'alst4'/'alst4x'/" $< > $@

clean::
	rm -f alst4x.{xml,vhd,sdc,qsf} jtagSupport_alst4x

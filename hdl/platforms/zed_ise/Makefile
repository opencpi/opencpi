Libraries=axi zynq sdp
ParamValues_useGP1=0/1
Configurations=\
  base \
  gp1 \
  zed_zipper_fmc_rx \
  zed_zipper_fmc_tx \
  zed_zipper_fmc_rx_tx
ExportFiles=zed_ise.ucf zed_ise.ut zed_ise.mk testbias_zed_ise_base.bitz

ifeq ($(filter clean%,$(MAKECMDGOALS)),)
  $(shell \
    test -e zed_ise.xml || sed "s/'zed'/'zed_ise'/" ../zed/zed.xml > zed_ise.xml; \
    test -e zed_ise.vhd || sed "s/zed_worker/zed_ise_worker/;" ../zed/zed.vhd > zed_ise.vhd; \
    mkdir -p gen; \
    for c in $(filter-out base,$(Configurations)); do \
      test -e gen/$$c.xml || ln -s ../../zed/$${c//zed_ise/zed}.xml gen/$$c.xml; \
    done)
endif


include $(OCPI_CDK_DIR)/include/hdl/hdl-platform.mk

zed_ise.xml: ../zed/zed.xml; sed "s/'zed'/'zed_ise'/" $< > $@

clean::
	rm -f zed_ise.{xml,vhd}

# The isim_pf platform.
HdlLibraries+=sdp
ExportFiles=runSimExec.isim probe.exe.tgz
include $(OCPI_CDK_DIR)/include/hdl/hdl-platform.mk

ifneq ($(findstring isim,$(HdlPlatforms)),)
# Make the minimal simulation executable to run to verify licensing
# since the xlicmgr tool is useless.  grrrr.
probe.exe.tgz:
	$(AT)echo Creating the tiny isim project for probing that isim is installed and licensed.
	$(AT)echo Output is in target-isim/probe.out
	$(AT)mkdir -p target-isim
	$(AT)(rm -r -f tmp && mkdir tmp && cd tmp && \
	  source $(OcpiXilinxIseDir)/settings64.sh && \
	  export LM_LICENSE_FILE=$(OcpiXilinxLicenseFile) && \
	  echo 'module probe(); endmodule' > probe.v && \
	  vlogcomp -v 2 -work probe=probe probe.v && \
	  fuse probe.probe -o probe.exe -lib probe=probe && \
          find . \( -name "*.[co]" -o -name "*.log" -o -name "*.xmsgs" -o -name "*.cmd" -o \
                    -name "*.html" \) -delete && \
	  rm -f -r probe probe.v */*/work && \
	  tar czf ../probe.exe.tgz probe.exe isim) > target-isim/probe.out 2>&1

#	  fuse probe.probe -v 2 -L unisims_ver -o probe.exe -lib probe=probe && \

all: probe.exe.tgz

clean::
	rm -r -f tmp probe.exe.tgz

endif

# The isim platform.
HdlLibraries+=sdp
ExportFiles=runSimExec.isim probe.exe.tgz
include $(OCPI_CDK_DIR)/include/hdl/hdl-platform.mk

ifneq ($(findstring isim,$(HdlPlatforms)),)
# Make the minimal simulation executable to run to verify licensing
# since the xlicmgr tool is useless.  grrrr.
# Also note that this result is put in the repo and not cleaned so that ocpirun -C shows it
# without building HDL.
# Also note that is must be built on centos6 for the same thing to be usable on both c6 and c7

probe.exe.tgz:
	$(AT)echo Creating the tiny isim project for probing that isim is installed and licensed.
	$(AT)echo Log output for this is in target-isim/probe.out
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
	  for i in `find . -name "*.exe"`; do ../fixabs.sh $$i || exit 1; done && \
	  tar czf ../probe.exe.tgz probe.exe isim) > target-isim/probe.out 2>&1 

#	  fuse probe.probe -v 2 -L unisims_ver -o probe.exe -lib probe=probe && \

all: probe.exe.tgz

clean::
	rm -r -f tmp

endif

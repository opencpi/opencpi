# The isim_pf platform.
HdlLibraries+=sdp
ExportFiles=runSimExec.isim probe.exe.tgz
include $(OCPI_CDK_DIR)/include/hdl/hdl-platform.mk

# Make the minimal simulation executable to run to verify licensing
# since the xlicmgr tool is useless.  grrrr.
probe.exe.tgz:
	( rm -r -f tmp && mkdir tmp && cd tmp && \
	  source $$OCPI_XILINX_TOOLS_DIR/settings64.sh && \
	  export LM_LICENSE_FILE=$$OCPI_XILINX_LICENSE_FILE && \
	  echo 'module probe(); endmodule' > probe.v && \
	  vlogcomp -v 2 -work probe=probe probe.v && \
	  fuse probe.probe -o probe.exe -lib probe=probe && \
          find . \( -name "*.[co]" -o -name "*.log" -o -name "*.xmsgs" -o -name "*.cmd" -o \
                    -name "*.html" \) -delete && \
	  rm -f -r probe probe.v */*/work && \
	  tar czf ../probe.exe.tgz probe.exe isim )

#	  fuse probe.probe -v 2 -L unisims_ver -o probe.exe -lib probe=probe && \

all: probe.exe.tgz

clean::
	rm -r -f tmp probe.exe.tgz


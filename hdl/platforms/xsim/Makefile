# The xsim platform.
HdlLibraries+=sdp
ExportFiles=runSimExec.xsim probe.exe.tgz
include $(OCPI_CDK_DIR)/include/hdl/hdl-platform.mk

ifneq ($(findstring xsim,$(HdlPlatforms)),)
probe.exe.tgz:
	$(AT)echo Creating the tiny xsim project for probing that xsim is installed and licensed.
	$(AT)echo Output is in target-xsim/probe.out
	$(AT)mkdir -p target-xsim
	$(AT)(rm -r -f tmp && mkdir -p tmp/probe && cd tmp && \
	  source $(OcpiXilinxVivadoDir)/.settings64-Vivado.sh && \
	  export LM_LICENSE_FILE=$(OcpiXilinxVivadoLicenseFile) && \
	  echo 'module probe(); endmodule' > probe.v && \
	  xvlog -v 2 probe.v && \
          xelab -debug typical -top probe -s probe.exe -verbose && \
	  rm -f -r probe.v */work && \
	  tar czf ../probe.exe.tgz xsim.dir/probe.exe) > target-xsim/probe.out 2>&1

all: probe.exe.tgz

clean::
	rm -r -f tmp
endif

# This is a top level makefile used when an HDL assembly is being built.
# It is executed by a local sub-make in the same directory to create a workers file in the
# gen subdirectory.  It generates the output that is used by the parent
# Makefile to establish dependencies of assemblies on their constituent workers.
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk
all: $(AssyWorkersFile)
-include $(AssyWorkersFile).deps
$(AssyWorkersFile): $(Worker_xml) | $(GeneratedDir)
	$(AT)$(OcpiGen) -D $(GeneratedDir)\
                        $(and $(Platform),-P $(Platform)) $(and $(Assembly),-S $(Assembly))\
                        -W $(Worker) $<



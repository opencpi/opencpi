ifndef Target
$(error The "Target" variable has not been set.)
endif
include $(OCPI_DIR)/include/hdl/hdl.mk
# Makefile fragment for HDL workers

ImplSuffix=_impl.v
SkelSuffix=_skel.v
DefsSuffix=_defs.v
# BF:=.edif
# LinkBinary=cp $< $@
# Compile should ultimately be compile for synthesis, not simulation
# and we should have the separate sim build.
# for now this is indeed compile for sim, but really just a syntax check
#Compile=iverilog -Wall $(IncludeDirs:%=-I%) $(IncludeDirs:%=-y%) $(VerilogIncludeDirs:%=-I%) -o $@ $<
# Issues:
# Compile a library of primitives
# Generate a stub with outputs tied.
# Build the component, referring to library for primtives lile BSV.
# lso says which libraries to search, and the lso mapping mentiones remote dirs for libraries.
# xst (-ifn foo.xst) 

# HDL workers are one per binary
ImplXmlFile=$(firstword $(ImplXmlFiles))

DefsFiles=$(foreach w,$(Workers),$(GeneratedDir)/$(w)$(DefsSuffix))
override VerilogIncludeDirs += $(IncludeDirs)

ifeq ($(HostTarget),Darwin-i386)
OBJ=.o
Compile=iverilog -Wall $(VerilogIncludeDirs:%=-I%) $(OcpiLibraries:%=-y%/lib/hdl) -o $@ $<
else
ifndef Core
Core=$(Worker)
endif
include $(OCPI_DIR)/include/hdl/xst.mk
endif
include $(OCPI_DIR)/include/xxx-worker.mk

ArtifactFile=
$(BinaryFile): $(AuthoredSourceFiles) $(GeneratedSourceFiles) $(DefsFiles) $(ImplHeaderFiles)
	$(Compile)

ifdef LibDir

ifndef Application
all: $(LibDir)/$(ImplXmlFile)

$(LibDir)/$(ImplXmlFile):
	$(AT)echo Creating link from $(LibDir) to $(ImplXmlFile) to expose the $(CwdName) xml.
	$(AT)$(call MakeSymLink,$(ImplXmlFile),$(LibDir))

endif
endif

# Generate the stub files
ifdef GenDir
ifndef Application
$(GenDir)/$(Worker)$(SourceSuffix): $(DefsFiles)
	$(AT)echo Creating link from $(GenDir) to $(DefsFiles) to expose the stub for worker "$(Worker)".
	$(AT)$(call MakeSymLink2,$(DefsFiles),$(GenDir),$(Worker)$(SourceSuffix))

all: $(GenDir)/$(Worker)$(SourceSuffix)

endif
endif


$(patsubst %,$(TargetDir)/%$(OBJ),$(basename $(notdir $(AuthoredSourceFiles)))): \
	 $(TargetDir)/%$(OBJ): %.v

$(DefsFiles): $(GeneratedDir)/%$(DefsSuffix): %.xml | $(GeneratedDir)
	$(AT)echo Generating the definition file: $@
	$(AT)$(OcpiGen) -d $<

$(foreach w,$(Workers),$(TargetDir)/$(w)$(OBJ)): $(TargetDir)/%$(OBJ): $(GeneratedDir)/%$(DefsSuffix) $(Libraries)



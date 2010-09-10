# This file is for building a core, which will also build the library for it...
# The core must 
include $(OCPI_DIR)/include/util.mk
ifndef Core
Core=$(CwdName)
$(info Core name "$(Core)" inferred from directory name.)
endif

ifdef CoreBlackBoxFile
ifeq ($(realpath $(CoreBlackBoxFile)),)
$(error CoreBlackBoxFile "$(CoreBlackBoxFile)" does not exist)
endif
else
$(error Variable "CoreBlackBoxFile" for $(Core) must be defined)
endif
ifndef Target
$(errr Variable \"Target\" for $(Core) must be defined)
endif

LibName=$(CwdName)
include $(OCPI_DIR)/include/hdl/hdl.mk
# Avoid compiling the black box file.
SourceFiles:=$(filter-out $(CoreBlackBoxFile),$(SourceFiles))
AuthoredSourceFiles=$(sort $(SourceFiles))

# More explicitly build the obj file, not as a side effect of the library

define DoCoreTarget

# The stub library
LibResults+=$(OutDir)$(call LibraryAccessTarget,$(1))/$(LibName)/$(call LibraryFileTarget,$(1))
$(OutDir)$(call LibraryAccessTarget,$(1))/$(LibName)/$(call LibraryFileTarget,$(1)): TargetDir=$(call LibraryAccessTarget,$(1))
$(OutDir)$(call LibraryAccessTarget,$(1))/$(LibName)/$(call LibraryFileTarget,$(1)): Target=$(call LibraryAccessTarget,$(1))
$(OutDir)$(call LibraryAccessTarget,$(1))/$(LibName)/$(call LibraryFileTarget,$(1)): SourceFiles=$(CoreBlackBoxFile) 
$(OutDir)$(call LibraryAccessTarget,$(1))/$(LibName)/$(call LibraryFileTarget,$(1)): | $(OutDir)$(call LibraryAccessTarget,$(1))
$(OutDir)$(call LibraryAccessTarget,$(1)): | $(OutDir)
	$(AT)mkdir $$@


CoreResults+=$(OutDir)$(1)/$(Core)$(BF)
$(OutDir)$(1)/$(Core)$(SourceSuffix): Target=$(1)
$(OutDir)$(1)/$(Core)$(BF): Target=$(1)
$(OutDir)$(1)/$(Core)$(BF): TargetDir=$(1)
$(OutDir)$(1)/$(Core)$(BF): $(SourceFiles) | $(OutDir)$(1)
$(OutDir)$(1): | $(OutDir)
	$(AT)mkdir $$@
endef

$(foreach t,$(Targets),$(eval $(call DoCoreTarget,$(t))))

ifdef DontBuildCores
$(CoreResults):
	$(AT)echo Exporting pre-built coregen NGC file $(Core).ngc for target \"$(Target)\"
	cd $(Target); rm -f $(Core).ngc; ln -s ../$(Core)-$(Target).ngc $(Core).ngc
else
$(CoreResults):
	$(AT)echo Building core \"$(Core)\" for target \"$(Target)\"
	$(Compile)
endif

#	$(MAKE) -f $(OCPI_DIR)/include/hdl/hdl-lib.mk \
#		CompiledSourceFiles="$(CoreBlackBoxFile) $(OCPI_DIR)/include/hdl/onewire.v"\
#		OCPI_DIR=$(OCPI_DIR)

$(LibResults): $(CoreBlackBoxFile)
	$(AT)echo Building core \"$(Core)\" stub/blackbox library for target \"$(Target)\" from \"$(CoreBlackBoxFile)\"
	$(Compile)

#$(CoreBBs): $(CoreResults)
#	$(AT)echo Creating link to $(CoreBlackBoxFile) to expose the black box file for core "$(Core)".
#	$(AT)$(call MakeSymLink2,$(CoreBlackBoxFile),$(Target),$(Core)$(SourceSuffix))

# Create stub library after core is built
#$(LibBBs): $(CoreResults)
#	$(AT)echo Building core $(Core) for $(Target)
#	$(Compile)

all: $(CoreResults) $(LibResults)

clean:
	rm -r -f $(foreach t,$(Targets),$(OutDir)$(t) $(OutDir)$(call LibraryAccessTarget,$(t)))

# Need to install both the stub library and the core itself
install: | $(InstallDir)
	$(AT)echo Installing stub library for core $(Core) for targets: $(Targets)
	$(AT)echo Family targets: $(foreach t,$(Targets),$(call LibraryAccessTarget,$(t)))
	$(foreach f,$(sort $(foreach t,$(Targets),$(call LibraryAccessTarget,$(t)))),\
		rm -r -f $(InstallDir)/$(f);\
		mkdir $(InstallDir)/$(f);\
		cp -r -p $(OutDir)$(f)/$(LibName)/* $(InstallDir)/$(f);)
	$(AT)echo Installing core file for core $(Core)
	$(foreach t,$(Targets),\
		rm -r -f $(InstallDir)/$(t);\
		mkdir $(InstallDir)/$(t);\
		cp -r -p $(OutDir)$(t)/$(Core)$(BF) $(InstallDir)/$(t);)

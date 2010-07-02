# Define the SourceFiles variable in a directory full of sources.
# This makefile will create precompiled libraries in both old (pre-v6) and new (v6+)
# formats.  If this library is named "foo", then the created libraries will be in $(OutDir)virtex5/foo
# and $(OutDir)virtex6/foo.  Then "make install" will install them in $(InstallDir)/lib/hdl/foo/virtex{5,6}
# "Family" is used here when it isn't quite accurate.  We are really talking about pre-v6 and v6-post.
# This file is xst-specific.  It should be split so the XST-specifics are elsewhere.
ifndef LibName
LibName=$(notdir $(abspath .))
endif
include $(OCPI_DIR)/include/hdl/hdl.mk
Core=onewire
override SourceFiles+=$(OCPI_DIR)/include/hdl/onewire.v
AuthoredSourceFiles=$(sort $(SourceFiles))
WorkLibrarySources+=$(OCPI_DIR)/include/hdl/onewire.v
ifndef Targets
# These libraries need to be built for "families", which really just means that the format of the library
# depends on the target to some extent.
Targets=virtex5 virtex6
endif

# all the per-family stuff
define DoLibraryTarget
OutLibFiles+=$(OutDir)$(1)/$(LibName)/$(call LibraryFileTarget,$(1))

$(OutDir)$(1)/$(LibName)/$(call LibraryFileTarget,$(1)): TargetDir=$(1)
$(OutDir)$(1)/$(LibName)/$(call LibraryFileTarget,$(1)): Target=$(1)
$(OutDir)$(1)/$(LibName)/$(call LibraryFileTarget,$(1)): SourceFiles+=$(SourceFiles_$(1))
$(OutDir)$(1)/$(LibName)/$(call LibraryFileTarget,$(1)): $(SourceFiles) $(SourceFiles_$(1)) | $(OutDir)$(1)

$(OutDir)$(1): | $(OutDir)
	$(AT)mkdir $$@

endef

$(foreach t,$(Targets),$(eval $(call DoLibraryTarget,$(t))))

$(OutLibFiles):
	$(AT)echo Building the $(LibName) library for $(Target)
	$(Compile)

all: $(OutLibFiles)

#	$(AT)(for i in $(SourceFiles);\
#		do echo verilog $(LibName) $(call FindRelative,$(OutDir)$(Family),.)/$$i;\
#	      done;\
#	      echo verilog work $(call FindRelative,$(OutDir)$(Family),$(OCPI_DIR)/include/hdl)/onewire.v)\
#	         >$(OutDir)$(Family)/bsv.prj
#	$(AT)echo -e set -xsthdpdir . \\n $(XstCmd) -ifn bsv.prj > $(OutDir)$(Family)/bsv.xst
#	cd $(OutDir)$(Family); $(TIME) xst -ifn bsv.xst > xst.out; grep -i error xst.out

install: | $(InstallDir)
	rm -f -r $(foreach f,$(Targets),$(foreach t,$(call FamilyCleanTargets,$(f)),$(InstallDir)/$(f)/$(t)))
	$(foreach f,$(Targets),if ! test -d $(InstallDir)/$(f); then mkdir $(InstallDir)/$(f); fi;\
		cp -r -p $(OutDir)$(f)/$(LibName)/* $(InstallDir)/$(f);)

clean:
	rm -r -f $(foreach f,$(Targets),$(OutDir)$(f))

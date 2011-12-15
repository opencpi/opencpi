# This file is bypassed if imports are already present.
# Thus imports only happen after: make cleanimports
ifdef Imports
ImportsDir:=$(OutDir)imports
#$(info WILD=$(wildcard $(ImportsDir)/*)=)

ifneq ($(if $(wildcard $(ImportsDir)/*),$(DIFF_IMPORTS),),)

ifndef OCPI_HDL_IMPORTS_DIR
$(error This primitive requires OCPI_HDL_IMPORTS_DIR to have a value, and it doesn't)
endif
.SECONDEXPANSION:

WImports:=$(wildcard $(Imports))
#$(info x$(WImports)y)
ifeq ($(strip $(WImports)),)
   $(error Wildcard imports set by the "Imports" variable did not expand to anything)
endif
$(foreach x,$(Imports),$(if $(wildcard $(x)),,$(error No files matching import name: "$(x)")))
$(foreach x,\
          $(ExcludeImports),\
	  $(if $(filter $(x),$(WImports) $(notdir $(WImports))),,\
	     $(error Excluded file $(x) is not in the Imports list)))
$(foreach i,\
	  $(WImports),\
	  $(if $(filter $(ExcludeImports) $(notdir $(ExcludeImports)),$(i)),,\
	    $(if $(realpath $(i)),,$(error Imported file $(i) does not exist.))))
NetImports:=\
  $(foreach i,\
	    $(WImports),\
            $(if $(filter $(ExcludeImports),$(i))$(filter $(notdir $(ExcludeImports)),$(notdir $(i))),,$(i)))
NetNames:=$(notdir $(NetImports))
$(foreach n,$(NetNames),$(if $(word 2,$(filter $(n),$(NetNames))),$(error Imported file $(n) is duplicated)))
#$(info im $(Imports))
#$(info wi $(WImports))
#$(info nn $(NetNames))
#$(info ni $(NetImports))
ifdef DIFF_IMPORTS #1
diff:
	$(AT) echo ========Comparing imports for the $(LibName) $(CwdName) primitive library.
	$(AT) for i in $(NetImports); do $(ECHO) Comparing $$i...; diff $$i $(OutDir)imports; done
else
out:=$(strip $(shell echo hihi; if test -d $(ImportsDir); then \
		echo Making imports subdirectory to receive imported files for the $(LibName) primitive library. ; \
	        mkdir $(ImportsDir); \
		for i in $(NetImports); do cp $$i $(OutDir)imports; done; \
	     fi))
$(if $(out),$(info $(out)))
endif
#$(info xxx $(shell echo $(ImportsDir)/*.[vV]))
#$(info yyy $(shell ls imports))
#$(info hdl-import1 csf $(CompiledSourceFiles))
#$(info hdl-import1a sf $(wildcard $(ImportsDir)/*))
#AAA:=$(sort $(wildcard $(ImportsDir)/*) $(CompiledSourceFiles))
#$(info aaa $(AAA))
#$(info hdl-import2 csf $(CompiledSourceFiles))
#$(info sf is $(flavor CompiledSourceFiles) $(origin CompiledSourceFiles))
#$(info aaa is $(flavor AAA) $(origin AAA))
endif
CompiledSourceFiles:=$(sort $(CompiledSourceFiles) $(shell echo $(ImportsDir)/*.[vV]))
$(call OcpiDbgVar,CompiledSourceFiles)
endif

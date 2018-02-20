# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

# This file is bypassed if imports are already present.
# Thus imports only happen after: make cleanimports
ifeq ($(MAKECMDGOALS),clean)
  override Imports=
endif

ifdef Imports
ImportsDir:=$(OutDir)imports
#$(info WILD=$(wildcard $(ImportsDir)/*)=)

ifneq ($(if $(wildcard $(ImportsDir)/*),$(DIFF_IMPORTS),x),)

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
ifdef DIFF_IMPORTS
diff:
	$(AT) echo ========Comparing imports for the $(LibName) $(CwdName) primitive library.
	$(AT) for i in $(NetImports); do $(ECHO) Comparing $$i...; diff $$i $(OutDir)imports; done
else
out:=$(strip \
       $(shell \
         if ! test -d $(ImportsDir); then \
	   echo Making subdirectory to receive imported files for the $(LibName) primitive library. ; \
           mkdir $(ImportsDir); \
	   echo Copying files to the imports subdirectory for the $(LibName) primitive library.; \
	   for i in $(NetImports); do \
             cp $$i $(OutDir)imports; \
           done; \
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

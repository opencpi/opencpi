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

# This file is shared between hdl-core2.mk and hdl-lib.mk
# It is used by hdl-core2.mk when the tool can't really support core building
# and can only build an implementation library for the core.

# Refine the target list as appropriate for the tool
$(call OcpiDbgVar,LibName)
OutLibFile=\
  $(call WkrTargetDir,$1,$2)/$(call HdlToolLibraryFile,$1,$(LibName)$(if $(filter 0,$2),,_c$2))
#  $(call WkrTargetDir,$1,$2)/$(call HdlToolLibraryFile,$1,$(LibName))
define DoLibTarget
$(call OcpiDbg,OutLibFile:$(call OutLibFile,$1,$2))
OutLibFiles+=$(call OutLibFile,$1,$2)
$(call OutLibFile,$1,$2): override TargetDir:=$(call WkrTargetDir,$1,$2)
$(call OutLibFile,$1,$2): override HdlTarget:=$1
$(call OutLibFile,$1,$2): override ParamConfig:=$2
$(call OutLibFile,$1,$2): override WorkLib:=$(WorkLib)$(and $(filter-out 0,$2),_c$2)
$(call OutLibFile,$1,$2): $(call HdlTargetSrcFiles,$1,$2)
$(call OutLibFile,$1,$2): \
  HdlSources=$$(call Unique,$$(filter-out %.vh,$$(call HdlTargetSrcFiles,$1,$2) $$(call HdlShadowFiles,$1,$2)))
$(call OutLibFile,$1,$2): \
$$$$(foreach l,$$$$(HdlLibrariesInternal),$$$$(call HdlLibraryRefDir,$$$$l,$$$$(HdlTarget),,DLT))

$(call OutLibFile,$1,$2): $$$$(HdlPreCore) $$$$(HdlSources) | $$$$(TargetDir)
	$(AT)echo Building the $(LibName) $(HdlMode) for $$(HdlTarget) \($$@\) $$(ParamConfig):$$(ParamMsg)
	$(AT)$$(HdlCompile)
	$(AT)$$(HdlPost)

endef

$(foreach f,$(HdlActualTargets),\
  $(eval $(foreach c,$(ParamConfigurations),$(call DoLibTarget,$f,$c))))

# If anything changes in the imports directory, we better rebuild
ifdef Imports
$(OutLibFiles): $(ImportsDir)
endif
$(call OcpiDbgVar,OutLibFiles,Before all:)
$(call OcpiDbgVar,LibName,Before all:)

all: $(OutLibFiles)

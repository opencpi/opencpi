# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
#
#    Mercury Federal Systems, Incorporated
#    1901 South Bell Street
#    Suite 402
#    Arlington, Virginia 22202
#    United States of America
#    Telephone 703-413-0781
#    FAX 703-413-0784
#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #

# This file is shared between hdl-core2.mk and hdl-lib.mk
# It is used by hdl-core2.mk when the tool can't really support core building
# and can only build an implementation library for the core.

# Refine the target list as appropriate for the tool
$(call OcpiDbgVar,LibName)
OutLibFile=$(OutDir)target-$(1)/$(call HdlToolLibraryFile,$(1),$(LibName))

define DoLibTarget
OutLibFiles+=$(call OutLibFile,$(1))
$(call OutLibFile,$(1)): override TargetDir:=$(OutDir)target-$(1)
$(call OutLibFile,$(1)): override HdlTarget:=$(1)
$(call OutLibFile,$(1)): HdlSources=$$(filter-out $$(CoreBlackBoxFile),$$(CompiledSourceFiles))
#$(call OutLibFile,$(1)): $$$$(filter-out $$$$(CoreBlackBoxFile),$$$$(CompiledSourceFiles)) | $$$$(TargetDir)

$(call OutLibFile,$(1)): \
$$$$(foreach l,$$$$(HdlLibrariesInternal),$$$$(call HdlLibraryRefDir,$$$$l,$$$$(HdlTarget)))


$(call OutLibFile,$(1)): $$$$(HdlPreCore) $$$$(HdlSources) | $$$$(TargetDir)
	$(AT)echo Building the $(LibName) $(HdlMode) for $$(HdlTarget) \($$@\) 
	$(AT)$$(HdlCompile)

#	$(AT)echo Building the $(LibName) library for $$(HdlTarget) \($$@\) cb $(CoreBlackBoxFile) hdls $$(HdlSources) deps $$^
endef
$(foreach f,$(HdlActualTargets),$(eval $(call DoLibTarget,$(f))))

# If anything changes in the imports directory, we better rebuild
ifdef Imports
$(OutLibFiles): $(ImportsDir)
endif
$(call OcpiDbgVar,OutLibFiles,Before all:)
$(call OcpiDbgVar,LibName,Before all:)

all: $(OutLibFiles)


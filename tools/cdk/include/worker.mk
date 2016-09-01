
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

# Generic top level worker makefile
include $(OCPI_CDK_DIR)/include/util.mk
$(OcpiIncludeProject)
$(call OcpiIncludeLibrary,..)
ifndef Package
  Package:= local
endif
ifndef Model
  $(error This directory named $(CwdName) does not end in any of: $(Models))
endif
ifdef Worker
  ifdef Workers
    $(error Cannot set both Worker and Workers variables in Makefile)
  else
    override Workers:=$(Worker)
  endif
else ifeq ($(origin Workers),file)
  override Worker:=$(firstword $(Workers))
else
  override Worker:=$(CwdName)
  override Workers:=$(Worker)
endif
$(call OcpiDbgVar,Worker)
$(call OcpiDbgVar,Workers)
# Now we must figure out the language since so many other things depend on it.
# But we want to keep the language in the XML and not in the Makefile
ImplXmlFiles:=$(Workers:%=%.xml)
$(foreach w,$(Workers),$(eval Worker_$w_xml:=$w.xml))

ifeq ($(MAKECMDGOALS),clean)
# Clean default OWD files when they are the default
$(foreach w,$(Workers),\
  $(if $(wildcard $(Worker_$w_xml)),\
    $(and $(call OcpiDefaultSpec,$w),$(shell if echo "$(call OcpiDefaultOWD,$w,$(Model))" | cmp -s - $(Worker_$w_xml); \
                  then echo hi; fi),\
	$(info Removing $(Worker_$w_xml) since it has default contents.) \
        $(shell rm $(Worker_$w_xml)))))
else
# Create default OWD files when they don't exist
$(foreach w,$(Workers),\
     $(if $(wildcard $(Worker_$w_xml)),,\
	 $(and \
            $(info Creating a default OWD in $(Worker_$w_xml) since it doesn't exist.)\
            $(shell echo "$(call OcpiDefaultOWD,$w,$(Model))" > $(Worker_$w_xml)),\
          )))
endif

$(eval $(call OcpiSetLanguage,$(ImplXmlFiles)))
$(call OcpiDbgVar,Workers)
$(call OcpiDbgVar,Worker)
include $(OCPI_CDK_DIR)/include/$(Model)/$(Model)-worker.mk

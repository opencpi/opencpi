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

# This file is for an application directory whose name is the name of the app.
#
# The "app" is either the foo.{cc,cxx,cpp} or foo.xml

include $(OCPI_CDK_DIR)/include/util.mk
$(eval $(OcpiEnsureToolPlatform))
# Allow old-style "APP" to rename
ifdef APP
OcpiApp:=$(APP)
endif
ifndef OcpiApp
OcpiApp:=$(CwdName)
endif
$(call OcpiIncludeProject)

# The existence of a C++ app file determines if this is an ACI app
OcpiAppCC:=$(strip $(foreach s,cc cxx cpp,$(wildcard $(OcpiApp).$s)))
ifdef OcpiAppCC
  OcpiApps:=$(OcpiApp)
  include $(OCPI_CDK_DIR)/include/aci.mk
  # If we are running in this Makefile, then we are running the TOOL_PLATFORM
  ifndef OcpiRunCC
    OcpiRunCC=$(OcpiRunBefore) $(call AciExe,$(OCPI_TOOL_PLATFORM),$(OcpiApp)) $(OcpiRunArgs) \
              $(OcpiRunAfter)
  endif
  all: aciapps
  ifndef OcpiAppNoRun
    run: all
	$(AT)echo Executing the $(OcpiApp) application.
	$(AT)$(OcpiRunCC)
  endif
else ifneq ($(wildcard $(OcpiApp).xml),)
  ifndef OcpiAppNoRun
    ifndef OcpiRunXML
      $(info OCPI_TOOL_PLATFORM:$(OCPI_TOOL_PLATFORM))
      OcpiRunXML=$(OcpiRunBefore) $(OCPI_CDK_DIR)/bin/$(OCPI_TOOL_DIR)/ocpirun $(OcpiRunArgs) $1 \
                 $(OcpiRunAfter)
    endif
    run: all
	$(AT)echo Executing the $(OcpiApp).xml application.
	$(AT)$(call OcpiRunXML,$(OcpiApp).xml)
  endif
else
  $(info No application found when looking for: $(OcpiApp).xml, $(OcpiApp).{cc,cxx,cpp})
  ifndef OcpiAppNoRun
    run:
  endif
endif

clean::
	$(AT)rm -r -f *~ timeData.raw

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

# This file is included to enable building ACI apps.
# The OcpiApps variable specifies apps.
# SourceFiles are other files to be compiled and used when linking all the apps
#  (we could later put them into a library).
# App files are expected to be main programs.
# App files need to end in cc, cxx, cpp
# SourceFiles need to end in c, cc, cxx, cpp
# This file is used at least for:
# -- The core of application.mk when the app is C++ (not XML-only).
# -- When a unit test dir needs to build C++ apps for generator/verifier/view scripts
# !!!!!!!!!!!!!! It is not yet considered "external" to be used in user Makefiles

ifndef OcpiApps
  $(error The aci.mk file included without OcpiApps being set)
endif
OcpiAppsCC:=$(foreach a,$(OcpiApps),\
              $(or $(foreach s,cc cxx cpp,$(wildcard $a.$s)), \
                   $(error No C++ file for application $a)))
# Set variables for rcc-worker.mk
OcpiBuildingACI:=1
Model:=rcc
OcpiLanguage:=c++
ifeq ($(OCPI_DYNAMIC),1)
  RccDynamicPrereqLibs+=$(OcpiPrereqLibs)
else
  RccStaticPrereqLibs+=$(OcpiPrereqLibs)
endif
include $(OCPI_CDK_DIR)/include/rcc/rcc-worker.mk
AciDir=target-$1
AciObj=$(call AciDir,$1)/$(basename $(notdir $2))$(OBJ)
AciExe=$(call AciDir,$1)/$(basename $2)

# Build a source file
# $(call DoAciSource,<platform>,<sourcefile>,<language>,<app>)
define DoAciSource
  $(call AciObj,$1,$2): RccPlatform=$1
  $(call AciObj,$1,$2): RccTarget=$(RccTarget_$1)
  $(call AciObj,$1,$2): $2 | $(call AciDir,$1)
	$(AT)echo Compiling source file: $$< for platform $1
	$(AT)$(Compile_$3)
  ifeq ($4,)
    $(foreach a,$(OcpiApps),$(call AciExe,$1,$a)): $(call AciObj,$1,$2)
  else
    $(call AciExe,$1,$4): $(call AciObj,$1,$2)
  endif
endef
# This is for dependencies of the executable on libraries
AciLib=$(foreach l,\
         $(if $(filter 1,$(call OcpiIsDynamic,$2)),$(strip\
           $(wildcard $1$(SOEXT_$(call RccOs,$2)))),$(strip\
           $(or $(wildcard $1$(AREXT_$(call RccOs,$2))),$(strip\
             $(wildcard $1$(SOEXT_$(call RccOs,$2))))))),$(infox ALr:$1:$2:$l)$l)

AciLibs=$(foreach l,$(RccLibrariesInternal) $(Libraries),\
         $(if $(findstring /,$l),\
           $(foreach p,$(dir $l)$1/lib$(notdir $l),\
             $(or $(call AciLib,$p,$1),\
               $(error No ACI library found for $l, tried $(strip\
                 $(if $(filter 1,$(call OcpiIsDynamic,$1))),\
                    $p$(SOEXT_$(call RccOs,$1)),\
                    $p$(AREXT_$(call RccOs,$1)) and $p$(SOEXT_$(call RccOs,$1)))))),\
	   $(or $(call AciLib,$(OCPI_CDK_DIR)/$1/lib/libocpi_$l,$1))))

# Build the executables
# $(call DoBuildAci,<platform>,<appfile>)
define DoBuildAci
  -include $$(call AciDir,$1)/*.deps
  $$(foreach s,$$(filter %.c,$(SourceFiles)),\
    $$(eval $$(call DoAciSource,$1,$$s,c)))
  $$(foreach s,$$(filter %.cxx %.cc %.cpp,$(SourceFiles)),\
    $$(eval $$(call DoAciSource,$1,$$s,cc)))
  $$(eval $$(call DoAciSource,$1,$2,cc,$(basename $2)))
  $(call AciExe,$1,$2): RccTarget=$(RccTarget_$1)
  $(call AciExe,$1,$2): RccPlatform=$1
  $(call AciExe,$1,$2): $$(call AciLibs,$1)
  $(call AciExe,$1,$2): | $(call AciDir,$1)
  $(call AciExe,$1,$2): $$(for s,$(SourceFiles) $2,$$(call AciObj,$1,$$s))
	$(AT)echo Creating executable for \"$(basename $(notdir $2))\" running on platform $1 from $$<
	$(AT)$$(call LinkBinary,$$^)
  aciapps: $(call AciExe,$1,$2)
endef
$(foreach p,$(RccPlatforms),$(foreach a,$(OcpiAppsCC),$(infox App=$a Platform=$p)$(eval $(call DoBuildAci,$p,$a))))

aciapps:
	$(AT)touch aciapps
clean::
	$(AT)rm -r -f target-* *~ aciapps

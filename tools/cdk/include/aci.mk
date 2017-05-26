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
AciDir=target-$(RccTarget_$1)
AciObj=$(call AciDir,$1)/$(basename $(notdir $2))$(OBJ)
AciExe=$(call AciDir,$1)/$(basename $2)
# Build a source file
# $(call DoAciSource,<platform>,<sourcefile>,<language>,<app>)
define DoAciSource
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
# $(call DoBuildAci,<platform>,<appfile>)
define DoBuildAci
  $$(foreach s,$$(filter %.c,$(SourceFiles)),\
    $$(eval $$(call DoAciSource,$1,$$s,c)))
  $$(foreach s,$$(filter %.cxx %.cc %.cpp,$(SourceFiles)),\
    $$(eval $$(call DoAciSource,$1,$$s,cc)))
  $$(eval $$(call DoAciSource,$1,$2,cc,$(basename $2)))
  $(call AciExe,$1,$2): RccTarget=$(RccTarget_$1)
  $(call AciExe,$1,$2): | $(call AciDir,$1)
  $(call AciExe,$1,$2): $$(for s,$(SourceFiles) $2,$$(call AciObj,$1,$$s))
	$(AT)echo Creating executable for $(OcpiApp) running on platform $1 from $$^
	$(AT)$$(call LinkBinary,$$^)
  # Does this need to be variable or is "all" always right?
  aciapps: $(call AciExe,$1,$2)
endef
$(foreach p,$(RccPlatforms),$(foreach a,$(OcpiAppsCC),$(eval $(call DoBuildAci,$p,$a))))

aciapps:
	touch aciapps
clean::
	$(AT)rm -r -f target-* *~ aciapps

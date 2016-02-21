# Do nothing about packages if we are cleaning
ifeq ($(filter clean%,$(MAKECMDGOALS)),)
  ifndef Package
    $(info The "Package" variable is not set. Assuming Package=local.)
    Package:=local
  endif
  PackageFile:=lib/package-name
  ifeq ($(call OcpiExists,lib),)
    $(shell mkdir -p lib)
  endif
  ifeq ($(call OcpiExists,$(PackageFile)),)
    $(shell echo $(Package) > $(PackageFile))
  else
    PackageName:=$(shell cat $(PackageFile))
    ifneq ($(Package),$(PackageName))
      $(error You must make clean after changing the package name.)
    endif
  endif
endif

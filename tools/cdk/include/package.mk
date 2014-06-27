ifndef Package
ifneq ($(MAKECMDGOALS),clean)
$(info The "Package" variable is not set. Assuming Package=local.)
endif
Package:=local
endif
PackageFile:=lib/package-name
ifeq ($(wildcard lib),)
$(shell mkdir -p lib)
endif
ifeq ($(wildcard $(PackageFile)),)
  $(shell echo $(Package) > $(PackageFile))
else
  PackageName:=$(shell cat $(PackageFile))
  ifneq ($(Package),$(PackageName))
    ifneq ($(MAKECMDGOALS),clean)
      $(error You must make clean after changing the package name.)
    endif
  endif
endif
clean::
	$(AT)rm -f -r lib

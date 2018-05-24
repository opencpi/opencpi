# This is the Makefile to build RPMs for OpenCPI

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

RPM_BASENAME:=opencpi

# Bare essentials check - these are things that we use in the Makefile; all
# other packages will be checked by the rpmbuild's BuildRequires flags.
# command_v returns the name of the command IF MISSING, otherwise blank. The
# redirect to /dev/null is required to force make to call a shell.
command_v=$(if $(shell command -v $1 2>/dev/null),,$1)
missing_commands:=$(strip $(foreach cmd,find gcc git locate perl rpmbuild,$(call command_v,$(cmd))))
ifneq "$(missing_commands)" ''
  $(warning Missing these commands: $(missing_commands))
ifneq "$(filter rpmbuild,$(missing_commands))" ''
  $(warning The rpmbuild command is in the rpm-build package.)
endif
  $(error Please install these basic utilities)
endif

# Find out if we are a git version branch explicitly
# (Strictly "v" followed only by numbers or '.' with NO suffix)
# This does not work on Jenkins because detached HEAD: GIT_BRANCH:=$(shell git symbolic-ref --short -q HEAD)
# GIT_BRANCH:=$(shell git ls-remote --heads origin | grep $(shell git rev-parse HEAD) | cut -d / -f 3 | sort | tail -1)
# Note: If somebody checks in between Jenkins builds, it will sometimes get "develop^2~37" etc,
# so anything like that is stripped.
# The BitBucket prefix of something like "bugfix--" is also stripped if present.
GIT_BRANCH:=$(notdir $(shell git name-rev --name-only HEAD | perl -pe 's/[~^\d]*$$//' | perl -pe 's/^.*?--//'))
GIT_VERSION:=$(shell echo $(GIT_BRANCH) | perl -ne '/^v[\.\d]+$$/ && print')
# For testing:
# GIT_VERSION:=$(shell echo v1.4.0 | perl -ne '/^v[\.\d]+$$/ && print')
RPM_RELEASE?=snapshot
# If there is a tag saying it was exported to github, e.g. "1.3.0_github_1414ef3" use that instead
# Note: This command fails on CentOS6 but no worries
COMMIT_HASH=$(shell git tag --points-at HEAD 2>/dev/null | grep github | head -n1)
ifeq "$(COMMIT_HASH)" ''
  COMMIT_HASH=$(shell git rev-list --max-count=1 HEAD)
endif

# FYI - "COMMIT_TAG" doesn't actually have commit hash any more, but wanted to minimize changes in specfiles
COMMIT_TAG=%{nil}
ifeq "$(GIT_VERSION)" ''
  # Get Jenkins number in there too
  ifneq "$(BUILD_NUMBER)" ""
    COMMIT_TAG+=_J$(BUILD_NUMBER)
  endif
  ifneq "$(GIT_BRANCH)" "undefined"
    ifneq "$(GIT_BRANCH)" "develop"
      COMMIT_TAG+=_$(shell echo $(GIT_BRANCH)|tr - _)
    endif
  endif
  # Generate an increasing number for version comparisons (since git hashes aren't)
  # Number of 6-minute increments since release in 5 digits (rolls over >1/yr)
  RELEASE_TAG=_$(shell printf %05d $(shell expr `date -u +"%s"` / 360 - 4231562))
  # When bumping this, don't miss configure.ac as well (AV-3433)
  RPM_VERSION:=1.4.0
else
  # This is a specific version branch
  RELEASE_TAG=%{nil}
  RPM_VERSION:=$(GIT_VERSION)
endif
# Let "assets" RPM get these as well. (AV-2768)
export COMMIT_HASH
export COMMIT_TAG
export RELEASE_TAG
export RPM_RELEASE
export RPM_VERSION


RPM_DRIVERNAME:=$(RPM_BASENAME)-driver

# There are various targets...
.PHONY: help
.SILENT: help

# Default
help:
	printf "This is the Makefile to build RPMs for OpenCPI\n\n"
	echo "The valid targets are:"
	echo ""
	echo " cdk                   - The CDK RPMs (base libraries and runtime)"
	echo " cdk-zynq              - The CDK RPMs (cross-compiled for Xilinx SW)"
	echo " cdk-pico              - The CDK RPMs (cross-compiled for Picoflexor SW)"
	echo " cdk-all               - The CDK RPMs (all CDKs built)"
	echo ""
	echo " driver                - The driver RPM (for RHEL hosts only)"
	echo ""
	echo " prereq                - All prereq RPMs for ALL platforms (opposite of cdk vs cdk-all!)"
	echo "                         - If cross-compilations fail, will continue!"
	echo " prereq-host           - The prereq packages for the HOST ONLY"
	echo " prereq-zynq           - The prereq packages (cross-compiled for Xilinx SW)"
	echo " prereq-pico           - The prereq packages (cross-compiled for Picoflexor SW)"
	echo "                         - The Pico packages ALSO require prereq-zynq to be built!"
	echo ""
	echo " assets                - Bundle assets for distribution"
	echo " bsp-XXX               - Bundle BSP XXX for distribution"
	echo ""
	echo " clean                 - Remove generated files from this directory"
	echo " cleanrpmbuild         - Cleans ~/rpmbuild/ (runs EVERY BUILD for consistency)"
	echo ""
	echo ""
	echo "If you are cross-compiling, you MIGHT need to set the OCPI_CROSS_BUILD_BIN_DIR"
	echo "  variable if your platforms/XXX/XXX-target.mk is not set correctly."

include myhostname.mk

.PHONY: clean cleanrpmbuild
.SILENT: clean cleanrpmbuild
.IGNORE: clean cleanrpmbuild
clean: myhostnameclean
	grep -v "^#" .gitignore | xargs -ri sh -c 'rm -rf {}'

cleanrpmbuild:
	rm -rf ~/rpmbuild/*
	mkdir -p ~/rpmbuild/SOURCES/

TAR_BASE=$(RPM_BASENAME)-$(RPM_VERSION)
PWD:=$(shell pwd)

.PHONY: cleancheck
.SILENT: cleancheck
# This target checks if you have local issues and if so, won't let you build RPMs.
SO_FOUND:=$(shell find .. -regex '.*\.so$$' | grep -v /releng/ | egrep -v '/platforms/.*/lib/' | xargs -rl dirname | sort -u)
SIM_FOUND:=$(shell find .. -name simulations -type d | xargs -rl echo | sort -u)
TGT_FOUND:=$(shell find .. -name "target-*" -type d | xargs -rl echo | sort -u)
GEN_FOUND:=$(shell find .. -type d -name gen | xargs -rl dirname | sort -u | grep -v zynq_kernel | grep -v kernel-headers | grep -v linux-xlnx)
PROJ_FOUND:=$(shell git clean -ndx ../projects | grep -v /bsps/)
ENV_VARS:=$(shell env | grep OCPI_ | sort | egrep -v '(_CDK_DIR=|_TOOL_HOST=|_LIBRARY_PATH=|_LICENSE_FILE=|_DIR=|_HOME=)' | cut -f1 -d=)
cleancheck:
	if [ -e ../Makefile ]; then \
	echo You have built in this checkout. That can have negative consequences on the RPM.; \
	echo Please run \"make -j distclean\" at the top level and try again.; \
	exit 99; \
	fi
ifneq "$(strip $(PROJ_FOUND))" ''
	echo "Found partially built or exported projects."
	git clean -ndx ../projects
	echo "Recommended solution (will remove files listed above):"
	echo 'git clean -fdx ../projects'
	exit 99
endif
ifneq "$(strip $(SO_FOUND))" ''
	echo "Found shared objects in the following directories:"
	echo "$(SO_FOUND)"
	echo "Please remove them before attempting to build an RPM."
	echo "Recommended solution:"
	echo "rm -rf $(SO_FOUND)"
	exit 99
endif
ifneq "$(strip $(SIM_FOUND))" ''
	echo "Found the following simulation directories:"
	echo "$(SIM_FOUND)"
	echo "Please remove them before attempting to build an RPM."
	echo "Recommended solution:"
	echo "rm -rf $(SIM_FOUND)"
	exit 99
endif
ifneq "$(strip $(TGT_FOUND))" ''
	echo "Found the following target-* directories:"
	echo "$(TGT_FOUND)"
	echo "Please remove them before attempting to build an RPM."
	echo "Recommended solution:"
	echo "rm -rf $(TGT_FOUND)"
	exit 99
endif
ifneq "$(strip $(GEN_FOUND))" ''
	echo "Found generated assets (gen subdirs) in some directories."
	echo "Please remove them before attempting to build an RPM."
	echo "Recommended solution:"
	echo 'for a in $(GEN_FOUND); do rm -rf $${a}/gen/; done'
	exit 99
endif
ifneq "$(OCPI_CDK_DIR)" ""
  ifneq "$(OCPI_CDK_DIR)" "/opt/opencpi/cdk"
	echo $(error OCPI_CDK_DIR="$(OCPI_CDK_DIR)" but expected blank or "/opt/opencpi/cdk")
  endif
  ifneq "$(OCPI_LIBRARY_PATH)" ""
    ifneq "$(OCPI_CDK_DIR)/lib/components" "$(OCPI_LIBRARY_PATH)"
	echo $(error OCPI_LIBRARY_PATH="$(OCPI_LIBRARY_PATH)" but expected blank or "$(OCPI_CDK_DIR)/lib/components")
    endif
  endif
endif
ifneq "$(strip $(ENV_VARS))" ''
	echo "Found illegal variables in environment."
	echo "Please 'unset' the following variables:"
	echo $(ENV_VARS)
	exit 99
endif

## TODO: Break up buildtar to allow a way to split out subdirs, e.g. can we remove runtime if building components and build
## machine already has the CDK installed?

# Note: Pattern rules can't be .SILENT
AT?=@
buildtar-%: TDIR:=/tmp/$(RPM_BASENAME)-rpmbuild-$(shell echo $$$$)
buildtar-%: TGT=$(@:buildtar-%=%)
buildtar-%: phony_explicit cleanrpmbuild
	$(AT)echo "Creating tarball... ($(TGT))"
	$(AT)rm -f $(TAR_BASE).tar
	$(AT)mkdir $(TDIR)
	$(AT)ln -s $(PWD)/../ $(TDIR)/$(TAR_BASE)
	$(AT)tar -C $(TDIR) -chf $(TAR_BASE).tar . $(TAR_EXCLUDES) \
	  --exclude-{backups,vcs} \
	  --exclude "ocpi" \
	  --exclude="releng/*tar" \
	  --exclude="releng/*rpm" \
	  --exclude="releng/*log" \
	  --exclude="releng/libmyhostname*" \
	  --exclude="releng/assets" \
	  --exclude="releng/prereq" \
	  --exclude="exports" \
	  --exclude="projects/core/exports" \
	  --exclude="projects/core/imports" \
	  --exclude="tests/*/exports" \
	  --exclude="tests/*/imports" \
	  --exclude="projects/assets" \
	  --exclude="projects/bsps" \
	  --exclude="projects/inactive" \
	  --exclude="project-registry" \
	  --exclude="target-cdk-staging" \
	  --exclude="*/tmp/*" \
	  --anchored \
	  --warning=no-file-removed || :
	@# Append the tarball with the assets applications directory needed for SD cards. The rest of assets is excluded.
	$(AT)tar -C $(TDIR) -rhf $(TAR_BASE).tar ./$(TAR_BASE)/projects/assets/applications || :
	@# Append any real directory named "ocpi", e.g. hdl/primitives/ocpi but not top-level ocpi link to tools/cdk/export
	$(AT)(cd $(TDIR) && find -L . -name ocpi -not -xtype l 2>/dev/null) | xargs -r tar -C $(TDIR) -rhf $(TAR_BASE).tar
	$(AT)cp $(TAR_BASE).tar ~/rpmbuild/SOURCES
	@# Note, we don't rm -rf and accidentally nuke the checkout!
	$(AT)rm $(TDIR)/$(TAR_BASE)
	$(AT)rmdir $(TDIR)

.PHONY: cdk cdk-all cdk-zynq cdk-pico phony_explicit prereq-host prereq-zynq prereq-pico core

phony_explicit:

.SILENT: cdk cdk-all cdk-zynq cdk-pico prereq-host prereq-zynq prereq-pico core

# Compromise for AV-3968:
core:
	echo "The Core Project is part of the CDK RPM; use 'make cdk' to generate that RPM or 'make help' for more."
	exit 99

#cdk: export OCPI_PROJECT_PATH=$(shell pwd)/../projects/core
# Some tools are temporarily needed for the HOST to cross-compile components
cdkbin-tools=ocpigen ocpirun ocpihdl ocpixml

cdk: TDIR:=/tmp/$(RPM_BASENAME)-rpmbuild-$(shell echo $$$$)
cdk: TAR_EXCLUDES= --exclude="*platforms/*/release*/*" --exclude="*platforms/*/kernel-headers/*" --exclude="*platforms/*/git/*"
cdk: cleancheck libmyhostname buildtar-base
	echo "Building standard CDK RPM..."
	set -o pipefail && $(SPOOF_HOSTNAME) rpmbuild -ba opencpi.spec \
	--define="BSP_COMMIT_HASH %{nil}" \
	--define="COMMIT_HASH $(COMMIT_HASH)" \
	--define="COMMIT_TAG $(shell echo "$(COMMIT_TAG)" | tr -d ' ')" \
	--define="IMPORT_BSP %{nil}" \
	--define="RELEASE_TAG $(RELEASE_TAG)" \
	--define="RPM_BASENAME ${RPM_BASENAME}" \
	--define="RPM_RELEASE $(RPM_RELEASE)" \
	--define="RPM_VERSION ${RPM_VERSION}" \
	--define="OCPI_TARGET_HOST $(shell OCPI_CDK_DIR= OCPI_PROJECT_PATH=$$(pwd)/../projects/core ../bootstrap/scripts/getPlatform.sh 2>/dev/null | cut -f4 -d" ")" \
	--define="OCPI_TARGET_PLATFORM $(shell OCPI_CDK_DIR= OCPI_PROJECT_PATH=$$(pwd)/../projects/core ../bootstrap/scripts/getPlatform.sh 2>/dev/null | cut -f5 -d" ")" \
	2>&1 | tee build-rpm.log
	cp -v ~/rpmbuild/RPMS/*/$(RPM_BASENAME)-* .
	mkdir $(TDIR)
	cp --target-directory=$(TDIR) $(foreach t,$(cdkbin-tools),~/rpmbuild/BUILD/$(RPM_BASENAME)-$(RPM_VERSION)/target-cdk-staging/bin/linux-*/$t)
	tar -C $(TDIR) -cf cdkbin.tar $(cdkbin-tools)
	$(foreach t,$(cdkbin-tools),rm $(TDIR)/$t &&) true
	rmdir $(TDIR)

# Under Jenkins, we want to package assets as well automagically (AV-2469)
ifneq "$(BUILD_NUMBER)" ""
JASSETS=assets
endif
cdk-all: cdk cdk-zynq cdk-pico $(JASSETS)

# For the OCPI_TARGET_CFLAGS, start with the ones in /usr/lib/rpm/redhat/macros for %__global_cflags and then take out the ones that fail on the target platform.
# Then add the ones that tune for the platform, starting with platforms/xxx/xxx-target.mk.
# Then feed them ALL back into platforms/xxx/xxx-target.mk which is what we use (as of AV-812, AV-2088)

# NOTE: This syntax BREAKS on CentOS 7 if you attempt to define OCPI_TARGET_KERNEL_DIR on the command line!
# http://savannah.gnu.org/bugs/?34806

# The keyword "autofound" will allow the platform-specific Makefiles receive an unset OCPI_TARGET_KERNEL_DIR
cdk-zynq: OCPI_TARGET_PLATFORM=xilinx13_3
# No!  The project registry should default from the OCPI_CDK_DIR
#cdk-zynq: export OCPI_PROJECT_REGISTRY_DIR=$(shell pwd)/../project-registry
cdk-zynq: OCPI_TARGET_KERNEL_DIR="unknown"
cdk-zynq: TAR_EXCLUDES= --exclude="*platforms/*/git/*"
cdk-zynq: cdk-cross-zynq

prereq-zynq: export PREREQ_TARGET=zynq
prereq-zynq: prereq-cross-zynq

# TODO: Have this use Jenkins' branch config to git checkout
cdk-pico: $(infox Auto-finding Pico BSP)$(if $(wildcard ../projects/bsps/picoflexor),$(eval RPM_PICOT6A=$(shell pwd)/../projects/bsps/picoflexor))$(infox Result: $(RPM_PICOT6A))
cdk-pico: OCPI_TARGET_PLATFORM=pico_t6a
cdk-pico: BSP_VARNAME=RPM_PICOT6A
cdk-pico: export OCPI_PROJECT_PATH=$(RPM_PICOT6A)
cdk-pico: OCPI_TARGET_KERNEL_DIR?=$(shell locate -b '\zynq_kernel' | tail -1)
cdk-pico: TAR_EXCLUDES= --exclude="*platforms/*/git/*"
cdk-pico: $(if $(RPM_PICOT6A),cdk-cross-pico,cdk-bsp-error)

prereq-pico: export PREREQ_TARGET=picoflexor
prereq-pico: prereq-cross-pico

cdk-bsp-error:
	@echo "Cross-compiling for $(OCPI_TARGET_PLATFORM) requested, but environment variable $(BSP_VARNAME) was not set for BSP!"
	@false

# The order of SPOOF_HOSTNAME and inode64 sourcing matters, because SPOOF_HOSTNAME is built at
# make's load time, meaning the LD_PRELOAD isn't seen. So it is set first and then the support
# shell script adds to it.
cdk-cross-%: inode64_check phony_explicit cleancheck libmyhostname buildtar-%
	@echo Cross-compiling for $(OCPI_TARGET_PLATFORM)...
	$(AT)if [ ! -e cdkbin.tar ]; then \
	  echo Could not find cdkbin.tar. Please build the standard CDK before attempting to cross-compile. ; \
	  exit 99; \
	fi
	@# We cheat here and use OCPI_PROJECT_PATH to imply there is a specific BSP defined because cleancheck would not let user define it
	$(AT)if [ -n "$(OCPI_PROJECT_PATH)" ]; then \
		echo "Creating tarball... (imported_bsp.tar from $(OCPI_PROJECT_PATH))" && \
	  tar -C $(OCPI_PROJECT_PATH) --exclude-{backups,vcs} -cf imported_bsp.tar . && \
	  cp -v imported_bsp.tar ~/rpmbuild/SOURCES; \
	fi
	$(AT)cp -v cdkbin.tar ~/rpmbuild/SOURCES
	$(AT)set -o pipefail && $(SPOOF_HOSTNAME) source /opt/opencpi/prerequisites/build_support/inode64/inode64.sh && \
	rpmbuild -ba opencpi.spec \
	--define="BSP_COMMIT_HASH $(if $(OCPI_PROJECT_PATH),$(shell cd $(OCPI_PROJECT_PATH) && git rev-parse --verify HEAD),%{nil})" \
	--define="COMMIT_HASH $(COMMIT_HASH)" \
	--define="COMMIT_TAG $(shell echo "$(COMMIT_TAG)" | tr -d ' ')" \
	--define="IMPORT_BSP $(if $(OCPI_PROJECT_PATH),1,%{nil})" \
	--define="RELEASE_TAG $(RELEASE_TAG)" \
	--define="RPM_BASENAME ${RPM_BASENAME}" \
	--define="RPM_RELEASE $(RPM_RELEASE)" \
	--define="RPM_VERSION ${RPM_VERSION}" \
	--define="OCPI_CROSS 1" \
	--define="OCPI_TARGET_KERNEL_DIR $(OCPI_TARGET_KERNEL_DIR)" \
	--define="OCPI_TARGET_PLATFORM $(OCPI_TARGET_PLATFORM)" \
	2>&1 | tee build-rpm-$(OCPI_TARGET_PLATFORM).log
	$(AT)cp -v ~/rpmbuild/RPMS/*/$(RPM_BASENAME)-* .
	$(AT)rm *-platform-*-devel*.rpm

# Some of the cross-compilers cannot handle 64-bit inodes, so they must be spoofed
.PHONY: inode64_check
.SILENT: inode64_check
inode64_check:
	if ! rpm -q ocpi-prereq-build_support-inode64 &>/dev/null; then \
	  echo "Could not find RPM: ocpi-prereq-build_support-inode64 - please install it."; \
	  false; \
	fi

.PHONY: driver
.SILENT: driver
# Since we only want a handful of files, we don't use the main "buildtar"
driver: TDIR:=/tmp/$(RPM_BASENAME)-rpmbuild-$(shell echo $$$$)
driver: TAR_BASE=$(RPM_DRIVERNAME)-$(RPM_VERSION)
driver: cleanrpmbuild libmyhostname
	-rm $(TAR_BASE).tar 2> /dev/null
	mkdir -p $(TDIR)/$(TAR_BASE)
	find ../os/linux/driver/ -maxdepth 1 -type f | xargs cp --target-directory=$(TDIR)/$(TAR_BASE)
	cp --target-directory=$(TDIR)/$(TAR_BASE) ../runtime/hdl/include/Hdl{NetDefs,OCCP,PciDriver}.h ../os/interfaces/include/KernelDriver.h ../COPYRIGHT ../LICENSE.txt
	tar -C $(TDIR) -chf $(TAR_BASE).tar .
	cp $(TAR_BASE).tar ~/rpmbuild/SOURCES
	rm -rf $(TDIR)
	$(SPOOF_HOSTNAME) rpmbuild -ba opencpi-driver.spec \
	--define="COMMIT_HASH $(COMMIT_HASH)" \
	--define="COMMIT_TAG $(shell echo "$(COMMIT_TAG)" | tr -d ' ')" \
	--define="RELEASE_TAG $(RELEASE_TAG)" \
	--define="RPM_BASENAME ${RPM_DRIVERNAME}" \
	--define="RPM_RELEASE $(RPM_RELEASE)" \
	--define="RPM_VERSION ${RPM_VERSION}"
	cp -v ~/rpmbuild/RPMS/*/$(RPM_DRIVERNAME)-* .

# AV-2469
.PHONY: assets
.SILENT: assets
assets:
	$(MAKE) -C assets rpms
	mv assets/*rpm .

# This will force any bsp to be rebuilt if you can find the directory
# Cannot use Make "if" because $@ is undefined in first pass; must be shell "if"
bsp-%: BSPDIR=$(subst -,_,$@)
bsp-%:
	@if [ -e "$(BSPDIR)/Makefile" ]; then \
    $(MAKE) -C $(BSPDIR) rpms; \
    mv $(BSPDIR)/*rpm .; \
  else \
    echo "Could not find directory $(BSPDIR)!"; \
    false; \
  fi

.PHONY: prereq
.SILENT: prereq
ifeq (,$(JENKINS_HOME))
prereq: export CROSS_FAIL_OK=1
endif
prereq:
	$(MAKE) -C prereq rpms

prereq-host: export PREREQ_TARGET=host
prereq-host: prereq-cross-host

# Cannot just call prereq above b/c then CROSS_FAIL_OK could get set.
prereq-cross-%: inode64_check phony_explicit cleancheck libmyhostname
	$(MAKE) -C prereq rpms

.PHONY: notify
.SILENT: notify
NEXE=$(shell bash -c "type -p notify-send")
notify: clean driver cdk-all
ifeq ($(NEXE),)
	echo Done!
else
	$(NEXE) -t 30000 'RPM build complete!'
endif

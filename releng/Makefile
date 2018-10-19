# This is the make file to make the documentation and BSP RPMs from the AV Team

# It used to build ALL RPMs before the OSS /packaging/ directory existed.
# Any nonsensical complications are likley due to its heritage.

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

# Bare essentials check - these are things that we use in the Makefile; all
# other packages will be checked by the rpmbuild's BuildRequires flags.
# command_v returns the name of the command IF MISSING, otherwise blank. The
# redirect to /dev/null is required to force make to call a shell.
command_v=$(if $(shell command -v $1 2>/dev/null),,$1)
missing_commands:=$(strip $(foreach cmd,find git locate perl rpmbuild,$(call command_v,$(cmd))))
ifneq "$(missing_commands)" ''
  $(warning Missing these commands: $(missing_commands))
ifneq "$(filter rpmbuild,$(missing_commands))" ''
  $(warning The rpmbuild command is in the rpm-build package.)
endif
  $(error Please install these basic utilities)
endif

# Find out if we are a git version branch explicitly
# (Strictly "v" followed only by numbers or '.' with NO suffix)
# This is complicated because Jenkins uses a detached HEAD
# Note: If somebody checks in between Jenkins builds, it will sometimes get "develop^2~37" etc,
# so anything like that is stripped.
# The BitBucket prefix of something like "bugfix--" is also stripped if present.
GIT_BRANCH:=$(notdir $(shell git name-rev --name-only HEAD | perl -pe 's/([~^]\d+)*$$//' | perl -pe 's/^.*?--//'))
GIT_VERSION:=$(shell echo $(GIT_BRANCH) | perl -ne '/^v[\.\d]+/ && print')
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
      # COMMIT_TAG is used in *.spec files for RPM release tag.
      # Any non alphanumeric (or .) strings converted to single _
      COMMIT_TAG+=_$(shell echo $(GIT_BRANCH) | sed -e 's/[^A-Za-z0-9.]\+/_/g')
    endif
  endif
  # Generate an increasing number for version comparisons (since git hashes aren't)
  # Number of 6-minute increments since release in 5 digits (rolls over >1/yr)
  RELEASE_TAG=_$(shell printf %05d $(shell expr `date -u +"%s"` / 360 - 4273900))
  # When bumping this, don't miss configure.ac as well (AV-3433)
  RPM_VERSION:=1.5.0
else
  # This is a specific version branch
  RELEASE_TAG=%{nil}
  RPM_VERSION:=$(GIT_VERSION)
endif
# Let BSP builders get these as well. (AV-2768)
export COMMIT_HASH
export COMMIT_TAG
export RELEASE_TAG
export RPM_RELEASE
export RPM_VERSION

# There are various targets...
.PHONY: help
.SILENT: help

# Default
help:
	printf "This is the Makefile to build RPMs for OpenCPI\n\n"
	echo "The valid targets are:"
	echo ""
	echo " doc                   - The documentation RPM"
	echo ""
	echo " bsp-XXX               - Bundle BSP XXX for distribution"
	echo ""
	echo " clean                 - Remove generated files from this directory"
	echo " cleanrpmbuild         - Cleans ~/rpmbuild/ (runs EVERY BUILD for consistency)"
	echo ""
	echo ""

include myhostname.mk

.PHONY: clean cleanrpmbuild
.SILENT: clean cleanrpmbuild
.IGNORE: clean cleanrpmbuild
clean: myhostnameclean
	grep -v "^#" .gitignore | xargs -ri sh -c 'rm -rf {}'

cleanrpmbuild:
	rm -rf ~/rpmbuild/*
	mkdir -p ~/rpmbuild/SOURCES/
	rm -rf $(CURDIR)/rpmbuild-doc-tmpdir

# AV-4267
# The _topdir was because the jenkins user didn't have a home in Docker containers, but that might not be the case any more
.PHONY: doc
.SILENT: doc
doc: libmyhostname cleanrpmbuild
	$(SPOOF_HOSTNAME) rpmbuild -ba opencpi-doc.spec \
	--define="COMMIT_HASH $(COMMIT_HASH)" \
	--define="COMMIT_TAG $(shell echo "$(COMMIT_TAG)" | tr -d ' ')" \
	--define="RELEASE_TAG $(RELEASE_TAG)" \
	--define="RPM_RELEASE $(RPM_RELEASE)" \
	--define="RPM_VERSION ${RPM_VERSION}" \
	--define="SRC_BASE $(CURDIR)/../" \
	--define="_topdir $(CURDIR)/rpmbuild-doc-tmpdir"
	# Prevent more than one doc rpm from lying around
	rm -f opencpi-doc-*
	cp -v $(CURDIR)/rpmbuild-doc-tmpdir/RPMS/noarch/opencpi-doc-* .

# This will force any bsp to be rebuilt if you can find the directory
# Cannot use Make "if" because $@ is undefined in first pass; must be shell "if"
bsp-%: BSPDIR=$(subst -,_,$@)
bsp-%: cleanrpmbuild
	@if [ -e "$(BSPDIR)/Makefile" ]; then \
    $(MAKE) -C $(BSPDIR) rpms; \
    mv $(BSPDIR)/*rpm .; \
  else \
    echo "Could not find directory releng/$(BSPDIR)!"; \
    false; \
  fi

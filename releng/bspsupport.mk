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

# This Makefile snippet expects certain variables to be set:

# Some from the "main" RPM Makefile:
# COMMIT_HASH
# COMMIT_TAG
# RELEASE_TAG
# RPM_RELEASE
# RPM_VERSION

# Some from the "calling" Makefile:
# PACKAGES
# BSPNAME
# Optional:
# GIT_PATH
# GIT_REPO

# It expects to be called from one level "deeper" in bsp_BSPNAME

# Some massaging of variables:
export COMMIT_TAG_FINAL:=$(shell echo "$(COMMIT_TAG)" | tr -d ' ')
export SPOOF_HOSTNAME

# Most of this was once prereq/Makefile
# Designed to check out another repo and match branch names when possible

.PHONY: all rpms
.SILENT: all rpms

-include ../config_files/custom_git_path.mk

include ../myhostname.mk

all: rpms

download: $(PACKAGES)

# This adds _rpm and _clean suffix to each package
RPM_TARGETS=$(foreach pkg, $(PACKAGES), $(pkg)_rpm)
CLEAN_TARGETS=$(foreach pkg, $(PACKAGES), $(pkg)_clean)
GIT_TARGETS=$(foreach pkg, $(PACKAGES), $(pkg)_gitcheck)
GIT_BRANCHES=$(foreach pkg, $(PACKAGES), $(pkg)_gitbranch)
export GIT_BRANCH:=$(notdir $(shell git name-rev --name-only HEAD | perl -pe 's/([~^]\d+)*$$//'))

.PHONY: $(RPM_TARGETS) $(CLEAN_TARGETS) $(GIT_TARGETS) $(GIT_BRANCHES)
.SILENT: $(RPM_TARGETS) $(CLEAN_TARGETS) $(GIT_TARGETS) $(GIT_BRANCHES) $(PACKAGES) clean cleanrpmbuild

# This creates the PACKAGE_rpm targets, setting PKG to the base package name, and requiring that they exist.
.SECONDEXPANSION:
$(RPM_TARGETS): PKG=$(@:_rpm=)
$(RPM_TARGETS): libmyhostname $$(PKG) $$(PKG)_gitbranch
	cd $(PKG)/rpm_support && $(SPOOF_HOSTNAME) ./build.sh rpm && cd ..

# This creates the PACKAGE_clean targets. The PKG does not need to exist.
.IGNORE: $(CLEAN_TARGETS)
$(CLEAN_TARGETS): PKG=$(@:_clean=)
$(CLEAN_TARGETS):
	[ -d $(PKG) ] && (cd $(PKG) && (grep -v "^#" .gitignore 2>/dev/null | xargs -ri sh -c 'rm -rf {}')) || :

# This creates the PACKAGE_gitcheck targets. The PKG does not need to exist.
# These checks ensure modified files don't accidentally get thrown away.
# The second one determines the current branch's upstream to compare.
.IGNORE: $(GIT_TARGETS)
$(GIT_TARGETS): PKG=$(@:_gitcheck=)
$(GIT_TARGETS):
	[ -d $(PKG) ] && [ `(cd $(PKG) && git status --porcelain | wc -l)` -ne 0 ] && echo ERROR: $(PKG) has local changes or untracked files || :
	[ -d $(PKG) ] && [ `(cd $(PKG) && git rev-parse --abbrev-ref --symbolic-full-name @{u} | xargs -IXXX git rev-list XXX..HEAD | wc -l)` -ne 0 ] && echo ERROR: $(PKG) has unpushed changes || :

rpms: clean $(RPM_TARGETS)
	echo Done building.
	cp */*rpm .
	rm -f *debuginfo*rpm
	ls -halF *rpm

clean: cleanrpmbuild myhostnameclean $(CLEAN_TARGETS)
	true

cleanrpmbuild:
	-rm -rf ~/rpmbuild/*
	-mkdir -p ~/rpmbuild/

# If the "jq" tool is there, try to use Jenkins configuration
# Note, it has to send a literal set of " around the BSP name or numbers will blow up (e.g. e310)
export JSON_GIT_REPO:=$(shell jq '.hdl_platforms."$(BSPNAME)".bsp_repo' ../jenkins/runtime/config.json 2>/dev/null || :)

$(PACKAGES):
	# They might already have the repo checked out elsewhere - the standard location of ../../projects/bsps/
ifneq '' "$(wildcard ../../projects/bsps/$(BSPNAME)/Makefile)"
	echo Re-using existing projects/bsps/$(BSPNAME) for $@
	ln -s ../../projects/bsps/$(BSPNAME) $@
else
	# If not, check JSON config
ifneq 'null' "$(JSON_GIT_REPO)"
	echo "Parsed Jenkins config.json and found repo for $(BSPNAME): $(JSON_GIT_REPO)"
	git clone $(JSON_GIT_REPO) $@
else
	# Lastly, fallback to GIT_REPO or GIT_PATH
ifeq '' "$(GIT_REPO)"
	git clone $(GIT_PATH)/$@.git
else
	echo "Warning: GIT_REPO defined, using '$(GIT_REPO)' to check out for $(BSPNAME) BSP instead of '$(GIT_PATH)/$@'"
	git clone $(GIT_REPO) $@
endif
endif
endif

# This creates the PACKAGE_gitbranch targets. This switches the branch or tag.
DEFAULT_GITBRANCH?=develop

$(GIT_BRANCHES): PKG=$(@:_gitbranch=)
$(GIT_BRANCHES):
	echo "====================== $(PKG) ======================"
	echo "Synchronizing repo information..."
	cd $(PKG) && git fetch --all
	echo "Switching to $(DEFAULT_GITBRANCH)..."
	cd $(PKG) && git checkout $(DEFAULT_GITBRANCH) # Always reset first
	echo "Trying to switch to $(GIT_BRANCH)... (OK if fails)"
	cd $(PKG) && git checkout $(GIT_BRANCH) || : # Try to check out
	echo "Pulling latest code..."
	cd $(PKG) && git pull
	echo "Currently working in AV branch '$(GIT_BRANCH)' and using '`cd $(PKG);git name-rev --name-only HEAD`' for $(PKG) branch."

.PHONY: distclean_warning distclean_nowarning
.SILENT: distclean_warning distclean_nowarning
distclean_warning: # This just has the warning come out before the listing
	echo This will remove EVERYTHING. You have ten seconds to abort.

distclean: distclean_warning $(GIT_TARGETS) clean
	test `$(MAKE) -s $(GIT_TARGETS) | wc -l` -eq 0 # error out if any warnings
	sleep 10
	-rm -rf $(PACKAGES) $(foreach pkg,$(PACKAGES),*$(pkg)*rpm)
	true

distclean_nowarning: $(GIT_TARGETS) clean
	test `$(MAKE) -s $(GIT_TARGETS) | wc -l` -eq 0 # error out if any warnings
	-rm -rf $(PACKAGES) $(foreach pkg,$(PACKAGES),*$(pkg)*rpm)
	true

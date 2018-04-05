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

##########################################################################################
ifndef OCPI_CDK_DIR
  export OCPI_CDK_DIR:=$(CURDIR)/cdk
endif

ifeq ($(wildcard exports),)
  ifeq ($(filter show help clean% distclean%,$(MAKECMDGOALS)),)
    $(info Exports have never been set up for this tree  Doing it now.)
    $(info $(shell ./scripts/makeExportLinks.sh - -))
  endif
endif

include $(OCPI_CDK_DIR)/include/util.mk
$(eval $(OcpiEnsureToolPlatform))
RccPlatforms:=$(call Unique,\
                $(or $(strip $(RccPlatforms) $(RccPlatform) $(Platforms) $(Platform)),\
	             $(OCPI_TOOL_PLATFORM)))
DoExports=for p in $(RccPlatforms); do ./scripts/makeExportLinks.sh $$p; done
# Get macros and rcc platform/target processing
include $(OCPI_CDK_DIR)/include/rcc/rcc-make.mk

##########################################################################################
# Goals that are not about projects
.PHONY: exports      framework      projects      driver \
        cleanexports cleanframework cleanprojects cleandriver clean
all framework:
	$(AT)$(MAKE) -C build/autotools install Platforms="$(RccPlatforms)"
	$(AT)$(DoExports)

cleanframework:
	$(AT)$(MAKE) -C build/autotools clean Platforms="$(RccPlatforms)"

exports:
	$(AT)$(DoExports)

cleanexports:
	$(AT)rm -r -f exports

driver:
	$(AT)set -e;\
	     $(foreach p,$(RccPlatforms),\
	       $(foreach t,$(RccTarget_$p),\
	         $(foreach o,$(call RccOs,$t),\
	           if test -d os/$o/driver; then \
	             echo Building the $o kernel driver for $(call RccRealPlatform,$p); \
	             (source $(OCPI_CDK_DIR)/scripts/ocpitarget.sh $p; \
		      $(MAKE) -C os/$o/driver); \
	           else \
	             echo There is no kernel driver for the OS '"'$o'"', so none built. ; \
	           fi;))) \
	     $(DoExports) \

cleandriver:
	$(AT)set -e;\
	     $(foreach p,$(RccPlatforms),\
	       $(foreach t,$(RccTarget_$p),\
	         $(foreach o,$(call RccOs,$t),\
	           if test -d os/$o/driver; then \
	             echo Cleaning the $o kernel driver for $(call RccRealPlatform,$p); \
	             (source $(OCPI_CDK_DIR)/scripts/ocpitarget.sh $p; \
		      $(MAKE) -C os/$o/driver topclean); \
	           else \
	             echo There is no kernel driver for the OS '"'$o'"', so none cleaned. ; \
	           fi;))) \

# Clean that respects platforms
clean: cleanframework cleanprojects

# Super clean, but not git clean, based on our patterns, not sub-make cleaning
cleaneverything distclean: clean cleandrivers
	$(AT)rm -r -f exports
	$(AT)find . -depth \( \
             -name '*~' -name '*.dSym' -o timeData.raw -o 'target-*' -o -name gen -o \
	     \( ! -path "*/rcc/platforms/*" -a -type d \)  \
             \) -exec rm -r {} \;

##########################################################################################
# Goals that are about exporting the CDK
# Make a tarball of exports
.PHONY: tar
opencpi-cdk-latest.tgz tar:
	$(AT)set -e; mydate=`date +%G%m%d%H%M%S`; \
	file=opencpi-cdk-$$mydate.tgz; \
	(cd exports; find . -follow -type l) > $$file.exclude; \
	if [ -s $$file.exclude ] ; then \
	  echo ==== These symlinks are broken for this export: ; \
	  cat $$file.exclude ; \
	  echo ==== End of broken symlinks; \
	fi; \
	echo Creating export file: $$file; \
	tar -z -h -f $$file -c -X $$file.exclude -C exports .;  \
	rm opencpi-cdk-latest.tgz; \
	ln -s $$file opencpi-cdk-latest.tgz; \
	ls -l opencpi-cdk-latest.tgz

# Create a relocatable RPM from what is exported for the given platforms
# The silly "rpmarch" determination is exporting RPM_ARCH that is available inside rpmbuild
.PHONY: rpm
rpm: exports
	$(AT)which -s rpmbuild || (echo Cannot build an RPM: rpmbuild is not available; exit 1)
	$(AT)echo Creating an RPM file from the current CDK for platforms: $(RccPlatforms)
	$(AT)OCPI_RCC_PLATFORMS="$(RccPlatforms)" \
	     rpmbuild --quiet -bb --define "_rpmdir $(CURDIR)" build/cdk.spec
	$(AT)rpmarch=$$(rpmbuild --showrc | grep '^install arch' | (read -a a;echo $${a[3]})) &&\
	     rpm=`ls -t $$rpmarch/*.rpm | head -1` && \
	     mv -f $rpm . && rm -r -f $$rpmarch && \
	     echo Created RPM file: $(basename $$rpm)

##########################################################################################
# Goals that are about projects
# A convenience to run various goals on all the projects that are here
Projects=$(dirname $(wildcard projects/*))
ProjectGoals=cleanhdl cleanrcc cleanocl rcc ocl hdl applications run runtest hdlprimitives \
             components cleancomponents test
# These are not done in parallel since we do not know the dependencies
DoProjects=$(foreach p,$(Projects),\
             echo Performing $1 on project $p;$(MAKE) -C projects/$p $1 &&) :
.PHONY: $(ProjectGoals)
$(ProjectGoals):
	$(AT)$(call DoProjects,$@)

projects:
	$(AT)$(call DoProjects,)

cleanprojects:
	$(AT)$(call DoProjects,clean)

rcc ocl hdl: exports

##########################################################################################
# Help
.PHONY: show help
.SILENT: show help
show help:
	echo "This top-level Makefile builds the framework and built-in projects."
	echo "Most targets take Platform(s) or RccPlatform(s) (equivalent) as a variable."
	echo "RccPlatforms can have build options/letters after a hyphen."
	echo "The valid goals that accept platforms are:"
	echo "   framework(default) - Build the framework for platfors and export them"
	echo "   exports            - Redo exports, including for indicated platforms"
	echo "                      - This is cumulative;  previous exports are not removed"
	echo "                      - This does not export projects"
	echo "   cleanframework     - Clean the specific platforms"
	echo "   projects           - Build the projects"
	echo "   cleanprojects      - Clean all projects"
	echo "   exportprojects     - Export all projects"
	echo "   driver             - Build the driver(s) for the platform(s)"
	echo "   cleandriver        - Clean the driver(s) for the platform(s)"
	echo "   cleaneverything    - Clean as much as we can without git cleaning"
	echo "                      - also distclean does this for historical/compatible reasons"
	echo "   tar                - Create the tarball for the current cdk exports"
	echo "   rpm                - Create the binary/relocatable CDK RPM for the platforms"
	echo "The projects for project-related goals use the Projects variable."
	echo "The default is all the built-in projects (including inactive)."
	echo "These various project-related goals simply perform the goal in all projects:"
	echo "   rcc ocl hdl applications test run runtest hdlprimitives components"
	echo "   cleanhdl cleanrcc cleanocl"
	echo "Variables that only affect project building can also be used, like HdlPlatforms."

etags: 
    ETAGS=etags ; \
    rm TAGS \
    find . -name '*.cpp' -o -name '*.h' -o -name '*.c' -print0 \
    | xargs $(ETAGS) --extra=+q --fields=+fksaiS --c++-kinds=+px --append

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

ifndef OCPI_CDK_DIR
  export OCPI_CDK_DIR:=$(CURDIR)/cdk
endif

ifeq ($(wildcard exports),)
  ifeq ($(filter clean% distclean%,$(MAKECMDGOALS)),)
    $(info Exports have never been set up for this tree  Doing it now.)
    $(info $(shell ./scripts/makeExportLinks.sh - -))
  endif
endif

include $(OCPI_CDK_DIR)/include/util.mk
$(eval $(OcpiEnsureToolPlatform))
RccPlatforms:=$(strip $(or $(strip $(RccPlatforms) $(RccPlatform) $(Platforms) $(Platform)),\
	                   $(OCPI_TOOL_PLATFORM)))
DoExports=for p in $(RccPlatforms); do ./scripts/makeExportLinks.sh $$p; done
# Get macros and rcc platform/target processing
include $(OCPI_CDK_DIR)/include/rcc/rcc-make.mk
.PHONY: exports framework projects driver
all: framework exports

exports:
	$(AT)$(DoExports)

framework:
	$(AT)$(MAKE) -C build/autotools install Platforms="$(RccPlatforms)"
	$(AT)$(DoExports)

cleanframework:
	$(AT)$(MAKE) -C build/autotools build Platforms="$(RccPlatforms)" clean

# A convenience to run various goals on all the projects that are here
Projects=core assets
ProjectGoals=cleanhdl cleanrcc cleanocl rcc ocl hdl applications run runtest hdlprimitives \
             components cleancomponents
DoProjects=$(foreach p,$(Projects),echo Performing $1 on project $p;$(MAKE) -C projects/$p $1 &&) :
.PHONY: $(ProjectGoals)
$(ProjectGoals):
	$(AT)$(call DoProjects,$@)

projects:
	$(AT)$(call DoProjects,)

cleanprojects:
	$(AT)$(call DoProjects,clean)

rcc ocl hdl: exports

driver:
	$(AT)set -e;\
	     $(foreach p,$(RccPlatforms),$(info P0:$p)\
	       $(foreach t,$(RccTarget_$p),$(info T0:$t)\
	         $(foreach o,$(call RccOs,$t),$(info O0:$o)\
	           if test -d os/$o/driver; then \
	             (source $(OCPI_CDK_DIR)/scripts/ocpitarget.sh $p$(info PPP:$p); \
		      env | grep OCPI; \
		      $(MAKE) -C os/$o/driver); \
	           else \
	             echo No driver for the OS '"'$o'"', so none built. ; \
	           fi;))) \
	     $(DoExports) \

cleandriver:
	$(AT)$(and $(wildcard os/$(OCPI_TARGET_OS)/driver),$(MAKE) -C os/$(OCPI_TARGET_OS)/driver topclean)

cleandrivers:
	for d in os/*/driver; do $(MAKE) -C $$d topclean; done

.PHONY: packages tar diff diff.q test $(PACKAGES)

clean: cleancomponents cleanapplications
	$(AT)$(MAKE) -C build/autotools clean
	$(AT)$(foreach p,$(ALLPACKAGES),\
		if test -f $p/Makefile.ocpi ; then \
			$(MAKE) --no-print-directory $(call DescendMake,$p) -f Makefile.ocpi $@ ; \
		else \
			$(MAKE) --no-print-directory $(call DescendMake,$p) -f $(call AdjustRelative,$p,)/Makefile.ocpi.for-pkg $@ ; \
		fi ; \
	)
	$(AT)make -C projects/core clean && make -C projects/assets clean
	$(AT)rm -r -f exports

cleanexports:
	$(AT)rm -r -f exports

distclean: clean
	find . -name '*~' -exec rm {} \;
	find . -depth -name '*.dSym' -exec rm -r {} \;
	-rm -f diff diff.q
	-rm -f *.exe *.obj *.o *.ilk *.sbr *.suo *.sln *.pdb *.bsc *~
	-rm -r -f lib

cleaneverything: distclean cleandrivers
	-find . -depth -name 'timeData.raw' -exec rm -f '{}' ';'
	-find . -depth -name 'target-*' -exec rm -r '{}' ';'
	-find . -depth -name 'gen' -exec rm -r '{}' ';'
	-find . -depth -name "lib" -a ! -path "*export*" -a ! -path "*/platforms/*" -a -type d -a -exec rm -r "{}" ";"
	$(AT)rm -r -f exports

export_cdk:
	mydate=`date +%G%m%d%H%M%S`; \
	file=opencpi-cdk-$$mydate.tgz; \
	(cd exports; find . -follow -type l) > $$file.exclude; \
	if [ -s $$file.exclude ] ; then \
	  echo ==== These symlinks are broken for this export: ; \
	  cat $$file.exclude ; \
	  echo ==== End of broken symlinks; \
	fi; \
	echo Creating export file: $$file; \
	tar -z -h -f $$file -c -X $$file.exclude -C exports .


etags: 
    ETAGS=etags ; \
    rm TAGS \
    find . -name '*.cpp' -o -name '*.h' -o -name '*.c' -print0 \
    | xargs $(ETAGS) --extra=+q --fields=+fksaiS --c++-kinds=+px --append

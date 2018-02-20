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

# This file is included to import cores and is driven by the definition of the
# ImportCoreDirs variable
ifdef ImportCoreDirs

# First, even when we do NOT know whether we are importing or not, we
# figure out the core and module names.  If the directory isn't there,
# we don't complain.
define FindImportCoreNames
#$$(info 1=$1=$(origin ImportCoreDir_$(notdir $1)))
ifdef ImportCoreDir_$$(notdir $1)
  $$(error Two directories ($1 and $(ImportCoreDir_$(notdir $1))) in ImportCoreDirs have the same name)
endif
ImportCoreName_$(notdir $1):=$(notdir $1)
ifeq ($(realpath $1/opencpi-core.mk),)
#$$(error NO OPENCPI $1/opencpi-core.mk)
# Not an error here...
else
Core=
Top=
SourceFiles=
PreBuiltCore=
ExcludeTargets=
OnlyTargets=
include $1/opencpi-core.mk
# So now Core and Top might be set
ifndef Core
Core=$(notdir $1)
endif
ifndef Top
Top:=$$(Core)
endif
ImportCoreTop_$(notdir $1):=$$(Top)
ImportCoreDir_$(notdir $1):=$1
ImportCoreFiles_$(notdir $1):=$$(SourceFiles)
ImportCorePreBuilt_$(notdir $1):=$$(PreBuiltCore)
ImportCoreOnly_$(notdir $1):=$$(OnlyTargets)
ImportCoreExclude_$(notdir $1):=$$(ExcludeTargets)
endif
endef

# First we retrieve core and module names from each directory
# (if they are present).
$(foreach d,$(ImportCoreDirs),$(eval $(call FindImportCoreNames,$d)))
#$(foreach d,$(ImportCoreDirs),\
$(info For $d:)\
$(info  Name: $(ImportCoreName_$(notdir $d)))\
$(info  Top: $(ImportCoreTop_$(notdir $d)))\
$(info  Dir: $(ImportCoreDir_$(notdir $d)))\
$(info  Files: $(ImportCoreFiles_$(notdir $d))))

# The first argument is the import directory, and the second is the
# core name, the third is the module name, the fourth is the notdir of $1
define DoImportCore
.PHONY: import_core_$4 clean_core_$4

diff_core_$4:
	$(AT)$(ECHO) ===========Comparing imported core $2 \(from $1, module $3\).
	$(AT)if test ! -d $1; then \
	       echo Imported core directory $1 nonexistent; exit 1; \
	     elif test ! -f $1/opencpi-core.mk; then \
	       echo Missing $1/opencpi-core.mk file; exit 1; \
	     elif test ! -f $1/$(ImportCoreTop_$4)_bb.v; then \
	       echo Missing $1/$(ImportCoreTop_$4)_bb.v file; exit 1; \
	     fi
	$(AT)r=0; \
	     for sf in $(ImportCoreFiles_$4); do \
	       if test ! -f $1/$$$$sf; then \
		  r=1; echo Imported file $$$$sf missing from $1; \
	       fi; \
	     done; \
	     exit $$$$r
	$(AT)$(ECHO) Comparing black box $1/$3_bb.v to $2/$3_bb.v...
	$(AT)diff $1/$3_bb.v $2/$3_bb.v
	$(AT)$(ECHO) Comparing source files...
	$(AT)for f in $(ImportCoreFiles_$4) $(ImportCorePreBuilt_$4); do \
	       $(ECHO) $1/$$$$f...; diff $1/$$$$f $2; \
	     done

import_core_$4:
	$(AT)if test ! -d $1; then \
	       echo Imported core directory $1 nonexistent; exit 1; \
	     elif test ! -f $1/opencpi-core.mk; then \
	       echo Missing $1/opencpi-core.mk file; exit 1; \
	     elif test ! -f $1/$(ImportCoreTop_$4)_bb.v; then \
	       echo Missing $1/$(ImportCoreTop_$4)_bb.v file; exit 1; \
	     fi
	$(AT)r=0; \
	     for sf in $(ImportCoreFiles_$4); do \
	       if test ! -f $1/$$$$sf; then \
		  r=1; echo Imported file $$$$sf missing from $1; \
	       fi; \
	     done; \
	     exit $$$$r
	$(AT)if test -d $2; then \
		$(ECHO) -n Removing and re-importing core' '; \
		rm -r -f $2; \
	     else \
		$(ECHO) -n Importing new core' '; \
	     fi
	$(AT)$(ECHO) $2 from $1.
	$(AT)mkdir $2
	$(AT)(\
	     echo Core=$(ImportCoreName_$4); \
	     echo Top=$(ImportCoreTop_$4); \
	     echo ExcludeTargets=$(ImportCoreExclude_$4); \
	     echo OnlyTargets=$(ImportCoreOnly_$4); \
	     echo SourceFiles=\\; \
	     $(foreach f,$(ImportCoreFiles_$4), echo ' '$(notdir $f)\\;) \
	     echo '#' end of files; \
	     $(foreach f,$(ImportCorePreBuilt_$4), echo PreBuiltCore=$(notdir $f);) \
	      echo include '$$$$(OCPI_CDK_DIR)'/include/hdl/hdl-core.mk  \
	   ) > $2/Makefile
	$(AT)cp $1/$3_bb.v $2
	$(AT)for f in $(ImportCoreFiles_$4) $(ImportCorePreBuilt_$4); do \
	       cp $1/$$$$f $2; \
	     done

clean_core_$4:
	$(AT)rm -r -f $2

import_cores: import_core_$4
diff_cores: diff_core_$4
cleanimports: clean_core_$4
ImportCores:=$$(ImportCores) $2
endef
$(foreach d,$(ImportCoreDirs),\
  $(foreach n,$(notdir $d),\
    $(eval $(call DoImportCore,$d,$(ImportCoreName_$n),$(ImportCoreTop_$n),$n))))
#$(info IPC $(ImportCores).$(ImportCoreDirs))
endif

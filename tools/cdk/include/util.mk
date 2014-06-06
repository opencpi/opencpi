# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
#
#    Mercury Federal Systems, Incorporated
#    1901 South Bell Street
#    Suite 402
#    Arlington, Virginia 22202
#    United States of America
#    Telephone 703-413-0781
#    FAX 703-413-0784
#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #

ifndef __UTIL_MK__
__UTIL_MK__=x
# ARRRRRRRRRRRRGH: the debian shell in make is /bin/sh and the SHELL env var is ignored by
# make in Debian.
override SHELL=/bin/bash
AT=@
OCPI_DEBUG_MAKE=
ifneq (,)
define OcpiDoInclude
ifndef OcpiThisFile
OcpiThisFile:=$(lastword $(MAKEFILE_LIST))
endif
OcpiSaveFile:=$(OcpiThisFile)
include $(OCPI_CDK_DIR)/include/$(1)
OcpiThisFile:=$(OcpiSaveFile)
endef
OcpiInclude=$(eval $(call OcpiDoInclude,$1))
endif
ifneq ($(OCPI_DEBUG_MAKE),)
define OcpiDbg
$(warning Debug: $(1))
endef
define OcpiDbgVar
$(call OcpiDbg,$(2)$(1) is <$(call $(1))> origin $(origin $(1)))
endef
endif
# Options we alway use and will assume everywhere
.DELETE_ON_ERROR:
.SUFFIXES:
.SECONDEXPANSION:
export AT
# Utilities used by many other makefile files
# Allow us to include this early by establishing the default initial target (all).
all:
Cwd:=$(realpath .)
$(call OcpiDbgVar,Cwd)
Empty:=
Space:=$(Empty) $(Empty)
# This variable is set to the character that is invalid in pathnames.
# It should be the one printable character that we will not support in pathnames.
Invalid:="
CwdDirName:=$(subst $(Invalid),$(Space),$(notdir $(subst $(Space),$(Invalid),$(Cwd))))
CwdName:=$(basename $(CwdDirName))
$(call OcpiDbgVar,CwdName)

ifndef Model
Model:=$(strip $(subst ., ,$(suffix $(CwdDirName))))
endif
$(call OcpiDbgVar,Model)
Models:=xm rcc hdl ocl
Language_rcc:=c
Languages_rcc:=c c++
Suffix_rcc_c:=c
Suffix_rcc_c++:=cc

Suffix_hdl_verilog:=v
Suffix_hdl_vhdl:=vhd
Language_hdl:=vhdl
Languages_hdl:=vhdl verilog

Language_ocl:=cl
Suffix_ocl_cl:=cl
Suffix_xm:=xm
CapModels=$(foreach m,$(Models),$(call Capitalize,$m))
UCModel=$(call ToUpper,$(Model))
CapModel=$(call Capitalize,$(Model))
HostSystem=$(shell uname -s | tr A-Z a-z)
AT=@
RM=rm
ifneq ($(HostSystem),darwin)
TIME=/usr/bin/time -f %E
OcpiLibraryPathEnv=LD_LIBRARY_PATH
else
TIME=/usr/bin/time
OcpiLibraryPathEnv=DYLD_LIBRARY_PATH
endif
# this is to ensure support for the -n flag
ECHO=/bin/echo
Empty=
#default assumes all generated files go before all authored files
CompiledSourceFiles=$(TargetSourceFiles) $(GeneratedSourceFiles) $(AuthoredSourceFiles)
Space=$(Empty) $(Empty)
# Just for history (thanks Andrew): this only works with tcsh, not traditional csh.  And csh isn't posix anywah
#Capitalize=$(shell csh -f -c 'echo $${1:u}' $(1))
#UnCapitalize=$(shell csh -f -c 'echo $${1:l}' $(1))
Capitalize=$(shell awk -v x=$(1) 'BEGIN {print toupper(substr(x,1,1)) substr(x,2,length(x)-1) }')
UnCapitalize=$(shell awk -v x=$(1) 'BEGIN {print tolower(substr(x,1,1)) tolower(x,2,length(x)-1) }')
ToUpper=$(shell echo $(1)|tr a-z A-Z)
ToLower=$(shell echo $(1)|tr A-Z a-z)
# function to add a ../ to pathnames, avoiding changing absolute ones
AdjustRelative2=$(foreach i,$(1),$(if $(filter /%,$(i)),$(i),../../$(patsubst ./%,%,$(filter-out .,$(i)))))
AdjustRelative=$(foreach i,$(1),$(if $(filter /%,$(i)),$(i),..$(patsubst %,/%,$(patsubst ./%,%,$(filter-out .,$(i))))))
HostProcessor=$(shell uname -m | tr A-Z a-z)
# Patch darwin's notion of x86 to linux's.  Assumes 64 bit machine...
ifeq ($(HostProcessor),i386)
HostProcessor=x86_64
endif
HostTarget=$(HostSystem)-$(HostProcessor)
OcpiHostTarget=$(HostTarget)
# Physical and realpath are broken on some NFS mounts..
OcpiAbsDir=$(foreach d,$(shell cd $1; pwd -L),$d)
OcpiAbsPath=$(strip \
  $(foreach p,$(strip \
    $(if $(filter /%,$1),$1,\
         $(if $(filter . ./,$1),$(call OcpiAbsDir,.),\
              $(if $(filter ./%,$1),$(call OcpiAbsDir,.)$(patsubst .%,%,$1),\
	           $(call OcpiAbsDir,.)/$1)))),$(abspath $p)))

# helper function to FindRelative, recursive
# arg 1 is from-list of path components, arg 2 is to-list
#$(info frs 1 $(1) 2 $(2))
FindRelativeStep=\
    $(if $(filter $(firstword $(1)),$(firstword $(2))),\
        $(call FindRelativeStep,$(wordlist 2,$(words $(1)),$(1)),$(wordlist 2,$(words $(2)),$(2))),\
	$(if $(1),$(subst $(Space),/,$(strip $(patsubst %,..,$(1))))$(if $(2),/),$(if $(2),,.))$(subst $(Space),/,$(2)))

# helper function for FindRelative
# arg1 is absolute-from arg2 is absolute-to arg3 is original from, arg4 is original to
#$(info 1 $(1) 2 $(2) 3 $(3) 4 $(4))
FindRelativeTop=\
        $(if $(strip $1),\
            $(if $(strip $2),\
	        $(if $(filter $(firstword $(strip $(subst /, ,$1))),$(firstword $(strip $(subst /, ,$2)))),\
                    $(call FindRelativeStep,$(strip $(subst /, ,$1)), $(strip $(subst /, ,$2))),\
		    $2),\
                $(error Invalid/non-existent path: to "$4" from "$3")),\
             $(error Invalid/non-existent path: from "$3" to "$4"))

# Function: return the relative path to get from $(1) to $(2).  Useful for creating symlinks
# Note return value must be nicely stripped
#$(info findrel 1 $(1).$(abspath $1) 2 $(2).$(abspath $2))
#$(info pwd:$(shell pwd) abs:$(abspath .) real:$(realpath .))
#FindRelative=$(strip $(call FindRelativeTop,$(call OcpiAbsPath,$1),$(call OcpiAbsPath,$2),$1,$2))
FindRelative=$(strip \
               $(foreach i,$(call FindRelativeTop,$(call OcpiAbsPath,$1),$(call OcpiAbsPath,$2),$(strip $1),$(strip $2)),$i))

# Function: retrieve the contents of a symlink - is this ugly or what!
# It would be easier using csh
SymLinkContents= `X=(\`ls -l $(1)\`);echo $${X[$${\#X[*]}-1]}`

# Function:
# Make a symlink, but don't touch it if it is already correct
#  First arg is local file to point to, second arg is dir to put link in.
#  e.g. $(call MakeSymLink,foo,linkdir) makes a link: dir/$(notdir foo) link to foo
# Funky because it might be executed in a loop
MakeSymLink2=	SL=$(2)/$(3); SLC=$(call FindRelative,$(2),$(1)); \
		if test -L $$SL; then \
		  OSLC="$(call SymLinkContents,$(2)/$(3))"; \
		else \
		  OSLC=; \
		fi;\
		if test "$$OSLC" != $$SLC; then \
		  rm -f $$SL; \
		  ln -s $$SLC $$SL; \
		fi
MakeSymLink=$(call MakeSymLink2,$(1),$(2),$(notdir $(1)))


# function of that puts stuff in a temporary file and returns its name.
MakeTemp=\
$(shell export TMPDIR=$(TargetDir);\
  TF=`mktemp -t -u`;echo "$(1)" | tr " " "\n"> $$TF;echo $$TF)

# Output directory processing.  OutDir is the internal variable used everywhere.
# It is set based on the public OCPI_OUTPUT_DIR, and is created as needed
ifndef OutDir
ifdef OCPI_OUTPUT_DIR
OutDir=$(OCPI_OUTPUT_DIR)/$(CwdName)/
$(OutDir):
	$(AT)mkdir $@
endif
endif
GeneratedDir=$(OutDir)gen
$(GeneratedDir): | $(OutDir)
	$(AT)mkdir $@

# Make all target dirs
TargetDir=$(OutDir)target-$($(CapModel)Target)
#$(AT)echo Creating target directory: $@
$(OutDir)target-%: | $(OutDIr)
	$(AT)mkdir $@

################################################################################
# $(call ReplaceIfDifferent,source-file-or-dir, dest-dir)
# A utility function to compare two trees that might contain binary files
# The first argument is the directory (or file) to be copied, and whose
# "tail" name should be placed in the destination directory
ifeq ($(HostSystem),darwin)
MD5=md5
else
MD5=md5sum -b -
endif
TreeHash=`(if test -f $(1); then \
             cat $(1); \
           elif test -e $(1); then \
             cd $(1); \
             find -L . -type f | xargs cat; \
           fi) \
           | $(MD5)`
ReplaceIfDifferent=\
  TAIL=`basename $(1)`; \
  while test 1; do \
    if test -f $(1); then OLD=$(2)/$$TAIL; else OLD=$(2); fi;\
    if test -e $$NEW; then \
      NEWHASH=$(call TreeHash,$(1));\
      OLDHASH=$(call TreeHash,$$OLD);\
      if test "$$OLDHASH" = "$$NEWHASH"; then\
        echo Installation suppressed for $(1). Destination is identical.; \
        break; \
      fi; \
      if test -e $$OLD; then \
        echo Removing previous installation for $(1) in $(2). ; \
        rm -r -f $$OLD; \
      else \
        echo No previous installation for $(1) in $(2). ;\
      fi; \
    fi; \
    if test ! -d $(2); then\
      mkdir -p $(2); \
    fi; \
    echo Installing $(1) into $(2); \
    if test -f $(1); then \
      cp -L -p $(1) $(2); \
    else \
      cp -L -R -p $(1)/* $(2); \
    fi; \
    break;\
  done
ReplaceContentsIfDifferent=\
  TAIL=`basename $(1)`; \
  while test 1; do \
    if test -e $(2); then \
      OLD=$(call TreeHash,$(1));\
      NEW=$(call TreeHash,$(2));\
      if test "$$OLD" = "$$NEW"; then\
        echo Installation suppressed for $(1). Destination is identical.; \
        break; \
      fi; \
      echo Removing previous installation for $(1) -\> $(2); \
      rm -r -f $(2); \
    fi; \
    if test ! -d $(2); then\
      mkdir -p $(2); \
    fi; \
    echo Installing $(1) -\> $(2); \
    cp -L -R -p $(1)/* $(2); \
    break;\
  done

################################################################################
# $(call Unique,words,already)
# A utility function to remove duplicates without reordering
# The second argument is just for recursion and should be blank on the call
Unique=$(strip $(foreach x,$(call Unique2,$1,),$x))
Unique2=$(if $1,$(call Unique2,$(wordlist 2,$(words $1),$1),$(strip\
                               $(foreach w,$(firstword $1),$(if $(filter $w,$2),$2,$2 $w)))),$2)

LibraryRefFile=$(call $(CapModel)LibraryRefFile,$1,$2)

################################################################################
# Tools for metadata and generated files
#ToolsTarget=$(OCPI_TOOL_HOST)
ToolsDir=$(OCPI_CDK_DIR)/bin/$(OCPI_TOOL_HOST)
ifeq ($(HostSystem),darwin)
DYN_PREFIX=DYLD_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/$(OCPI_TOOL_HOST)
else
DYN_PREFIX=LD_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/$(OCPI_TOOL_HOST)
endif
#$(info OCDK $(OCPI_CDK_DIR))
DYN_PREFIX=
OcpiGenTool=$(ToolsDir)/ocpigen $(patsubst %,-I"%",$(call Unique,$(XmlIncludeDirs)))
OcpiGenArg=$(DYN_PREFIX) $(OcpiGenTool) $1 -M $(GeneratedDir)/$(@F).deps
OcpiGen=$(call OcpiGenArg,)
# Return stderr and the exit status as variables
# Return non-empty on failure, empty on success, and set var
# $(call DoShell,<command>,<status var>,<value var>)
# 2 limitations:
# - The "#" character is changed to "<pound>" in the output
# - On success, the value will be a combination of stderr and stdout
# Example:
#  $(if $(call DoShell,ls -l,Value),$(error $(Value)),$(Value))
#DoShell=$(eval X:=$(shell X=`bash -c '$1; exit $$?' 2>&1`;echo $$?; echo "$$X" | sed "s/\#/<pound>/g"))$(strip \
#
DoShell=$(eval X:=$(shell X=`bash -c '$1; exit $$?'`;echo $$?; echo "$$X" | sed "s/\#/<pound>/g"))$(strip \
	     $(call OcpiDbg,DoShell($1,$2):X:$X) \
             $(eval $2:=$(wordlist 2,$(words $X),$X))\
	     $(call OcpiDbgVar,$2) \
             $(filter-out 0,$(firstword $X)))


# Like the builtin "dir", but without the trailing slash
OcpiDir=$(foreach d,$1,$(patsubst %/,%,$(dir $1)))

# Grab the language attribute out of an XML file the hard way
OcpiGetLangScript:="s/^.*[lL]anguage= *['\"]\([^'\"]*\).*/\1/"
OcpiGetLanguage=$(strip \
    $(call ToLower,\
       $(shell grep -i 'language *= *' $1 | sed $(OcpiGetLangScript))))

# Set the language attribute from the list of xml files in $1
define OcpiSetLanguage
  OcpiLanguage:=$$(sort $$(foreach f,$1,\
		    $$(and $$(realpath $$f),$$(call OcpiGetLanguage,$$f))))
  $$(and $$(word 2,$$(OcpiLanguage)),\
     $$(error Multiple languages found in the worker xml files: $$(OcpiLanguage)))
  $$(call OcpiDbgVar,OcpiLanguage)
  ifndef OcpiLanguage
    OcpiLanguage:=$(Language_$(Model))
  endif
  ifndef Suffix_$(Model)_$$(OcpiLanguage)
    $$(error The language "$$(OcpiLanguage)" is not supported for the "$(Model)" model.)
  endif
endef

# Generate the default XML contents for $1 a worker and $2 a model
# Executed with CWD being the worker directory
OcpiDefaultOWD=$(strip \
  <$(call Capitalize,$2)Worker name='$1' \
    language='$(Language_$(Model))' \
    spec='$(notdir $(strip \
      $(or $(wildcard ../specs/$1_spec.xml),\
           $(wildcard ../specs/$1-spec.xml))))'/>)

endif # ifndef __UTIL_MK__


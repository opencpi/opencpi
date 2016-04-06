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
export AT
export OCPI_DEBUG_MAKE
AT=@
ifneq ($(wildcard $(OCPI_CDK_DIR)/include/autoconfig_import*),)
# Import autotool/RPM-based settings
ifneq ($(OCPI_CROSS_HOST),)
include $(OCPI_CDK_DIR)/include/autoconfig_import-$(OCPI_CROSS_HOST).mk
else
include $(OCPI_CDK_DIR)/include/autoconfig_import.mk
endif
endif
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
.PHONY: all
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
Models:=xm rcc hdl ocl assy
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
FindRelativeTop=$(infoxx FRT:$1:$2:$3:$4)$(strip\
  $(foreach t,\
        $(if $(strip $1),\
            $(if $(strip $2),\
	        $(if $(filter $(firstword $(strip $(subst /, ,$1))),$(firstword $(strip $(subst /, ,$2)))),\
                    $(call FindRelativeStep,$(strip $(subst /, ,$1)), $(strip $(subst /, ,$2))),\
		    $2),\
                $(error Invalid/non-existent path: to "$4" from "$3")),\
             $(error Invalid/non-existent path: from "$3" to "$4")),\
  $(infoxx FRTr:$t:$(CURDIR))$t))

# Function: return the relative path to get from $(1) to $(2).  Useful for creating symlinks
# Note return value must be nicely stripped
#$(info findrel 1 $(1).$(abspath $1) 2 $(2).$(abspath $2))
#$(info pwd:$(shell pwd) abs:$(abspath .) real:$(realpath .))
#FindRelative=$(strip $(call FindRelativeTop,$(call OcpiAbsPath,$1),$(call OcpiAbsPath,$2),$1,$2))
FindRelative=$(strip $(infox FR:$1:$2)\
               $(foreach i,$(call FindRelativeTop,$(call OcpiAbsPath,$1),$(call OcpiAbsPath,$2),$(strip $1),$(strip $2)),$i))

# Function: retrieve the contents of a symlink - is this ugly or what!
# It would be easier using csh
SymLinkContents= `X=(\`ls -l $(1)\`);echo $${X[$${\#X[*]}-1]}`

# Function:
# Make a symlink, but don't touch it if it is already correct
#  First arg is local file to point to, second arg is dir to put link in.
#  e.g. $(call MakeSymLink,foo,linkdir) makes a link: dir/$(notdir foo) link to foo
# Funky because it might be executed in a loop
MakeSymLink2=	$(infox MSL2:$1:$2:$3)SL=$(2)/$(3); SLC=$(call FindRelative,$2,$1); \
		if test -L $$SL; then \
		  OSLC="$(call SymLinkContents,$2/$3)"; \
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
             find -L . -type f | sort | xargs cat; \
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
        echo Installation suppressed for $(1) in $(2). Destination is identical.; \
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
Unique=$(infox Unique:$1)$(strip $(foreach x,$(call Unique2,$1,),$x))
Unique2=$(infox Unique2:$1:$2:)$(if $1,$(call Unique2,$(wordlist 2,$(words $1),$1),$(strip\
                               $(foreach w,$(firstword $1),$(if $(filter $w,$2),$2,$2 $w)))),$2)

LibraryRefFile=$(call $(CapModel)LibraryRefFile,$1,$2)

################################################################################
# Tools for metadata and generated files
DateStamp := $(shell date +"%c")
ToolsDir=$(OCPI_CDK_DIR)/bin/$(OCPI_TOOL_DIR)
ifeq ($(HostSystem),darwin)
DYN_PREFIX=DYLD_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/$(OCPI_TOOL_DIR)
else
DYN_PREFIX=LD_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/$(OCPI_TOOL_DIR)
endif
#$(info OCDK $(OCPI_CDK_DIR))
#DYN_PREFIX=
OcpiGenTool=$(ToolsDir)/ocpigen $(patsubst %,-I"%",$(call Unique,$(XmlIncludeDirsInternal)))
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
OcpiDefaultSpec=$(or $(wildcard ../specs/$1_spec.xml),$(wildcard ../specs/$1-spec.xml))
OcpiDefaultOWD=$(if $(call OcpiDefaultSpec,$1),,$(error No default spec found for worker $1))$(strip \
  <$(call Capitalize,$2)Worker name='$1' \
    language='$(Language_$(Model))' \
    spec='$(notdir $(call OcpiDefaultSpec,$1))'/>)

# Function to generate target dir from target: $(call WkrTargetDir,target,config)
# FIXME: shouldn't really be named "Wkr"
WkrTargetDir=$(OutDir)target$(if $(filter 0,$2),,-$2)-$1

Comma:=, 
ParamMsg=$(and $(ParamConfigurations), $(strip \
  '($(foreach n,$(WorkerParamNames),$n=$(ParamMsg_$(ParamConfig)_$n)$(eval o:=1)))'))

RmRv=$(if $(filter %_rv,$1),$(patsubst %_rv,%,$1),$1)

OcpiAdjustLibraries=$(foreach l,$1,$(if $(findstring /,$l),$(call AdjustRelative,$l),$l))

ifndef OCPI_PREREQUISITES_INSTALL_DIR
  export OCPI_PREREQUISITES_INSTALL_DIR:=/opt/opencpi/prerequisites
endif

################################################################################
# This works when wildcard doesn't.
# (Note: make's wildcard function caches results so can't probe something that
# might come into existence during execution of make)
# There are strange NFS mount use cases that might not return the real path,
# so if that happens, drop to the older/slower Shell call.
OcpiExists=$(infox OEX:$1)$(foreach y,$(foreach x,$(realpath $1),$(if $(filter /%,$x),$1,$(strip $(shell if test -e $1; then echo $1; fi)))),$(infox OEX return $y)$y)

OcpiCheckLinks=$(strip \
  $(foreach d,$1,$d$(shell test -L $d -a ! -e $d && echo " (a link to non-existent/unbuilt?)")))

define OcpiComponentSearchError
The component library "$1" was not found in any of these locations: $(call OcpiCheckLinks,$2)
OCPI_HDL_COMPONENT_LIBRARY_PATH is: $(OCPI_HDL_COMPONENT_LIBRARY_PATH)
OCPI_COMPONENT_LIBRARY_PATH is: $(OCPI_COMPONENT_LIBRARY_PATH)
OCPI_PROJECT_PATH is: $(OCPI_PROJECT_PATH)
OCPI_CDK_DIR is: $(OCPI_CDK_DIR)
endef

# Given a location of a component library, return the relevant subdirectory
# This normalizes between exported libraries and source libraries
OcpiComponentLibraryExists=$(or $(call OcpiExists,$1/lib),$(call OcpiExists,$1))

# Search for a component library by name, independent of target
# This is not used for component libraries specified by location (with slashes)
# $(call OcpiSearchComponentPath,lib)
OcpiSearchComponentPath=\
  $(eval OcpiTempPlaces:=$(strip\
       $(subst :, ,$(OCPI_HDL_COMPONENT_LIBRARY_PATH)) \
       $(subst :, ,$(OCPI_COMPONENT_LIBRARY_PATH)) \
       $(foreach d,$(OcpiGetProjectPath),$d/lib)))\
  $(eval OcpiTempDirs:= $(strip \
    $(foreach p,$(OcpiTempPlaces),\
       $(foreach d,$p/$1,$(call OcpiComponentLibraryExists,$d)))))\
  $(or $(OcpiTempDirs)$(infox HTD:$(OcpiTempDirs)),\
    $(if $(filter clean,$(MAKECMDGOALS)),,$(error $(call OcpiComponentSearchError,$1,$(OcpiTempPlaces)))))



# Collect component libraries independent of targets.
# Normalize the list at the spec level
# No arguments
OcpiComponentLibraries=$(strip\
    $(foreach c,$(call Unique,$(ComponentLibraries) $(ComponentLibrariesInternal)),$(infox HCL:$c)\
      $(if $(findstring /,$c),\
         $(or $(call OcpiComponentLibraryExists,$c),\
              $(error Component library $c (from ComponentLibraries) not found.)),\
         $(call OcpiSearchComponentPath,$c))))

# Return the list of XML search directories for component libraries
# This currently has a sort of HACK in that it searches the hdl subdir
# since hdl workers need to be referenced by rcc workers.
OcpiXmlComponentLibraries=$(infox HXC)\
  $(eval OcpiTempDirs:= $(strip \
    $(foreach c,$(OcpiComponentLibraries),$c/hdl $c/$(Model) $c))) \
  $(infox OcpiXmlComponentLibraries returned: $(OcpiTempDirs))\
  $(OcpiTempDirs)

OcpiGetProjectPath=$(strip \
                     $(foreach p,$(subst :, ,$(OCPI_PROJECT_PATH)) $(OCPI_CDK_DIR),\
                       $(or $(call OcpiExists,$p/exports),$(call OcpiExists,$p),\
                         $(info Warning: The path $p in OCPI_PROJECT_PATH does not exist.))))

# Add a directory to the front of a path in the environment
# $(call OcpiPrependEnvPath,var-name,dir)
OcpiPrependEnvPath=\
  $(eval tmp:=$(wildcard $2))\
  $(infox PREPEND:$1:$2:$(tmp))\
  $(and $(tmp),$(eval export $1:=$(subst $(Space),:,$(call Unique,$(tmp) $(subst :, ,$($1))))))

############ Project related functions

# Set the given directory as the project directory, include the Project.mk file that is there
# and setting an environment variable OCPI_PROJECT_DIR to that place.
# This allows any path-related settings to be relative to the project dir
define OcpiSetProject
  # This might already be set
  OcpiTempProjDir:=$$(call OcpiAbsDir,$1)
  $$(infox OTPD:$1:$$(OcpiTempProjDir))
  ifdef OCPI_PROJECT_DIR
    ifneq ($$(OcpiTempProjDir),$$(OCPI_PROJECT_DIR))
      $$(error OCPI_PROJECT_DIR in environment is $$(OCPI_PROJECT_DIR), but found Project.mk in $1)
    endif
  endif
  override OCPI_PROJECT_DIR=$$(OcpiTempProjDir)
  export OCPI_PROJECT_DIR
  include $1/Project.mk
  # The project package defaults to "local".
  ifndef ProjectPackage
    ProjectPackage:=local
  endif
  # Any project dependencies are added to the project path
  ifdef ProjectDependencies
    export OCPI_PROJECT_PATH:=$$(subst $$(Space),:,$$(call Unique,$$(ProjectDependencies) $$(subst :, ,$$(OCPI_PROJECT_PATH))))
  endif
  # A project is always added to the below-project/non-project search paths
  # I.e. where the project path looks for other projects, and their exports,
  # the current project is searched internally, not in exports
  # when looking for (non-slash) primitives, look in this project, not exports
  $$(call OcpiPrependEnvPath,OCPI_HDL_PRIMITIVE_PATH,$$(OcpiTempProjDir)/hdl/primitives)
  # when looking for platforms, look in this project
  $$(call OcpiPrependEnvPath,OCPI_HDL_PLATFORM_PATH,$$(OcpiTempProjDir)/hdl/platforms)
  # when looking for XML specs and protocols, look in this project
  $$(call OcpiPrependEnvPath,OCPI_XML_INCLUDE_PATH,$$(OcpiTempProjDir)/specs)
  # when looking for component libraries, look in this project
  $$(call OcpiPrependEnvPath,OCPI_COMPONENT_LIBRARY_PATH,$$(OcpiTempProjDir)$$(strip\
    $$(and $$(filter libraries,$$(call OcpiGetDirType,$$(OcpiTempProjDir)/components)),/components)))
  # when looking for HDL component libraries, look in this project
  # This variable is becoming obsolete - only used in legacy ocpiassets
  #  $$(call OcpiPrependEnvPath,OCPI_HDL_COMPONENT_LIBRARY_PATH,$$(OcpiTempProjDir)/hdl)
  # when executing applications, look in this project
  $$(call OcpiPrependEnvPath,OCPI_LIBRARY_PATH,\
     $$(OcpiTempProjDir)/components/lib/rcc \
     $$(OcpiTempProjDir)/components/*.test/assemblies/*/container*/target-* \
     $$(OcpiTempProjDir)/components/*/lib/rcc \
     $$(OcpiTempProjDir)/components/*/*.test/assemblies/*/container*/target-* \
     $$(OcpiTempProjDir)/hdl/assemblies/*/container*/target-*)
endef

# Look into a directory in $1 and determine which type of directory it is by looking at the Makefile.
# Return null if there is no type to be found
OcpiGetDirType=$(strip\
  $(and $(wildcard $1/Makefile),\
        $(foreach d,$(shell sed -n \
                      's=^[ 	]*include[ 	]*.*OCPI_CDK_DIR.*/include/\(.*\).mk$$=\1=p' \
                      $1/Makefile | tail -1),\
          $(infox OGT: found type: $d)$(notdir $d))))

# Recursive
OcpiIncludeProjectX=$(infox OIPX:$1:$2:$3)\
  $(if $(wildcard $1/Project.mk),\
    $(if $(wildcard $1/Makefile),\
      $(if $(filter project,$(call OcpiGetDirType,$1)),\
       $(infox found project in $1)$(eval $(call OcpiSetProject,$1)),\
       $(error no proper Makefile found in the directory where Project.mk was found ($1))),\
      $(error no Makefile found in the directory where Project.mk was found ($1))),\
    $(if $(foreach r,$(realpath $1/..),$(filter-out /,$r)),\
      $(call OcpiIncludeProjectX,$1/..,$2,$3),\
      $(call $2,$2: no Project.mk was found here ($3) or in any parent directory)))

# One arg is what to do if not found: error, warning, nothing
# FIXME: can we avoid this when cleaning?
OcpiIncludeProject=$(call OcpiIncludeProjectX,$(or $(OCPI_PROJECT_DIR),.),$1,$(call OcpiAbsDir,.))

define OcpiSetLibrary
  ifeq ($(filter library lib,$(call OcpiGetDirType,$1)),)
    $($2 This directory ($(call OcpiAbsDir,$1)) is not a library)
  endif
  ifneq ($(wildcard $1/Library.mk),)
    include $1/Library.mk
  endif
endef

OcpiIncludeLibrary=$(eval $(call OcpiSetLibrary,$1,$2))

# Find the subdirectories that make a Makefile that includes something
OcpiFindSubdirs=$(strip \
  $(foreach a,$(wildcard */Makefile),\
    $(shell grep -q '^[ 	]*include[ 	]*.*/include/$1.mk' $a && echo $(patsubst %/,%,$(dir $a)))))

endif # ifndef __UTIL_MK__

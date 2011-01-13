
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
.DELETE_ON_ERROR:
__UTIL_MK__=x
# Utilities used by many other makefile files
# Allow us to include this early by establishing the default initial target (all).
all:
Cwd=$(realpath .)
CwdWords=$(subst /, ,$(Cwd))
CwdName=$(word $(words $(CwdWords)),$(CwdWords))
#$(info cwd $(Cwd) words =$(CwdWords)= name $(CwdName))
Models=xm rcc hdl
Model=$(strip $(subst ., ,$(suffix $(CwdName))))
UCModel=$(call ToUpper,$(Model))
AT=@
RM=rm
TIME=/usr/bin/time -f %E
# this is to ensure support for the -n flag
ECHO=/bin/echo
Empty=
#default is to only compile things that are NOT generated
CompiledSourceFiles=$(AuthoredSourceFiles)
Space=$(Empty) $(Empty)
Capitalize=$(shell csh -f -c 'echo $${1:u}' $(1))
UnCapitalize=$(shell csh -f -c 'echo $${1:l}' $(1))
ToUpper=$(shell echo $(1)|tr a-z A-Z)
# function to add a ../ to pathnames, avoiding changing absolute ones
AdjustRelative2=$(foreach i,$(1),$(if $(filter /%,$(i)),$(i),../../$(patsubst ./%,%,$(filter-out .,$(i)))))
AdjustRelative=$(foreach i,$(1),$(if $(filter /%,$(i)),$(i),..$(patsubst %,/%,$(patsubst ./%,%,$(filter-out .,$(i))))))
HostSystem=$(shell uname | tr A-Z a-z)
HostTarget=$(shell echo `uname -s`-`uname -m` | tr A-Z a-z)
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
        $(if $(realpath $(1)),\
            $(if $(realpath $(2)),\
	        $(if $(filter $(firstword $(strip $(subst /, ,$(1)))),$(firstword $(strip $(subst /, ,$(2))))),\
                    $(call FindRelativeStep, $(strip $(subst /, ,$(1))), $(strip $(subst /, ,$(2)))),\
		    $(2)),\
                $(error Invalid/non-existent path: to "$(4)" from "$(3)")),\
             $(error Invalid/non-existent path: from "$(3)" to "$(4)"))

# Function: return the relative path to get from $(1) to $(2).  Useful for creating symlinks
# Note return value must be nicely stripped
#$(info findrel 1 $(1) 2 $(2))
FindRelative=$(strip $(call FindRelativeTop,$(abspath $(1)),$(abspath $(2)),$(1),$(2)))
#FindRelative=$(strip \
#               $(info findrel 1 $(1) 2 $(2))\
#               $(call FindRelativeTop,$(realpath $(1)),$(realpath $(2)),$(strip $(1)),$(strip $(2))))

#               $(foreach i,$(call FindRelativeTop,$(realpath $(1)),$(realpath $(2)),$(strip $(1)),$(strip $(2))),$(info FR:$(i))$(i)))

# Function: retrieve the contents of a symlink - is this ugly or what!
# It would be easier using csh
SymLinkContents= `X=(\`ls -l $(1)\`);echo $${X[$${\#X[*]}-1]}`

# Function:
# Make a symlink, but don't touch it if it is already correct
#  First arg is local file to point to, second arg is dir to put link in.
#  e.g. $(call MakeSymLink,foo,linkdir) makes a link: dir/$(notdir foo) link to foo
# Funky because it might be executed in a loop
MakeSymLink2=	SL=$(2)/$(3); SLC=$(call FindRelative,$(2),$(1)); \
		if test -L $$SL; then OSLC="$(call SymLinkContents,$(2)/$(3))"; fi;\
		if test "$$OSLC" != $$SLC; then \
		  rm -f $$SL; \
		  ln -s $$SLC $$SL; \
		fi
MakeSymLink=$(call MakeSymLink2,$(1),$(2),$(notdir $(1)))


# function of that puts stuff in a temporary file and returns its name.
MakeTemp=$(shell export TMPDIR=$(TargetDir);TF=`mktemp -t -u`;echo "$(1)" | tr " " "\n"> $$TF;echo $$TF)

# Output directory processing.  OutDir is the internal variable used everywhere.
# It is set based on the public OCPI_OUTPUT_DIR, and is created as needed
ifndef OutDir
ifdef OCPI_OUTPUT_DIR
OutDir=$(OCPI_COMPONENT_OUT_DIR)/$(CwdName)/
$(OutDir):
	$(AT)mkdir $@
endif
endif

# Make all target dirs
TargetDir=$(OutDir)target-$(Target)
$(OutDir)target-%: | $(OutDIr)
	$(AT)echo Creating target dir $@
	$(AT)mkdir $@
endif

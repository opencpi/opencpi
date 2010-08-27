ifndef __UTIL_MK__
__UTIL_MK__=x
# Utilities used by many other makefile files
# Allow us to include this early by establishing the default initial target (all).
all:
Cwd=$(realpath .)
CwdName=$(notdir $(Cwd))
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
FindRelativeStep=\
    $(if $(filter $(firstword $(1)),$(firstword $(2))),\
        $(call FindRelativeStep,$(wordlist 2,$(words $(1)),$(1)),$(wordlist 2,$(words $(2)),$(2))),\
	$(if $(1),$(subst $(Space),/,$(strip $(patsubst %,..,$(1))))$(if $(2),/))$(subst $(Space),/,$(2)))

# helper function for FindRelative
# arg1 is absolute-from arg2 is absolute-to arg3 is original from, arg4 is original to
#	$(info 1 $(1) 2 $(2) 3 $(3) 4 $(4))
FindRelativeTop=\
        $(if $(1),\
            $(if $(2),\
	        $(if $(filter $(firstword $(strip $(subst /, ,$(1)))),$(firstword $(strip $(subst /, ,$(2))))),\
                    $(call FindRelativeStep, $(strip $(subst /, ,$(1))), $(strip $(subst /, ,$(2)))),\
		    $(2)),\
                $(error Invalid/non-existent path: to "$(4)" from "$(3)")),\
             $(error Invalid/non-existent path: from "$(3)" to "$(4)"))

# Function: return the relative path to get from $(1) to $(2).  Useful for creating symlinks
# Note return value must be nicely stripped
FindRelative=$(strip $(call FindRelativeTop,$(realpath $(1)),$(realpath $(2)),$(1),$(2)))

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
# It is set based on the public OCPI_OUT_DIR, and is created as needed
ifndef OutDir
ifdef OCPI_OUT_DIR
OutDir=$(OCPI_OUT_DIR)/$(CwdName)/
$(OutDir):
	$(AT)mkdir $@
endif
endif
endif

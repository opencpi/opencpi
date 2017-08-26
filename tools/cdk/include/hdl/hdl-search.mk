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

# This file contains utilities related to searching for primitive cores/libraries,
# component libraries and associated cores, and XML directories for workers.
# it is included by hdl-make but is a separate file to collect all the search-related functions
# in one place
ifndef __HDL_SEARCH_MK__
__HDL_SEARCH_MK__=x

################################################################################
# Project concept.
# While component libraries and primitive libraries (or cores) are each in their own
# world, and the HDL build process can provide lists of dependencies at each level,
# and now can also provide a search path for each to "find the library somewhere in this path",
# there is value in having a higher level *thing* that encompasses a package of
# open-cpi-related deliverables that you might depend on.
# Also, projects have both source and exported vertions, but we will only look for
# the expported stuff from projects
# The shape of a project is like the CDK.
#
# Component library place within an exported project/cdk: for lib xxx
#    binary:  lib/xxx (specs) and lib/xxx/hdl (workers)
# Primitive library place within an exported project/cdk
#    binary   lib/hdl/xxx/ttt


################################################################################
# Component Library searching
#  Component libraries are searched for 2 reasons:
#  1. To find the XML associated with HDL workers: to find if the worker is there at all,
#     and information about it, and its parameter/build configurations.
#  2. To find the binary artifact(s) for the worker for a given target family and 
#     parameter configuration
#
#  The ComponentLibraries user Makefile variable contains a list of libraries, with two types of
#  elements in the list:
#  1. names (no slashes anywhere),
#  2. paths (slashes).
#  For type 1 (names, no slashes), the name means:
#   Search for the named library in all places indicated in
#   OCPI_COMPONENT_LIBRARY_PATH (colon separated) and then look
#   for the named library in all the frameworks indicated in
#   OCPI_PROJECT_PATH (colon separated), then finally look in the framework at
#   OCPI_CDK_DIR.  Essentially the OCPI_CDK_DIR is always the last framework in the path.
#  This means that one name in the ComponentLibraries list
#   expands to a list of same-named libraries where earlier ones can
#   shadow later ones.
#  Rationale for OCPI_COMPONENT_LIBRARY_PATH:
#   allows for a place to contain a batch of libraries, without being a whole framework
#   allows complete ignorance of the notion of frameworks
#  Rational for OCPI_PROJECT_PATH
#   allows convenient use of the place for a batch of built stuff that is the same
#   structure as the delivered core (which may or may not have built FPGA stuff).
#  For type 2 (slashes, whether relative or absolute), this path simply means:
#   Search the library that lives at this path, whether a source library or an exported/installed
#   library.
#  This search structure supports shadowing (where earlier libraries in the path can supply
#  workers that also appear in later libraries).
#  Shadowing is simple-minded - you don't keep searching
#  past libraries that have the worker XML but aren't built for the target, or have the worker,
#  but not the desired parameter configuration.
#  
#  Pattern matching at targeted locations.
#  When a location is examined as a component library, it can be either a source component
#  library, where:
#   - the export subdirectory is called "lib" (where specs live), and
#   - lib/hdl subdirectory is where HDL worker XML and build files live, and 
#   - lib/hdl/<target> where target-specific built artifacts live.
#
#  Alternatively, when a library is exported, its directory (whose name is the library name),
#  acts the same as the "lib" subdirectory in the source library.  Thus in this case the
#  hdl and hdl/<target> diretories are used.
#
#  A final special case in ComponentLibraries is when the path contains target-% which indicates
#  a target-specific directory of a single worker.  This is mostly used internally, but can
#  be useful in very simple cases where there is no library - just a worker's directory by
#  itself.

# Return list of target directories in all possible component libraries
HdlTargetComponentLibraries=$(infox HTCL:$1:$(OcpiComponentLibraries):$(ComponentLibraries):$2)\
  $(or $(strip $(foreach f,$(call HdlGetFamily,$1),\
                  $(foreach d,$(OcpiComponentLibraries),$(infox HTCL1:$d:$f)\
                     $(call HdlExists,$d/hdl/$f)))),\
      $(error No component libraries were found for target $1.  Perhaps not built yet?))

# Return the list of XML search directories for component libraries
# This also includes any top level specs directories in the project path,
# since the search order must be project oriented.
HdlXmlComponentLibraries=$(infox HXC)\
  $(eval HdlTempDirs:= $(strip \
    $(foreach c,$(OcpiComponentLibraries),$c $c/hdl)) $(OCPI_CDK_DIR)/specs) \
  $(infox HdlXmlComponentLibraries returned: $(HdlTempDirs))\
  $(HdlTempDirs)

################################################################################
# Primitive Library searching (actually primitive library and/or core searching)
#  Primitive libraries are searched to find a (usually) precompiled library or core
#  when building higher level entities
#  Primitive libraries and cores (collectively: primitives) differ in that cores are incorporated
#  whole when used, whereas libraries are collections of modules that are only used and instanced
#  if iindividually instanced at a higher level.
#
#  The "Libraries" or "HdlLibraries" or "PrimitiveLibraries" variables provide a list of
#  libraries that should be available to the code at the level they are specified.
#  Similarly the "Cores" variable provides a list of primitive cores that should be available
#  for instantiation.
#
# These variables may be set in the appropriate Makefiles for workers (including device workers
#  and platform workers), and assemblies (application, platform configuration, and container).
#
# The Libraries and Cores variables are used like the ComponentLibraries variable
# to provide a list of either paths (with slashes) or names (without slashes).
# Similarly, the OCPI_HDL_PRIMITIVE_PATH is used to provide an ordered list of places to find
# the libraries and cores mentioned in the Libraries and Cores variable (those items without slashes).

define HdlPrimitiveSearchError
The primitive core/library "$1" was not found in any of these locations: $(call OcpiCheckLinks,$2)
OCPI_HDL_PRIMITIVE_PATH is: $(OCPI_HDL_PRIMITIVE_PATH)
OCPI_PROJECT_PATH is: $(OCPI_PROJECT_PATH)
OCPI_CDK_DIR is: $(OCPI_CDK_DIR)
HdlLibraries is: $(HdlLibraries) $(Libraries)

endef
# Search for a primitive library/core by name, independent of target
# This is not used for primitive libraries/cores specified by location (with slashes)
# $(call HdlSearchPrimitivePath,lib,non-existent-ok,from)
HdlSearchPrimitivePath=$(infox HSPP:$1:$2:$3)\
  $(eval HdlTempPlaces:=$(strip\
       $(subst :, ,$(OCPI_HDL_PRIMITIVE_PATH)) \
       $(foreach d,$(OcpiGetProjectPath),$d/lib/hdl)))\
  $(eval HdlTempDirs:=$(firstword $(strip\
    $(foreach p,$(HdlTempPlaces),$(infos HTPS:$(HdlTempPlaces):$1)\
       $(foreach d,$p/$1,$(infox HTPSD:$d:$(call HdlExists,$d))$(call HdlExists,$d))))))\
  $(or $(HdlTempDirs)$(infox HTD:$(HdlTempDirs)),\
    $(if $(filter clean,$(MAKECMDGOALS))$2,,$(error $(call HdlPrimitiveSearchError,$1,$(HdlTempPlaces)))))

# Collect component libraries independent of targets.
# Normalize the list at the spec level
# No arguments
HdlPrimitiveLibraries=$(infox HPL:$(HdlMyLibraries))$(strip\
    $(foreach p,$(HdlMyLibraries),\
      $(if $(findstring /,$p),\
         $(or $(call HdlExists,$p),\
              $(error Primitive library $p (from HdlLibraries) not found.)),\
         $(call HdlSearchPrimitivePath,$p,,HPL))))

ifneq (,)
# Return list of target directories in all possible primitive libraries
HdlTargetPrimitiveLibraries=$(infox HTPL:$1)\
  $(or $(strip $(foreach f,$(call HdlGetFamily,$1),$(infox F:$f)\
                  $(foreach d,$(HdlPrimitiveLibraries),$(infox D:$d)\
                     $(or $(call HdlExists,$d/$f),$(call HdlExists,$d/target-$f/$(notdir $d)))))),\
      $(if $(HdlPrimitiveLibraries),\
       $(error No primitive libraries were found for target $1.  Perhaps not built yet?)))
endif

################################################################################
# $(call HdlLibraryRefDir,location-dir,target)
# $(call HdlCoreRefDir,location-dir,target)
# These functions take a user-specified (friendly, target-independent) library
# or core location and a target name.  They return the actual directory of that
# library/core that the tool wants to see for that target.
# These are not for component libraries, but a primitive libraries and cores
# HdlLibraryRefDir=$(infox HLRD:$1:$2)$(foreach i,$(call HdlLibraryRefFile,$1,$2),$i)

# The first attempt is:
# If there is a suffix or there is a "target" path component or a component matches the target,
# Then try the path itself with the suffix

# Try to match the alternative HdlBins to the suffix of $1
HdlSuffixContainsAlternativeBin=$(strip $(infox HDSCAB:$1,$2)\
  $(if $(word 1,$2),\
    $(or $(filter $(word 1,$2),$(suffix $1)),\
      $(call HdlSuffixContainsAlternativeBin,$1,$(filter-out $(word 1,$2),$2)))))

# Check whether $1 contains a suffix (either HdlBin or one in
# HdlBinAlternatives_<tool>. Return the suffix of $1 if it
# matches an HdlBin option
HdlSuffixContainsHdlBin=$(strip \
  $(or $(filter $(HdlBin),$(suffix $1)),\
    $(call HdlSuffixContainsAlternativeBin,$1,$(HdlBinAlternatives_$(HdlToolSet)))))

# Iterate through $2 - the list of possible HdlBin suffices
# Return $1 with a suffix appended (the first in $2 that exists)
HdlChooseHdlBinThatExists=$(strip $(infox HCHBTE:$1:$2)\
  $(if $2,\
    $(or $(call HdlExists,$1$(word 1,$2)),$(call HdlChooseHdlBinThatExists,$1,$(filter-out $(word 1,$2),$2)))))

# Check if $1$(HdlBin) exists. If not, try each alternative HdlBin value.
# Return the first one that exists
HdlBinExists=\
  $(call HdlChooseHdlBinThatExists,$1,$(HdlBin) $(HdlBinAlternatives_$(HdlToolSet)))

HdlCRF=$(strip \
  $(foreach r,\
    $(or $(and $(HdlBin),$(call HdlSuffixContainsHdlBin,$1),$(call HdlExists,$1)),$(strip \
         $(infox ff:$(filter $2 target-%,$(subst /, ,$1)):$1$(call HdlSuffixContainsHdlBin,$1):$(call HdlBinExists,$1))\
         $(and $(or $(HdlBin),$(filter $2 target-%,$(subst /, ,$1))),$(call HdlBinExists,$1))),$(strip \
         $(infox ff1:$(filter $2 target-%,$(subst /, ,$1)):$1$(call HdlSuffixContainsHdlBin,$1))\
         $(call HdlBinExists,$1/target-$2/$3)),$(strip \
         $(call HdlBinExists,$1/$3)),$(strip \
         $(call HdlBinExists,$1/$2/$3)),\
	 $1/$2),\
     $(infox HCRF:$1,$2,$3->$r,bin:$(call HdlSuffixContainsHdlBin,$1),t:$(HdlTarget))$r))

# Check for given target or family target
HdlCoreRef1=$(strip \
   $(foreach c,$(notdir $1),\
     $(infox checking $c)$(or $(call HdlExists,$(call HdlCRF,$1,$2,$c)),\
	  $(and $2,$(call HdlExists,$(call HdlCRF,$1,$(call HdlGetFamily,$2),$c$(infox HdlCoreRef1 returning '$c'for '$1' and '$2')))))))

# Look everywhere (including component libraries in some modes), return an error if not found
HdlCoreRef=$(infox HCR:$1:$2:$(HdlMode))$(strip \
  $(or $(strip\
     $(if $(findstring /,$1),\
        $(call HdlCoreRef1,$1,$2),\
        $(firstword \
          $(and $(filter assembly config container,$(HdlMode)),\
                $(foreach l,$(call HdlTargetComponentLibraries,$2,HCR),\
                    $(call HdlCoreRef1,$l/$1,)))\
          $(and $(filter worker container platform,$(HdlMode)),\
                $(foreach l,$(call HdlSearchPrimitivePath,$1,,HCR),$(infox HCR.l:$l)\
                    $(call HdlCoreRef1,$l,$2)))))),\
     $(error No core found for "$1" on target "$2"))\
)

# $(call HdlCoreRefMaybeTargetSpecificFile,core-path-or-name,target)
# Check whether arg 1 is actually a path to a core. This check is done
#   by checking if the string contains '/', if the filename itself
#   contains a '.', and if the path to the file actually exists.
#   This is not foolproof, but gives us a good idea of whether arg 1
#   is a path to a core that should be left as is. If so, return it.
# If arg 1 is NOT a path that should be left alone, determine the
#   tool-specific path to the core.
HdlCoreRefMaybeTargetSpecificFile=$(infox HCRMTSF:$1:$2)$(strip \
  $(if $(and $(findstring /,$1),$(findstring .,$(basename $1)),$(call HdlExists,$1)),\
    $1,\
    $(call HdlCoreRef,$(call HdlToolCoreRef,$1),$2)))

################################################################################
# $(call HdlLibraryRefFile,location-dir,target)
# This function takes a user-specified (friendly, target-independent) library
# or core location and a target name.  It returns the actual pathname of that
# library/core file or directory for "make" dependencies.
# We rely on the underlying tool for the actual filename, if it is not just
# the directory

HdlLRF=$(infox HdlLRF:$1:$2:$3)$(strip \
  $(foreach r,\
    $(if $(call HdlExists,$1/target-$2),\
       $1/target-$2/$(call HdlToolLibraryBuildFile,$(notdir $1)),\
       $(if $(findstring /hdl/$2/,$1),$1,\
         $1/$2/$(call HdlToolInstallFile,$(notdir $1),$t))),\
       $(infox LRF Result:$r)$r))

# Look everywhere (including component libraries in some modes), return an error if not found
HdlLibraryRefDir=$(infox HLRD:$1:$2:$4:$(HdlMode))$(strip \
  $(or $(strip\
     $(if $(findstring /,$1),\
        $(call HdlLRF,$1,$2,slash),\
        $(firstword \
          $(and $(filter assembly config container,$(HdlMode)),\
                $(foreach l,$(call HdlTargetComponentLibraries,$2,HLR),\
                    $(call HdlLRF,$l/$1,,assy)))\
          $(and $(filter worker container platform,$(HdlMode)),\
                $(foreach l,$(call HdlSearchPrimitivePath,$1,x,LRF),$(infox HLR.l:$l)\
                    $(call HdlLRF,$l,$2,wkr)))))),\
     $(if $3,,$(error No library found for "$1" on target "$2")))\
)

endif # of ifndef __HDL_SEARCH_MK__

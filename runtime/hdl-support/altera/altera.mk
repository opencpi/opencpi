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

ifndef _ALTERA_MK
_ALTERA_MK=1
# Functions to retrieve and default pathnames for Altera tools
# FIXME: it may be worth caching these singular values
# FIXME: this is a copy-and-edit version of he xilinx.mk file and they could share more
# The functions to retrieve dirs take an optional second argument which is used in place of
# "error" in error messages.  This is used when the caller wants an empty return rather than
# an error

# The default during installation is sometimes in the home directory
OcpiAlteraDir=$(strip $(foreach t,$(or $(OCPI_ALTERA_DIR),$(wildcard ~/altera),$(wildcard /opt/Altera)),$(infox TT is $t)\
		 $(if $(shell test -d $t && echo 1),$t,\
		    $(call $(or $1,error), Directory "$t" for OCPI_ALTERA_DIR not found))))

# the license file can be either:
# - an absolute path (which can contain @ or :)
# - a network address with and @ or :
# - a path relative to OcpiAlteraDir
OcpiAlteraLicenseFile=$(strip\
  $(foreach e,$(or $(OCPI_ALTERA_LICENSE_FILE),Altera-License.lic),$(infox XX:$e)\
    $(foreach f,$(if $(filter /%,$e),$e,\
                  $(if $(or $(findstring @,$e),$(findstring :,$e)),$e,$(call OcpiAlteraDir,$1)/$e)),\
      $(infox ALF:$f)$(if $(filter /%,$f),\
        $(or $(call OcpiExists,$f),\
          $(call $(or $1,error), File "$f" for the Altera Quartus License File, not found)),\
        $f))))

OcpiAlteraQuartusDir=$(strip\
  $(foreach i,\
    $(or $(OCPI_ALTERA_TOOLS_DIR),\
      $(foreach t,$(call OcpiAlteraDir,$1),\
        $(foreach v,\
          $(if $(filter-out undefined,$(origin OCPI_ALTERA_VERSION)),\
            $(foreach e,$(OCPI_ALTERA_VERSION),\
              $(if $(shell test -d $t/$e && echo 1),$e,\
                $(call $(or $1,error), Directory "$t/$e" for OCPI_ALTERA_VERSION not found))),\
            $(or $(shell for i in \
                        `shopt -s nullglob && echo $t/*  | tr ' ' '\n' | sort -n -r`; \
                       do \
                         [ -d $$i -a -d $$i/quartus ] && echo `basename $$i` && break; \
                       done),\
                 $(call $(or $1,error), No version directory under $t/*/q* for Altera tools))),\
        $(infox VV:$v)$t/$v)))/quartus,\
    $(infox II:$i.)\
    $(if $(shell test -d $i && echo 1),$i,\
      $(call $(or $1,error), Directory "$i" for Altera Quartus tools, not found))))
# We call this if all we really need is lab tools (e.g. impact)
# First look for lab tools, then look for ise
OcpiAlteraProgrammerDir=$(strip\
$(if $(OCPI_ALTERA_LAB_TOOLS_DIR),\
  $(foreach d,$(OCPI_ALTERA_LAB_TOOLS_DIR),\
    $(or $(wildcard $d/quartus),$(wildcard $d/qprogrammer),\
      $(call $(or $1,error),OCPI_ALTERA_LAB_TOOLS_DIR, $d, missing or has no quartus/qprogrammer subdirectory))),\
  $(foreach t,$(call OcpiAlteraDir,$1),\
      $(foreach v,\
        $(if $(filter-out undefined,$(origin OCPI_ALTERA_VERSION)),\
          $(foreach e,$(OCPI_ALTERA_VERSION),\
            $(or $(shell test -d $t/$e && echo $e),\
              $(call $(or $1,error), Directory "$t/$e", for OCPI_ALTERA_VERSION, not found))),\
          $(or $(shell \
               for i in `shopt -s nullglob && echo $t/*  | tr ' ' '\n' | sort -n -r`; \
                       do \
                   [ -d "$$i/quartus" -o -d "$$i/qprogrammer" ] && echo `basename $$i` && break; \
                       done),\
            $(call $(or $1,error), No version directory under $t for Altera Quartus or programming tools))),\
      $(foreach d,$(or $(wildcard $t/$v/quartus),$(wildcard $t/$v/qprogrammer),\
                    $(call $(or $1,error), Directory $t/$v has no ISE or LabTools under it)),\
        $(patsubst %/,%,$(dir $d)))))))

# There is no default install place for these kits so we assume a default of "kits"
# under the basic altera dir, with versions under that directory since kit versions
# do not track tool versions.
OcpiAlteraKitsDir=$(strip\
$(if $(OCPI_ALTERA_KITS_DIR),\
  $(foreach d,$(OCPI_ALTERA_KITS_DIR),\
    $(or $(wildcard $d/*),\
      $(call $(or $1,error),OCPI_ALTERA_KITS_DIR, $d, is missing or has no subdirectories))),\
  $(foreach t,$(call OcpiAlteraDir,$1),\
    $(or $(shell test -d $t/kits && echo $t/kits),\
      $(call $(or $1,error), Directory "$t/kits", the default for OCPI_ALTERA_KITS_DIR, not found)))))

# The trick here is to filter the output and capture the exit code.
# Note THIS REQUIRES BASH not just POSIX SH due to pipefail option
DoAltera=(set -o pipefail; set +e; \
         export LM_LICENSE_FILE=$(call OcpiAlteraLicenseFile,) ; \
         $(TIME) $(OcpiAlteraQuartusDir)/bin/$1 --64bit $2 > $3-$4.tmp 2>&1; \
         ST=\$$?; sed 's/^.\[0m.\[0;32m//' < $3-$4.tmp > $3-$4.out ; \
         rm $3-$4.tmp ; \
         if test \$$ST = 0; then \
           $(ECHO) -n $1:' succeeded ' ; \
           tail -1 $3-$4.out | tee x1 | tr -d \\\n | tee x2; \
           $(ECHO) -n ' at ' && date +%T ; \
         else \
           $(ECHO)  $1: failed with status \$$ST \(see results in $(TargetDir)/$3-$4.out\); \
         fi; \
         exit \$$ST)

DoAltera1=(set -o pipefail; set +e; \
         export LM_LICENSE_FILE=$(call OcpiAlteraLicenseFile,) ; \
         $(TIME) $(OcpiAlteraQuartusDir)/bin/$1 --64bit $2 > $3-$4.tmp 2>&1; \
         ST=$$$$?; sed 's/^.\[0m.\[0;32m//' < $3-$4.tmp > $3-$4.out ; \
         rm $3-$4.tmp ; \
         if test $$$$ST = 0; then \
           $(ECHO) -n $1:' succeeded ' ; \
           tail -1 $3-$4.out | tee x1 | tr -d \\\n | tee x2; \
           $(ECHO) -n ' at ' && date +%T ; \
         else \
           $(ECHO)  $1: failed with status $$$ST \(see results in $(TargetDir)/$3-$4.out\); \
         fi; \
         exit $$$$ST)

# emit shell assignments - allowing errors etc.
ifdef ShellIseVars
  ShellQuartusVars=1
endif
ifdef ShellQuartusVars

AlteraCheck=ignore
all:

$(info OcpiAlteraQuartusDir=$(call OcpiAlteraQuartusDir,$(AlteraCheck));\
       OcpiAlteraProgrammerDir=$(call OcpiAlteraProgrammerDir,$(AlteraCheck));\
       OcpiAlteraKitsDir=$(call OcpiAlteraKitsDir,$(AlteraCheck));\
       OcpiAlteraLicenseFile=$(call OcpiAlteraLicenseFile,$(AlteraCheck));)

endif
endif

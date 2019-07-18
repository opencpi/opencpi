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

include $(OCPI_CDK_DIR)/include/util.mk

$(OcpiIncludeProject)

ifeq ($(origin Applications),undefined)
  Applications:=$(call OcpiFindSubdirs,application) $(wildcard *.xml)
  $(call OcpiDbgVar,Applications)
endif

.PHONY: all test clean run

FILTER=$(strip \
         $(if $(OcpiApp),\
            $(foreach a,$(Applications),$(and $(filter $(OcpiApp),$(basename $a)),$a)),\
            $(foreach a,$(Applications),\
              $(and $(filter-out $(ExcludeApplications),$(basename $a)),$a))))

DOALL=$(AT)\
  set -e;\
  for i in $(filter-out %.xml,$(FILTER)); do\
    echo ========$1 $$i: ; $(MAKE) --no-print-directory -C $$i $2;\
  done
OcpiUse=$(if $(filter undefined,$(origin OcpiRun$1_$2)),$(OcpiRun$1),$(OcpiRun$1_$2))
ifndef OcpiRunXML

  OcpiRunXML=$(strip \
    $(call OcpiUse,Before,$1) \
    $(if $(or $2,$(OCPI_LIBRARY_PATH)),,OCPI_LIBRARY_PATH=$(call OcpiGetDefaultLibraryPath)) \
    $(if $2,ocpirun,$(OCPI_CDK_DIR)/$(OCPI_TOOL_DIR)/bin/ocpirun) \
    $(call OcpiUse,Args,$1) \
    $1 \
    $(call OcpiUse,After,$1))

endif

all:
	$(call DOALL,Building apps,)

clean::
	$(call DOALL,Cleaning,clean)

test:
	$(call DOALL,Building tests for,test)

run:
	$(call DOALL,Running,run)
	$(AT)$(infox FILTER:$(FILTER))$(infox Applications:$(Applications))$(and $(filter %.xml,$(FILTER)),\
	       echo "======== Running local XML application(s):" $(filter %.xml,$(FILTER)) \
               $(foreach a,$(basename $(filter %.xml,$(FILTER))),\
                && echo "========= $(call OcpiRunXML,$a,x)" && $(call OcpiRunXML,$a)))

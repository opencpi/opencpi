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

# The makefile fragment for libraries, perhaps under a project, perhaps not.

include $(OCPI_CDK_DIR)/include/util.mk
include $(OCPI_CDK_DIR)/include/lib.mk

.PHONY: generate run $(OcpiTestGoals) showincludes showpackage showworkers showtests
.SILENT: showincludes showpackage showworkers showtests
run: runtest # generic "run" runs test
OldTests=$(foreach i,$(TestImplementations),\
           $(shell [ -f $i/Makefile ] && grep -q '(OCPI_CDK_DIR)/include/test.mk' $i/Makefile || echo $i))
$(filter-out test cleantest,$(OcpiTestGoals)):
	$(AT)set -e; for i in $(filter-out $(OldTests),$(TestImplementations)); do \
	  echo ==============================================================================;\
	  echo ==== Performing goal \"$@\" for unit tests in $$i;\
	  $(MAKE) --no-print-directory -C $$i $@ ; \
	done

# The ordering here assumes HDL cannot depend on RCC.
generate:
	$(call BuildModel,hdl,generate)
	$(call BuildModel,ocl,generate)
	$(call BuildModel,rcc,generate)
	$(call BuildModel,test,generate)

ifdef ShellLibraryVars
showlib:
	$(info Tests="$(TestImplementations); Workers="$(Implementations)"; Package="$(Package)";)
showtests:
	$(info Tests="$(TestImplementations)";)
showworkers:
	$(info Workers="$(Implementations)";)
showpackage:
	$(info Package="$(Package)";)
showincludes:
	$(call OcpiSetXmlIncludes)
	$(info XmlIncludeDirsInternal="$(XmlIncludeDirsInternal)";)
endif

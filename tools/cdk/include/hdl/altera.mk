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

# Common definition for altera tools
ifndef OCPI_ALTERA_TOOLS_DIR
  ifneq (clean,$(MAKECMDGOALS))
    $(error The variable OCPI_ALTERA_TOOLS_DIR must be defined.)
  endif
endif
# The trick here is to filter the output and capture the exit code.
# Note THIS REQUIRES BASH not just POSIX SH due to pipefail option
DoAltera=(set -o pipefail; set +e; \
         export LM_LICENSE_FILE=$(OCPI_ALTERA_LICENSE_FILE) ; \
         $(TIME) $(OCPI_ALTERA_TOOLS_DIR)/quartus/bin/$1 --64bit $2 > $3-$4.tmp 2>&1; \
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
         export LM_LICENSE_FILE=$(OCPI_ALTERA_LICENSE_FILE) ; \
         $(TIME) $(OCPI_ALTERA_TOOLS_DIR)/quartus/bin/$1 --64bit $2 > $3-$4.tmp 2>&1; \
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


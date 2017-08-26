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

ifneq ($(filter $(MAKECMDGOALS),test),)
  ALL:=bias2_xml bias_xml cat_xml copy ext2file_xml fsk_modem_xml hello hello_xml ptest vsadd xml
else
  ALL:=$(wildcard *[^~])
endif

DOALL=$(AT)\
  set -e;\
  set -o pipefail;\
  for i in $(ALL); do\
    if test -d $$i; then\
      echo ========$1 $$i: ;\
      export TGT=$2;\
      $(MAKE) --no-print-directory -C $$i $2 2>&1 | tee $${i}_$${TGT:-build}.log;\
    fi;\
  done

all:
	$(call DOALL,Building,)
clean:
	$(call DOALL,Cleaning,clean)
	$(AT)rm -r -f *.log simulations
test run:
	$(call DOALL,Running,run)

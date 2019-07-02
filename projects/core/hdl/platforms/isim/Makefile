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

# The isim platform.
HdlLibraries+=sdp
ExportFiles=runSimExec.isim probe.exe.tgz
include $(OCPI_CDK_DIR)/include/hdl/hdl-platform.mk

ifneq ($(findstring isim,$(HdlPlatforms)),)
# Make the minimal simulation executable to run to verify licensing
# since the xlicmgr tool is useless.  grrrr.
# Also note that this result is put in the repo and not cleaned so that ocpirun -C shows it
# without building HDL.
# Also note that is must be built on centos6 for the same thing to be usable on both c6 and c7

probe.exe.tgz:
	$(AT)echo Creating the tiny isim project for probing that isim is installed and licensed.
	$(AT)echo Log output for this is in target-isim/probe.out
	$(AT)mkdir -p target-isim
	$(AT)(rm -r -f tmp && mkdir tmp && cd tmp && \
	  source $(OcpiXilinxIseDir)/settings64.sh && \
	  export LM_LICENSE_FILE=$(OcpiXilinxLicenseFile) && \
	  echo 'module probe(); endmodule' > probe.v && \
	  vlogcomp -v 2 -work probe=probe probe.v && \
	  fuse probe.probe -o probe.exe -lib probe=probe && \
          find . \( -name "*.[co]" -o -name "*.log" -o -name "*.xmsgs" -o -name "*.cmd" -o \
                    -name "*.html" \) -delete && \
	  rm -f -r probe probe.v */*/work && \
	  for i in `find . -name "*.exe"`; do ../fixabs.sh $$i || exit 1; done && \
	  tar -cz --owner=0 --group=0 -f ../probe.exe.tgz probe.exe isim) > target-isim/probe.out 2>&1

#	  fuse probe.probe -v 2 -L unisims_ver -o probe.exe -lib probe=probe && \

all: probe.exe.tgz

clean::
	rm -r -f tmp

endif

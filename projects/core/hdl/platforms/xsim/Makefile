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

# The xsim platform.
HdlLibraries+=sdp
ExportFiles=runSimExec.xsim probe.exe.tgz
include $(OCPI_CDK_DIR)/include/hdl/hdl-platform.mk

ifneq ($(findstring xsim,$(HdlPlatforms)),)
probe.exe.tgz:
	$(AT)echo "Creating the tiny xsim project for probing that xsim is installed and licensed."
	$(AT)echo "Output is in target-xsim/probe.out"
	$(AT)mkdir -p target-xsim
	(export CUR=`pwd` && \
	  export DES=`mktemp -d` && \
	  mkdir -p $${DES}/probe && cd $${DES} && \
	  source $(OcpiXilinxVivadoDir)/.settings64-Vivado.sh && \
	  export LM_LICENSE_FILE=$(OcpiXilinxVivadoLicenseFile) && \
	  echo 'module probe(); endmodule' > probe.v && \
	  xvlog -v 2 probe.v && \
	  xelab -debug typical -top probe -s probe.exe -verbose && \
	  rm -f -r probe.v */work && \
	  tar -c -z --owner=0 --group=0 -f $${CUR}/probe.exe.tgz xsim.dir/probe.exe && \
	  cd $${CUR} && \
	  rm -r -f $${DES}) > target-xsim/probe.out 2>&1

all: probe.exe.tgz

endif

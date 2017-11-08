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

# This is a top level makefile used when an HDL assembly is being built.
# It is executed by a local sub-make in the same directory to create a workers file in the
# gen subdirectory.  It generates the output that is used by the parent
# Makefile to establish dependencies of assemblies on their constituent workers.
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk
all: $(AssyWorkersFile)
-include $(AssyWorkersFile).deps
$(AssyWorkersFile): $(Worker_xml) | $(GeneratedDir)
	$(AT)$(call OcpiGen, -D $(GeneratedDir)\
                        $(and $(Platform),-P $(Platform)) $(and $(Assembly),-S $(Assembly))\
			$(and $(PlatformDir),-F $(PlatformDir)) \
                        -W $(Worker) $<)



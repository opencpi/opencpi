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

# During the build process, swap HdlTarget with 'actual' target part of the target platform
# We do this to build for a part with the correct number of resources, such as hardware
# multipliers, rather than the default smallest part in the family
# zynq => xc7z020 (ZedBoard, Matchstiq-Z1, Picoflexor-T6A)
# virtex6 => xc6vlx240t (ml605 development board)
# alst4 => ep4sgx230k (Stratix4 development board)
HdlExactPart=\
	$(or $(and $(filter zynq_ise,$(HdlTarget)),xc7z020_ise_alias-1-clg484), \
	$(and $(filter zynq,$(HdlTarget)),xc7z020-1-clg484), \
	$(and $(filter virtex6,$(HdlTarget)),xc6vlx240t-1-ff1156), \
	$(and $(filter stratix4,$(HdlTarget)),ep4sgx230k-c2-f40))

include $(OCPI_CDK_DIR)/include/worker.mk

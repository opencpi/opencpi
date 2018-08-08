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

#OnlyPlatforms=matchstiq_z1 #List of tested platforms
# Ideally, this worker would only build for the Matchstiq-Z1 platform. However, this is blocked by AV-2044.
# For now, only build for the target that represents the Matchstiq-Z1.
OnlyTargets=zynq
Libraries=i2c
#Param_NUSERS_p=5
#Param_SLAVE_ADDRESS_p=0x45,0x71,0x48,0x21,0x20
#ParamValues_CLK_FREQ_p=100e6/50e6
HdlExactPart=xc7z020-1-clg484
include $(OCPI_CDK_DIR)/include/worker.mk

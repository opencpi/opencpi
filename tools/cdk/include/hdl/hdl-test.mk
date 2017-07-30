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

OCPI_CDK_DIR=../..
include hdl-pre.mk
HdlError:=warning
define DoFamily
$(info For $(1) Family:)
$(info -> $(call HdlGetFamily,$(1)))
$(info For $(1) Tool:)
$(info -> $(call HdlGetToolSet,$(1)))
endef
x:=$(call DoFamily,123)
y:=$(call DoFamily,virtex5)
z:=$(call DoFamily,isim)
q:=$(call DoFamily,xc6vlx240t)
x:=$(call DoFamily,xilinx)
x:=$(call DoFamily,verilator)
q:=$(call DoFamily,xc5vsx95t)
x: $(call DoFamily,test1)
x: $(call DoFamily,all)



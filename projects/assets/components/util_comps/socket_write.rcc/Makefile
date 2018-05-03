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

# This is the Makefile for worker socket_write.rcc
RccOnlyPlatforms=centos6 centos7 macos10_12 pico_t6a xilinx13_3 xilinx13_4

# Two parameters: cross-host and if second is empty it will crash
boost_static_finder=$(or $(shell locate libboost_thread | grep "\.a" | grep "$1" | head -1 | perl -ne 'm|(.*)/libboost| && print $$1'),$(or $2,$(error Could not find boost static libraries for $1)))

# This must be defined before worker.mk imported (and uses non-crashing version of boost_static_finder)
ifeq (0,$(call boost_static_finder,arm-none-linux-gnueabi,0))
$(warning Could not find boost static libraries for pico_t6a; disabling that platform.)
RccExcludePlatforms+=pico_t6a
endif
ifeq (0,$(call boost_static_finder,^/usr/lib,0))
$(warning Could not find boost static libraries for centos6; disabling that platform. If running on CentOS6, ensure mlocate and boost-static RPMs installed.)
RccExcludePlatforms+=centos6
endif

# We can use ASIO in standalone mode starting with gcc 4.7 (AV-2003)
# Yes, this is ugly and messy, to support CentOS 6 (4.4 with local boost) and Picoflexor t6a (4.4 with DRS-provided libboost)
CC_Common=-I$(dir $@)asio/include -I./ext_src
CC_Standalone=-DASIO_STANDALONE $(CC_Common)
CC_Boost=-I./asio/local_boost $(CC_Common)

# This is for SDP only; will not work on older framework
RccExtraCompileOptionsCC_centos6+=--std=c++0x $(CC_Boost)
RccExtraCompileOptionsCC_centos7+=--std=c++11 $(CC_Standalone)
RccExtraCompileOptionsCC_macos10_12+=--std=c++0x $(CC_Standalone)
RccExtraCompileOptionsCC_pico_t6a+=--std=c++0x $(CC_Boost)
RccExtraCompileOptionsCC_xilinx13_3+=--std=c++0x $(CC_Standalone)
RccExtraCompileOptionsCC_xilinx13_4+=--std=c++0x $(CC_Standalone)

MakeASIO_Host=cd $(dir $@) && cp -R ../asio . && $(MAKE) -sC asio -f Makefile.av ASIO_NO_BOOST=$1 && cd ..
MakeASIO_Cross=cd $(dir $@) && cp -R ../asio . && \
               $(MAKE) -C asio -f Makefile.av ASIO_NO_BOOST=$2 OCPI_CROSS_HOST=$1 OCPI_CROSS_BUILD_BIN_DIR=$3 \
               && cd ..
Setup_ASIO_linux-c6-x86_64=$(call MakeASIO_Host,0)
Setup_ASIO_linux-c7-x86_64=$(call MakeASIO_Host,1)
Setup_ASIO_linux-x13_3-arm=$(call MakeASIO_Cross,arm-xilinx-linux-gnueabi,1,$(dir $(firstword $(Gc++_linux-x13_3-arm))))
Setup_ASIO_linux-x13_4-arm=$(call MakeASIO_Cross,arm-xilinx-linux-gnueabi,1,$(dir $(firstword $(Gc++_linux-x13_4-arm))))
Setup_ASIO_linux-zynq-arm_cs=$(call MakeASIO_Cross,arm-none-linux-gnueabi,0,$(dir $(firstword $(Gc++_linux-zynq-arm_cs))))
Setup_ASIO_macos-10_12-x86_64=$(call MakeASIO_Host,1)

RccCompileDependencies+=$(dir $@)/asio

# Boost library location
RccLocalLibs_linux-c6-x86_64+=-lboost_thread-mt -Xlinker -rpath -Xlinker '$${ORIGIN}'
RccLocalLibs_linux-zynq-arm_cs+=$(foreach lib, thread system, $(call boost_static_finder,arm-none-linux-gnueabi)/libboost_$(lib).a) -lrt

ext_src_files=connection connection_manager server

SourceFiles=$(foreach f,$(ext_src_files),ext_src/$f.cpp) # AV-1560

include $(OCPI_CDK_DIR)/include/worker.mk

%/asio: | %
	$(call Setup_ASIO_$(RccTarget))

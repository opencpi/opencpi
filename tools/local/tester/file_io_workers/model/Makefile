# Copyright (c) 2009 Mercury Federal Systems.
# 
# This file is part of OpenCPI.
# 
# OpenCPI is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# OpenCPI is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

CPI_RCC_GEN = ../../../../..//tools/local/binder/linux-bin/cpi-rcc-gen

all: ../include/fileSource.h ../include/fileSink.h

../include/fileSource.h: FileSource.scd.xml FileSourceComponent.prf.xml
	$(CPI_RCC_GEN) \
		--name=FileSource \
		--gen-header-file=$@ \
		FileSource.scd.xml

../include/fileSink.h: FileSink.scd.xml FileSinkComponent.prf.xml
	$(CPI_RCC_GEN) \
		--name=FileSink \
		--gen-header-file=$@ \
		FileSink.scd.xml

clean:
	rm -rf *~

distclean: clean

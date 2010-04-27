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

ICON = dashboard256-1.ico
IMAGES = appfactory.gif application.gif component.gif \
	device.gif devicemanager.gif domainmanager.gif \
	resourcefactory.gif service.gif
KITDIR = $(abspath ../../tcl/tclkit)
BWIDGET = $(abspath ../../tcl/bwidget)
COMBAT = $(abspath ../../tcl/combat)
WINKIT = $(KITDIR)/tclkit-win32.upx.exe
LINKIT = $(KITDIR)/tclkit-linux-x86

ifeq ($(findstring Cygwin,$(shell uname -a)),Cygwin)
SDX = $(KITDIR)/tclkitsh-win32.upx.exe $(shell cygpath --mixed $(KITDIR)/sdx.kit)
else
SDX = $(LINKIT) $(KITDIR)/sdx.kit
endif

all: exe
exe: scatman.exe scatman-linux-x86
kit: scatman.kit

scatman.exe: scatman.kit $(ICON)
	rm -rf temp
	mkdir temp
	cp $< temp/scatman.kit
	(cd temp; $(SDX) unwrap scatman.kit)
	cp $(ICON) temp/scatman.vfs/tclkit.ico
	cp $(WINKIT) temp/tclkit.exe
	(cd temp; $(SDX) wrap scatman.kit -runtime tclkit.exe)
	mv temp/scatman.kit $@
	rm -rf temp

scatman-linux-x86: scatman.kit
	rm -rf temp
	mkdir temp
	cp $< temp/scatman.kit
	(cd temp; $(SDX) unwrap scatman.kit)
	cp $(LINKIT) temp/tclkit.bin
	(cd temp; $(SDX) wrap scatman.kit -runtime tclkit.bin)
	mv temp/scatman.kit $@
	rm -rf temp

scatman.kit: scatman.tcl $(IMAGES)
	rm -rf temp
	mkdir temp
	cp scatman.tcl temp
	(cd temp; $(SDX) qwrap scatman.tcl)
	(cd temp; $(SDX) unwrap scatman.kit)
	cp $(IMAGES) temp/scatman.vfs/lib/app-scatman/
	cp -R $(BWIDGET) temp/scatman.vfs/lib/
	cp -R $(COMBAT) temp/scatman.vfs/lib/
	(cd temp; $(SDX) wrap scatman.kit)
	mv temp/scatman.kit $@
	rm -rf temp

distclean: clean
	rm -rf scatman-linux-x86
	rm -rf *.exe *.kit *.zip *.tar.gz

clean:
	find . -name '*~' -exec rm -f {} \;
	rm -rf config.*
	rm -rf core a.out *~ *.o *.I
	rm -rf temp

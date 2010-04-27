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

KITDIR = $(abspath ../../tcl/tclkit)
TKTABLE = $(abspath ../../tcl/tktable)
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
exe: lumberjack.exe lumberjack-linux-x86
kit: lumberjack.kit

lumberjack.exe: lumberjack.kit
	rm -rf temp
	mkdir temp
	cp $^ temp/lumberjack.kit
	(cd temp; $(SDX) unwrap lumberjack.kit)
	cp lumberjack.ico temp/lumberjack.vfs/tclkit.ico
	cp $(WINKIT) temp/tclkit.exe
	(cd temp; $(SDX) wrap lumberjack.kit -runtime tclkit.exe)
	mv temp/lumberjack.kit $@
	rm -rf temp

lumberjack-linux-x86: lumberjack.kit
	rm -rf temp
	mkdir temp
	cp $^ temp/lumberjack.kit
	(cd temp; $(SDX) unwrap lumberjack.kit)
	cp $(LINKIT) temp/tclkit.bin
	(cd temp; $(SDX) wrap lumberjack.kit -runtime tclkit.bin)
	mv temp/lumberjack.kit $@
	rm -rf temp

lumberjack.kit: lumberjack.tcl
	rm -rf temp
	mkdir temp
	mkdir temp/lumberjack
	cp lumberjack.tcl temp/lumberjack
	(cd temp/lumberjack; $(SDX) qwrap lumberjack.tcl)
	(cd temp/lumberjack; $(SDX) unwrap lumberjack.kit)
	cp -R $(TKTABLE) temp/lumberjack/lumberjack.vfs/lib
	cp -R $(BWIDGET) temp/lumberjack/lumberjack.vfs/lib
	cp -R $(COMBAT) temp/lumberjack/lumberjack.vfs/lib
	(cd temp/lumberjack; $(SDX) wrap lumberjack.kit)
	mv temp/lumberjack/lumberjack.kit $@
	rm -rf temp

distclean: clean
	rm -rf lumberjack-linux-x86
	rm -rf *.exe *.kit *.zip *.tar.gz

clean:
	find . -name '*~' -exec rm -f {} \;
	rm -rf config.*
	rm -rf core a.out *~ *.o *.I
	rm -rf temp

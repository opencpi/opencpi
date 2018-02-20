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

.PHONY: all clean
.SILENT: all clean

all: lib/inode64.so lib64/inode64.so

lib64/inode64_empty.c:
	mkdir -p lib64
	touch $@

lib64/inode64.o: lib64/inode64_empty.c
	mkdir -p lib64
	gcc -c -fPIC -m64 $^ -o $@

lib64/inode64.so: lib64/inode64.o
	ld -shared -melf_x86_64 --version-script vers -o $@ $^

lib/inode64.o: inode64.c
	mkdir -p lib
	gcc -c -fPIC -m32 $^ -o $@ -ldl

lib/inode64.so: lib/inode64.o
	ld -shared -melf_i386 --version-script vers -o $@ $^ /usr/lib/libdl.so

clean:
	-rm -rf lib lib64

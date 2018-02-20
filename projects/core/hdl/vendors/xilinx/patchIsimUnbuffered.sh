#!/bin/sh
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

# patch the opensource STLPort library that is embedded in this Xilinx library to
# force streamio to be unbuffered.
# Ideally there would be some way for a VHDL file to be specified as unbuffered,
# but this patch is a big hammer - all output through this iostream library is unbuffered.
cp $OCPI_XILINX_TOOLS_DIR/ISE/lib/lin64/libhsimengine.so.orig .
gdb -n -q -write libhsimengine.so <<EOF | grep -v '(gdb) (gdb)'| grep -v 'Reading symbols'
# add code at the bottom of the xsputn method to call the
# overflow method to flush the buffer, and jmp to exit the method with the proper return value
#   0xd0441:	mov    0x0(%rbp),%rax
#   0xd0445:	mov    $0xffffffff,%esi
#   0xd044a:	jmpq   0xd0371
#   0xd0371:	callq  *0x68(%rax)
#   0xd0374:	jmpq   0xd0424 <_ZN8stlp_std15basic_streambufIcNS_11char_traitsIcEEE6xsputnEPKcl+132>
set {int}0xd0440 = 0x458b48ac
set {int}0xd0444 = 0xffffbe00
set {int}0xd0448 = 0x22e9ffff
set {int}0xd044c = 0x90ffffff
set {int}0xd0370 = 0xef8948ff
set {int}0xd0374 = 0xe96850ff
set {int}0xd0378 = 0x000000a8

# patch the xsputn method to jump to the above patch
# 0xd03f7 <_ZN8stlp_std15basic_streambufIcNS_11char_traitsIcEEE6xsputnEPKcl+87>:	cmp    %r14,%r13
# 0xd03fa <_ZN8stlp_std15basic_streambufIcNS_11char_traitsIcEEE6xsputnEPKcl+90>:	jle    0xd0441
set {int}0xd03f8 = 0x457ef539
quit
EOF

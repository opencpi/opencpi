# This spoofs the build host for both 32- and 64-bit applications

# To use:
# 1. Add libmyhostname as a target that calls rpmbuild
# 2. Add "myhostnameclean" as a target to your "clean"
# 3. Call rpmbuild or any other program with $(SPOOF_HOSTNAME) prefix

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

MYHOSTNAME_MNAME:=$(shell uname -m)
libmyhostname:=libmyhostname_$(MYHOSTNAME_MNAME).so
MYHOSTNAME_PWD?=$(shell pwd)
# This mess is to NOT expand ${LIB} or $LIB if found in current LD_PRELOAD
ifneq (,$(LD_PRELOAD))
  ifneq (,$(L))
    $(error L is defined but it cannot be. Sorry.)
  endif
  ifneq (,$(LIB))
    $(error LIB is defined but it cannot be. Sorry.)
  endif
  export L=_X___LIB___X_
  export LIB=_X___LIB___X_
  export OLD_LD_PRELOAD1=$(subst _X___LIB___X_IB,\$$LIB,$(LD_PRELOAD))
  export OLD_LD_PRELOAD=$(subst _X___LIB___X_,\$$LIB,$(OLD_LD_PRELOAD1))
  unexport L
  unexport LIB
endif
SPOOF_HOSTNAME:=LD_PRELOAD="$(MYHOSTNAME_PWD)/myhostname/\$$LIB/$(libmyhostname):$(OLD_LD_PRELOAD)"

.PHONY: myhostnameclean
.SILENT: myhostnameclean
.IGNORE: myhostnameclean
myhostnameclean:
	rm -rf $(MYHOSTNAME_PWD)/myhostname

# Linux doesn't support explicit 32- vs. 64-bit LD paths like Solaris, but ld.so
# does accept a literal "$LIB" in the path to expand to lib vs lib64. So we need
# to make our own private library tree myhostname/lib{,64} to feed to rpmbuild.
.PHONY: libmyhostname
.SILENT: libmyhostname
libmyhostname: /usr/include/gnu/stubs-32.h /usr/include/gnu/stubs-64.h /lib/libgcc_s.so.1
	mkdir -p $(MYHOSTNAME_PWD)/myhostname/lib{,64}
	$(MAKE) -I $(MYHOSTNAME_PWD) -s --no-print-directory -C $(MYHOSTNAME_PWD)/myhostname/lib   -f $(MYHOSTNAME_PWD)/Makefile $(libmyhostname) MYHOSTARCH=32
	$(MAKE) -I $(MYHOSTNAME_PWD) -s --no-print-directory -C $(MYHOSTNAME_PWD)/myhostname/lib64 -f $(MYHOSTNAME_PWD)/Makefile $(libmyhostname) MYHOSTARCH=64

.SILENT: /usr/include/gnu/stubs-32.h /usr/include/gnu/stubs-64.h /lib/libgcc_s.so.1
/usr/include/gnu/stubs-32.h:
	echo "You need to install the 'glibc-devel.i686' package."
	echo "'sudo yum install glibc-devel.i686' should do it for you."
	false

/usr/include/gnu/stubs-64.h:
	echo "You need to install the 'glibc-devel.x86_64' package."
	echo "'sudo yum install glibc-devel.x86_64' should do it for you."
	false

/lib/libgcc_s.so.1:
	echo "You need to install the 'libgcc.i686' package."
	echo "'sudo yum install libgcc.i686' should do it for you."
	false

.SILENT: libmyhostname $(libmyhostname) libmyhostname_$(MYHOSTNAME_MNAME).o libmyhostname_$(MYHOSTNAME_MNAME).c
$(libmyhostname): libmyhostname_$(MYHOSTNAME_MNAME).o
	echo "Building $(MYHOSTARCH)-bit version of hostname spoofing library."
	gcc -m$(MYHOSTARCH) -shared -o $@ $<

libmyhostname_$(MYHOSTNAME_MNAME).o: libmyhostname_$(MYHOSTNAME_MNAME).c
	gcc -m$(MYHOSTARCH) -fPIC -rdynamic -g -c -Wall $<

ifdef NODE_NAME
THISHOST=$(NODE_NAME)_$(MYHOSTNAME_MNAME).jenkins
else
THISHOST=buildhost_$(MYHOSTNAME_MNAME)
endif
libmyhostname_$(MYHOSTNAME_MNAME).c:
	echo "$$libmyhostname_body" > $@

define libmyhostname_body
#include <string.h>
#include <asm/errno.h>

int gethostname(char *name, size_t len) {
	const char *myhostname = "$(THISHOST).fakehostname";
	if (len < strlen(myhostname))
		return(EINVAL);
	strcpy(name, myhostname);
	return(0);
}
endef
export libmyhostname_body

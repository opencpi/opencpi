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

# Note: before AV-3485, this file would do git checkouts and synchronize branch names, etc.
# Most of that is still present in releng/assets/Makefile. Or check this file's history.

.PHONY: help all clean rpms cleanrpmbuild
.SILENT: help all clean rpms cleanrpmbuild

# Default
help:
	printf "This is the Makefile to build prerequisite RPMs for OpenCPI\n\n"
	echo "The valid targets are:"
	echo " all / rpms            - Builds all RPMs"
	echo " clean                 - 'cleanrpmbuild' and then runs 'git clean -fx .'"
	echo " cleanrpmbuild         - Cleans ~/rpmbuild/"
	echo ""
	echo "Internally used:"
	echo " $(firstword $(PACKAGES))_rpm, etc - Builds specific RPM"
	echo ""
	echo "Packages available: $(PACKAGES)"
	echo ""
	echo "If you are cross-compiling, you MIGHT need to set the CROSS_DIR environmental"
	echo "  variable if your cross-compiler cannot be found using the 'locate' command."
	printf "  e.g. make all CROSS_DIR=/opt/Xilinx/14.7/ISE_DS/EDK/gnu/arm/lin/bin/\n\n"

include ../myhostname.mk

all: rpms

PACKAGES=build_support ad9361 gmp gtest liquid-dsp lzma patchelf

# This adds _rpm suffix to each package
RPM_TARGETS=$(foreach pkg, $(PACKAGES), $(pkg)_rpm)

.PHONY: $(RPM_TARGETS)
.SILENT: $(RPM_TARGETS)

# This creates the PACKAGE_rpm targets, setting PKG to the base package name, and requiring that they exist.
.SECONDEXPANSION:
$(RPM_TARGETS): PKG=$(@:_rpm=)
$(RPM_TARGETS): libmyhostname $$(PKG)
	cd $(PKG) && $(SPOOF_HOSTNAME) ./build.sh rpm && cd ..

rpms: clean $(RPM_TARGETS)
	echo Done building.
	cp */*rpm .
	rm -f *debuginfo*rpm
	ls -halF *rpm

clean: cleanrpmbuild myhostnameclean
	git clean -fx .
	true

cleanrpmbuild:
	-rm -rf ~/rpmbuild/*
	-mkdir -p ~/rpmbuild/

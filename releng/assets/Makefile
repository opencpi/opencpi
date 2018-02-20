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

# Some massaging of variables:
export COMMIT_TAG_FINAL:=$(shell echo "$(COMMIT_TAG)" | tr -d ' ')
export SPOOF_HOSTNAME

.PHONY: all rpms clean cleanrpmbuild
.SILENT: all rpms clean cleanrpmbuild

include ../myhostname.mk

all: rpms

rpms: clean libmyhostname
	cd ../../projects/assets/rpm_support && $(SPOOF_HOSTNAME) ./build.sh rpm
	echo Done building.
	mv ../../projects/assets/*rpm .
	rm -f *debuginfo*rpm
	ls -halF *rpm

clean: cleanrpmbuild myhostnameclean
	rm -f *rpm

cleanrpmbuild:
	-rm -rf ~/rpmbuild/*
	-mkdir -p ~/rpmbuild/

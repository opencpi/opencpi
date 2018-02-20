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

%global debug_package %{nil}
%global _hardened_build 1

Name: ocpi-prereq-build_support
Version: 0.2
Release: 4%{?dist}
Packager: ANGRYVIPER team <discuss@lists.opencpi.org>
Summary: Utilities provided to support OpenCPI usage
License: GPL
%define _prefix /opt/opencpi/prerequisites/build_support
Prefix: %{_prefix}
Source0: %{name}.tar

%description
This base package is useless and should be deleted.

%package inode64
Summary: 64-bit to 32-bit inode mapper for 32-bit executables on 64-bit file systems
License: GPLv2
URL: http://www.tcm.phy.cam.ac.uk/sw/inodes64.html

%description inode64
This set of libraries intercepts certain 32-bit calls and ensures inodes do
not exceed 32 bits. It is most helpful when using a 32-bit compiler on a
64-bit host with large disks, e.g. a build server running XFS.

%prep
%setup -n %{name} -c -q

%build
cd inode64
make %{?_smp_mflags}

%install
%{__mkdir_p} %{buildroot}/%{_prefix}/inode64/lib{,64}
%{__mv} inode64/lib/inode64.so %{buildroot}/%{_prefix}/inode64/lib/
%{__mv} inode64/lib64/inode64.so %{buildroot}/%{_prefix}/inode64/lib64/
%{__mv} inode64/inode64.sh %{buildroot}/%{_prefix}/inode64/

%pre inode64
if [ "$1" == "1" ]; then
  if [ -e "%{_prefix}/inode64" ]; then
    echo "Problem: %{_prefix}/inode64 already exists!"
    false
  fi
fi

# If there are ever any other packages in this group, this should be copied into each one,
# because no single package "owns" it: (AV-3240)
%postun inode64
# Attempt to erase prefix if no more packages...
if [ "$(ls -1 %{_prefix} | wc -l)" == "0" ]; then
  rmdir %{_prefix} || :
fi

%clean
%{__rm} -rf %{buildroot}

%files inode64
%{_prefix}/inode64


%changelog
* Wed Feb  7 2018 - 0.2-4
- AV: Rename RPMs
* Mon Aug 21 2017 - 0.2-3
- AV: Remove directory if empty

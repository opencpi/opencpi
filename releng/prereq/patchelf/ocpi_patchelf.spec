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

# This is just from git clone and a configure run...
%{!?_ocpi_patchelf_dir: %define _ocpi_patchelf_dir /opt/opencpi/prerequisites/patchelf}
%define         _prefix %{_ocpi_patchelf_dir}
# CentOS 6 needs these two:
%define         _mandir %{_prefix}/share/man
%define         _docdir %{_prefix}/share/doc
Prefix:         %{_ocpi_patchelf_dir}

%global debug_package %{nil}
%global _hardened_build 1

Name: ocpi-prereq-patchelf
Version: %{OCPI_PATCHELF_VERSION}
Release: 3%{?OCPI_PATCHELF_COMMIT_SHORT}%{?dist}
Packager: ANGRYVIPER team <discuss@lists.opencpi.org>
Summary: RPM build (and modification) of patchelf, a utility for patching ELF binaries for OpenCPI
License: GPL
Group: Development/Tools
URL: http://nixos.org/patchelf.html
Source0: %{name}-%{version}.tar
Patch0: opencpi-patchelf-%{version}.patch
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
Prefix: /usr

%description
PatchELF is a simple utility for modifing existing ELF executables and
libraries.  It can change the dynamic loader ("ELF interpreter") of
executables and change the RPATH of executables and libraries.

This version is built from git revision %{OCPI_PATCHELF_COMMIT}

%prep
%setup -n patchelf-%{OCPI_PATCHELF_COMMIT} -q
%patch0

%build
if [ ! -z "%{?OCPI_PATCHELF_COMMIT_SHORT}" ]; then
  export OCPI_PATCHELF_COMMIT_SHORT=`echo %{?OCPI_PATCHELF_COMMIT_SHORT} | tr . -`
fi
./bootstrap.sh
%configure
make %{?_smp_mflags}
make check

%install
%make_install

# Make a symlink to allow it to have a path like all other prerequisites do (AV-450)
ln -s . ${RPM_BUILD_ROOT}/%{_prefix}/%{OCPI_TARGET_HOST}

%clean
rm -rf $RPM_BUILD_ROOT

%pre
if [ "$1" == "1" ]; then
  if [ -e "%{_prefix}" ]; then
    echo "Problem: %{_prefix} already exists!"
    false
  fi
fi

%postun
# 0 means totally uninstalled (not an update)
if [ "$1" == "0" ]; then
  rm -rf %{_prefix}/ || :
fi

%files
%{_bindir}/patchelf
%doc %{_docdir}/patchelf/README
%{_mandir}/man1/patchelf.1
%{_prefix}/%{OCPI_TARGET_HOST}

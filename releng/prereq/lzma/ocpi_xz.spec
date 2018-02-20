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

%{!?_ocpi_lzma_dir: %define _ocpi_lzma_dir /opt/opencpi/prerequisites/lzma}
%{!?_ocpi_xz_dir: %define _ocpi_xz_dir /opt/opencpi/prerequisites/xz}
%define         _prefix %{_ocpi_xz_dir}
%define         _libdir %{_prefix}/%{OCPI_TARGET_HOST}/lib
%define         _datadir %{_exec_prefix}/bin
%define         _mandir %{_prefix}/share/man
%define         _incdir %{_prefix}/include
%define         _sharedir %{_prefix}/share
Prefix:         %{_ocpi_xz_dir}
%global         _hardened_build 1

# This must be defined before the macro is used
Version:        %{OCPI_BUNDLED_VERSION}
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
Name:           ocpi-prereq-xz
%else
Name:           ocpi-prereq-xz-platform-%{OCPI_PLATFORM}
BuildArch:      noarch
# This is provided by glibc.i686, but there is no way to check cross-architecture from a spec file, so we say the file itself.
# The end user can do "sudo yum install /lib/ld-linux.so.2" to get it. The Xilinx gcc tool chain is 32-bit only.
BuildRequires:  /lib/ld-linux.so.2
AutoReqProv:    no
%define _binaries_in_noarch_packages_terminate_build 0
%define __strip %{OCPI_CROSS_DIR}/%{OCPI_CROSS_PREFIX}-strip
%define debug_package %{nil}
%endif
Release:        2%{?dist}
Packager:       ANGRYVIPER team <discuss@lists.opencpi.org>
Summary:        RPM build of the package that was once lzma, now xz for OpenCPI
Group:          Development/Tools
License:        Public Domain
URL:            tukanni.org
# Source0:        http://tukaani.org/xz/xz-%%{version}.tar.xz
# Official release: Source0:        https://github.com/xz-mirror/xz/releases/download/v%{version}/xz-%{version}.tar.xz
Source0:        https://github.com/xz-mirror/xz/archive/v%{version}/v%{version}.tar.gz
Patch0:         opencpi-xz-%{version}%{dist}.patch

BuildRequires:  autoconf gettext-devel

%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%description
XZ Utils are an attempt to make LZMA compression easy to use on free (as in
freedom) operating systems. This is achieved by providing tools and libraries
which are similar to use than the equivalents of the most popular existing
compression algorithms.

LZMA is a general purpose compression algorithm designed by Igor Pavlov as
part of 7-Zip. It provides high compression ratio while keeping the
decompression speed fast.
%else
%description
This package contains the XZ libraries for cross-compiling for the
%{OCPI_PLATFORM} target platform.
%endif

%prep
%setup -q -n xz-%{version}
%patch0 -p 1

%build
autoreconf -i --symlink -f
autoconf
%global _sharedstatedir %{_prefix}
# See AV-1256 for "--disable-symbol-versions" problem
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
export CFLAGS="--std=c99 %{__global_cflags}"
export CXXFLAGS="--std=c++0x %{__global_cflags}"
%configure --enable-static --enable-shared=yes --enable-debug=no --disable-symbol-versions
%else
%global __global_cflags %{OCPI_CFLAGS} %{?_hardened_cflags} %{?_performance_cflags}
%global optflags %{__global_cflags}
%global _build x86_64-redhat-linux-gnu
%global _host %{OCPI_CROSS_PREFIX}
export CFLAGS="--std=c99 %{__global_cflags}"
export CXXFLAGS="--std=c++0x %{__global_cflags}"
PATH=%{OCPI_CROSS_DIR}:$PATH %configure --enable-static --enable-shared=yes --enable-debug=no --with-os-ver=%{OCPI_SWPLATFORM} --disable-symbol-versions
%endif
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%make_install
%{__ln_s} -f xz %{buildroot}/%{_ocpi_lzma_dir}
%else
PATH=%{OCPI_CROSS_DIR}:$PATH %make_install
rm -rf %{buildroot}%{_incdir}
%endif

# Make arm_cs version if arm (for picoflexor)
%if "x%{?OCPI_TARGET_HOST}" == "xlinux-x13_3-arm"
ln -sf %{_prefix}/%{OCPI_TARGET_HOST} %{buildroot}/%{_prefix}/linux-zynq-arm_cs
%endif

# Remove junk
rm -rf %{buildroot}{%{_datadir},%{_sharedir},%{_mandir}}

%clean
rm -rf %{buildroot}

%pre
if [ "$1" == "1" ]; then
  if [ -e "%{_libdir}" ]; then
    echo "Problem: %{_libdir} already exists!"
    false
  fi
fi

%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%postun
# 0 means totally uninstalled (not an update)
if [ "$1" == "0" ]; then
  rm -rf %{_prefix}/ || :
fi
%endif

%files
%defattr(-,root,root,-)
%{_libdir}/liblzma.a
%{_libdir}/liblzma.la
%{_libdir}/liblzma.so
%{_libdir}/liblzma.so.*
%{_libdir}/pkgconfig/liblzma.pc
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%{_ocpi_lzma_dir}
%{_incdir}/lzma.h
%{_incdir}/lzma
%endif
%if "x%{?OCPI_TARGET_HOST}" == "xlinux-x13_3-arm"
%{_prefix}/linux-zynq-arm_cs
%endif

%changelog
* Wed Feb  7 2018 - 5.2.3-2
- AV: Rename RPMs
* Wed Oct 11 2017 - 5.2.3-1
- AV: Bump to 5.2.3
* Fri Jul  7 2017 - 5.2.2-14
- AV: Refuse to install with non-AV version already installed
* Fri Mar 24 2017 - 5.2.2-13
- AV: Changed to C99/C++0x defaults
* Wed Feb  1 2017 - 5.2.2-12
- AV: Removed devel subpackage, command-line utils, and docs

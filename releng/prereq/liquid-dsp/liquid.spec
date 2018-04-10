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

%global         _hardened_build 1
%{!?_ocpi_liquid_dir: %define _ocpi_liquid_dir /opt/opencpi/prerequisites/liquid}
%define         _prefix %{_ocpi_liquid_dir}/%{OCPI_TARGET_HOST}
%define         _exec_prefix %{_ocpi_liquid_dir}/%{OCPI_TARGET_HOST}
%define         _libdir /lib
%define         _datadir %{_exec_prefix}/bin
%define         _mandir %{_prefix}/share/man
%define         _sharedir %{_prefix}/share
Prefix:         %{_ocpi_liquid_dir}

%if "x%{?OCPI_CROSS_PREFIX}" == "x"
Name:           ocpi-prereq-liquid
%else
Name:           ocpi-prereq-liquid-platform-%{OCPI_PLATFORM}
Requires:       ocpi-prereq-liquid
Requires(postun): ocpi-prereq-liquid
BuildArch:      noarch
AutoReqProv:    no
%define _binaries_in_noarch_packages_terminate_build 0
%define __strip %{OCPI_CROSS_DIR}/%{OCPI_CROSS_PREFIX}-strip
%define debug_package %{nil}
%endif
Version:        %{OCPI_BUNDLED_VERSION}
Release:        3%{?dist}
Packager:       ANGRYVIPER team <discuss@lists.opencpi.org>
Summary:        RPM build of liquid dsp for OpenCPI

Group:          Development/Tools
License:        MIT
URL:            liquidsdr.org
Source0:        v%{OCPI_BUNDLED_VERSION}.tar.gz
# Needed for CentOS6
Patch0:         aclocal.patch
Patch1:         malloc.patch

%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%description
liquid-dsp is a free and open-source digital signal processing (DSP) library
designed specifically for software-defined radios on embedded platforms.

The aim is to provide a lightweight DSP library that does not rely on a myriad
of external dependencies or proprietary and otherwise cumbersome frameworks.

All signal processing elements are designed to be flexible, scalable, and
dynamic, including filters, filter design,oscillators, modems, synchronizers,
and complex mathematical operations.

This version is built from OSS release %{OCPI_BUNDLED_VERSION}.
%else
%description
This package contains the liquid-dsp libraries for cross-compiling
for the %{OCPI_PLATFORM} target platform.

This version is built from OSS release %{OCPI_BUNDLED_VERSION}.
%endif

%prep
%setup -q -n liquid-dsp-%{OCPI_BUNDLED_VERSION}
%patch0
%patch1

%build
./bootstrap.sh
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
export CFLAGS="--std=gnu99 %{__global_cflags}"
export CXXFLAGS="--std=c++0x %{__global_cflags}"
%configure
make %{?_smp_mflags}
%else
%global __global_cflags %{OCPI_CFLAGS} %{?_hardened_cflags} %{?_performance_cflags}
%global optflags %{__global_cflags}
%global _build x86_64-redhat-linux-gnu
%global _host %{OCPI_CROSS_PREFIX}
export CFLAGS="--std=gnu99 %{__global_cflags}"
export CXXFLAGS="--std=c++0x %{__global_cflags}"
export LDFLAGS="%{OCPI_CFLAGS}"
PATH=%{OCPI_CROSS_DIR}:$PATH %configure --target=%{OCPI_CROSS_PREFIX}
# --with-gcc-arch=cortex-a9
make %{?_smp_mflags} CC=%{?OCPI_CROSS_PREFIX}-gcc
%endif

%install
rm -rf $RPM_BUILD_ROOT
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%make_install
%else
PATH=%{OCPI_CROSS_DIR}:$PATH %make_install
%endif

# Make arm_cs and 13_4 versions if arm
%if "x%{?OCPI_TARGET_HOST}" == "xlinux-x13_3-arm"
ln -sf %{_prefix} %{buildroot}/%{_prefix}/../linux-x13_4-arm
ln -sf %{_prefix} %{buildroot}/%{_prefix}/../linux-zynq-arm_cs
%endif

# Have "include" dir be top-level and provided only by native version
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
mv %{buildroot}/%{_prefix}/include %{buildroot}/%{_prefix}/../
%else
rm -rf %{buildroot}/%{_prefix}/include
%endif

%clean
rm -rf $RPM_BUILD_ROOT

%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%check
make %{?_smp_mflags} check
%endif

%pre
if [ "$1" == "1" ]; then
  if [ -e "%{_prefix}/lib" ]; then
    echo "Problem: %{_prefix}/lib/ already exists!"
    false
  fi
fi

%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%postun
# 0 means totally uninstalled (not an update)
if [ "$1" == "0" ]; then
  rm -rf %{_ocpi_liquid_dir} || :
fi
%endif

%files
%defattr(-,root,root,-)
%dir %{_prefix}
%{_prefix}/lib/libliquid.a
%{_prefix}/lib/libliquid.so
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%dir %{_ocpi_liquid_dir}
%{_ocpi_liquid_dir}/include/liquid/liquid.h
%endif
%if "x%{?OCPI_TARGET_HOST}" == "xlinux-x13_3-arm"
%{_prefix}/../linux-x13_4-arm
%{_prefix}/../linux-zynq-arm_cs
%endif

%changelog
* Mon Feb 26 2018 - 1.3.1-3
- AV: Add Xilinx 13_4 alias
* Wed Feb  7 2018 - 1.3.1-2
- AV: Rename RPMs
* Mon Aug  7 2017 - 1.2.0-8
- AV: More cleanup
* Fri Jul  7 2017 - 1.2.0-7
- AV: Fix lib dir
* Fri Jul  7 2017 - 1.2.0-6
- AV: Refuse to install with non-AV version already installed
* Fri Mar 24 2017 - 1.2.0-5
- AV: Added build-time self check. Changed to C99/C++0x defaults

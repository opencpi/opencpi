# #####
#
#  GTEST is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  GTEST is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
########################################################################### #
%global         _hardened_build 1
%{!?_ocpi_gtest_dir: %define _ocpi_gtest_dir /opt/opencpi/prerequisites/gtest}
%define         _prefix %{_ocpi_gtest_dir}
%define         _libdir %{_prefix}/%{OCPI_TARGET_HOST}/lib
%define         _datadir %{_exec_prefix}/bin
%define         _mandir %{_prefix}/share/man
%define         _sharedir %{_prefix}/share
Prefix:         %{_ocpi_gtest_dir}

%if "x%{?OCPI_CROSS_PREFIX}" == "x"
Name:           ocpi-prereq-gtest
%else
Name:           ocpi-prereq-gtest-platform-%{OCPI_PLATFORM}
BuildArch:      noarch
# This is provided by glibc.i686, but there is no way to check cross-architecture from a spec file, so we say the file itself.
# The end user can do "sudo yum install /lib/ld-linux.so.2" to get it. The Xilinx gcc tool chain is 32-bit only.
BuildRequires:  /lib/ld-linux.so.2
AutoReqProv:    no
%define _binaries_in_noarch_packages_terminate_build 0
%define __strip %{OCPI_CROSS_DIR}/%{OCPI_CROSS_PREFIX}-strip
%define debug_package %{nil}
%endif
Version:        %{OCPI_BUNDLED_VERSION}
Release:        2%{?dist}
Packager:       ANGRYVIPER team <discuss@lists.opencpi.org>
Summary:        RPM build of gtest for OpenCPI

Group:          Development/Tools
License:        BSD
URL:            https://github.com/google/googletest
Source0:        release-%{version}.zip
Patch0:         opencpi-gtest-%{version}.patch

BuildRequires:  python libtool autoconf

%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%description
Google's framework for writing C++ tests on a variety of platforms
(GNU/Linux, Mac OS X, Windows, Windows CE, and Symbian). Based on the
xUnit architecture. Supports automatic test discovery, a rich set of
assertions, user-defined assertions, death tests, fatal and non-fatal
failures, various options for running the tests, and XML test report
generation.
%else
%description
This package contains the GTEST static libraries for cross-compiling
for the %{OCPI_PLATFORM} target platform.
%endif

%prep
%setup -q -n googletest-release-%{version}/googletest
%patch0 -p 1

%build
autoreconf -vfi
autoconf
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
export CFLAGS="--std=c99 %{__global_cflags}"
export CXXFLAGS="--std=c++0x %{__global_cflags}"
%configure --enable-dynamic --enable-static --with-pic
%else
%global __global_cflags %{OCPI_CFLAGS} %{?_hardened_cflags} %{?_performance_cflags}
%global optflags %{__global_cflags}
export CFLAGS="--std=c99 %{__global_cflags}"
export CXXFLAGS="--std=c++0x %{__global_cflags}"
%global _build x86_64-redhat-linux-gnu
%global _host %{OCPI_CROSS_PREFIX}
PATH=%{OCPI_CROSS_DIR}:$PATH %configure --enable-dynamic --enable-static --with-pic --with-os-ver=%{OCPI_SWPLATFORM}
%endif
# This next line can become make_build once we move to RHEL7+ only
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%make_install
%else
PATH=%{OCPI_CROSS_DIR}:$PATH %make_install
# Remove files that would normally be in -devel package:
rm -rf %{buildroot}/%{prefix}/{include,bin}
# If picoflexor, move
%if "x%{?OCPI_PLATFORM}" == "xpicoflexor"
echo "Renaming picoflexor"
mv -v %{buildroot}/%{prefix}/linux*arm %{buildroot}/%{prefix}/%{OCPI_TARGET_HOST}
%endif
%endif
# TODO: Fix this?
find %{buildroot} -type f -name "*.la" -delete

%clean
rm -rf $RPM_BUILD_ROOT

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
%{_libdir}/libgtest.a
%{_libdir}/libgtest_main.a
%{_libdir}/libgtest*.so*
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%defattr(-,root,root,-)
%{_exec_prefix}/bin/aclocal/gtest.m4
%{_prefix}/include/gtest/
%endif

%changelog
* Wed Feb  7 2018 - 1.8.0-2
- AV: Rename RPMs
* Fri Oct 13 2017 - 1.8.0-1
- AV: Bump to 1.8.0. Fix upstream URL. Picoflexor now "real" build.
* Fri Jul  7 2017 - 1.7.0-15
- AV: Refuse to install with non-RPM version already installed
* Fri Mar 24 2017 - 1.7.0-14
- AV: Added dynamic libraries and changed to C99/C++0x defaults
* Wed Feb  1 2017 - 1.7.0-13
- AV: Removed devel subpackage
* Mon Dec  7 2015 - 1.7.0-5
- Full removal in cleanup
* Wed Oct 14 2015 - 1.7.0-4
- Hardened build / PIE/PIC code
* Wed Aug 19 2015 - 1.7.0-3
- The cross-compilers need 32-bit runtime glibc installed.
* Mon Jul 13 2015 - 1.7.0-2
- Added --with-pic to configure call

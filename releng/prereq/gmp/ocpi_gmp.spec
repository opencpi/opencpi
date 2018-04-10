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

# RPM Spec file taken from http://vault.centos.org/7.2.1511/os/Source/SPackages/gmp-6.0.0-11.el7.src.rpm
# and modified/mangled for OpenCPI use
# Removed the FIPS and i686 support
# Removed shared libraries

%{!?_ocpi_gmp_dir: %define _ocpi_gmp_dir /opt/opencpi/prerequisites/gmp}
%define         _prefix %{_ocpi_gmp_dir}
%define         _libdir %{_prefix}/%{OCPI_TARGET_HOST}/lib
%define         _datadir %{_exec_prefix}/share
%define         _mandir %{_prefix}/share/man
%define         _incdir %{_prefix}/include
%define         _sharedir %{_prefix}/share
Prefix:         %{_ocpi_gmp_dir}
%global         _hardened_build 1

# This must be defined before the macro is used
Version:        %{OCPI_BUNDLED_VERSION}
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
Name:           ocpi-prereq-gmp
%else
Name:           ocpi-prereq-gmp-platform-%{OCPI_PLATFORM}
Requires:       ocpi-prereq-gmp
Requires(postun): ocpi-prereq-gmp
BuildArch:      noarch
# This is provided by glibc.i686, but there is no way to check cross-architecture from a spec file, so we say the file itself.
# The end user can do "sudo yum install /lib/ld-linux.so.2" to get it. The Xilinx gcc tool chain is 32-bit only.
BuildRequires:  /lib/ld-linux.so.2
AutoReqProv:    no
%define _binaries_in_noarch_packages_terminate_build 0
%define __strip %{OCPI_CROSS_DIR}/%{OCPI_CROSS_PREFIX}-strip
%define debug_package %{nil}
%endif
Release:        7%{?dist}
Packager:       ANGRYVIPER team <discuss@lists.opencpi.org>
Summary:        RPM build of the GNU arbitrary precision library for OpenCPI
Group:          System Environment/Libraries
License:        LGPLv3+ or GPLv2+
URL:            http://gmplib.org/
Source0:        ftp://ftp.gnu.org/pub/gnu/gmp/gmp-%{version}.tar.xz
Source2:        gmp.h
Source3:        gmp-mparam.h

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: autoconf automake libtool

%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%description
The gmp package contains GNU MP, a library for arbitrary precision
arithmetic, signed integers operations, rational numbers and floating
point numbers. GNU MP is designed for speed, for both small and very
large operands. GNU MP is fast because it uses fullwords as the basic
arithmetic type, it uses fast algorithms, it carefully optimizes
assembly code for many CPUs' most common inner loops, and it generally
emphasizes speed over simplicity/elegance in its operations.
%else
%description
This package contains the GNU MP libraries and headers for
cross-compiling for the %{OCPI_PLATFORM} target platform.
%endif

%prep
%setup -q -n gmp-%{version}

%build
autoreconf -ifv
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
if as --help | grep -q execstack; then
  # the object files do not require an executable stack
  export CCAS="gcc -c -Wa,--noexecstack"
fi
export CFLAGS="--std=c99 %{__global_cflags}"
export CXXFLAGS="--std=c++0x %{__global_cflags}"
%endif
mkdir base
cd base
ln -s ../configure .
%if "x%{?OCPI_CROSS_PREFIX}" != "x"
%global __global_cflags %{OCPI_CFLAGS} %{?_hardened_cflags} %{?_performance_cflags}
%global optflags %{__global_cflags}
export CFLAGS="--std=c99 %{__global_cflags}"
export CXXFLAGS="--std=c++0x %{__global_cflags}"
%global _build x86_64-redhat-linux-gnu
%global _host %{OCPI_CROSS_PREFIX}
# _target_platform is because CentOS6's configure macro sets "--target" while CentOS7's does not.
# ./configure will barf if the two are not equal (they are on native build) AV-3338
%global _target_platform %{OCPI_CROSS_PREFIX}
export CONFIGFLAGS+="--with-os-ver=%{OCPI_PLATFORM}"
export PATH=%{OCPI_CROSS_DIR}:$PATH
%endif

%configure --enable-fat --enable-cxx --with-pic ${CONFIGFLAGS}

sed -e 's|^hardcode_libdir_flag_spec=.*|hardcode_libdir_flag_spec=""|g' \
    -e 's|^runpath_var=LD_RUN_PATH|runpath_var=DIE_RPATH_DIE|g' \
    -e 's|-lstdc++ -lm|-lstdc++|' \
    -i libtool
export LD_LIBRARY_PATH=`pwd`/.libs
make CFLAGS="$RPM_OPT_FLAGS%{?optflags}" %{?_smp_mflags}
cd ..

%install
cd base
export LD_LIBRARY_PATH=`pwd`/.libs
%if "x%{?OCPI_CROSS_PREFIX}" != "x"
export PATH=%{OCPI_CROSS_DIR}:$PATH
%endif
make install DESTDIR=%{buildroot}
install -m 644 gmp-mparam.h ${RPM_BUILD_ROOT}%{_includedir}
rm -f %{buildroot}%{_libdir}/lib{gmp,mp,gmpxx}.la
rm -f %{buildroot}%{_infodir}/dir
/sbin/ldconfig -n %{buildroot}%{_libdir}
ln -sf libgmpxx.so.4 %{buildroot}%{_libdir}/libgmpxx.so
cd ..
# We don't bundle:
rm -rf %{buildroot}%{_infodir}
%if "x%{?OCPI_CROSS_PREFIX}" != "x"
rm -rf %{buildroot}%{_sharedir}
%endif

# Rename gmp.h to gmp-<arch>.h and gmp-mparam.h to gmp-mparam-<arch>.h to
# avoid file conflicts on multilib systems and install wrapper include files
# gmp.h and gmp-mparam-<arch>.h
# TODO: There may be more than one ARM in the future. Will need to ensure they are the same
# and then split into separate RPMs like ocpi-prereq-gmp-platform-common-arm!
# If not, need to add OCPI-specific stuff to that wrapper.
basearch=%{_arch}
%if "x%{?OCPI_TARGET_HOST}" == "xlinux-x13_3-arm"
basearch=arm
%endif
# Rename files and install wrappers
mv %{buildroot}/%{_includedir}/gmp.h %{buildroot}/%{_includedir}/gmp-${basearch}.h
install -m644 %{SOURCE2} %{buildroot}/%{_includedir}/gmp.h
mv %{buildroot}/%{_includedir}/gmp-mparam.h %{buildroot}/%{_includedir}/gmp-mparam-${basearch}.h
install -m644 %{SOURCE3} %{buildroot}/%{_includedir}/gmp-mparam.h

# Don't want cross-compiled version to have top-level headers
%if "x%{?OCPI_CROSS_PREFIX}" != "x"
rm -rf %{buildroot}%{_includedir}/gmp{,xx,-mparam}.h
%endif

# Make arm_cs and 13_4 versions if arm
%if "x%{?OCPI_TARGET_HOST}" == "xlinux-x13_3-arm"
ln -sf %{_prefix}/%{OCPI_TARGET_HOST} %{buildroot}/%{_prefix}/linux-x13_4-arm
ln -sf %{_prefix}/%{OCPI_TARGET_HOST} %{buildroot}/%{_prefix}/linux-zynq-arm_cs
%endif

%check
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
cd base
export LD_LIBRARY_PATH=`pwd`/.libs
make %{?_smp_mflags} check
cd ..
%endif

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
  rm -rf %{_prefix} || :
fi
%endif

%files
%defattr(-,root,root,-)
%dir %{_incdir}
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%{!?_licensedir:%global license %%doc}
%license COPYING COPYING.LESSERv3 COPYINGv2 COPYINGv3
%doc NEWS README
%endif
%{_includedir}/*.h
%{_libdir}/libgmp.a
%{_libdir}/libgmpxx.a
%{_libdir}/libgmp*.so*
%if "x%{?OCPI_TARGET_HOST}" == "xlinux-x13_3-arm"
%{_prefix}/linux-x13_4-arm
%{_prefix}/linux-zynq-arm_cs
%endif

%changelog
* Mon Feb 26 2018 - 6.1.2-7
- AV: Add Xilinx 13_4 alias
* Wed Feb  7 2018 - 6.1.2-6
- AV: Rename RPMs
* Thu Jan 18 2018 - 6.1.2-5
- AV: Force --with-pic at configure time for SWIG
* Thu Oct  5 2017 - 6.1.2-4
- AV: Added --enable-fat to run on many x86_64 processors (closes AV-3338, filed Red Hat #1493218)
* Mon Aug  7 2017 - 6.1.2-3
- AV: More cleanup
* Fri Jul  7 2017 - 6.1.2-2
- AV: Refuse to install with non-AV version already installed
* Fri Mar 24 2017 - 6.1.2-1
- AV: Bumped to 6.1.2. Added dynamic libraries and changed to C99/C++0x defaults

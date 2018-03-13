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

%{!?_ocpi_cdk_dir: %define _ocpi_cdk_dir /opt/opencpi/cdk}
%{!?prereqdir: %define prereqdir /opt/opencpi/prerequisites}
%{!?OCPI_CROSS:%define OCPI_CROSS 0}
%define         _prefix %{_ocpi_cdk_dir}
%define         _libdir %{_prefix}/lib
%global         prereqdir /opt/opencpi/prerequisites
%global         projregistry_final %{_prefix}/../project-registry
%global         projregistry %{buildroot}%{projregistry_final}
%global         sddir_final %{_prefix}/../boot_support
%global         sddir %{buildroot}%{sddir_final}
%global         projsourcedir_final %{_prefix}/../projects
%global         projsourcedir %{buildroot}%{projsourcedir_final}
%global         coreprojdir %{projsourcedir}/core
# These are the core prereqs: devel subpackage always needs and we always need for building:
%global         all_prereqs    ocpi-prereq-ad9361 ocpi-prereq-gmp ocpi-prereq-gtest ocpi-prereq-patchelf ocpi-prereq-xz

%if !%{OCPI_CROSS}
Name:           %{RPM_BASENAME}
%else
# Bring in the standard cross-compilation variables and parse out the ones we need. (AV-812, AV-2088)
# This seems to happen 2X. Not sure why. Hopefully quick enough not to matter.
%global OCPI_ALL_VARS_TMP %(mktemp --tmpdir ocpi_rpm_vars.XXX)
# Before calling ocpitarget.sh, we want to force it into "bootstrap" mode. This will allow you to build an RPM for a
# platform that you don't have an RPM already installed for BUT have ANY OpenCPI RPM installed (e.g. only the base one)
%global OCPI_ALL_VARS_CMD %(unset OCPI_CDK_DIR; cd ..; source bootstrap/scripts/ocpitarget.sh %{OCPI_TARGET_PLATFORM} && env | grep OCPI > %{OCPI_ALL_VARS_TMP})
%global OCPI_CROSS_BUILD_BIN_DIR %(grep OCPI_CROSS_BUILD_BIN_DIR %{OCPI_ALL_VARS_TMP} | cut -f2- -d=)
%global OCPI_CROSS_HOST %(grep OCPI_CROSS_HOST %{OCPI_ALL_VARS_TMP} | cut -f2- -d=)
%global OCPI_TARGET_HOST %(grep OCPI_TARGET_HOST %{OCPI_ALL_VARS_TMP} | cut -f2- -d=)
%global OCPI_ALL_VARS_CLEANUP %(rm %{OCPI_ALL_VARS_TMP})
# flag to determine
%global CORE_RCC %([ -d ../projects/core/rcc/platforms/%{OCPI_TARGET_PLATFORM} ] && echo 1)
# End of bringing in variables
# Determine Xilinx release:
%global xilinx_release %(perl -e '"%{OCPI_TARGET_PLATFORM}" =~ /xilinx([\\d_]+)/ && print $1')

Name:           %{RPM_BASENAME}-sw-platform-%{OCPI_TARGET_PLATFORM}
Requires:       %{RPM_BASENAME}
Obsoletes:      %{RPM_BASENAME}-platform-%{OCPI_TARGET_PLATFORM}
# Make the runtime package install first and uninstall last:
Requires(pre,postun): %{RPM_BASENAME}
BuildArch:      noarch
AutoReqProv:    no
%define _binaries_in_noarch_packages_terminate_build 0
Source1:        cdkbin.tar
%endif
%if 0%{IMPORT_BSP}
Source2:        imported_bsp.tar
%endif
Version: %{RPM_VERSION}
Release: %{RPM_RELEASE}%{?RELEASE_TAG}%{?COMMIT_TAG}%{?dist}
Prefix: %{_ocpi_cdk_dir}
Summary: A framework to simplify and enable code portability of real-time systems

Group:          Applications/Engineering
License:        LGPLv3+
Source:         %{RPM_BASENAME}-%{version}.tar
Vendor:         ANGRYVIPER Team
Packager:       ANGRYVIPER team <discuss@lists.opencpi.org>

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-buildroot
BuildRequires:  glibc-static glibc-devel
BuildRequires:  autoconf automake gawk libtool pax perl python which
BuildRequires:  tar >= 1.22
# Things like readelf and xargs:
BuildRequires:  binutils findutils util-linux
BuildRequires:  %{all_prereqs}
# This is provided by glibc.i686, but there is no way to check cross-architecture from a spec file, so we say the file itself.
# The end user can do "sudo yum install /lib/ld-linux.so.2" to get it. The Xilinx gcc tool chain is 32-bit only.
BuildRequires:  ld-linux.so.2
Requires:       perl time
Requires(pre):  perl shadow-utils
Requires(post,postun): findutils coreutils
%if %{OCPI_CROSS}
# Determine base architecture of prereqs based on target platform
%if %{OCPI_TARGET_PLATFORM} == "xilinx%{xilinx_release}" || %{OCPI_TARGET_PLATFORM} == "pico_t6a"
%global prereq_arch zynq
%endif
%if x%{?prereq_arch} == x
%{error:Cannot determine prereq architecture needed!}
%endif
%if %{OCPI_TARGET_PLATFORM} == "pico_t6a"
Requires:       ocpi-prereq-build_support-inode64
BuildRequires:  ocpi-prereq-build_support-inode64
%endif
%global         all_prereqs_pf ocpi-prereq-ad9361-platform-%{prereq_arch} ocpi-prereq-gmp-platform-%{prereq_arch} ocpi-prereq-gtest-platform-%{prereq_arch} ocpi-prereq-xz-platform-%{prereq_arch}
BuildRequires:  %{all_prereqs_pf}
Requires:       %{all_prereqs_pf}
%endif

# These come from platforms/centos7/centos7-packages.sh - not sure if they really apply...
Requires:       tcl pax python-devel
BuildRequires:  tcl pax python-devel
# Some of the JTAG scripts use old-school "ed":
BuildRequires:  ed
Requires:       ed
# The rpm-build is obvious, but when on a new container or VM, this works:
# grep Requires *spec | cut -f3- -d: | grep -v { | grep -v ocpi-prereq | xargs -r yum install -y
BuildRequires:  gcc-c++ rpm-build fakeroot
#Optional but preferred (AV-2071) BuildRequires:  valgrind-devel

# For XML dependency stuff (AV-2684):
BuildRequires:  python-lxml

%if "0%{?dist}" != "0.el6"
# For SWIG interfaces (AV-3699)
BuildRequires:  swig
%endif

# description tag and "release id" are repeated to avoid tons of whitespace
%if !%{OCPI_CROSS}
%description
Open Component Portability Infrastructure (OpenCPI)

An open source software (OSS) framework to simplify complexity and enable
code portability of real-time systems

 *   Creates a hardware abstraction layer for embedded real-time systems
 *   Real-time middleware for embedded systems
 *   Middleware for Waveform-Ready™ Processing Platforms leading to improved
     waveform code portability with standards-based interfaces
 *   Increased interoperability using container technology
 *   Real-time virtualization
 *   Component Portability Infrastructure (CPI) is a real-time embedded (RTE)
     middleware framework that simplifies programming of heterogeneous
     processing applications requiring, in a system or across a tech refresh,
     a mix of processing and interconnect technologies.
%if "x%{?COMMIT_HASH}" != "x"
Release ID: %{COMMIT_HASH}
%endif
%if 0%{IMPORT_BSP}
BSP Release ID: %{BSP_COMMIT_HASH}
%endif
%else
%description
This package contains the OpenCPI static libraries for cross-compiling
for the %{OCPI_TARGET_PLATFORM} target platform, along with core components.
%if "x%{?COMMIT_HASH}" != "x"
Release ID: %{COMMIT_HASH}
%endif
%endif
%package devel
Requires:   %{name} = %{version}-%{release}
Requires(pre,postun): %{name} = %{version}-%{release}
Requires:   %{all_prereqs}
Requires:   gcc-c++
# These 32-bit X11 libraries are required for ModelSIM, even in CLI mode (AV-567):
# ncurses-libs.i686:
Requires:   libncurses.so.5
# libXft.i686:
Requires:   libXft.so.2
# libXext.i686:
Requires:   libXext.so.6
# Xilinx 32-bit compiler (as above):
Requires:   ld-linux.so.2
# Altera tools needed this one:
#AV-3645 Requires:   libXdmcp.i686
# Used by a lot of testbenches, etc (AV-1261, AV-1299):
Requires:   numpy scipy python-matplotlib
# For XML dependency stuff (AV-2684):
Requires:   python-lxml
# For bash completion (AV-2398)
# (Optional) Requires:  bash-completion
# For ocpishow and others (AV-3548)
Requires:   python-argparse

# Used by the core project extract script:
Requires:   xz
Requires:   tar >= 1.22
Summary:    The OpenCPI development package
Group:      Development/Libraries

%description devel
This package ensures that all requirements for OpenCPI development are
installed. It also provides a useful development utilities.
%if "x%{?COMMIT_HASH}" != "x"
Release ID: %{COMMIT_HASH}
%endif

# Hardware platform packages (AV-2272)
%if %{OCPI_TARGET_PLATFORM} == "xilinx%{xilinx_release}"
%package -n %{RPM_BASENAME}-hw-platform-zed
Requires:   %{name} = %{version}-%{release}
AutoReqProv: no
Summary:    The OpenCPI support package for the Zed development board

%description -n %{RPM_BASENAME}-hw-platform-zed
This package contains development support files for the Zed platform,
e.g. an example SD card load.
%if "x%{?COMMIT_HASH}" != "x"
Release ID: %{COMMIT_HASH}
%endif

%endif
%if %{OCPI_TARGET_PLATFORM} == "pico_t6a"
%package -n %{RPM_BASENAME}-hw-platform-picoflexor
Requires:   %{name} = %{version}-%{release}
AutoReqProv: no
Summary:    The OpenCPI support package for the DRS Picoflexor T6A-S1

%description -n %{RPM_BASENAME}-hw-platform-picoflexor
This package contains development support files for the DRS Picoflexor T6A-S1 platform,
e.g. an example SD card load.
%if "x%{?COMMIT_HASH}" != "x"
Release ID: %{COMMIT_HASH}
%endif

%endif
# End of hardware platforms

%if !%{OCPI_CROSS}
%if "0%{?dist}" != "0.el6"
# Interface packages
%package interface-python
Requires:    %{name} = %{version}-%{release}
AutoReqProv: no
Requires(pre,postun): %{name} = %{version}-%{release}
%{?python_provide:%python_provide python-OcpiApi}
Summary: The Python interface to the OpenCPI ACI

%description interface-python
This package includes SWIG bindings to allow Python-based top-level ACI applications.
%if "x%{?COMMIT_HASH}" != "x"
Release ID: %{COMMIT_HASH}
%endif
# Restore executable mode. Necessary due to the workaround to avoid this getting its debug symbols stripped
%global swig_arch_cleanup %{__chmod} 755 %{buildroot}/%{python_sitearch}/_OcpiApi.so;
# End of interface packages
%endif
%endif

# host_platform=linux-c7-x86_64 host_platform_short=centos7
%global host_platform %(OCPI_CDK_DIR= OCPI_PROJECT_PATH=$OCPI_PROJECT_PATH:$(pwd)/../projects/core ../platforms/getPlatform.sh 2>/dev/null | cut -f4 -d" ")
%global host_platform_short %(OCPI_CDK_DIR= OCPI_PROJECT_PATH=$OCPI_PROJECT_PATH:$(pwd)/../projects/core ../platforms/getPlatform.sh 2>/dev/null | cut -f5 -d" ")
%global _hardened_build 1

# Restore XML that had been saved out.
# Minimize our shipping size after everything has been possibly mangled.
# (Copied and modified from /usr/lib/rpm/redhat/macros)
%define __arch_install_post \
%(pwd)/restore_xml.sh \
%{?swig_arch_cleanup} %{?ocpi_arch_cleanup} /usr/lib/rpm/check-buildroot \
%{nil}

# CentOS6 RPM building has a problem where they call /usr/lib/rpm/redhat/brp-strip-comment-note
# so we disable debug builds for C6 (sorry!)
%if "x%{?dist}" == "x.el6"
%define debug_package %{nil}
%endif

%if %{OCPI_CROSS}
%define debug_package %{nil}
# %%undefine _missing_build_ids_terminate_build # AV-419 if we ever get cross-compiling working
%global __strip %{OCPI_CROSS_BUILD_BIN_DIR}/%{OCPI_CROSS_HOST}-strip
# Clean up temporary tools used by makeSD, etc. (must end in semicolon)
%global ocpi_arch_cleanup %{__rm} -rf %{buildroot}/%{_bindir}/%{host_platform}/;
%endif

%prep
%setup -q -n %{RPM_BASENAME}-%{version}

%build
[ -d projects/core/rcc/platforms/%{OCPI_TARGET_PLATFORM} ]&& echo 1
# Export variables needed for "make" calls
export OCPI_CDK_DIR=%{buildroot}%{_prefix}
# This also exists in acinclude/ocpi_import_platforms and is pulling in all existing
# projects that have rcc platforms in them
export OCPI_PROJECT_PATH=$OCPI_PROJECT_PATH:$(find projects -type d  | grep rcc/platforms | cut -f 1-2 -d/ | sort -u | xargs -I xxx echo -n "$(pwd)/xxx:")
%if %{OCPI_CROSS}
export OCPI_XILINX_VERSION=14.7
export OCPI_TARGET_PLATFORM=%{OCPI_TARGET_PLATFORM}
%endif
# build the core framework
./reconf
# export OCPI_CONFIGURE_IS_RUNNING=1
%if !%{OCPI_CROSS}
  %global _configure ./ocpi-configure
%else
  %global __global_cflags %{?_hardened_cflags} %{?_performance_cflags}
  %global optflags %{__global_cflags}
  %global _build x86_64-redhat-linux-gnu
  %global _host %{OCPI_CROSS_HOST}
  %global _configure ./cross-configure
%endif
%configure --with-cdk-dir=%{_ocpi_cdk_dir} --without-sharedlibs

# More clean-up
find projects/core/components -name lib -type d -delete
find projects/core/hdl -name lib -type d -delete

# Do the actual work (driver may build if cross-compiled)
make %{?_smp_mflags}

%install
export OCPI_CDK_DIR=%{buildroot}%{_prefix}
export OCPI_TOOL_HOST=%{host_platform}
export OCPI_TOOL_PLATFORM=%{host_platform_short}
# This also exists in acinclude/ocpi_import_platforms and is pulling in all existing
# projects that have rcc platforms in them
export OCPI_PROJECT_PATH=$OCPI_PROJECT_PATH:$(find projects -type d  | grep rcc/platforms | cut -f 1-2 -d/ | sort -u | xargs -I xxx echo -n "$(pwd)/xxx:")
%if %{OCPI_CROSS}
# Export variables needed for "make" calls
export OCPI_TARGET_PLATFORM=%{OCPI_TARGET_PLATFORM}
%if "%{OCPI_TARGET_KERNEL_DIR}" != "autofound"
export OCPI_TARGET_KERNEL_DIR="%{OCPI_TARGET_KERNEL_DIR}"
%endif
export OCPI_XILINX_VERSION=14.7
export OCPI_ALTERA_VERSION=REPLACEME_IN_RPM_SPECFILE
%if "%{OCPI_TARGET_PLATFORM}" == "xilinx%{xilinx_release}"
export OCPI_TARGET_KERNEL_DIR=%{_builddir}/%{buildsubdir}/projects/core/rcc/platforms/xilinx%{xilinx_release}/opencpi-zynq-linux-release-%{xilinx_release}/kernel-headers
%endif
%endif
# Tell tar / xz to compress as much as possible with all cores in use
export XZ_OPT="-e9 -v -T0"

# Set up the symlinks in exports once, and bring in /specs
make exports
%make_install
# At this point, everything is installed in some crazy location, like /home/user/rpmbuild/BUILDROOT/opencpi-versionstring.el7.centos.x86_64/home/user/rpmbuild/BUILD/opencpi-1.0.0/
# This is because we need to use "abs_top_srcdir" for rpath stuff. So we'll move it to where it belongs. (AV-592)
%{__mv} %{buildroot}%{_builddir}/%{buildsubdir}/target-cdk-staging/ .
%{__rm} -rf %{buildroot}/*
%{__mkdir_p} %{buildroot}%{_prefix}/
# Set up the symlinks (again)
make exports

# Now we will "install" the CDK
make export_cdk
tar xf opencpi-cdk-exported.tar -C %{buildroot}%{_prefix}/

# AV-1160 fix symlinks
set +x
pax -v -f opencpi-cdk-exported.tar | grep == | awk '{print $9 $10 $11}' | (cd %{buildroot}%{_prefix} ; python -c '
import fileinput
import os.path
for line in fileinput.input():
  files = line.rstrip().split("==")
  src = files[0]
  dst = files[1]
  os.remove(src)
#   print "Fixing symlink", src, "=>", os.path.join( os.path.relpath(os.path.dirname(dst), os.path.dirname(src)), os.path.basename(dst))
  os.symlink(os.path.join( os.path.relpath(os.path.dirname(dst), os.path.dirname(src)), os.path.basename(dst)), src)
')
set +x

# Driver stuff
%if %{OCPI_CROSS}
# Temporary - copy native tools
%{__mkdir_p} %{buildroot}/%{_bindir}/%{host_platform}/
tar xf %{SOURCE1} -C %{buildroot}/%{_bindir}/%{host_platform}/

%if "%{OCPI_TARGET_PLATFORM}" == "pico_t6a"
mkdir %{buildroot}/av.bsp.picoflexor
tar xf %{SOURCE2} -C %{buildroot}/av.bsp.picoflexor
%endif
if [ -e releng/env.d/platform_%{OCPI_TARGET_PLATFORM}.sh.example ]; then
  %{__mkdir_p} %{buildroot}%{_prefix}/env.d
  %{__mv} releng/env.d/platform_%{OCPI_TARGET_PLATFORM}.sh.example %{buildroot}%{_prefix}/env.d/
fi
%else
# Not cross-compiled
# "env" subdirs
%{__mv} ./env %{buildroot}%{_prefix}
%{__rm} -f releng/env.d/platform_*
%{__mv} releng/env.d %{buildroot}%{_prefix}

# Add udev rules to RPM (AV-3588)
%{__mkdir_p} %{buildroot}/etc/udev/rules.d/
cp releng/udev.rules/51-opencpi-usbblaster.rules %{buildroot}/etc/udev/rules.d/

# Point the user to the CDK and add our tools to their path as well as bash completion
%{__mkdir_p} %{buildroot}/etc/profile.d
%{__ln_s} -f %{_prefix}/env/rpm_cdk.sh %{buildroot}/etc/profile.d/opencpi.sh
%{__sed} -i s/@OCPI_TOOL_HOST@/%{OCPI_TARGET_HOST}/g %{buildroot}/%{_prefix}/env/rpm_cdk.sh
%{__mkdir_p} %{buildroot}/etc/bash_completion.d
%{__ln_s} -f %{_prefix}/scripts/ocpidev_bash_complete %{buildroot}/etc/bash_completion.d/ocpidev_bash_complete.sh

# Move the test script into path
%{__mv} scripts/test-opencpi-rpm.sh %{buildroot}/%{_bindir}/%{OCPI_TARGET_HOST}/test-opencpi-rpm

# Put all our executables into /usr/bin (cannot use RPM's default _bindir because we override that)
# TODO: Make below line ocpi* to not pollute their path
%{__mkdir_p} %{buildroot}/usr/bin/
for exe in %{buildroot}/%{_bindir}/%{OCPI_TARGET_HOST}/*; do
  [ -d ${exe} ] && continue # Skip subdirectory links like ctests
  bn=$(basename ${exe})
  %{__ln_s} -f %{_bindir}/%{OCPI_TARGET_HOST}/${bn} %{buildroot}/usr/bin/
done

# Put our .so files into path
%{__mkdir_p} %{buildroot}/etc/ld.so.conf.d
echo %{_libdir}/%{OCPI_TARGET_HOST} >> %{buildroot}/etc/ld.so.conf.d/opencpi.conf

# We handle these in the native "driver" RPM
%{__rm} -f %{buildroot}%{_prefix}/LICENSE.txt
%{__mv} %{buildroot}/%{_libdir}/%{OCPI_TARGET_HOST}/LICENSE.txt %{buildroot}%{_prefix}
%{__rm} %{buildroot}/%{_libdir}/%{OCPI_TARGET_HOST}/*-opencpi.rules

# We don't ship platform mk files in CDK (they're in the core/assets projects)
%{__rm} -rf %{buildroot}%{_prefix}/lib/platforms/

# Bring in simulators (AV-1889)
set -x
old_nullglob=`shopt -p nullglob` || :
shopt -s nullglob
for plat in hdl/platforms/*; do
  bplat=`basename ${plat}`
  if [ -f ${plat}/runSimExec.${bplat} ]; then
    echo "Installing runSimExec script for ${bplat}."
    %{__mkdir_p} %{buildroot}%{_libdir}/platforms/${bplat}
    %{__cp} --target-directory=%{buildroot}%{_libdir}/platforms/${bplat} ${plat}/runSimExec.${bplat}
    for f in ${plat}/probe*; do
      echo "Installing ${bplat} probe assembly: ${f}"
      %{__cp} --target-directory=%{buildroot}%{_libdir}/platforms/${bplat} ${f}
    done
  fi
done
${old_nullglob}

# CDK "Project"
# The projects dir is needed for use with 'imports' (AV-3028)
%{__mkdir_p} %{projregistry}
%{__mkdir_p} %{projsourcedir}

# Core Project
%{__cp} -rf projects/core %{coreprojdir}

# Copy our local/limited base_project contents to the base_project destination
# This is for legacy support only
%{__mv} releng/projects/* %{projsourcedir}

%if "0%{?dist}" != "0.el6"
# Move SWIG stuff out
%{__mkdir_p} %{buildroot}/%{python_sitearch}
%{__cp} --target-directory=%{buildroot}/%{python_sitearch} target-cdk-staging/lib/%{host_platform}/_OcpiApi.so
%{__cp} --target-directory=%{buildroot}/%{python_sitearch} swig/OcpiApi.py
# Workaround which tells the build to skip passing this file into find-debuginfo.sh and stripping out debug symbols
# Restored in swig_arch_cleanup
%{__chmod} 644 %{buildroot}/%{python_sitearch}/_OcpiApi.so
%endif # Not C6
set -x
# End of non-cross-compiled
%endif

# This next line (AV-669) allows the non-exported HDL platforms to be seen by "make components"
# TODO: OCPI_TOOL_DIR needs to handle "library build modes" if we allow RPM building to have them...
export OCPI_HDL_PLATFORM_PATH=%{coreprojdir}/hdl/platforms
export OCPI_TOOL_DIR=%{host_platform}
%{__mkdir_p} %{buildroot}/%{_libdir}/platforms

# Now that the CDK is locally installed, we build our components
# This fake/empty dir must exist
%{__mkdir_p} %{buildroot}/%{_libdir}/components
%if !%{OCPI_CROSS}
pushd %{coreprojdir} > /dev/null
make imports
make rcc -C components
make exports
rm -f project.xml
rm -f .gitattributes
# make -C components OCPI_LOG_LEVEL=11 V=1 AT='' OCPI_DEBUG_MAKE=1
popd > /dev/null
%else
export OCPI_HDL_PLATFORM_PATH=`pwd`/projects/core/hdl/platforms
make -C projects/core imports
make rcc -C projects/core/components
make -C projects/core exports
# make -C components OCPI_LOG_LEVEL=11 V=1 AT='' OCPI_DEBUG_MAKE=1
%endif
# Want this to fail if not empty
rmdir %{buildroot}/%{_libdir}/components


# Move components over to core, then copy out the ones we built
%if %{OCPI_CROSS}
mkdir -p %{coreprojdir}/components/lib
for r in projects/core/components/*.rcc; do
  for t in `find $r -name "target-*"`; do
    %{__mkdir_p} %{coreprojdir}/components/$(basename $r)
    %{__cp} -R $r/$(basename $t) %{coreprojdir}/components/$(basename $r)
  done
done
%{__cp} -R projects/core/components/lib/rcc %{coreprojdir}/components/lib/
%{__rm} -rf projects/core/components/lib
# only ship the rcc platforms that are in core i.e xilinx13_3 into the built version of
# the core project
if [ -d projects/core/rcc/platforms/%{OCPI_TARGET_PLATFORM} ]; then
  %{__mkdir_p} %{coreprojdir}/rcc/platforms
  %{__cp} -R projects/core/rcc/platforms/%{OCPI_TARGET_PLATFORM} %{coreprojdir}/rcc/platforms/
fi
%endif

%if %{OCPI_CROSS}
# This only needs to be done in cross-compiled env because "make clean"
# is run on ALL components below if not cross.
set +x
for comp in `make -sC projects/core/components showrcc 2>/dev/null`; do
  echo Cleaning ${comp}...
  make -sC projects/core/components/${comp} clean
done

set -x
%endif

# This is needed by the HDL
echo ocpi > %{buildroot}/%{_libdir}/package-id

# Remove the non-static versions of the workers (AV-629)
# Want to do this before makeSD is called
for targetdir in %{coreprojdir}/components/*.rcc/target-*; do
  find $targetdir -mindepth 1 -not -name "*_s\.so" | xargs rm -rf
done
find %{coreprojdir}/components/lib/rcc/ -not -type d | grep \.so | grep -v _s\.so | xargs rm -rf

%if %{OCPI_CROSS}
# Build SD card image for zed
%if "%{OCPI_TARGET_PLATFORM}" == "xilinx%{xilinx_release}"
# Point it to the examples
export EXAMPLES_ROOTDIR=`pwd`/projects/assets
pushd %{coreprojdir}/rcc/platforms/xilinx%{xilinx_release}/ >/dev/null
%{__mkdir_p} %{coreprojdir}/hdl/platforms/zed/
%{buildroot}%{_prefix}/platforms/zynq/createOpenCPIZynqSD.sh %{xilinx_release} zed xilinx%{xilinx_release}
# Copy in the default ones (AV-2328)
cp %{buildroot}%{_prefix}/platforms/zynq/default_zynq_setup.sh opencpi-zynq-linux-release-%{xilinx_release}/OpenCPI-SD-zed/opencpi/default_mysetup.sh
cp %{buildroot}%{_prefix}/platforms/zynq/default_zynq_net_setup.sh opencpi-zynq-linux-release-%{xilinx_release}/OpenCPI-SD-zed/opencpi/default_mynetsetup.sh
%{__mkdir_p} %{sddir}
%{__mkdir_p} %{sddir}/zed
mv opencpi-zynq-linux-release-%{xilinx_release}/OpenCPI-SD-zed %{sddir}/zed
du -sh opencpi-zynq-linux-release-%{xilinx_release}
tar -Jcf opencpi-zynq-linux-release-%{xilinx_release}.tar opencpi-zynq-linux-release-%{xilinx_release}
du -sh opencpi-zynq-linux-release-%{xilinx_release}.tar
popd >/dev/null
# Clean up stuff we don't want to ship (AV-1603)
%{__rm} -rf %{coreprojdir}/rcc/platforms/xilinx%{xilinx_release}/opencpi-zynq-linux-release-%{xilinx_release}/
%{__rm} -rf %{coreprojdir}/rcc/platforms/xilinx%{xilinx_release}/release
%{__rm} -rf %{coreprojdir}/rcc/platforms/xilinx%{xilinx_release}/xilinx-zynq-binary-release-for-%{xilinx_release}/
# Add udev rules to RPM (AV-2396)
%{__mkdir_p} %{sddir}/zed/udev.rules
cp releng/udev.rules/98-zedboard.rules %{sddir}/zed/udev.rules/
%endif

%if %{OCPI_TARGET_PLATFORM} == "pico_t6a"
# SD card has similar script call as zed
# Point it to the examples
export EXAMPLES_ROOTDIR=`pwd`/projects/assets
pushd %{buildroot}/av.bsp.picoflexor/rcc/platforms/pico_t6a >/dev/null
%{__mkdir_p} %{coreprojdir}/hdl/platforms/picoflexor
./makeSD.sh -
%{__mkdir_p} %{sddir}/picoflexor
mv OpenCPI-SD %{sddir}/picoflexor/OpenCPI-SD-picoflexor
rm makeSD.sh
# Create documentation
cp -r ../pico_t6a %{buildroot}%{_prefix}/platforms/
# Not available on CentOS6, so just checked PDF in
# rubber -d Picoflexor_T6A_Notes.tex
popd >/dev/null
%{__rm} -rf %{buildroot}/av.bsp.picoflexor
%{__rm} -rf %{buildroot}%{_prefix}/platforms/pico_t6a/{bin,doc,README.*}
# Add udev rules to RPM (AV-2396)
%{__mkdir_p} %{sddir}/picoflexor/udev.rules
cp releng/udev.rules/98-picoflexor.rules %{sddir}/picoflexor/udev.rules/
%endif

# Want the cross-compile Makefile only for this platform
# (Starting with AV 1.2 we only build on C7...)
# %{__cp} -L %{buildroot}%{_prefix}/platforms/%{OCPI_TARGET_PLATFORM}/rcc=%{host_platform_short}=%{OCPI_TARGET_PLATFORM}.mk save.mk
# %{__rm} %{buildroot}%{_prefix}/platforms/%{OCPI_TARGET_PLATFORM}/*=*.mk
# %{__mv} save.mk %{buildroot}%{_prefix}/platforms/%{OCPI_TARGET_PLATFORM}/rcc=%{host_platform_short}=%{OCPI_TARGET_PLATFORM}.mk
%endif

# Don't ship the "packages" script with the RPM, since the RPM can't be installed without them already.
%{__rm} -rf %{coreprojdir}/rcc/platforms/centos*/centos*-packages.sh

set +x
# Take out any platform that is not for this RPM (except zynq* and xilinx*, AV-2152) - Eventually these should be in their own package as well.
for i in %{coreprojdir}/rcc/platforms/* %{buildroot}%{prefix}/platforms/*; do
  if [ -d $i ]; then
    bn=`basename $i`
    case $bn in
%if "%{OCPI_TARGET_PLATFORM}" == "xilinx%{xilinx_release}"
      xilinx%{xilinx_release})
        echo Keeping platform info for Xilinx: $bn
        (cd $i; ln -sf ../../imports/ocpi.cdk/platforms/zynq/README .)
        ;;
      zynq)
        echo Keeping platform info for Zynq: $bn
        rm -rf $i/git/ || : # This is the default build-helper
        ;;
%else
      %{OCPI_TARGET_PLATFORM})
        true # Do nothing
        ;;
%endif
      *)
        echo Removing unneeded platform info for $bn
        rm -rf $i
    esac
  fi
done

%{__rm} %{buildroot}/%{prefix}/platforms/README

# Don't ship testRpl; it's outdated, but Jim would prefer it is still compiled;
# he'd like to eventually bring it back. (AV-857)
%{__rm} -rf %{buildroot}/%{_bindir}/%{OCPI_TARGET_HOST}/testRpl

# Store off any artifact data (AV-960)
./releng/save_xml.sh

# If cross-compiled, can't create debug packages, but need to remove absolute paths
# from the compiled components (AV-960)
%if %{OCPI_CROSS}
for f in %{coreprojdir}/components/lib/rcc/*/*.so; do
  echo Stripping RCC Component `basename $f`...
  %{__strip} -g $f
done

# Any SD card images should be added to this list:
old_nullglob=`shopt -p nullglob` || :
shopt -s nullglob
for f in %{coreprojdir}/hdl/platforms/zed/OpenCPI-SD-zed/opencpi/artifacts/*.so \
         %{coreprojdir}/hdl/platforms/picoflexor/OpenCPI-SD-picoflexor/opencpi/artifacts/*.so; do
  echo Stripping SD Artifact `basename $f`...
  %{__strip} -g $f
done
${old_nullglob}
%endif

# AV-1113 - check for incorrect platform leftovers
# Only checking libdir and not bindir etc. If it has one, it has all.
echo "Checking for invalid leftovers..."
PROB=n"`find %{coreprojdir}/components/ -type d | egrep 'x86_64\$' | grep -v %{OCPI_TARGET_HOST} | :``find %{buildroot}%{_libdir} -type d | egrep 'x86_64\$' | grep -v %{OCPI_TARGET_HOST} | :`"
if [ n != "${PROB}" ]; then
  printf "Incorrect platform libraries found in RPM build area!\\${PROB}\n"
  false
fi

# Fix embedded buildroot strings in all shipped components (including artifacts on SD cards)
for fname in `find %{buildroot} -name '*_s.so' | egrep '(components|artifacts)' | grep -v saved_xml`; do
  echo Fixing RPM_BUILD_ROOT string in ${fname}.
  /usr/lib/rpm/debugedit -b %{buildroot} -d / ${fname}
done

# Delete .build files
find %{coreprojdir}/components/lib/rcc/ \( -name '*.build' -o -name '*-build.xml' \) -delete

%if !%{OCPI_CROSS}
# Compress the coreprojdir the same way assets is done
%{__rm} -rf %{coreprojdir}/imports
# Manually register project for correct name and relative link
pushd %{projregistry}
%{__ln_s} -f ../projects/$(basename %{coreprojdir}) ocpi.core
# Also manually register the CDK. This link is static and the user should not change it
%{__ln_s} -f ../cdk ocpi.cdk
popd
%endif

# Stop some spamming from the debuginfo generation
unset XZ_OPT

# AV-1160 (again)
# Specifically symlink ocpidev and ocpishow. CentOS 7 already has them.
# CentOS 6 needs this or it these files will be duplicated in bin instead of symlinked.
%{__rm} -f %{buildroot}/%{_bindir}/%{OCPI_TARGET_HOST}/ocpidev
%{__rm} -f %{buildroot}/%{_bindir}/%{OCPI_TARGET_HOST}/ocpishow
%{__ln_s} -f ../../scripts/ocpidev %{buildroot}/%{_bindir}/%{OCPI_TARGET_HOST}/ocpidev
%{__ln_s} -f ../../scripts/ocpishow %{buildroot}/%{_bindir}/%{OCPI_TARGET_HOST}/ocpishow

%clean
%{__rm} -rf --preserve-root %{buildroot}

%pre
# This is exact copy from opencpi-driver.spec; if changed, change there as well.
# Check if somebody is installing on the wrong platform (AV-721)
# Starting with 1.2, we ship the EL7 version for both EL6 and EL7 when it comes to platforms
%if !%{OCPI_CROSS}
if [ -n "%{dist}" ]; then
  PKG_VER=`echo %{dist} | perl -ne '/el(\d)/ && print $1'`
  THIS_VER=`perl -ne '/release (\d)/ && print $1' /etc/redhat-release`
  if [ -n "${PKG_VER}" -a -n "${THIS_VER}" ]; then
    if [ ${PKG_VER} -ne ${THIS_VER} ]; then
      for i in `seq 20`; do echo ""; done
      echo "WARNING: This RPM is for CentOS${PKG_VER}, but you seem to be running CentOS${THIS_VER}"
      echo "You might want to uninstall these RPMs immediately and get the CentOS${THIS_VER} version."
      for i in `seq 5`; do echo ""; done
    fi
  fi
fi
%endif

# Recipe from https://fedoraproject.org/wiki/Packaging:UsersAndGroups
# -M is don't create home dir, -r is system account, -s is shell
# -c is comment, -n is don't create group, -g is group name/id
getent group opencpi >/dev/null || groupadd -r opencpi
getent passwd opencpi >/dev/null || \
  useradd -M -r -s /sbin/nologin \
    -c "OpenCPI System Account" -n -g opencpi opencpi > /dev/null

# This is the permission executables should have: executable by all (no setgid, see AV-1242)
# The .sh files that user should "source" should not be executable
%global gexec_perm 755

%check
# This will check that ocpirun would have the right symbols exported and other self-checks.

# This also exists in acinclude/ocpi_import_platforms and is pulling in all existing
# projects that have rcc platforms in them
export OCPI_PROJECT_PATH=$OCPI_PROJECT_PATH:$(find projects -type d  | grep rcc/platforms | cut -f 1-2 -d/ | sort -u | xargs -I xxx echo -n "$(pwd)/xxx:")
%if !%{OCPI_CROSS}
make %{?_smp_mflags} check
%endif

%files
%defattr(-,opencpi,opencpi,-)
%dir %{_prefix}
%{projsourcedir_final}/core/components/*.rcc/target-*%{OCPI_TARGET_HOST}/*
%{projsourcedir_final}/core/components/lib/rcc/%{OCPI_TARGET_HOST}
%if !%{OCPI_CROSS}
%{projsourcedir_final}/core/.project
%{projsourcedir_final}/core/specs
%{projsourcedir_final}/core/exports
%{projsourcedir_final}/core/components/lib/package-id
%{projsourcedir_final}/core/Makefile
%{projsourcedir_final}/core/Project.mk
%{projsourcedir_final}/core/Project.exports
# These directories, even if empty, are owned by the base RPM
%dir %{_bindir}
%dir %{_libdir}
%dir %{_libdir}/%{OCPI_TARGET_HOST}
%{_libdir}/package-id
%config %{_prefix}/env
# For security reasons, these should be root owned:
%attr(755,root,root) %{_prefix}/env/rpm_cdk.sh
%attr(755,root,root) %dir %config(noreplace) %{_prefix}/env.d
%attr(644,root,root) %{_prefix}/env.d/*.sh.example
%dir %{_prefix}/platforms
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/platforms/getPlatform.sh
%{projsourcedir_final}/core/rcc/platforms/centos*
#%dir %{_prefix}/specs
%dir %{_prefix}/scripts
#%{_prefix}/specs
/etc/profile.d/opencpi.sh
%config /etc/ld.so.conf.d/opencpi.conf
%config %{_prefix}/default-system.xml
%doc %{_prefix}/COPYRIGHT
%doc %{_prefix}/LICENSE.txt
%{_libdir}/%{OCPI_TARGET_HOST}/*.so
# Explicitly list scripts (AV-500)
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/findJtagByESN_xilinx
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/getESNfromUSB_xilinx
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/loadBitStream
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/loadFlash
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/loadFlash_altera
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/loadFlash_xilinx
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/ocpibootstrap.sh
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/ocpidriver
# Want to keep ocpi_linux_driver as "real" and not a symlink in case somebody NFS mounts the CDK (AV-966)
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/ocpi_linux_driver
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/ocpisetup.sh
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/ocpisudo
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/probeJtag
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/probeJtag_altera
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/probeJtag_xilinx
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/genProjMetaData.py*
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/freeze_project.sh
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/thaw_project.sh
%attr(644,opencpi,opencpi) %{_prefix}/scripts/util.sh
# These are needed for use with 'imports' (AV-3028)
%attr(775,opencpi,opencpi) %config(noreplace) %{projregistry_final}
# This is needed by the driver RPM to not require devel (AV-2150)
%{_prefix}/include/autoconfig_import.mk
# HDL Simulator out-of-the-box support (AV-1889)
%{_libdir}/platforms
%{_prefix}/include/hdl/xilinx.mk
%{_prefix}/include/hdl/vivado.mk
%{_prefix}/include/hdl/vivado-synth.tcl
%{_prefix}/include/hdl/vivado-impl.tcl
%{_prefix}/include/hdl/vivado-util.tcl
# Symlinked scripts and executables
/usr/bin/loadFlash
/usr/bin/ocfrp_check
/usr/bin/ocpi*
/usr/bin/probeJtag
# udev rules
%attr(644,root,root) /etc/udev/rules.d/*.rules
%else
# This is cross-compiled - these normally would be in the devel package for the host
%dir %{_libdir}/%{OCPI_TARGET_HOST}
%dir %{_bindir}/%{OCPI_TARGET_HOST}/xfer_tests
%dir %{_bindir}/%{OCPI_TARGET_HOST}/utils
%{_libdir}/%{OCPI_TARGET_HOST}/*.a
%{_libdir}/%{OCPI_TARGET_HOST}/*.so*
# The drivers (AV-622)
%{_libdir}/%{OCPI_TARGET_HOST}/*.ko
%{_libdir}/%{OCPI_TARGET_HOST}/*.rules
%doc %{_libdir}/%{OCPI_TARGET_HOST}/LICENSE.txt
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocfrp_check
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocpidds
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocpigen
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/utils/*
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/xfer_tests/*
%{_prefix}/include/autoconfig_import-%{OCPI_TARGET_PLATFORM}.mk
%global this_platform %{projsourcedir_final}/core/rcc/platforms/%{OCPI_TARGET_PLATFORM}
%if 0%{CORE_RCC}
  %{projsourcedir_final}/core/rcc/platforms/%{OCPI_TARGET_PLATFORM}/include
  %{this_platform}/target
  %dir %{this_platform}
  %{this_platform}/%{OCPI_TARGET_PLATFORM}-target.mk
  %{this_platform}/rcc=*=%{OCPI_TARGET_PLATFORM}.mk
%endif
# All platforms require:
%if !%{OCPI_CROSS}
%attr(%{gexec_perm},opencpi,opencpi) %{this_platform}/%{OCPI_TARGET_PLATFORM}-check.sh
%endif

%if "%{OCPI_TARGET_PLATFORM}" == "xilinx%{xilinx_release}"
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocpizynq
# Xilinx and Zynq subdirs
#%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/platforms/xilinx%{xilinx_release}/xilinx%{xilinx_release}-check.sh
                                     %{projsourcedir_final}/core/rcc/platforms/xilinx%{xilinx_release}/rcc=*
                                     %{projsourcedir_final}/core/rcc/platforms/xilinx%{xilinx_release}/target
                                     %{projsourcedir_final}/core/rcc/platforms/xilinx%{xilinx_release}/*-target.mk
                                     %{projsourcedir_final}/core/rcc/platforms/xilinx%{xilinx_release}/opencpi-zynq-linux-release-%{xilinx_release}.tar
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/platforms/zynq/formatOpenCPIZynqSD.sh
                                     %{_prefix}/platforms/zynq/*setup.sh
                                     %{_prefix}/platforms/zynq/zynq_system.xml
%doc %{this_platform}/README*
%doc %{_prefix}/platforms/zynq/README*
%endif
%if "%{OCPI_TARGET_PLATFORM}" == "pico_t6a"
#%doc %{this_platform}/doc/Picoflexor_T6A_Notes.pdf
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocpizynq

# CMH do we need these now hat the sd card is built ?
#%config(noreplace) %{this_platform}/system.xml
#                                     %{this_platform}/mynetsetup.sh
#                                     %{this_platform}/mysetup.sh
%attr(644,root,root) %{_prefix}/env.d/platform_pico_t6a.sh.example
%endif
# End of cross-compiled
%endif
%dir %{_bindir}/%{OCPI_TARGET_HOST}
%dir %{_bindir}/%{OCPI_TARGET_HOST}/ctests
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ctests/*
                                     %{_bindir}/%{OCPI_TARGET_HOST}/loadFlash
                                     %{_bindir}/%{OCPI_TARGET_HOST}/ocpidriver
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocpihdl
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocpiocl
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocpiocltest
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocpiremote
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocpirun
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocpiserve
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocpitests
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocpixml
                                     %{_bindir}/%{OCPI_TARGET_HOST}/probeJtag

# Hardware platform packages (AV-2272)
%if %{OCPI_TARGET_PLATFORM} == "xilinx%{xilinx_release}"
%files -n %{RPM_BASENAME}-hw-platform-zed
%define this_platform_sd %{sddir_final}/zed/OpenCPI-SD-zed
%config(noreplace) %{this_platform_sd}/opencpi/system.xml
%{this_platform_sd}
%{sddir_final}/zed/udev.rules
%endif

%if %{OCPI_TARGET_PLATFORM} == "pico_t6a"
%files -n %{RPM_BASENAME}-hw-platform-picoflexor
%define this_platform_sd %{sddir_final}/picoflexor/OpenCPI-SD-picoflexor
                               %{this_platform_sd}
%attr(%{gexec_perm},root,root) %{this_platform_sd}/opencpi/to_install/fix_mac.sh
%attr(%{gexec_perm},root,root) %{this_platform_sd}/opencpi/to_install/mdev
%config(noreplace)             %{this_platform_sd}/opencpi/mynetsetup.sh
%config(noreplace)             %{this_platform_sd}/opencpi/mysetup.sh
%config(noreplace)             %{this_platform_sd}/opencpi/system.xml
%dir %{sddir_final}/picoflexor
%{sddir_final}/picoflexor/udev.rules
%endif

%files devel
%defattr(-,opencpi,opencpi,-)
%dir %{_bindir}/%{OCPI_TARGET_HOST}/xfer_tests
%dir %{_bindir}/%{OCPI_TARGET_HOST}/utils
%{_libdir}/%{OCPI_TARGET_HOST}/*.a
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocfrp_check
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocpidds
                                     %{_bindir}/%{OCPI_TARGET_HOST}/ocpidev
                                     %{_bindir}/%{OCPI_TARGET_HOST}/ocpishow
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocpigen
                                     %{_bindir}/%{OCPI_TARGET_HOST}/ocpiview
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/xfer_tests/*
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/utils/*
%{_prefix}/include
%{_prefix}/ocpisetup.mk
%{_prefix}/project-package-id
%if %{OCPI_CROSS}
# This RPM gets thrown away by the build scripts, so put the conflicting stuff into this one.
%{_libdir}
%{_prefix}/COPYRIGHT
%{_prefix}/LICENSE.txt
%{_prefix}/platforms
# All scripts are thrown away in cross-compile
%{_prefix}/scripts/*
#%{_prefix}/specs
%{_prefix}/default-system.xml
%else
# Explicitly list scripts (AV-500)
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/makeExportLinks.sh
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/makeProjectExports.sh
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/makeStaticWorker
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/maybeExport.sh
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/ocpidev
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/ocpiutil.py*
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/upgradeApp_v1_3.py*
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/hdltargets.py*
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/ocpishow
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/ocpidev_bash_complete
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/ocpitarget.sh
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/ocpiview
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/setsid.py*
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/testrun.sh
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/testrunprep.sh
%attr(%{gexec_perm},opencpi,opencpi) %{_prefix}/scripts/testrunremote.sh
%attr(644,opencpi,opencpi)           %{_prefix}/scripts/testutil.sh
%attr(644,opencpi,opencpi)           %{_prefix}/scripts/clean-env.sh
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/test-opencpi-rpm
/usr/bin/test-opencpi-rpm
/etc/bash_completion.d/ocpidev_bash_complete.sh
#TODO: Remove the following line which is needed to support legacy testing
# devel RPMs need the actual source files in components and hdl as well as the
# new_project_source script so that writable copies of installed projects can be
# created
%{projsourcedir_final}/README
%{projsourcedir_final}/core/components
%{projsourcedir_final}/core/hdl
%attr(%{gexec_perm},opencpi,opencpi) %{projsourcedir_final}/new_project_source
%endif

%if !%{OCPI_CROSS}
%if "0%{?dist}" != "0.el6"
%files interface-python
%{python_sitearch}
# Since there's nowhere better to put the GNU Radio binding tool
%attr(%{gexec_perm},opencpi,opencpi) %{_bindir}/%{OCPI_TARGET_HOST}/ocpigr
%endif
%endif

%if !%{OCPI_CROSS}
%post -p /sbin/ldconfig

%postun
/sbin/ldconfig
# Clean up empty directories if final uninstall (AV-3240)
# 0 means totally uninstalled (not an update)
if [ "$1" == "0" ]; then
  # Delete broken symlinks (in lib)
  find -L %{_prefix} -type l -delete
  for dir in ../projects/core include lib platforms; do
    if [ "$(find %{_prefix}/${dir} -type f 2>/dev/null | wc -l)" == "0" ]; then
      rm -rf %{_prefix}/${dir} || :
    else
      >&2 echo "Unexpected file(s) in $(realpath %{_prefix}/${dir}) directory:"
      >&2 find %{_prefix}/${dir} -type f
    fi
  done
  # unregister any user installed projects when rpms are uninstalled
  rm -rf %{projregistry_final}
fi

%endif

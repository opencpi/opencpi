%{!?_ocpi_ad9361_dir: %define _ocpi_ad9361_dir /opt/opencpi/prerequisites/ad9361}
%define         _prefix %{_ocpi_ad9361_dir}
Prefix:         %{_ocpi_ad9361_dir}

%global _hardened_build 1

%if "x%{?OCPI_CROSS_PREFIX}" == "x"
Name:           ocpi-prereq-ad9361
%else
Name:           ocpi-prereq-ad9361-platform-%{OCPI_PLATFORM}
Requires:       ocpi-prereq-ad9361
Requires(postun): ocpi-prereq-ad9361
BuildArch:      noarch
# This is provided by glibc.i686, but there is no way to check cross-architecture from a spec file, so we say the file itself.
# The end user can do "sudo yum install /lib/ld-linux.so.2" to get it. The Xilinx gcc tool chain is 32-bit only.
BuildRequires:  /lib/ld-linux.so.2
AutoReqProv:    no
%define _binaries_in_noarch_packages_terminate_build 0
%define __strip %{OCPI_CROSS_DIR}/%{OCPI_CROSS_PREFIX}-strip
%define debug_package %{nil}
%endif
# Version: %{OCPI_AD9361_VERSION}.%(date +%y%m%d)
Version: snapshot.%(date +%y%m%d)
Release: 4.%{OCPI_AD9361_COMMIT_SHORT}%{?dist}
Packager: ANGRYVIPER team <discuss@lists.opencpi.org>
Summary: RPM build of the AD9361 driver libraries for OpenCPI
License: GPL
Group: Development/Tools
URL: https://github.com/analogdevicesinc/no-OS
Source0: ad9361-bundled.tar
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
BuildRequires: ed
Prefix: /usr

%description
The 'no-OS' version of the AD9361 driver with various tweaks provided
by the OpenCPI OSS project.

This version is built from git revision %{OCPI_AD9361_COMMIT_SHORT}.

%prep
%setup -n ad9361 -q

echo "Applying OSS patches:"
mkdir buildtmp
mv install-ad9361.sh buildtmp
mkdir -p buildtmp/scripts/
mv ad9361.patch buildtmp/scripts/
cd buildtmp
set -o pipefail; ./install-ad9361.sh | tee install-ad9361.log
cat install-ad9361.log |  grep "RPM " | cut -f2- -d' ' > RPM_Commands.txt

%build
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%define myCC gcc
%define myAR ar
%else
%define myCC %{OCPI_CROSS_DIR}/%{OCPI_CROSS_PREFIX}-gcc
%define myAR %{OCPI_CROSS_DIR}/%{OCPI_CROSS_PREFIX}-ar
%endif

cd buildtmp

# At this point, we have an "RPM_Commands.txt" that looks like this:
# CC: -fPIC -I. -I../no-OS/ad9361/sw/platform_generic -I../no-OS/ad9361/sw -DAXI_ADC_NOT_PRESENT -c ../no-OS/ad9361/sw/ad9361.c ../no-OS/ad9361/sw/ad9361_api.c ../no-OS/ad9361/sw/ad9361_conv.c ../no-OS/ad9361/sw/util.c
# AR: -rs libad9361.a ad9361.o ad9361_api.o ad9361_conv.o util.o
# INC: /home/user/rpmbuild/BUILD/ad9361/build-/../no-OS/ad9361/sw/ad9361_api.h
# INC: /home/user/rpmbuild/BUILD/ad9361/build-/../no-OS/ad9361/sw/ad9361.h
#cat RPM_Commands.txt

egrep '^CC' RPM_Commands.txt | while read -r line; do
#   echo "${line}"
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
  %{myCC} --std=c99 %{optflags} `echo ${line} | cut -f2- -d:`
%else
  %{myCC} --std=c99 %{OCPI_CFLAGS} `echo ${line} | cut -f2- -d:`
%endif
done

egrep '^AR' RPM_Commands.txt | while read -r line; do
#   echo "${line}"
  %{myAR} `echo ${line} | cut -f2- -d:`
done

%install
cd buildtmp
%define dest %{buildroot}/%{_prefix}/%{OCPI_TARGET_HOST}
%{__mkdir_p} %{dest}/lib
%{__cp} libad9361.a %{dest}/lib

%if "x%{?OCPI_CROSS_PREFIX}" == "x"
%define dest_inc %{buildroot}/%{_prefix}/include
%{__mkdir_p} %{dest_inc}

egrep '^INC' RPM_Commands.txt | while read -r line; do
#   echo "${line}"
  %{__cp} `echo ${line} | cut -f2- -d:` %{dest_inc}
done
%endif

# Make arm_cs and 13_4 versions if arm
%if "x%{?OCPI_TARGET_HOST}" == "xlinux-x13_3-arm"
ln -sf %{_prefix}/%{OCPI_TARGET_HOST} %{buildroot}/%{_prefix}/linux-x13_4-arm
ln -sf %{_prefix}/%{OCPI_TARGET_HOST} %{buildroot}/%{_prefix}/linux-zynq-arm_cs
%endif

%clean
rm -rf %{buildroot}

%postun
%if "x%{?OCPI_CROSS_PREFIX}" == "x"
# 0 means totally uninstalled (not an update)
if [ "$1" == "0" ]; then
  rm -rf %{_prefix}/ || :
fi
%endif

%files
%{_prefix}
%if "x%{?OCPI_TARGET_HOST}" == "xlinux-x13_3-arm"
%{_prefix}/linux-x13_4-arm
%{_prefix}/linux-zynq-arm_cs
%endif

%changelog
* Mon Feb 26 2018 - snapshot.YYMMDD-4
- AV: Add Xilinx 13_4 alias
* Wed Feb  7 2018 - snapshot.YYMMDD-3
- AV: Rename RPMs
* Thu Nov 16 2017 - snapshot.YYMMDD-2
- AV: Include fake arm_cs symlink
* Fri Oct 13 2017 - snapshot.YYMMDD-1
- AV: Bumped 2016_R2 pointer. Renamed master => snapshot.
* Fri Mar 24 2017 - master.YYMMDD-5
- AV: Changed to C99/C++0x defaults

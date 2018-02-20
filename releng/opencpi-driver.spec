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

%define         _prefix /opt/opencpi/driver
Name:           %{RPM_BASENAME}
Version: %{RPM_VERSION}
Release: %{RPM_RELEASE}%{?RELEASE_TAG}%{?COMMIT_TAG}%{?dist}
BuildArch: noarch
Prefix: %{_prefix}

Summary:   The OpenCPI Linux Kernel Driver
Group:     System Environment/Kernel

License:        LGPLv3+
Source:         %{RPM_BASENAME}-%{version}.tar
Vendor:         ANGRYVIPER Team
Packager:       ANGRYVIPER team <discuss@lists.opencpi.org>

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
# You can't say "kernel-devel" = "your installed kernel" (AV-2407)
Requires: gcc-c++ kernel-devel
Requires: chkconfig initscripts
Requires(post): gcc-c++ kernel-devel
Requires(post,preun): chkconfig initscripts
Requires(pre): perl shadow-utils

%description
This package installs the source files for the kernel driver and puts a
startup script that will rebuild the driver as needed.
%if "0%{?COMMIT_HASH}"
Release ID: %{COMMIT_HASH}
%endif

%prep
%setup -q -n %{RPM_BASENAME}-%{version}

%install
%{__mkdir_p} %{buildroot}/%{_prefix}/
%{__cp} --target-directory=%{buildroot}/%{_prefix} *
%{__mkdir_p} %{buildroot}/%{_initddir}
%{__mv} %{buildroot}/%{_prefix}/opencpi-driver-check %{buildroot}/%{_initddir}
%{__ln_s}f %{_initddir}/opencpi-driver-check %{buildroot}/%{_prefix}/opencpi-driver-check

%clean
rm -rf --preserve-root %{buildroot}

%files
%defattr(-,opencpi,opencpi,-)
%dir %{_prefix}
%{_prefix}/
%{_initddir}/opencpi-driver-check
%doc %{_prefix}/README
%doc %{_prefix}/COPYRIGHT
%doc %{_prefix}/LICENSE.txt

%pretrans
# Check if the driver is currently loaded/running
if [ -n "$(lsmod | grep opencpi | grep -v ' 0')" ]; then
  echo ERROR: Cannot install or upgrade driver RPM until the current driver is no longer in use.
  exit 1
fi

# This is exact copy from opencpi.spec; if changed, change there as well.
%pre
# Check if somebody is installing on the wrong platform (AV-721)
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

# Recipe from https://fedoraproject.org/wiki/Packaging:UsersAndGroups
# -M is don't create home dir, -r is system account, -s is shell
# -c is comment, -n is don't create group, -g is group name/id
getent group opencpi >/dev/null || groupadd -r opencpi
getent passwd opencpi >/dev/null || \
  useradd -M -r -s /sbin/nologin \
    -c "OpenCPI System Account" -n -g opencpi opencpi > /dev/null

%post
# Delete first in case their runlevels have changed in an upgrade (safe to do if doesn't exist):
/sbin/chkconfig --del opencpi-driver-check
# Adding will automatically turn on:
/sbin/chkconfig --add opencpi-driver-check
# SELinux fixes (not sure if actually needed)
chcon system_u:object_r:initrc_exec_t:s0 %{_initddir}/opencpi-driver-check || :
chcon -h system_u:object_r:initrc_exec_t:s0 %{_initddir}/opencpi-driver-check || :
rm -rf %{_prefix}/`uname -r` 2>/dev/null || :
%{_prefix}/opencpi-driver-check start || :
touch /tmp/opencpi_driver_just_installed

%preun
%{_prefix}/opencpi-driver-check stop || :
if [ -n "`lsmod | grep opencpi`" ]; then
  echo ERROR: Cannot uninstall driver RPM until the current driver is no longer in use.
  exit 1
fi
# 0 means totally uninstalled (not an update)
if [ "$1" == "0" ]; then
  /sbin/chkconfig opencpi-driver-check off
  /sbin/chkconfig --del opencpi-driver-check
fi

%postun
# 0 means totally uninstalled (not an update)
if [ "$1" == "0" ]; then
  rm -rf %{_prefix}/ || :
fi

%triggerin -- kernel
if [ -e /tmp/opencpi_driver_just_installed ]; then
  rm /tmp/opencpi_driver_just_installed
else
  echo New kernel detected - OpenCPI drivers should be rebuilt on next reboot.
  echo If they do not, manually run "sudo %{_initddir}/opencpi-driver-check start"
fi

%verifyscript
# AV-2407
set -e
>&2 %{_initddir}/opencpi-driver-check check
>&2 %{_initddir}/opencpi-driver-check status

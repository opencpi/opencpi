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

##########################################################################################
# This RPM spec is for a package that contains the kernel driver source code and the various
# scripting around it so that it is built on the system it is install on (for the running
# kernel at the of installation) *AND* is automatically rebuilt whenever the kernel on the
# installed system is upgraded.
Name:      %{RPM_NAME}-driver
Version:   %{RPM_VERSION}
Release:   %{RPM_RELEASE}%{?dist}
BuildArch: noarch
%global    prefix0 /opt/opencpi
Prefix:    %{prefix0}
%global    prefix1 %{_initddir}
Prefix:    %{prefix1}

Summary:   The OpenCPI Linux Kernel Driver
Group:     System Environment/Kernel

License:   LGPLv3+
Vendor:    ANGRYVIPER Team
Packager:  ANGRYVIPER Team <discuss@lists.opencpi.org>

# BuildRoot is deprecated, but maybe this was for OLD rpmbuild in centos6?
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

%{?RPM_HASH:ReleaseID: %{RPM_HASH}}

%prep
# Empty; rpmlint recommendeds it is present anyway

%build
# Empty; rpmlint recommendeds it is present anyway

%install
cd %{RPM_OPENCPI}
set -e
rm -r -f %{buildroot}%{prefix0}
./packaging/prepare-rpm-files.sh driver %{RPM_PLATFORM} "%{?RPM_CROSS:1}" \
                                        %{buildroot} %{prefix0} %{_builddir}

# Do the global installation parts
%{__mkdir_p} %{buildroot}/%{prefix1}
# This will be re-done at installation if relocated
%{__ln_s} -f %{prefix0}/driver/opencpi-driver-check %{buildroot}/%{prefix1}/opencpi-driver-check
echo %%{prefix1}/opencpi-driver-check >> %{_builddir}/driver-files

%files -f driver-files
#%%doc %%{_prefix0}/README
#%%doc %%{_prefix0}/COPYRIGHT
#%%doc %%{_prefix0}/LICENSE.txt

%pretrans
# Check if the driver is currently loaded/running
if [ -n "$(lsmod | grep opencpi | grep -v ' 0')" ]; then
  echo ERROR: Cannot install or upgrade driver RPM until the current driver is no longer in use. >&2
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
      echo "WARNING: This RPM is for CentOS${PKG_VER}, but you seem to be running CentOS${THIS_VER}" >&2
      echo "You might want to uninstall these RPMs immediately and get the CentOS${THIS_VER} version." >&2
      for i in `seq 5`; do echo "" >&2; done
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
chcon system_u:object_r:initrc_exec_t:s0 %{prefix1}/opencpi-driver-check || :
chcon -h system_u:object_r:initrc_exec_t:s0 %{prefix1}/opencpi-driver-check || :
rm -rf %{prefix0}/driver/`uname -r` 2>/dev/null || :
%{prefix1}/opencpi-driver-check start || :
touch /tmp/opencpi_driver_just_installed

%preun
%{prefix1}/opencpi-driver-check stop || :
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
  rm -rf %{prefix0}/driver || :
fi

%triggerin -- kernel
if [ -e /tmp/opencpi_driver_just_installed ]; then
  rm /tmp/opencpi_driver_just_installed
else
  echo New kernel detected - OpenCPI drivers should be rebuilt on next reboot.
  echo If they do not, manually run "sudo %{prefix1}/opencpi-driver-check start"
fi

%verifyscript
# AV-2407
set -e
>&2 %{prefix1}/opencpi-driver-check check
>&2 %{prefix1}/opencpi-driver-check status

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

%global         prefix0 /opt/opencpi
%global         deploy_dir cdk
%global         prefix1 /etc
# Since this package is for files that will be pushed to another platform
# We do not need error out to arch packages and we also do not need
# to generate dependices on the so files we have
%define _binaries_in_noarch_packages_terminate_build   0
%define __strip %{RPM_CROSS_COMPILE}strip
AutoReqProv: no
Name:           %{RPM_NAME}
Requires(pre,postun):  opencpi = %{RPM_VERSION}
Requires:               opencpi = %{RPM_VERSION}
Requires:         opencpi-devel = %{RPM_VERSION}
Requires(post): hardlink
BuildArch:      noarch
Version:        %{RPM_VERSION}
Release:        %{RPM_RELEASE}%{?dist}
Prefix:         %{prefix0}
Prefix:         %{prefix1}
Summary: OpenCPI's hw-platform-%{RPM_PLATFORM} Support Project

Group:          Applications/Engineering
License:        LGPLv3+
Source0:        %{RPM_PLATFORM}
Vendor:         OpenCPI
Packager:       ANGRYVIPER Team <discuss@lists.opencpi.org>

%description
Contains the content needed to deploy opencpi to %{RPM_PLATFORM}

%if "x%{?RPM_HASH}" != "x"
Release ID: %{RPM_HASH}
%endif

%prep
# Empty; rpmlint recommendeds it is present anyway

%build
# Empty; rpmlint recommendeds it is present anyway

%install
%{__mkdir_p} %{buildroot}%{prefix0}/%{deploy_dir}/%{RPM_PLATFORM}
%{__mv} %{SOURCE0} %{buildroot}%{prefix0}/%{deploy_dir}/

%clean
%{__rm} -rf --preserve-root %{buildroot}

%post
hardlink ${prefix0}/%{deploy_dir}/ || :
shopt -s nullglob
[ "$RPM_INSTALL_PREFIX0" != "%{prefix0}" ] \
  && path0=$RPM_INSTALL_PREFIX0/%{RPM_PLATFORM}/host-udev-rules \
  || path0=%{prefix0}/%{deploy_dir}/%{RPM_PLATFORM}/host-udev-rules
[ "$RPM_INSTALL_PREFIX1" != "%{prefix1}" ] \
  && path1=$RPM_INSTALL_PREFIX1 \
  || path1=%{prefix1}
for link in $path0/*; do
  mkdir -p $path1/udev/rules.d
  ln -s -f $link $path1/udev/rules.d/$(basename $link)
done
# CentOS7 doesn't use inotify any more with systemd's udev, and this is harmless on C6:
udevadm control --reload-rules || :

%postun
# Uninstall any broken links left in udev rules (e.g. ones installed in %%post but now deleted)
shopt -s nullglob
[ "$RPM_INSTALL_PREFIX1" != "%{prefix1}" ] \
  && path1=$RPM_INSTALL_PREFIX1 \
  || path1=%{prefix1}
for link in $path1/udev/rules.d/*; do
  [ ! -e "$link" ] && rm $link || :
done
udevadm control --reload-rules || :
# Always exit with a good status
:

%files
%defattr(644,opencpi,opencpi,755)
%dir %{prefix0}/%{deploy_dir}/%{RPM_PLATFORM}
%{prefix0}/%{deploy_dir}/%{RPM_PLATFORM}/*

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

%global         projects_dir /opt/opencpi/projects
%global         assets_dir %{projects_dir}/assets

Name:           opencpi-project-assets
BuildRequires:  perl
Requires(pre):  opencpi
Requires:       opencpi opencpi-devel xz
# AV-2772:
#### TEMPORARY FOR BUILD2 Requires:       ocpi-prereq-liquid
Requires:       tar >= 1.22
# AV-3314 (socket_write, etc):
Requires:       autoconf automake libtool
Requires(pre,postun): opencpi
BuildArch:      noarch
Version:        %{RPM_VERSION}
Release:        %{RPM_RELEASE}%{?RELEASE_TAG}%{?COMMIT_TAG}%{?dist}
Prefix:         %{assets_dir}
Summary: OpenCPI's "ocpi.assets" project

Group:          Applications/Engineering
License:        LGPLv3+
Source0:        opencpi-assets-%{version}.tar.xz
Source1:        assets_source.tar
Vendor:         ANGRYVIPER Team
Packager:       ANGRYVIPER team <discuss@lists.opencpi.org>

%description
opencpi-project-assets contains the "ocpi.assets" project containing the extensions
to OpenCPI provided by the ANGRYVIPER team.

%if "x%{?COMMIT_HASH}" != "x"
Release ID: %{COMMIT_HASH}
%endif

%prep
# this unpackages the extra support files from the rpm_support repo that were packaged up by
# running build.sh so that new_project_source is available.
tar -x --strip-components=1 -f %{SOURCE1}

%install
%{__mkdir_p} %{buildroot}%{projects_dir}/
%{__mkdir_p} %{buildroot}%{assets_dir}/
%{__mv} %{SOURCE0} %{buildroot}%{assets_dir}

%clean
%{__rm} -rf --preserve-root %{buildroot}

# This is the permission executables should have: executable by all (no setgid, see AV-1242)
# The .sh files that user should "source" should not be executable
%global gexec_perm 755

%files
%defattr(444,opencpi,opencpi,755)
%dir %{assets_dir}
%{assets_dir}/opencpi-assets*.tar.xz

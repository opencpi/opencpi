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

Name: opencpi-doc
Version: %{RPM_VERSION}
Release: %{RPM_RELEASE}%{?RELEASE_TAG}%{?COMMIT_TAG}%{?dist}
BuildArch: noarch
BuildRequires: ghostscript rubber unoconv

Summary:   OpenCPI Documentation
Group:     Documentation

License:        LGPLv3+
Vendor:         ANGRYVIPER Team
Packager:       ANGRYVIPER Team <discuss@lists.opencpi.org>

%description
PDFs and HTML index file installed into %{_pkgdocdir}
%if "0%{?COMMIT_HASH}"
Release ID: %{COMMIT_HASH}
%endif

%build
cd %{SRC_BASE}
make doc
find doc/pdfs -type d \( -name '*@tmp' -o -name logs \) -print0 | xargs -r0 rm -rf

%install
%{__mkdir_p} %{buildroot}/%{_pkgdocdir}/
cd %{SRC_BASE}/doc/pdfs
%{__cp} -R . %{buildroot}/%{_pkgdocdir}/
%{__mkdir_p} %{buildroot}/opt/opencpi/
%{__ln_s} -f %{_pkgdocdir}/ %{buildroot}/opt/opencpi/doc
%{__ln_s} -f %{_pkgdocdir}/index.html %{buildroot}/opt/opencpi/documentation.html

%files
%defattr(-,opencpi,opencpi,-)
%dir %{_pkgdocdir}
%doc %{_pkgdocdir}
%doc /opt/opencpi/doc
%doc /opt/opencpi/documentation.html

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
# The RPM spec for the runtime and devel packages
# The runtime is the primary/main package, and devel is a subpackage which depends on runtime
Name:        %{RPM_NAME}
Version:     %{RPM_VERSION}
Release:     %{RPM_RELEASE}%{?dist}
Summary:     A framework to simplify and enable code portability of real-time systems
Group:       Applications/Engineering
License:     LGPLv3+
%global      prefix0 /opt/opencpi
Prefix:      %{prefix0}
# Apparently rpmbuild does not do this for you, only RPM_INSTALL_DIR0/1/2 etc.
%global      prefix1 /etc
Prefix:      %{prefix1}
Vendor:      ANGRYVIPER Team
Packager:    ANGRYVIPER Team <discuss@lists.opencpi.org>
%include %{RPM_OPENCPI}/packaging/target-%{RPM_PLATFORM}/runtime-requires
# Then the "requires" that are only needed by the rpm installation itself rather than
# OpenCPI.
# This is required for adduser/chcon
Requires(pre):    shadow-utils perl
%if !%{RPM_CROSS}
##########################################################################################
# Native/development host package
# Replaces the old prereq packages
Conflicts: ocpi-prereq-ad9361 ocpi-prereq-gmp ocpi-prereq-gtest ocpi-prereq-patchelf ocpi-prereq-xz
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

%{?RPM_HASH:ReleaseID: %{RPM_HASH}}
%else
##########################################################################################
# Cross build package
AutoReqProv: no  # This must preceed the %description.  Go figure.
%define      __strip %{RPM_CROSS_COMPILE}strip
BuildArch:   noarch
%define      _binaries_in_noarch_packages_terminate_build 0
Requires:    %{name} = %{version}-%{release}
Requires(pre,postun): %{name} = %{version}-%{release}
Obsoletes:   %{RPM_BASENAME}-platform-%{RPM_PLATFORM}
%description
This package contains the OpenCPI static libraries for cross-compiling
for the %{RPM_PLATFORM} target platform, along with core components.

%{?RPM_HASH:ReleaseID: %{RPM_HASH}}
%endif
# suppress post processing that bytecompiles python for two reasons:
# 1. We're doing it in general for all distributions so its already done
# 2. We use non-system-default-version python and potentially mixed versions
%global __python NO_PYTHON_BYTE_COMPILATION
##########################################################################################
# install for both runtime and devel ############################
%install
set -e
cd %{RPM_OPENCPI}
# This avoids any aliasing from previous runs
rm -r -f %{buildroot}%{prefix0} %{buildroot}%{prefix1}
# Copy the needed files to buildroot, and create the files lists in builddir
for p in runtime devel; do
  ./packaging/prepare-rpm-files.sh \
    $p %{RPM_PLATFORM} %{RPM_CROSS} %{buildroot} %{prefix0} %{_builddir}
done

%if !%{RPM_CROSS}
  ##########################################################################################
  # Enable globally-installed /etc files, symlink them to their "real" copies if possible

  # 1. Tell ld.so to look in our lib dir when looking for dynamic libraries
  #    Really only needed with a dynamic library installation
  #    But we always have this directory (due to plugins and swigs), so it probably does no harm
  #    We could make it conditional for a slight performance optimization
  #    Since prefix0 is relocatable, we re-do the file contents %postun
# we are disabling this until we sort out dynamic libraries
%if 0
  dir=ld.so.conf.d file=opencpi.conf
  %{__mkdir_p} %{buildroot}%{prefix1}/$dir
  echo %{prefix0}/cdk/%{RPM_PLATFORM}/lib > %{buildroot}%{prefix1}/$dir/$file
  echo %%{prefix1}/$dir/$file >> %{_builddir}/runtime-files
%endif
  # 2. Enable a global login .profile script by dropping ours into the directory that is used
  #    globally for all bash login scripts.  This drop-in is a symlink.
  dir=profile.d file=opencpi.sh
  %{__mkdir_p} %{buildroot}%{prefix1}/$dir
  %{__ln_s} -f %{prefix0}/cdk/env/rpm_cdk.sh %{buildroot}%{prefix1}/$dir/$file
  echo "%%attr(644,root,root)" %%{prefix1}/$dir/$file >> %{_builddir}/runtime-files

  # 3. Enable bash completion of our commands by dropping a script into a directory that
  #    is used when interactive bash scripts startup.  Only for the devel package.
  dir=bash_completion.d file=opencpi_complete.bash
  %{__mkdir_p} %{buildroot}%{prefix1}/$dir
  %{__ln_s} -f %{prefix0}/cdk/scripts/ocpidev_bash_complete %{buildroot}%{prefix1}/$dir/$file
  echo %%{prefix1}/$dir/$file >> %{_builddir}/devel-files

  # 4. Add our udev-rules into the drop-in directry for udev rules, using symlinks
  dir=udev/rules.d file=51-opencpi-usbblaster.rules
  %{__mkdir_p} %{buildroot}%{prefix1}/$dir
  %{__ln_s} -f %{prefix0}/udev-rules/$file %{buildroot}%{prefix1}/$dir
  echo "%%attr(644,root,root)" %%{prefix1}/$dir/$file >> %{_builddir}/runtime-files

  # A very special case that will go away at some point
  cp releng/projects/new_project_source %{buildroot}%{prefix0}/projects
  echo %%{prefix0}/projects/new_project_source >> %{_builddir}/devel-files
%endif

##########################################################################################
# files for runtime
%files -f runtime-files
%defattr(-,opencpi,opencpi,-)
%dir %{prefix0}

##########################################################################################
# The development (sub) package, that adds to what is installed after the runtime package.
%package devel
%include %{RPM_OPENCPI}/packaging/target-%{RPM_PLATFORM}/devel-requires
AutoReqProv: no
Requires:   %{name} = %{version}-%{release}
Requires(pre,postun): %{name} = %{version}-%{release}
Summary:    The OpenCPI development package
Group:      Development/Libraries
Prefix:     %{prefix0}
Prefix:     %{prefix1}
%description devel
This package ensures that all requirements for OpenCPI development are
installed. It also provides a useful development utilities.
%{?RPM_HASH:ReleaseID: %{RPM_HASH}}

%if !%{RPM_CROSS}
  # If not cross-compiled, the devel packages replace the old prereq packages
  Obsoletes: ocpi-prereq-ad9361 ocpi-prereq-gmp ocpi-prereq-gtest ocpi-prereq-patchelf
  Obsoletes: ocpi-prereq-xz
  Provides: ocpi-prereq-ad9361 ocpi-prereq-gmp ocpi-prereq-gtest ocpi-prereq-patchelf
  Provides: ocpi-prereq-xz
%endif

##########################################################################################
# files for devel
%files devel -f devel-files
%defattr(-,opencpi,opencpi,-)

##########################################################################################
# The debug subpackage - only added for dev platforms
%if %{RPM_CROSS}
  %define debug_package %{nil}
%else
  # Suppress errors, usually from rcc cross platforms that leak in here
  %undefine _missing_build_ids_terminate_build
  # Restore the artifact xml that was stripped by os_install_post
  # Use global vs define so it is immediately evaluated
  %global __os_install_post \
    %{__os_install_post} \
    oxml=%{RPM_OPENCPI}/cdk/%{RPM_PLATFORM}/bin/ocpixml \
    for a in %{RPM_OPENCPI}/cdk/%{RPM_PLATFORM}/artifacts/*; do \
      r=%{buildroot}%{prefix0}/cdk/%{RPM_PLATFORM}/artifacts/$(basename $a) \
      ! $oxml check $r && $oxml check $a && $oxml get $a | $oxml add $r - \
    done \
    %{nil}
  %debug_package
%endif

##########################################################################################
# The preinstall scriptlet for runtime
%pre
# This is exact copy from opencpi-driver.spec; if changed, change there as well.
# Check if somebody is installing on the wrong platform (AV-721)
# Starting with 1.2, we ship the EL7 version for both EL6 and EL7 when it comes to platforms
%if !%{RPM_CROSS}
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
%endif
if [ "$RPM_INSTALL_PREFIX1" = %{prefix1} -a "$RPM_INSTALL_PREFIX0" = %{prefix0} ]; then
  # Recipe from https://fedoraproject.org/wiki/Packaging:UsersAndGroups
  # -M is don't create home dir, -r is system account, -s is shell
  # -c is comment, -n is don't create group, -g is group name/id
  getent group opencpi >/dev/null || groupadd -r opencpi
  getent passwd opencpi >/dev/null || \
    useradd -M -r -s /sbin/nologin \
      -c "OpenCPI System Account" -n -g opencpi opencpi > /dev/null
else
  # We are in relocated mode.
  if [ "$RPM_INSTALL_PREFIX1" = %{prefix1} -o "$RPM_INSTALL_PREFIX0" = %{prefix0} ]; then
     cat<<-EOF
	This OpenCPI installation is partially relocated, which is not supported.
	The %{prefix0} directory is relocated to $RPM_INSTALL_PREFIX0
	The %{prefix1} directory is relocated to $RPM_INSTALL_PREFIX1
	Either relocate both or neither
EOF
     exit 1
  fi
  cat<<-EOF
	This OpenCPI installation is relocated.
	The %{prefix0} directory is relocated to $RPM_INSTALL_PREFIX0
	The %{prefix1} directory is relocated to $RPM_INSTALL_PREFIX1
	The warnings about the "opencpi" user and group not existing can be ignored.
	EOF
fi
##########################################################################################
# The postinstall scriptlet for runtime
%post
if [ "$RPM_INSTALL_PREFIX1" != %{prefix1} -o "$RPM_INSTALL_PREFIX0" != %{prefix0} ]; then
  echo This installation is relocated, so warnings about the \"opencpi\" user and group not existing can be ignored. >&2
  echo The user and group IDs of all files will be set to the login user and group. >&2
fi
%if !%{RPM_CROSS}
  # We need to relocate all the global files that point to other global files
  # The files have been installed, but we must change them now.

  # Relocate the global files that point to other files to respect relocations
#this is disabled until we sort out dynamic libraries
%if 0
  link=$RPM_INSTALL_PREFIX0/cdk/%{RPM_PLATFORM}/lib
  [ $(< $RPM_INSTALL_PREFIX1/ld.so.conf.d/opencpi.conf) != $link ] &&
    echo $link > $RPM_INSTALL_PREFIX1/ld.so.conf.d/opencpi.conf || :
%endif
  link=$RPM_INSTALL_PREFIX0/cdk/env/rpm_cdk.sh
  [ $(readlink $RPM_INSTALL_PREFIX1/profile.d/opencpi.sh) != $link ] &&
    ln -s -f $link $RPM_INSTALL_PREFIX1/profile.d/opencpi.sh || :
  link=$RPM_INSTALL_PREFIX0/udev-rules/51-opencpi-usbblaster.rules
  [ $(readlink $RPM_INSTALL_PREFIX1/udev/rules.d/51-opencpi-usbblaster.rules) != $link ] &&
    ln -s -f $link $RPM_INSTALL_PREFIX1/udev/rules.d/ || :
  # if not relocated, tell dynamic loader to find the (perhaps) new location
  [ "$RPM_INSTALL_PREFIX1" = %{prefix1} -a "$RPM_INSTALL_PREFIX0" = %{prefix0} ] &&
      ( : || /sbin/ldconfig)  # not enabled until we sort out dynamic libraries
%endif

[ "$RPM_INSTALL_PREFIX1" != %{prefix1} -o "$RPM_INSTALL_PREFIX0" != %{prefix0} ] && {
  user=`logname`
  group=`eval stat --format=%G ~$user`
  # echo USER: $user GROUP: $group
  chown -R $user $RPM_INSTALL_PREFIX0 $RPM_INSTALL_PREFIX1
  chgrp -R $group $RPM_INSTALL_PREFIX0 $RPM_INSTALL_PREFIX1
} || :
##########################################################################################
# The postuninstall scriptlet for runtime
%postun
# Nothing to do on upgrade
[ "$1" = 1 ] && exit 0
# if not relocated, tell dynamic loader to forget our lib dir
if [ "$RPM_INSTALL_PREFIX1" = %{prefix1} -a "$RPM_INSTALL_PREFIX0" = %{prefix0} ]; then
  : || /sbin/ldconfig # not enabled until we sort out dynamic libraries
else
  cat <<-EOF >&2
	The OpenCPI installation being removed was relocated.
	The %{prefix0} directory was relocated to $RPM_INSTALL_PREFIX0
	The %{prefix1} directory was relocated to $RPM_INSTALL_PREFIX1
	While in a global installation the %{prefix1} directory would not be removed,
	in this relocated installation $RPM_INSTALL_PREFIX1 will be removed if empty.
	EOF
  owner=`stat --format=%U $RPM_INSTALL_PREFIX1`
  if [ -z "$owner" -o "$owner" = root -o "$owner" = opencpi ]; then
    echo Owner of $RPM_INSTALL_PREFIX1 is \"$owner\".  It is not being deleted. >&2
  else
    rmdir $RPM_INSTALL_PREFIX1 || :
  fi
fi
##########################################################################################
# The preinstall scriptlet for devel
%pre devel
prefixes=(`rpm -q --qf '[%{INSTPREFIXES}\n]' opencpi`)
# This comparison against "package" happens on centos6...
if [ -n "${prefixes[0]}" -a -n "${prefixes[1]}" -a "${prefixes[0]}" != package -a \( \
     "${prefixes[0]}" != "$RPM_INSTALL_PREFIX0" -o \
     "${prefixes[1]}" != "$RPM_INSTALL_PREFIX1" \) ]; then
   cat <<-EOF >&2
	The pre-existing OpenCPI runtime installation was relocated differently than is specified
	for this opencpi-devel package installation, which is not allowed.
	The existing runtime installation has:
	    %{prefix0} relocated to ${prefixes[0]}
	    %{prefix1} relocated to ${prefixes[1]}
	while this installation is requested to be:
	    %{prefix0} relocated to $RPM_INSTALL_PREFIX0
	    %{prefix1} relocated to $RPM_INSTALL_PREFIX1
	EOF
   exit 1
fi
if [ "$RPM_INSTALL_PREFIX1" != %{prefix1} -o "$RPM_INSTALL_PREFIX0" != %{prefix0} ]; then
  cat<<-EOF
	This OpenCPI development installation is relocated consistent with the runtime install.
	The %{prefix0} directory is relocated to $RPM_INSTALL_PREFIX0
	The %{prefix1} directory is relocated to $RPM_INSTALL_PREFIX1
	The warnings about the opencpi user and group not existing can be ignored.
	EOF
fi
##########################################################################################
# The postinstall scriptlet for devel
%post devel
if [ "$RPM_INSTALL_PREFIX1" != %{prefix1} -o "$RPM_INSTALL_PREFIX0" != %{prefix0} ]; then
  echo This installation is relocated, so warnings about the \"opencpi\" user and group not existing can be ignored. >&2
  echo The user and group IDs of all files will be set to the login user and group. >&2
fi
%if !%{RPM_CROSS}
  # We need to relocate all the global files that point to other global files.
  # The files have been installed, but we must change them now.
  link=$RPM_INSTALL_PREFIX0/cdk/scripts/ocpidev_bash_complete
  [ $(readlink $RPM_INSTALL_PREFIX1/bash_completion.d/opencpi_complete.bash) != $link ] &&
    ln -s -f $link $RPM_INSTALL_PREFIX1/bash_completion.d/opencpi_complete.bash || :
%endif
[ "$RPM_INSTALL_PREFIX1" != %{prefix1} -o "$RPM_INSTALL_PREFIX0" != %{prefix0} ] && {
  user=`logname`
  group=`eval stat --format=%G ~$user`
  # echo USER: $user GROUP: $group
  chown -R $user $RPM_INSTALL_PREFIX0 $RPM_INSTALL_PREFIX1
  chgrp -R $group $RPM_INSTALL_PREFIX0 $RPM_INSTALL_PREFIX1
} || :

##########################################################################################
# Suppress cleaning so we can easily look - cleanpackaging does it all at a higher level
%clean

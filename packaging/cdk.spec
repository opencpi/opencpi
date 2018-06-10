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
# Aparently rpmbuild does not do this for you, only RPM_INSTALL_DIR0/1/2 etc.
%global      prefix1 /etc
Prefix:      %{prefix1}
Vendor:      ANGRYVIPER Team
Packager:    ANGRYVIPER team <discuss@lists.opencpi.org>
%if !0%{?RPM_CROSS:1}
##########################################################################################
# Native/development host package
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
%else
##########################################################################################
# Cross build package
AutoReqProv: no  # This must preceed the %description.  Go figure.
%global      __strip ${OCPI_CROSS_COMPILE}-strip
BuildArch:   noarch
%define _binaries_in_noarch_packages_terminate_build 0
#Requires(pre,postun): %{RPM_BASENAME}
Obsoletes:   %{RPM_BASENAME}-platform-%{RPM_CROSS}
%description
This package contains the OpenCPI static libraries for cross-compiling
for the %{RPM_PLATFORM} target platform, along with core components.
%endif

%{?RPM_HASH:ReleaseID: %{RPM_HASH}}

# suppress generic post processing that is done for all packagers, like python bytecompile,
# stripping, etc. This does not suppress things like check-buildroot
%global __os_install_post %{nil}
%install
cd %{RPM_OPENCPI}
set -e
rm -r -f $buildroot$prefix
# Copy the needed files to buildroot, and create the files lists in builddir
for p in runtime devel; do
  ./packaging/prepare-rpm-files.sh $p %{RPM_PLATFORM} "%{?RPM_CROSS:1}" \
                                   %{buildroot} %{prefix0} %{_builddir}
done

%if !0%{?RPM_CROSS:1}
  ##########################################################################################
  # Enable globally-installed /etc files, symlink them to their "real" copies if possible

  # 1. Tell ld.so to look in our lib dir when looking for dynamic libraries
  #    Really only needed with a dynamic library installation
  #    But we always have this directory (due to plugins and swigs), so it probably does no harm
  #    We could make it conditional for a slight performance optimization
  #    Since prefix0 is relocatable, we re-do the file contents %postun
  dir=ld.so.conf.d file=opencpi.conf
  %{__mkdir_p} %{buildroot}%{prefix1}/$dir
  echo %{prefix0}/%{RPM_PLATFORM}/lib > %{buildroot}%{prefix1}/$dir/$file
  echo %%{prefix1}/$dir/$file >> %{_builddir}/runtime-files

  # 2. Enable a global login .profile script by dropping ours into the directory that is used
  #    globally for all bash login scripts.  This drop-in is a symlink.
  dir=profile.d file=opencpi.sh
  %{__mkdir_p} %{buildroot}%{prefix1}/$dir
  %{__ln_s} -f %{prefix0}/cdk/env/rpm_cdk.sh %{buildroot}%{prefix1}/$dir/$file
  echo %%{prefix1}/$dir/$file >> %{_builddir}/runtime-files

  # 3. Enable bash completion of our commands by dropping a script into a directory that
  #    is used when interactive bash scripts startup.  Only for the devel package.
  dir=bash_completion.d file=opencpi_complete.bash
  %{__mkdir_p} %{buildroot}%{prefix1}/$dir
  %{__ln_s} -f %{prefix0}/scripts/ocpidev_bash_complete %{buildroot}%{prefix1}/$dir/$file
  echo %%{prefix1}/$dir/$file >> %{_builddir}/devel-files

  # 4. Add our udev-rules into the drop-in directry for udev rules, using symlinks
  dir=udev/rules.d file=51-opencpi-usbblaster.rules
  %{__mkdir_p} %{buildroot}%{prefix1}/$dir
  %{__ln_s} -f %{prefix0}/udev-rules/$file %{buildroot}%{prefix1}/$dir
  echo %%{prefix1}/$dir/$file >> %{_builddir}/runtime-files

  # A very special case that will go away at some point
  cp releng/projects/new_project_source %{buildroot}%{prefix0}/projects
  echo %%{prefix0}/projects/new_project_source >> %{_builddir}/devel-files
%endif

%files -f runtime-files

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

%files devel -f devel-files

%pre
# This is exact copy from opencpi-driver.spec; if changed, change there as well.
# Check if somebody is installing on the wrong platform (AV-721)
# Starting with 1.2, we ship the EL7 version for both EL6 and EL7 when it comes to platforms
%if !0%{?RPM_CROSS:1}
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


%post

# Since we want to make the global installation aspects optional, we deal with the opencpi
# user and group optionally after all the files are installed

# Deal with the opencpi user and group if we are installing into /etc
if [ "$RPM_INSTALL_PREFIX1" = %{prefix1} ] ; then
  # Recipe from https://fedoraproject.org/wiki/Packaging:UsersAndGroups
  # -M is don't create home dir, -r is system account, -s is shell
  # -c is comment, -n is don't create group, -g is group name/id
  getent group opencpi >/dev/null || groupadd -r opencpi
  getent passwd opencpi >/dev/null || \
      useradd -M -r -s /sbin/nologin \
      -c "OpenCPI System Account" -n -g opencpi opencpi > /dev/null
  chown -R opencpi:opencpi $RPM_INSTALL_PREFIX0
fi

# We need to relocate all the global files that point to other global files
# The files have been installed, but we must change them now.

# Relocate the global files that point to other files to respect relocations
echo $RPM_INSTALL_PREFIX0/cdk/%{RPM_PLATFORM}/lib \
      > $RPM_INSTALL_PREFIX1/ld.so.conf.d/opencpi.conf
ln -s -f $RPM_INSTALL_PREFIX0/cdk/env/rpm_cdk.sh $RPM_INSTALL_PREFIX1/profile.d/opencpi.sh
ln -s -f $RPM_INSTALL_PREFIX0/cdk/udev-rules/51-opencpi-usbblaster.rules \
         $RPM_INSTALL_PREFIX1/udev/rules.d/

# Since these are all symlinks we are actually changing ownership of the underlying file
if [ "$RPM_INSTALL_PREFIX1" = %{prefix1} ] ; then
  chown -R root:root $RPM_INSTALL_PREFIX0/cdk/env.d
  chown root:root $RPM_INSTALL_PREFIX0/cdk/env/rpm_cdk.sh
  chown root:root $RPM_INSTALL_PREFIX0/cdk/opencpi-setup.sh
fi

# restore initial
%preun
# I tried this to see if it allowed directories to be removed by it didn;t
#if [ "$RPM_INSTALL_PREFIX1" = %{prefix1} ] ; then
  # chown -R root:root $RPM_INSTALL_PREFIX0
#fi
%postun
echo I AM MANUALLY REMOVING $RPM_INSTALL_PREFIX0
rm -r -f -v $RPM_INSTALL_PREFIX0

%post devel
# We need to relocate all the global files that point to other global files.
# The files have been installed, but we must change them now.
ln -s -f $RPM_INSTALL_PREFIX0/cdk/scripts/ocpidev_bash_complete \
         $RPM_INSTALL_PREFIX1/bash_completion.d/opencpi_complete.bash
if [ "$RPM_INSTALL_PREFIX1" = %{prefix1} ] ; then
  # We could be surgical about this but it won't likely help anything.
  # We are (re)touching runtime files and doing what has already been done before.
  # It might change the "last status change" date, but that shouldn't matter
  # FIXME: only change what is not already correct
  chown -R opencpi:opencpi $RPM_INSTALL_PREFIX0
  chown -R root:root $RPM_INSTALL_PREFIX0/cdk/env.d
  chown root:root $RPM_INSTALL_PREFIX0/cdk/env/rpm_cdk.sh
  chown root:root $RPM_INSTALL_PREFIX0/cdk/opencpi-setup.sh
  # This allows users in the opencpi group to register/unregister projects
  chmod 775 $RPM_INSTALL_PREFIX0/project-registry
  # This is to enable the creationg of import links which is BOGUS and will be fixed
  chmod 775 $RPM_INSTALL_PREFIX0/projects/core
  chmod 775 $RPM_INSTALL_PREFIX0/projects/assets
fi

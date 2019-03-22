#!/bin/bash --noprofile
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


proxydir=lib
proxy=ocpi2rh_proxy
function help {
  cat <<EOF >&2
This script exports an OpenCPI application or component as an installable RedHawk component.
The result is an RPM file that can be installed into a RedHawk environment.
If an OpenCPI component (rather than an application) is specified, a simple application
consisting that component, with all ports made external, will be created from the component.
With the -i option, the created RPM file is directly installed into the local RedHawk environment.

After options, and the first argument, which is the OpenCPI component or application xml file,
are deployment XML files.  If there are no deployment files, one will be created.
These files imply all the execution possibilities that the exported RPM should support.
Options are:
 -f <file>  Add the given file to the output RPM for the app to use at runtime.
 -d <dir>   The destination directory where the RPM will be placed (default is ".")
 -t <tmp>   The staging directory used for this processs.  If not specified, the directory will
            be a temporary directory.  The staging directory will be removed unless -k is set.
 -k         Keep the staging directory rather than removing it
 -p <pkg>   Specify component package/group when installed in RedHawk (default is "ocpi")
            Used as:  the RPM package prefix, the IDE group, and the installed directory name
 -c         Make an implicit application based on a single component, rather than an application.
            This implies that the first argument is a component name, not an application XML file.
            Any OpenCPI package prefix will be stripped from the resulting Redhawk component,
            then the -p pkg argument will be applied.
 -i         Install the resulting RPM, using the -y option to yum install.
            In any case the RPM will be available in the destination directory.
EOF
  exit 1
}
function bad {
  echo Error: $* >&2
  exit 1
}
function myshift {
  unset argv[0]
  argv=(${argv[@]})
}
function takeval {
  [ -z "${argv[1]}" ] && bad missing value after ${argv[0]} option for $1
  [[ "${argv[1]}" == -* ]] && bad value after ${argv[0]} option: ${argv[1]}
  eval $1=${argv[1]}
  myshift
}
function addval {
  [ -z "${argv[1]}" ] && bad missing value after ${argv[0]} option for $1
  [[ "${argv[1]}" == -* ]] && bad value after ${argv[0]} option: ${argv[1]}
  eval $1="(\${$1[@]} ${argv[1]})"
  myshift
}
# Poor mans xml parsing of machine generated XML files...
# Global variables: TAG_NAME, ATTRIBUTES, ARTIFACT
function read_element {
    local IFS=\>
    read -d \< ENTITY CONTENT
    local ret=$?
    TAG_NAME=${ENTITY%% *}
    ATTRIBUTES=${ENTITY%/}
    # echo ENTITY is \'$ENTITY\'
    # echo ATTRIBUTES is \'$ATTRIBUTES\'
    return $ret
}
function get_artifact {
    eval local $ATTRIBUTES
    ARTIFACT=$artifact
}
set -e
[ "$#" == 0 -o "$1" == -help -o "$1" == --help -o "$1" == -h ] && help
# default package for now
pkg=ocpi
files=()
depfiles=()
argv=($*)
directory=.
while [[ "${argv[0]}" != "" ]] ; do
  if [[ "${argv[0]}" == -* ]] ; then
    case ${argv[0]} in
      (-v) verbose=1;;
      (-k) keep=1;;
      (-f) addval files;;
      (-d) takeval directory;;
      (-p) takeval pkg;;
      (-t) takeval tmpdir;;
      (-i) install=1;;
      (-c) component=1;;
      (*)  bad unknown option: ${argv[0]} ;;
    esac
  else
      f=${argv[0]}
      if [ "$appfile" -o -z "$component" ] && [ ! -f $f ]; then
	  if [ -f $f.xml ]; then
	      f=$f.xml
	  else
	      bad Neither \"$f\" nor \"$f.xml\" exist.
	  fi
      fi
      if [ -z "$appfile" ]; then
	  appfile=$f
      else
	  depfiles=(${depfiles[@]} $f)
      fi
  fi
  myshift
done
[ -f $directory ] && {
    echo The destination directory, \"$directory\", is not a directory.
    exit 1
}
if [ -z "$tmpdir" ]; then
    tmpdir=$(mktemp -d 2>/dev/null || mktemp -d -t ocpi2rhXXX)
    istemp=1
else
    case $tmpdir in
	(/*);;
	(*) tmpdir=`pwd`/$tmpdir;;
    esac
    mkdir -p $tmpdir
    rm -r -f $tmpdir/*
fi
[ -n "$component" ] && {
    component=${appfile##*.}
    qualcomp=${appfile}
    appfile=$tmpdir/$component.xml
    cat <<-EOF >$appfile
	<application>
	  <instance component='$qualcomp' externals='true'/>
	</application>
	EOF
}

mkdir -p $directory
source $OCPI_CDK_DIR/scripts/ocpitarget.sh ""
app=$(basename $appfile .xml)
[ -n "$verbose" ] && {
  echo files are \(${files[@]}\)
  echo depfiles are \(${depfiles[@]}\)
  echo tmpdir is $tmpdir
  echo output directory is $directory
  echo app is $app file is $appfile
}
if [ -z "$depfiles" ]; then
  echo No deployment files supplied, so we will deploy the application here.
  depfiles=($tmpdir/depfile.xml)
  if $OCPI_CDK_DIR/bin/$OCPI_TOOL_DIR/ocpirun --no-execute --deploy-out $depfiles $appfile; then
    echo Deployment file \"$depfiles\" created successfully.
  else
    bad Failed creating a deployment for application file \"$app\"
  fi
fi
# Extract all the artifact files from all the deployment XML files
for i in ${depfiles[@]}; do
  while read_element; do
    # echo READ ELEMENT $TAG_NAME
    if [ "$TAG_NAME" = instance ]; then
      get_artifact
      artifacts=($artifacts $ARTIFACT)
    fi
  done < $i
done
n=0
# echo ARTIFACTS: $(echo ${artifacts[@]} | sort -u)
for a in $(for x in ${artifacts[@]}; do echo $x; done | sort -u); do
  # echo ARTIFACT_PATH=$a
  mkdir -p $tmpdir/artifacts
  cp $a $tmpdir/artifacts/$(printf a%03u-${a##*/} $n)
  n=$((n + 1))
done
echo Generating the SCA XML files corresponding to this deployment file.
if $OCPI_CDK_DIR/bin/$OCPI_TOOL_DIR/ocpisca ${pkg:+-p $pkg} -d $depfiles -x -D $tmpdir $appfile; then
    echo SCA XML files created successfully.
else
    bad Failed creating SCA XML files from application and deployment files.
fi
# Put in the ancillary files in the dir where we will execute
# Someday we could imply a hierarchy/tarball or something
if [ -n "$files" ]; then
    mkdir -p $tmpdir/cpp
    for i in ${files[@]}; do
      cp $i $tmpdir/cpp
    done
fi
# Create the script that runs the proxy in the OpenCPI installation
(echo '#!/bin/sh --noprofile';
 echo 'OCPI_BOOTSTRAP=$OCPI_CDK_DIR/scripts/ocpibootstrap.sh && source $OCPI_BOOTSTRAP';
 echo 'OCPI_RH_APP='$app' exec $OCPI_CDK_DIR/bin/$OCPI_TOOL_DIR/ocpirh_proxy $*') > $tmpdir/$app
chmod 755 $tmpdir/$app
pkgname=${pkg:+${pkg}.}$app
cat <<EOF > $tmpdir/$pkgname.spec
%{!?_sdrroot: %global _sdrroot /var/redhawk/sdr}
%define _prefix %{_sdrroot}
Prefix:         %{_prefix}
Name:           $pkgname
Version:        1.0.0
Release:        1%{?dist}
Summary:        Component %{name} from OpenCPI
License:        None
Requires:       redhawk >= 2.0

%description
Component %{name} from OpenCPI
 * Commit: __REVISION__
 * Creation Date/Time: __DATETIME__

%install
%define _mydir \$RPM_BUILD_ROOT%{_prefix}/dom/components/$pkg${pkg:+/}$app
mkdir -p %{_mydir}/cpp %{_mydir}/artifacts %{_mydir}/data
for i in $files; do
  if [ -d \$i ]; then
     cp -r \$i %{_mydir}/data
  else
     cp \$i %{_mydir}/data
  fi
done
# The generated SCA XML files
cp $tmpdir/*.xml %{_mydir}
# The generated app-executing script
cp $tmpdir/$app %{_mydir}/cpp
# The original app XML file
cp $appfile %{_mydir}
cp -r $tmpdir/artifacts %{_mydir}
%files
%defattr(-,redhawk,redhawk,-)
%define _installeddir %{_prefix}/dom/components/$pkg${pkg:+/}$app
%{_installeddir}
%post
# Make sure this temporary directory exists and is writable by all
mkdir -p %{_installeddir}/runs
chown redhawk %{_installeddir}/runs
chgrp redhawk %{_installeddir}/runs
chmod 777 %{_installeddir}/runs
%preun
[ \$1 = 0 ] && rm -r -f %{_installeddir}/runs
EOF
#rpmbuild -vv -bi --short-circuit -bb
# Run rpmbuild without really building, with the "build" directory to be the CWD here
# where we are being executed, and all the other files and directories it insists on creating
# in our temporary directory.
echo Creating RPM file for delivering OpenCPI application \"$appfile\" to run on RedHawk.
rpmbuild ${verbose:=--quiet} -bb \
	 --define "_topdir $tmpdir/rpmbuild" \
	 --define "_builddir `pwd`" \
	 $tmpdir/$pkgname.spec
rpm=$tmpdir/rpmbuild/RPMS/*/*.rpm
[ -f $rpm ] || {
  echo No RPM file was created.
  exit 1
}
mkdir -p $directory
cp $rpm $directory
echo The resulting RPM is $(basename $rpm), now in $directory/$(basename $rpm)
[ -z "$keep" ] && rm -r -f $tmpdir
[ -n "$install" ] && {
  cmd=(sudo yum -y install $directory/$(basename $rpm))
  echo Installing the resulting RPM using the command: ${cmd[@]}
  eval ${cmd[@]}
}

# Disable the built-in post-processing of files, namely compressing man pages and stripping
# %global __os_install_post %{nil}
Name:           opencpi-cdk
Version:        1.0
Release:        1%{?dist}
Summary:        The test RPM that is basically a tarball of the exports tree
License:        LGPLv3+
Prefix:         /opt/opencpi

%description 
test

%install
cd $OCPI_CDK_DIR
# exclude platforms that are built but not mentioned
for p in $(ls -d1 */bin | cut -d/ -f1); do
  grep -s " $p " <<< " $OCPI_RCC_PLATFORMS " ||
    find_excludes+=" -a ! -path ./$p/\*" cp_excludes+="|^$p\$"
done    
# Create the list of files for the %files section below, and copy the same files to BUILD_ROOT
eval find -L . -type f $find_excludes | sed "s=^\./=%%{prefix}/=" > $RPM_BUILD_DIR/files
rm -r -f $RPM_BUILD_ROOT/%{prefix} # in case there's junk from previous runs - needed?
mkdir -p $RPM_BUILD_ROOT/%{prefix}
cp -R -L $(ls -d1 *| egrep -v /$cp_excludes) $RPM_BUILD_ROOT/%{prefix}

%files -f files

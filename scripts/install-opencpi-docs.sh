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

################################################################################
# This script installs packages required to build OpenCPI documentation and then
# builds the documents in the framework and builtin projects, both tex and odt.
# It does not depend on the framework or projects being built, although it usually
# runs after install-opencpi.sh
# It currently is only supported on centos7

# Just check if it looks like we are in the source tree.
[ -d runtime -a -d build -a -d scripts -a -d tools ] || {
  echo "Error:  this script ($0) is not being run from the top level of the OpenCPI source tree."
  exit 1
}
set -e
# We do some bootstrapping here (that is also done in the scripts we call), in order to know
# whether the platform we are building

# Ensure exports (or cdk) exists and has scripts
source ./scripts/init-opencpi.sh
# Ensure CDK and TOOL variables
OCPI_BOOTSTRAP=`pwd`/cdk/scripts/ocpibootstrap.sh; source $OCPI_BOOTSTRAP
platform=$1
[ -n "$1" ] && shift
set -e
sanity=--setopt=skip_missing_names_on_install=False
echo Installing all the standard packages required to build OpenCPI documentation using "sudo yum install"...
sudo yum install -y $sanity texlive-latex rubber ghostscript
sudo yum install -y $sanity texlive-latex-bin texlive-texconfig-bin texlive-metafont-bin texlive-cm texlive-pdftex-def texlive-ifluatex texlive-zapfding texlive-helvetic texlive-times texlive-symbol texlive-titlesec texlive-multirow texlive-dvips texlive-fancyhdr texlive-collection-fontsrecommended texlive-microtype texlive-rotating texlive-placeins texlive-appendix
echo Building/creating OpenCPI documentation in the doc/pdfs directory.
make doc
echo PDFs and the index.html have been created in the doc/pdfs directory.
echo To see them all, open the file docs/pdfs/index.html in a browser.



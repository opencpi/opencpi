# This file is "sourced" so no shell call here.

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

# User variables that affect things:
# CROSS_FAIL_OK will allow cross-compilations to fail without aborting; this is useful
#   if you only have one cross-compiler.
# DOWNLOAD_LOCK will only allow URLs with the given pattern to be downloaded
#   (uses "expr" to evaluate, e.g. '.*\.example\.com')
# NO_DOWNLOADS will cause builds to fail if there already isn't a downloaded copy of
#   source when needed.
# PREREQ_TARGET might be set to "host" or a specific platform (AV-3853)

# For future:
# export DOWNLOAD_LOCK='.*\.bia-boeing\.com'

# Global init stuff:
if [ -z "$OCPI_TARGET_HOST" ]; then
    if [ -f /etc/redhat-release ]; then
        temp=$(cat /etc/redhat-release)
        temp2=$(sed -e 's/\([0-9]\)\..*/\1/' /etc/redhat-release)
        v2=$(sed -e 's/^[^0-9]*\([0-9]\+\)\..*$/\1/' /etc/redhat-release)
        v3=$(echo $v2 | sed 's/\ (Final)//')
        v1=$(echo "$temp" | sed 's/^\(.\).*/\1/' | tr A-Z a-z)
        export OCPI_TARGET_HOST="linux-${v1}${v3}-x86_64"
    else
        echo "Could not determine OCPI_TARGET_HOST!"
        exit 99
    fi
fi
if [ -z "$OCPI_PREREQUISITES_INSTALL_DIR" ]; then
  export OCPI_PREREQUISITES_INSTALL_DIR=/opt/opencpi/prerequisites/
fi

skip_host() {
  # No parameters; will return TRUE if caller should skip building for host
  if [ -z "${PREREQ_TARGET}" -o "x${PREREQ_TARGET}" == "xhost" ]; then
    return 1
  fi
  return 0
}

cross_build() {
  # Parameters:
  # 1: platform, e.g. arm-xilinx-linux-gnueabi
  # 2: target host, e.g. linux-x13_3-arm
  # 3: software platform, e.g. x13_3
  # 4: RPM platform nice name, e.g. zynq
  # 5: CFLAGS, e.g. "-O2 -g -pipe -Wall"
  #    For the OCPI_CFLAGS, start with the ones in /usr/lib/rpm/redhat/macros for %__global_cflags and then take out
  #    the ones that fail on the target platform.
  #    Then add the ones that tune for the platform.
  #    It goes through an "echo" to evaluate if things like ${CROSS_DIR} were passed in.
  # 6-9: Any extra rpmbuild options, e.g. "--define=OCPI_AD9361_COMMIT_SHORT ${OCPI_AD9361_COMMIT_SHORT}"
  [ -z "$5" ] && echo "All parameters not given to cross_build function (internal error)!" && exit 99 || :
  if [ -n "${PREREQ_TARGET}" ]; then
    if [ x"${PREREQ_TARGET}" != x"$4" ]; then
      echo "Skipping cross_build for $4 because ${PREREQ_TARGET} explicitly requested"
      return
    fi
  fi
  platform=$1
  echo "======================================================================="
  echo "Cross-compiling for ${platform} ($4)"
  # eval echo "\${$(eval echo CROSS_DIR_${platform})}" too ugly; copypasta nightmare instead
  CROSS_DIR_EXPLICIT=$(eval echo "\${$(eval echo CROSS_DIR_${4})}")
  unset CROSS_DIR
  if [ -n "${CROSS_DIR_EXPLICIT}" ]; then
    echo "Using explicitly set path for CROSS_DIR for $4: ${CROSS_DIR_EXPLICIT}"
    CROSS_DIR=${CROSS_DIR_EXPLICIT}
  else
    # First try one that includes x86 in the path, then fall back to first found:
    CROSS_DIR="$(locate ${platform}-ranlib | grep x86 | head -1 | xargs -r dirname)"
    if [ -z "${CROSS_DIR}" ]; then
      CROSS_DIR="$(locate ${platform}-ranlib | head -1 | xargs -r dirname)"
    fi
    echo "Automatically using cross-compiler from ${CROSS_DIR}"
  fi
  if [ ! -x ${CROSS_DIR}/${platform}-gcc ]; then
    echo Could not figure out where compilers live for ${platform}! Try setting the \"CROSS_DIR_${4}\" environmental variable.
    [ -n "${CROSS_DIR}" ] && echo "I tried: ${CROSS_DIR}/${platform}-gcc"
    if [ -n "${CROSS_FAIL_OK}" ]; then
      echo "CROSS_FAIL_OK is set; ignoring failure"
      return
    fi
    echo "Set CROSS_FAIL_OK if this is acceptable."
    exit 99
  fi
  rm -rf ~/rpmbuild/BUILD/ # Must be done for each platform
  rpmbuild -ba ${SPECFILE} \
    --define="OCPI_TARGET_HOST ${2}" \
    --define="OCPI_SWPLATFORM ${3}" \
    --define="OCPI_BUNDLED_VERSION ${OCPI_BUNDLED_VERSION}" \
    --define="OCPI_CROSS_PREFIX ${platform}" \
    --define="OCPI_CROSS_DIR ${CROSS_DIR}" \
    --define="OCPI_PLATFORM ${4}" \
    --define="OCPI_CFLAGS $(eval echo ${5})" ${6:+"${6}"} ${7:+"${7}"} ${8:+"${8}"} ${9:+"${9}"}
}

verify_safe_URL() {
  # Parameters:
  # 1: URL to check
  if [ -z "${DOWNLOAD_LOCK}" ]; then return; fi
  echo "Validating URL: ${1}"
  if [ "$(expr match "${1}" "${DOWNLOAD_LOCK}")" -gt 0 ]; then
    return
  fi
  echo "URL \"${1}\" does not match allowed DOWNLOAD_LOCK of \"${DOWNLOAD_LOCK}\""
  exit 99
}

prereq_init_git() {
  # Parameters:
  # 1: subdirectory to [re]use
  # 2: git checkout URL
  # 3: (optional) git branch
  SUBDIR=${1}
  if [ ! -d ${SUBDIR} ]; then
    if [ -n "${NO_DOWNLOADS}" ]; then
      echo "NO_DOWNLOADS set and no local repo for ${SUBDIR}!"
      false
    fi
    verify_safe_URL ${2}
    echo "Cloning the repo ${2}"
    git clone ${2} ${SUBDIR}
    if [ -n "${3}" ]; then
      pushd ${SUBDIR} >/dev/null 2>&1
      git checkout ${3} >/dev/null 2>&1 || echo "Could not checkout: " && git checkout ${3}
      popd >/dev/null 2>&1
    fi
  else
    echo "Re-using existing git repo (${SUBDIR}) - if it is corrupted, please erase it."
    pushd ${SUBDIR} >/dev/null 2>&1
    if [ -n "${3}" ]; then
      git checkout ${3} >/dev/null 2>&1 || echo "Could not checkout: " && git checkout ${3}
    fi
    git reset --hard
    git clean -fdx
    git status
    popd >/dev/null 2>&1
  fi
}

prereq_init_sharedir() {
  # Parameters:
  # 1: tarball to [re]use
  # 2: shared directory source (including filename)
  TARBALL=${1}
  if [ ! -e ${TARBALL} ]; then
    echo "Copying the distribution file:"
    cp -v ${2} ./${TARBALL}
  else
    echo Re-using ${TARBALL} - if it is corrupted, please erase it.
  fi
}

prereq_init_tarball() {
  # Parameters:
  # 1: tarball to [re]use
  # 2: source URL (including filename)
  TARBALL=${1}
  if [ ! -e ${TARBALL} ]; then
    if [ -n "${NO_DOWNLOADS}" ]; then
      echo "NO_DOWNLOADS set and no local copy of ${TARBALL}!"
      false
    fi
    verify_safe_URL ${2}
    echo Downloading the distribution file: ${TARBALL}
    curl -O -L ${2}
  else
    echo Re-using ${TARBALL} - if it is corrupted, please erase it.
  fi
}

#!/bin/bash
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


# This is expected to be called by Jenkins either within Docker or standalone to build a unit test.
# Sometimes it is run (when building) while sometimes sourced (when running tests to get the first half)

# Expected variables:
# HDL_PLATFORM
# TEST_PATH

set -e
# set +x
shopt -s nullglob

if [ -z "${JENKINS_HOME}" ]; then
  echo "Do not run this script directly - it is for Jenkins only."
  exit 1
fi

. ../jenv.sh
export VERBOSE="-v"

if [ -d /opt/Xilinx ]; then
  XIL_VER=$(ls -1 /opt/Xilinx/ | sort | grep -E "^[0-9]+\.[0-9]+\$" | tail -1)
fi

for d in /opt/{A,a}ltera /home/jek/altera ; do
  if [ -d $d ]; then
    ALT_HOME=$d
    ALT_VER=$(ls -1 ${ALT_HOME} | sort | grep -E "^[0-9]+\.[0-9]+" | tail -1)
  fi
done

# Continue to set up env
export OCPI_PROJECT_REGISTRY_DIR=$(pwd)/ocpi_registry
export OCPI_ALTERA_DIR=${ALT_HOME}
export OCPI_ALTERA_VERSION=${ALT_VER}
export OCPI_MODELSIM_DIR=/opt/Modelsim/modelsim_dlx/
export OCPI_XILINX_VERSION=${XIL_VER}
export XILINXD_LICENSE_FILE=/opt/Xilinx/${XIL_VER}/WebPACK.lic
export OCPI_SUPPRESS_HDL_NETWORK_DISCOVERY=1
# Still needed? export CONTAINERS=xxx

echo "For debug, here is all known OCPI variables:"
env | egrep '^OCPI' | sort

# This script might be called multiple times if building all tests in same workspace (AV-4529)
rm -rf ${OCPI_PROJECT_REGISTRY_DIR}
ocpidev create registry ocpi_registry

# Find every project that is built and register them
for found_proj in $(compgen -o dirnames | grep -v releng | grep -v ocpi_registry); do
  echo "Processing project: ${found_proj}"
  # Just in case, erase any old imports from previous jobs:
  ocpidev -d ${found_proj} unset registry
  ocpidev -d ${found_proj} register project
done

# Running the tests can share this code up to this point
if [ -z "${SKIP_BUILD}" ]; then
  # For licensing issues, etc, give it up to ten attempts
  /bin/bash -c "
  set +e
  set -o pipefail
  for i in \$(seq 1 10); do
    date
    if [ 10 == \$i ]; then set -e; fi
    echo ====================== Jenkins Iteration \$i ====================== | tee -a run.log
    ocpidev ${VERBOSE} -d ${TEST_PATH} build --rcc-hdl-platform ${HDL_PLATFORM} --hdl-platform ${HDL_PLATFORM} 2>&1 | tee -a run.log
    RES=\$?
    echo ================= End Jenkins Iteration \$i : \${RES} ================= | tee -a run.log
    if [ 0 == \$RES ]; then echo \"Jenkins Iteration \$i: Success\" | tee -a run.log; exit 0; fi
    LAST_FILE=\$(find . -name \"*.out\" -printf \"%T@ %p\n\" | sort -n | cut -f2 -d' ' | tail -1 || :)
    if [ -n \"\${LAST_FILE}\" ]; then LAST_GREP=\$(grep -i \"Unable to checkout msimpevsim license\" \${LAST_FILE} || :); fi
    if [ -n \"\${LAST_GREP}\" ]; then
      echo Modelsim could not get license. Sleeping for \$i | tee -a run.log
      sleep \$i
    fi
  done
  date
  "
fi

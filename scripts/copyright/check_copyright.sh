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

# This script will check compared to dedicated white list

# Let it be run from the top or in scripts/copyright/
if [ '.' == "$(dirname $0)" ]; then # in scripts/copyright/
  cd ../..
fi

if [ ! -d scripts/copyright ]; then
  echo "Cannot determine location. Run from root of checkout or scripts/copyright/."
  exit 99
fi

function cleanup() {
  rm -rf copyright.log missing.log missing_diff.log
}

trap cleanup EXIT

OCPI_SCAN_COPYRIGHTS=x ./scripts/copyright/copyright.py
grep ":ERROR:" copyright.log | cut -f5 -d: | sort > missing.log
# Put a single space before each whitelist item to match the diff output.
# That space is needed or the later greps will see "---" as an issue.
diff -u <( sed -e 's/^/ /' scripts/copyright/whitelist) missing.log > missing_diff.log
RET=$?

if [ "${RET}" == "1" ]; then
  if egrep -q '^\+ ' missing_diff.log; then
    echo ""
    echo "The following file(s) need to have their copyright clause added"
    echo "or put into the whitelist. Run scripts/copyright/copyright.py"
    echo "on each to add copyright blurb:"
    egrep '^\+ ' missing_diff.log | cut -c3-
  fi
  if egrep -q '^\- ' missing_diff.log; then
    echo ""
    echo "The following file(s) were listed as exemptions but no longer exist:"
    egrep '^\- ' missing_diff.log | cut -c3-
  fi
  exit 10 # Arbitrary non-0/1 value for Jenkins to decide we are unstable
fi

echo "All expected files have copyright block set."

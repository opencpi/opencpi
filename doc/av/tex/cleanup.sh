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

# This will cleanup LaTeX source by fixing "smartquotes" when pasted from word
# processors, remove trailing whitespace, etc.

# For use in assets:
# for d in $(find -type d -regex '.*docs?'); do (cd $d; ..../cleanup.sh); done

set -e

replace() {
  for f in $(git grep -Il $1 *.tex); do
    echo $f: Fixing $1=\>$2
    perl -ap -i.bak -e "s/$1/$2/g" $f
  done
}

# Fix "smartquotes"
replace '“' '``'
replace '”' "''"
replace "’" "'"

# Remove trailing whitespace
for f in $(git grep -Il --perl-regexp "\s+$" *.tex); do
  echo $f: Removing trailing whitespace
  perl -ap -i.bak -e 's/\s+$/\n/g' $f
done

# Other cleanup
echo "If you were to run 'git clean -fx':"
git clean -nx

# Bad closing quotes:
echo "Mismatched quotes:"
git grep -I  '``.*?"'

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


# Note: The job "Maintenance/Branch_Check_Configure" needs to be re-run whenever this file is changed!
declare -A merged
declare -A ignores
declare -A authors
declare -A authors_delete

ignores[master]=1
ignores[develop]=1
ignores[develop_oss_merge]=1
ignores[open_source_pristine]=1

# This file has a bunch of aliases defined like
# aliases["billy.boy"]="Billy Boy"
# aliases["bboy"]="Billy Boy"
declare -A aliases
if [ -f ../../../config_files/git_aliases.sh ]; then
  source ../../../config_files/git_aliases.sh
  # printf '%s\n' "${aliases[@]}"
fi

# Jenkins does this automatically:
if [ -z "${JENKINS_HOME}" ]; then
  echo "Pulling and pruning..."
  git pull --prune || exit 1
fi

# Get merged list for "develop"
for branch in $(git branch -r --merged origin/develop | egrep '^\s*origin/' | cut -f2- -d/ | grep -v -- '->'); do
  merged[${branch}]=1
done

# Get list of remote branches
for branch in $(git branch -r | egrep '^\s*origin/' | cut -f2- -d/ | egrep -v '^v[1-9]' | grep -v -- '->' | sort); do
  # git log -1 --pretty="format:Checking branch ${branch}: %an %h%d (%ar)%n" "origin/${branch}"
  author=$(git log -1 --pretty="format:%an" "origin/${branch}")
  [ -n "${aliases[${author}]}" ] && author="${aliases[${author}]}"
  if [ -n "${merged[${branch}]}" -a -z "${ignores[${branch}]}" ]; then
    # echo "** Branch seems to be merged **"
    authors_delete["${author}"]="${authors_delete["${author}"]} ${branch}"
    # Need to initalize the author in case this is their only branch AV-4808
    authors["${author}"]="${authors["${author}"]}"
  else
    authors["${author}"]="${authors["${author}"]} ${branch}"
  fi
done

# unset authors_delete; declare -A authors_delete # for debugging return values, etc

{ # Begin tee redirect ended below
  # echo "Author report:"
  for author in "${!authors[@]}"; do
    echo "${author}:"
    for branch in ${authors_delete["${author}"]}; do
      git log -1 --pretty="format:   ${branch}: %ar (merged)%n" "origin/${branch}"
    done
    for branch in ${authors["${author}"]}; do
      git log -1 --pretty="format:   ${branch}: %ar%n" "origin/${branch}"
    done
  done

  if (( ${#authors_delete[@]} )); then
    printf "\n===\n\n"
    echo "Branches ready to delete:"
    for author in "${!authors[@]}"; do
      if [ -n "${authors_delete[${author}]}" ]; then
        echo "${author}:"
      fi
      for branch in ${authors_delete["${author}"]}; do
        git log -1 --pretty="format:   ${branch} (%ar)%n" "origin/${branch}"
      done
    done
  fi
} | tee branch_report.log

(( ${#authors_delete[@]} )) && exit 2 # If anything reported, return 2 for Jenkins "unstable"
exit 0 # Script success if this far (otherwise would return 1 because above line FAILED)

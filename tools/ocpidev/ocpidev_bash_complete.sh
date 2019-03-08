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

_ocpidev()
{
    local cur prev verb
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    oneopts="-v -f -s -p -t -n -u -H -J -X -x"
    i=$((1))
    # this first section is deciding to bash complete on the current word some options take a unique
    # word after some don't.  this code is dealing with that difference and selecting which nouns and
    # verbs go where.
    while [[ ${COMP_WORDS[i]} == -* ]]
    do
      if [[ $oneopts != *"${COMP_WORDS[i]}"* ]] ; then
        i=$(( $i + 1 ))
      fi
      i=$(( $i + 1 ))
    done
    verb="${COMP_WORDS[i]}"
    i=$(( $i + 1 ))
    while [[ ${COMP_WORDS[i]} == -* ]]
    do
      if [[ $oneopts != *"${COMP_WORDS[i]}"* ]] ; then
        i=$(( $i + 1 ))
      fi
      i=$(( $i + 1 ))
    done
    noun1="${COMP_WORDS[i]}"
    i=$(( $i + 1 ))
    while [[ ${COMP_WORDS[i]} == -* ]]
    do
      if [[ $oneopts != *"${COMP_WORDS[i]}"* ]] ; then
        i=$(( $i + 1 ))
      fi
      i=$(( $i + 1 ))
    done
    noun2="${COMP_WORDS[i]}"
    i=$(( $i + 1 ))
    while [[ ${COMP_WORDS[i]} == -* ]]
    do
      if [[ $oneopts != *"${COMP_WORDS[i]}"* ]] ; then
        i=$(( $i + 1 ))
      fi
      i=$(( $i + 1 ))
    done
    noun3="${COMP_WORDS[i]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    # valid verbs
    verbs="build delete create clean register unregister set unset show refresh run utilization"
    # valid nouns in first slot
    nouns1="project registry application component protocol test library worker hdl"
    # valid nouns in second slot
    nouns2="assembly card slot device platform platforms targets primitive primitives"
    # valid nouns in third slot
    nouns3="library core"
    #valid build options
    bopts="--help -v -d -l -P --rcc --hdl --no-assemblies --hdl-assembly --hdl-target \
           --hdl-platform --rcc-platform --rcc-hdl-platform --worker --clean-all --hdl-library"
    utilopts="--help -v -d --library --hdl-library -P --hdl-target --hdl-platform --format=latex --format=table"
    # show verb has different options and nouns
    showopts="-h --help -v -d --table --json --local-scope --global-scope --simple"
    runopts="-h --help -v --verbose --keep-simulations --accumulate-errors -G --only-platform -Q \
	--exclude-platform --rcc-platform --hdl-platform -d -l --case --before --after --run-args \
	--mode --remotes --view"
    copts="--help -v -f -d -s -p -t -n -l -F -D -K -N -S -P -L -V -E -W -R -r -g -q -u -I -A -O -C -Y -y \
	-T -Z -G -Q -U -M -B -H -J -X -x --version"
    # for project noun, add the --register option
    if [[ ${noun1} == project ]]; then
      copts="${copts} --register"
    fi
    if [[ ${verb} == create  || ${verb} == delete ]]; then
      # valid nouns in first slot
      nouns1="project registry application component protocol test library worker hdl"
      # valid nouns in second slot
      nouns2="assembly card slot device platform primitive"
      # valid nouns in third slot
      nouns3="library core"
    elif [[ ${verb} == build  || ${verb} == clean ]]; then
      # valid nouns in first slot
      nouns1="project application test library worker hdl"
      # valid nouns in second slot
      nouns2="assembly device platform platforms primitives primitive"
      # valid nouns in third slot
      nouns3="library core"
    elif [[ ${verb} == *register  || ${verb} == refresh ]]; then
      # valid nouns in first slot
      nouns1="project"
      nouns2=""
      nouns3=""
    elif [[ ${verb} == *set ]]; then
      # valid nouns in first slot
      nouns1="registry"
      nouns2=""
      nouns3=""
    elif [[ ${verb} == show ]] ; then
      # valid nouns in first slot
      nouns1="projects registry components workers platforms targets hdl rcc project libraries \
             tests component"
      # valid nouns in second slot
      nouns2="platforms targets"
      # valid nouns in third slot
      nouns3=""
    elif [[ ${verb} == run ]] ; then
      # valid nouns in first slot
      nouns1="test tests library application applications project"
      # valid nouns in second slot
      nouns2=""
      # valid nouns in third slot
      nouns3=""
    elif [[ ${verb} == utilization ]]; then
      # valid nouns in first slot
      nouns1="project library worker workers hdl"
      # valid nouns in second slot
      nouns2="platform platforms assembly assemblies"
      # valid nouns in third slot
      nouns3=""
    fi
    # if current word starts with a - it is an option of some sort
    if [[ ${prev} == --hdl-platform || ${prev} == --build-hdl-platform ]] ; then
      plats=`ocpidev show hdl platforms --simple 2>/dev/null`
      COMPREPLY=( $(compgen -W "${plats}" -- ${cur}) )
      return 0
    elif [[ ${prev} == --hdl-target || ${prev} == --build-hdl-target ]] ; then
      tgts=`ocpidev show hdl targets --simple 2>/dev/null`
      COMPREPLY=( $(compgen -W "${tgts}" -- ${cur}) )
      return 0
    # Not supported yet:
    #elif [[ ${prev} == --rcc-target || ${prev} == --build-rcc-target ]] ; then
    #  tgts=`ocpidev show rcc targets --simple`
    #  COMPREPLY=( $(compgen -W "${tgts}" -- ${cur}) )
    #  return 0
    elif [[ ${prev} == --rcc-platform || ${prev} == --build-rcc-platform ]] ; then
      plats=`ocpidev show rcc platforms --simple 2>/dev/null`
      COMPREPLY=( $(compgen -W "${plats}" -- ${cur}) )
      return 0
    elif [[ ${cur} == -* ]] ; then
      if [[ (${verb} == build || ${verb} == clean) && ${cur} == --* ]] ; then
        COMPREPLY=( $(compgen -W "${bopts}" -- ${cur}) )
        return 0
      elif [[ ${verb} == show ]] ; then
        COMPREPLY=( $(compgen -W "${showopts}" -- ${cur}) )
        return 0
      elif [[ ${verb} == run ]] ; then
        COMPREPLY=( $(compgen -W "${runopts}" -- ${cur}) )
        return 0
      elif [[ ${verb} == create || ${verb} == delete ]] ; then
        COMPREPLY=( $(compgen -W "${copts}" -- ${cur}) )
      elif [[ ${verb} == build || ${verb} == clean ]] ; then
        COMPREPLY=( $(compgen -W "${bopts}" -- ${cur}) )
      elif [[ ${verb} == utilization ]] ; then
        COMPREPLY=( $(compgen -W "${utilopts}" -- ${cur}) )
      else
        COMPREPLY=( $(compgen -W "${copts} ${runopts} ${showopts} ${bopts} ${utilopts}"  -- ${cur}) )
        return 0
      fi
    elif [[ ${verb} == unregister && ${prev} == project ]]; then
      projs=`ocpidev show registry --simple 2>/dev/null`
      COMPREPLY=( $(compgen -W "${projs}" -- ${cur}) )
      return 0
     elif [[ ${verb} == run && ${prev} == --mode ]]; then
       modes="gen_build prep_run_verify clean_all gen prep run prep_run verify view clean_run \
	clean_sim all"
      COMPREPLY=( $(compgen -W "${modes}" -- ${cur}) )
      return 0
    # if current word is in the verb slot
    elif [[ ${verb} == ${cur} ]] ; then
      COMPREPLY=( $(compgen -W "${verbs}" ${cur}) )
      # AV-3871 added this to do completion for words that only have one option
      # If COMPREPLY is size 1 we know what word is to be completed
      if [ ${#COMPREPLY[@]} -eq 1 ] ; then
        if [[ ${COMPREPLY} == register || ${COMPREPLY} == unregister ]] ; then
          COMPREPLY="${COMPREPLY} project"
        elif [[ ${COMPREPLY} == set || ${COMPREPLY} == unset ]] ; then
          COMPREPLY="${COMPREPLY} registry"
        fi
      fi
      return 0
    # if current word is in the first noun slot
    elif [[ ${noun1} == ${cur} ]] ; then
      COMPREPLY=( $(compgen -W "${nouns1}" ${cur}) )
      return 0
    # if current word is in the second noun slot
    elif [[ ${noun1} == hdl && ${noun2} == ${cur} ]] || [[ ${noun1} == rcc && ${noun2} == ${cur} && ${verb} == show ]] ; then
      COMPREPLY=( $(compgen -W "${nouns2}" ${cur}) )
      return 0
    # if current word is in the third noun slot
    elif [[ ${noun2} == primitive && ${noun3} == ${cur} ]] ; then
      COMPREPLY=( $(compgen -W "${nouns3}" ${cur}) )
      return 0
    fi
}
complete -o default -F _ocpidev ocpidev

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

##########################################################################################
# This script generates the Makefile.am by processing input descriptions of "places" in the
# code tree.  Places are code sub-trees that are built resulting in 0 or 1 libraries and 0 or
# more programs.  Places have attributes.

# See the "../places" file for its syntax and rules.

# So this script takes that "build-system-agnostic" description file, as standard input,
# one line per source directory, and generates a Makefile.am file, on standard output,
# that implements the required build rules using libtool and automake

# Libraries are of 5 types, and they are each handled differently.
# How each type of library gets built depends on whether we are in static or dynamic mode.

# normal runtime: static mode:  build 2 ways: static and static+pic
#                 dynamic mode:  build 2 ways, static+pic, and dynamic
# plugin/driver:  always build only for dynamic library
# stub:           always build only for dynamic library
# swig:           static mode:  build a dynamic lib using static+pic versions of the normal libs
#                 dynamic mode: build a dynamic lib using dynamic versions of the normal libs
# tool:           static mode: static
#                 dynamic mode: dynamic

# Executables are of 3 types:

# Normal executables that end up in "<platform>/bin
# Executables that end up in a subdirectory of "<platform>/bin"
# Not really used, or only usable deep inside this build tree

# Header files from earlier places are made available to later places.
# Tool libraries only access runtime libraries up to the end-of-runtime-tools marker in "places".

# Boilerplate contents of Makefile.am
cat <<'EOF'
# DESTDIR must be absolute even though we would prefer it not to be.
DESTDIR=$(CURDIR)
prefix=/staging
bindir=${exec_prefix}/bin
libdir=${exec_prefix}/lib
ocpi_build_dir = .
# automake insists on these initializations even though bash and make do not
noinst_LTLIBRARIES=
lib_LTLIBRARIES=
# These variables must be predeclared like this
#bin_PROGRAMS=
#internal_PROGRAMS=
python_PYTHON=
# Cause swig python outputs to be in the same place as the shared libraries so they
# both can be trivially used using PYTHONPATH
pythondir=$(libdir)
# It doesn't appear that autoconf macros actually gives us the proper runtime prefix
PYTHON_INCLUDES=$(or @OcpiPythonInclude@,$(shell echo `@PYTHON@ -c "import sys;print(sys.prefix)"`/include/python@PYTHON_VERSION@*))
# Avoid automake limitations by defining a variable used later
ocpi_extra_libs=$(patsubst %,-l%,@OcpiExtraLibs@)

# This hook does things that libtool and automake cannot do for us.  See comments in the script.
# We use the install-data-hook because, if you can believe it, binaries in subdirectories
# are installed in the data phase. WTF?
install-data-hook:
	$(AM_V_at)cd staging; V=$V ../../install-hook.sh \
	     @OcpiDynamicLibrarySuffix@  @ocpi_dynamic@ @OcpiPlatform@  $(OcpiThisPlatform) \
	     "$(ocpi_drivers)" "$(ocpi_swigs)" "$(ocpi_prereqs)" "@prerequisite_dir@"
EOF

function bad {
    echo Error: $* >&2
    exit 1
}

# output the compilation flags, used for libraries and programs
# args are: 1. amname 2. extra C/CXX flags 3. type
# the "includes" variable is a global that is used here.
function do_flags {
    printf "${1}_CPPFLAGS = @common_cppflags@"
    [ -z "$foreign" -a "$3" != swig ] && printf " @strict_cppflags@"
    incs="$ocpi_incs_ordered $ocpi_prereq_incs"
    [ -n "$tools" ] && incs="$ocpi_incs_for_tools $ocpi_prereq_incs"
    [ "$3" = stubs ] && incs=
    for i in $includes $incs; do # includes is global
	printf ' \\\n  -I%s' $i
    done
    printf '\n'"${1}_CFLAGS = $2 @common_cflags@"
    [ -z "$foreign" -a "$3" != swig ] && printf " @strict_cflags@"
    printf '\n'"${1}_CXXFLAGS = $2 @common_cxxflags@"
    [ -z "$foreign" -a "$3" != swig ] && printf " @strict_cxxflags@"
    echo
}

# This function takes a list of libraries that the (library or executable) depends on, and puts
# them out as dependencies for automake (on the right side of the assignment)
# It deals with them differently depending on how they are named and whether it is a library or
# an executable that is depending on them.
# args are 1. automake-lib-or-pgm-varname 2. suffix 3: name_suffix  4:+ libs-we-are-depending-on
# name_suffix is before the extension (like _s or _d)
function print_lib_names {
  printf "$1 ="; shift; suff=$1; shift; namesuffix=$1; shift
  for s in $*; do
    case $s in
      /*) if [ $type = program ]; then
            s="$s$suff"
	  else
	    l=$(basename $s); s="-L$(dirname $s) -l${l//lib}"
	  fi;;
      -l*|\$*|-L*);;
      *_stubs|*_d|*_s) s='$(ocpi_build_dir)/'$s.la;;
      *.*|*@) s='$(ocpi_build_dir)/'$s;;
      *)   s='$(ocpi_build_dir)/'$s$namesuffix.la;;
    esac
    printf ' \\\n  '"$s"
  done
  printf ' \\\n  $(ocpi_extra_libs)\n'

}
# Print libraries to depend on:
# for programs, static: use pathnames to .a, dynamic: use -L and -l
# for libraries, static: use nothing, dynamic: use -L and -l
# Args are: 1. type 2. suffix for prereqs for programs 3. libvarname 4:+ libraries
function print_libs {
  lib=$1; shift; type=$1; shift; suff=$1; shift
  [ -n "$verbose" ] && echo printing libraries for type:$type suffix:$suff libs:$* >&2
  case $type in
   normal|driver)
    print_lib_names $lib $suff "" $*;;
   program)
     echo if ocpi_is_dynamic
     print_lib_names $lib $suff "" $*
     echo else
     print_lib_names $lib $suff _s $*
     echo endif;;
   swig)
     echo if ocpi_is_dynamic
     print_lib_names $lib $suff "" $* $dynamic_prereqs
     echo else
     print_lib_names $lib $suff _d $* $static_prereqs
     echo endif;;
  esac
}

# create a library
# args are 1:<type> 2:<name> 3:<sources> 4:<extra c/cxxflags> 5:<ldflags> 6:<ldadd> 7:<dest>
function do_library {
  [ -n "$verbose" ] && echo for $1 library $2 sources are \"$3\"  >&2
  printf "# for $1 library $2\n"
  if [ "$1" = swig ]; then
      amname=__ocpi_build_dir___${2}_la
  else
      amname=__ocpi_build_dir__libocpi_${2}_la
  fi
  do_flags $amname "$4" $1 
  printf "${amname}_LDFLAGS = $5\n"
  printf "${amname}_SOURCES ="
  for s in $3; do printf ' \\\n  '$s; done
  echo
  # libraries link against other libraries that are dynamic
  # are resolved, without pulling in code from the other libraries
  if [ -n "$6" ]; then
    [ $1 = driver ] && printf "if ocpi_can_remove_needed\n"
    if [ "$1" != stubs -a $1 != static -a $1 != static-pic ]; then
      print_libs ${amname}_LIBADD $1 @OcpiDynamicLibrarySuffix@  $6
    fi
    [ $1 = driver ] && printf "endif\n"
  fi
  if [ "$1" = swig ]; then
    printf "${7}_LTLIBRARIES += \$(ocpi_build_dir)/_$2.la\n"
    printf "${amname}_CPPFLAGS+=-I\$(PYTHON_INCLUDES)\\n"
    printf "\$(ocpi_build_dir)/$3: $swig\\n"
    printf "\\t\$(AT)@OcpiSWIG@ -classic -c++ -python -outdir \$(@D) -o \$@ "
    printf "\$(${amname}_CPPFLAGS) \$<\\n"
    printf "python_PYTHON+=$(dirname $swig)/$2.py\\n"
  else
    printf "\n${7}_LTLIBRARIES += \$(ocpi_build_dir)/libocpi_$2.la\n"
  fi
}
function maybe_add_directory {
  if [[ "$directories" != *:$1* ]]; then
    directories="$directories:$1"
    echo ${1}_PROGRAMS=
    [ $1 = bin ] || echo ${1}dir = '$(bindir)/'$1
  fi
}

[ "$1" = -v ] && verbose=1

while read path opts; do
  case "$path" in
      \#*|"")
	  continue;;
      prerequisites)
	  for p in $opts; do
	      dir=@prerequisite_dir@/$p/@OcpiPlatform@
	      # look in arch-specific include first, then arch-agnostic
	      ocpi_prereq_incs+=" $dir/include"
	      ocpi_prereq_incs+=" @prerequisite_dir@/$p/include"
	      ocpi_prereq_libs+=" $dir/lib/lib$p"
	      #ocpi_prereq_ldflags+=" -Wl,-rpath -Wl,$dir/lib"
	      # LATER dynamic_prereqs+=" -L$dir/lib -locpi_$p"
	      dynamic_prereqs+=" -L$dir/lib -l$p"
	      static_prereqs+=" $dir/lib/lib${p}@OcpiStaticLibrarySuffix@"
	  done
	  echo ocpi_dynamic_prereqs=$dynamic_prereqs
	  echo if ocpi_is_dynamic
	  echo   ocpi_program_prereqs=$dynamic_prereqs
          echo else
	  echo   ocpi_program_prereqs=$static_prereqs
	  echo endif
          echo ocpi_prereqs=$opts
	  continue;;
      end-of-runtime-for-tools)
	  ocpi_libs_for_tools=$ocpi_libs_ordered
	  ocpi_incs_for_tools=$ocpi_incs_ordered
	  continue;;
  esac
  [ -n "$verbose" ] && echo for $path options are \"$opts\"  >&2
  directory= library=$(basename $path) dest=lib options=($opts) foreign= tools= driver= useobjs=
  exclude= includes= libs= xincludes=
  while [ -n "$options" ] ; do
      case $options in
	  -l) library=${options[1]}; unset options[1];;
	  -d) directory=${options[1]}; unset options[1];;
	  -n) dest=noinst;;
	  -f) foreign=1;;
	  -D) defs="$defs -D${options[1]}"; unset options[1];;
	  -t) tools=1;;
	  -v) driver=1;;
          -s) useobjs=1;;
	  -L) lib=${options[1]}; unset options[1]
	      case $lib in 
		  /*|@*|*/*);;
		  *) lib=libocpi_$lib;;
	      esac
	      libs+=" $lib";;
          -I) xincludes="$xincludes ${options[1]}"; unset options[1];;
          -x) exclude="$exclude -not -regex ${options[1]} -a"; unset options[1];;
	  -T) tops="$tops ${options[1]}"; unset options[1];;
          -r) ;; # not used here, only in exports
          -*) bad Invalid option: $options;;
          *)  bad Unexpected value in options: $options;;
      esac
      unset options[0]
      options=(${options[*]})
  done
  echo '################################################################################'
  echo '# For location: '$path
  includes=" $(find -H $path $exclude -type d -a -name include) $xincludes" 
  includes="$(for i in $includes; do echo @srcdir@/$i; done)"
  sources=$(find -H $path $exclude -not -name "*_main.c*" -a -not -name "*_[sS]tubs.c*" -a \
                         \( -name "[^.#]*.cxx" -o -name "[^.#]*.cc" -o -name "[^.#]*.c" \))
  stubs=$(find -H $path $exclude -name "[^.#]*_[sS]tubs.c" -o -name "[^.#]*_[sS]tubs.cxx")
  swig=$(find -H $path $exclude -path "*/src/[^.#]*.i")
  programs=$(find -H $path $exclude -name "[^.#]*_main.cxx")
  [ -n "$verbose" ] && echo for $path sources are \"$sources\"  >&2
  lname=${library//-/_}
  ldadd=$libs
  if [ -n "$tools" ]; then
      ldadd+=" $ocpi_libs_for_tools"
  else
      ldadd+=" $ocpi_libs_ordered"
  fi
  [ -n "$verbose" ] && echo For $path swigs are \"$swig\"  >&2
  [ -n "$stubs" ] && {
     [ -n "$verbose" ] && echo For $path stubs are \"$stubs\"  >&2
     # Have to force the prefix here to force libtool to NOT create a static library.
     # Note that stubs are not noinst since they are needed in the staging area when
     # the wonderful libtool "relink" happens.  We don't export them.
     # do_library stubs ${lname}_stubs "$stubs" "" "-rpath \$(prefix) \
     #           @libtool_dynamic_library_flags@ @OcpiDynamicLibraryFlags@" "" lib
     do_library stubs ${lname}_stubs "$stubs" "" \
                "@libtool_dynamic_library_flags@ @OcpiDynamicLibraryFlags@" "" lib
     ldadd+=' libocpi_'${lname}_stubs
  }
  [ -n "$sources" -a -z "$useobjs" ] && {
    if [ "$dest" = noinst ] ; then
      ldflags="@libtool_static_library_flags@"
      ltype=noinst
    elif [ -n "$driver" ]; then
      ldflags="@libtool_plugin_library_flags@ @OcpiPluginFlags@"
      ltype=driver
      drivers="$drivers $library"
    else
      ldflags="@libtool_dynamic_library_flags@ @ocpi_library_flags@"
      ltype=normal
    fi
    # 1. Make the STATIC NON-PIC library if noinst or normal library and static mode
    #    Even in dynamic mode we need the static library when locally linking executables.
    if [ $ltype = normal -o $ltype = noinst ]; then
      [ -z "$programs" ] && echo if "!ocpi_is_dynamic"
      do_library static ${lname}_s "$sources" "-prefer-non-pic" "@libtool_static_library_flags@\
                 @OcpiStaticLibraryFlags@" "" $dest
      [ -z "$programs" ] && echo endif
    fi
    # 2. Make the STATIC PIC library if normal
    #    This enables making "big dynamic libraries" like static swigs
    if [ $ltype = normal ]; then
      do_library static-pic ${lname}_d "$sources" "-prefer-pic" \
		 "@libtool_static_library_flags@ @OcpiStaticLibraryFlags@" "" $dest
    fi
    # 3. Make the DYNAMIC library if not noinst (type is driver or normal)
    #    If in static mode overall, we still use this for error checking
    if [ $ltype != noinst ]; then
      # Force the extension in case libtool messes with it, which happens on darwin
      do_library $ltype $lname "$sources" "-prefer-pic" "-shrext @OcpiDynamicLibrarySuffix@ \
                 $ldflags" "$ldadd \$(ocpi_dynamic_prereqs)" $dest
    fi
    # 4. Build a SWIG library one way or the other
    if [ -n "$swig" ]; then
	base=$(basename $swig .i)
	wrap="$(dirname $swig)/${base}_wrap.cxx"
	swigs="$swigs $base"
	# It appears that libtool ignores -export-dynamic
#	ldflags="-module -export-dynamic -shrext @OcpiDynamicLibrarySuffix@ 
	# Even on a mac, the swig suffix is always .so...
	ldflags="-module -export-dynamic -shrext .so \
                 @libtool_dynamic_library_flags@ @ocpi_swig_flags@"
	ldflags+=" -lpython@PYTHON_VERSION@ $ocpi_prereq_ldflags"
	ldadd="libocpi_${lname} $ldadd"
	echo "if !ocpi_is_cross"
#	echo "if ocpi_is_dynamic"
#	do_library "swig" $base $wrap "" "$ldflags" "$ldadd $dynamic_prereqs" $dest
#	echo else
	do_library "swig" $base $wrap "-prefer-pic" "$ldflags" "$ldadd" $dest
#		   "$(for l in $ldadd; do echo ${l}_d; done) $static_prereqs" $dest
#	echo endif
	echo endif
    fi
  }
  [ -n "$includes" -a  -z "$driver" ] && ocpi_incs_ordered="$includes $ocpi_incs_ordered"
  [ -n "$programs" ] && {
    [ -n "$verbose" ] && echo For $path programs are \"$programs\"  >&2
    [ -z "$directory" ] && directory=bin
    maybe_add_directory $directory
    ldflags="@libtool_program_flags@ @ocpi_program_flags@"
    [ -n "$sources" -a -z "$useobjs" ] && {
	# Use the local library statically if it is not getting installed.
	if [ $ltype = noinst ] ; then
	  ldadd="libocpi_${lname}_s $ldadd"
	else
	  ldadd="libocpi_${lname} $ldadd"
	fi
    }
    for p in $programs; do
      pname=$(basename ${p%%_main.*})
      dir=$directory
      for t in $tops; do
        if [ $t = $pname ]; then
	  maybe_add_directory bin
          dir=bin
          break
	fi    
      done
      echo "${dir}_PROGRAMS += \$(ocpi_build_dir)/$pname"
    done
    echo
    for p in $programs; do
      pname=$(basename ${p%%_main.*} | tr \- _)
      printf "__ocpi_build_dir__${pname}_SOURCES = $p"
      if [ -n "$useobjs" ]; then
	  for s in $sources; do printf ' \\\n  '$s; done
      fi
      echo
      do_flags __ocpi_build_dir__${pname} "" program
      printf "__ocpi_build_dir__${pname}_LDFLAGS = $ldflags $ocpi_prereq_ldflags\n"
      print_libs __ocpi_build_dir__${pname}_LDADD program @ocpi_library_suffix@ $libs $ldadd \
		 '$(ocpi_program_prereqs)'
    done
  }
  [ -n "$sources" -a "$dest" != "noinst" -a -z "$driver" ] &&
      ocpi_libs_ordered="libocpi_$lname $ocpi_libs_ordered"
done
echo ocpi_drivers=$drivers
echo ocpi_swigs=$swigs
[ -n "$verbose" ] && echo Done generating Makefile.am  >&2
exit 0

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


# This script performs operations on projects.

# FIXME: add option to copy "other" files when creating worker

[ -z "$OCPI_CDK_DIR" -o ! -d "$OCPI_CDK_DIR" ] && echo "Error: OCPI_CDK_DIR environment setting not valid: '${OCPI_CDK_DIR}'" && exit 1
source $OCPI_CDK_DIR/scripts/util.sh

# Database of models, languages and suffixes, redundant with util.mk
Models=(rcc hdl ocl)
Language_rcc=c++
Languages_rcc=(c:c c++:cc)
Language_hdl=vhdl
Languages_hdl=(vhdl:vhd)
Language_ocl=cl

CheckCDK='$(if $(realpath $(OCPI_CDK_DIR)),,\
  $(error The OCPI_CDK_DIR environment variable is not set correctly.))'

# Print error message and exit
function bad {
  echo "Error: $*" >&2
  exit 1
}

function warn {
  echo "Warning: $*" >&2
}

function noslash {
  [ "$(dirname $1)" != . ] && bad no slashes allowed in \"$1\": perhaps use the -d option\?
  return 0
}
function noexist {
  [ -e "$1" ] && bad the file/directory $directory/$1 already exists
  return 0
}
function libexist {
  if [ -n "$library" ] ; then
    [ -d "$libdir" ] || bad the specified library \"$library\" does not exist
  fi
  return 0
}
# make sure the components dir is a library we can use, from the project level
function check_components {
  if [ -n "$2" ] ; then
    compdir=$2
  else
    compdir=components
  fi
  [ -d "$compdir" ] || bad the \"components\" library does not exist
  get_dirtype $compdir
  case "$dirtype" in
    (library|lib)
      ;;
    ("")
      [ "$(ls $compdir)" == "" ] && bad the directory \"$compdir\" is empty
      bad there are component libraries under \"$compdir/\" in this project, so the \"$compdir\" library cannot be $1
      ;;
    (libraries)
      bad must specify a library \'\(not "$compdir"\)\' when there are libraries under the \"$compdir\" directory
      ;;
    (*)
      bad the \"$compdir\" directory appears to have the wrong type of Makefile $dirtype
      ;;
  esac
}

function needname {
  [ -z "$1" ] && bad a name argument is required after the command: $verb $hdl$noun
  [ "$1" == '*' ] && bad you can not use \* as a name
  return 0
}
# Look in a directory and determine the type of the Makefile, set dirtype
# If there is no Makefile or no appropriate line in the Makefile, dirtype is ""
function get_dirtype {
  [ ! -e "$1" ] && bad $1 should exist and does not
  [ ! -d "$1" ] && bad $1 should be a directory and is not
  if [ -f "$1"/Makefile ]; then
    dirtype=$(sed -n 's=^[ 	]*include[ 	]*.*OCPI_CDK_DIR.*/include/\(.*\)\.mk.*=\1=p' $1/Makefile | tail -1)
    dirtype=${dirtype##hdl\/}
    dirtype=${dirtype##rcc\/}
  else
    dirtype=
  fi
  if [ -z "$dirtype" ]; then
    if [ -e "$1/project-package-id" ]; then
      dirtype=project
    fi
  fi
}

# Look in a directory and determine the type of the Makefile, set dirtype
function check_dirtype {
  get_dirtype $1
  [ -z "$dirtype" ] && bad $1/Makefile is not correctly formatted.  No \"include *.mk\" lines.
  [ "$dirtype" != "$2" ] && bad $1/Makefile has unexpected type \"$dirtype\", expected \"$2\".
}
# Determine the path to the current project's top level
function get_project_top {
  # TODO get project, project.dir
  project_top=`python3 -c "\
import sys; sys.path.append(\"$OCPI_CDK_DIR/$OCPI_TOOL_PLATFORM/lib/\");
import _opencpi.util as ocpiutil; print(ocpiutil.get_path_to_project_top());"`
  [ "$project_top" != None ] || bad failure to find project containing path \"`pwd`\"
  echo $project_top
}
# Determine the package-id of the current project
function get_project_package {
  # TODO get project, project.package_id
  project_pkg=`python3 -c "\
import sys; sys.path.append(\"$OCPI_CDK_DIR/$OCPI_TOOL_PLATFORM/lib/\");
import Asset; print(Asset.Project(directory=\".\").package);"`
  [ "$project_pkg" != None ] || bad failure to find project package for path \"`pwd`\"
  echo $project_pkg
}
# Create the link to a project in the installation registry (OCPI_PROJECT_REGISTRY_DIR)
# based on the project's package name
function py_try_return_bool {
  python3 -c "\
import sys; sys.path.append(\"$OCPI_CDK_DIR/$OCPI_TOOL_PLATFORM/lib/\")
import logging
import _opencpi.util as ocpiutil
try:
    $1
    sys.exit(0)
except ocpiutil.OCPIException as err:
    logging.error(err)
    sys.exit(1)"
}

function register_project {
  if [ -n "$1" ]; then
    project="$1"
  else
    project="."
  fi
  if [ -n "$force" ]; then
    py_force=True
  else
    py_force=False
  fi
  py_try_return_bool "
    from _opencpi.assets import factory, registry;
    factory.AssetFactory.factory(\"registry\",
                                 registry.Registry.get_registry_dir(\"$project\")).add(\"$project\",
                                                                                       force=$py_force)" 1>&2 || return

  # We want to export a project on register, but only if it is not
  # an exported project itself
  is_exported=`python3 -c "\
import sys; sys.path.append(\"$OCPI_CDK_DIR/$OCPI_TOOL_PLATFORM/lib/\");
import _opencpi.util as ocpiutil; print(ocpiutil.is_path_in_exported_project(\"$project\"));"`
  if [ "$is_exported" == "False" ]; then
    make -C $project ${verbose:+AT=} exports 1>&2 || echo Could not export project \"$project\". You may not have write permissions on this project. Proceeding...
  else
     [ -z "$verbose" ] || echo Skipped making exports because this is an exported standalone project. 1>&2
  fi
}
# Remove the link to a project in the installation registry which should be named
# based on the project's package name. If a project does not exist with as
# specified by $1, assume $1 is actually a package name/link name itself
function unregister_project {
  if [ -n "$1" ]; then
    project="$1"
  else
    project="."
  fi
  py_try_return_bool "
    from _opencpi.assets import factory, registry;
    import os;
    if os.path.exists(\"$project\") or  \"/\" in \"$project\":
        # Create registry instance from project dir, and remove project by its dir
        factory.AssetFactory.factory(\"registry\", registry.Registry.get_registry_dir(\"$project\")).remove(directory=\"$project\")
    else:
        # Create registry instance from current dir, and remove project by its package-ID
        factory.AssetFactory.factory(\"registry\", registry.Registry.get_registry_dir(\"$project\")).remove(\"$project\")" 1>&2
}

function delete_project {

  if [ -n "$1" ]; then
    project="$1"
  else
    project="."
  fi

  if [ -n "$force" ]; then
    py_force=True
  else
    py_force=False
  fi
  py_try_return_bool "
    from _opencpi.assets import factory, registry;
    import os;
    if (os.path.exists(\"$project\")) and (os.path.isdir(\"$project\")):
        my_proj = factory.AssetFactory.factory(\"project\", \"$project\")
        my_proj.delete($py_force)
    else:
        my_reg = factory.AssetFactory.factory(\"registry\", registry.Registry.get_registry_dir(\".\"))
        my_proj = my_reg.get_project(\"$project\")
        my_proj.delete($py_force)"
}


# This function sets the subdir variable based on the current directory
# and whether the library/platform options are set. This is for use only
# when creating assets meant to reside inside libraries. This is NOT
# for use when creating platforms, assemblies, primitives, libraries
# apps, projects, or primitives. That being said, this script essentially
# does nothing except call get_dirtype . when one of those nouns is
# being operated on.
function get_subdir {
  case "$noun" in
    (worker|device|spec|component|protocol|properties|signals|test|card|slot) libasset=1;;
    (library|assembly|assemblies|platform|platforms|primitive|primitives|application|applications|project|registry) libasset=;;
    (*) bad invalid noun \"$noun\";;
  esac
  get_dirtype .
  subdir=.
  # Set the subdir based on our current dirtype, what type of noun we are
  # operating on, and any library/platform options set
  [ -z "$libasset" ] || {
    [ -z "$standalone" ] && case "$dirtype" in
      (project)
        [ "$libbase" != hdl -o -d "hdl" -o "$verb" != create ] || make_hdl_dir
        if [ -n "$library" ] ; then
          case "$library" in
            (components) subdir=$library;;
            (hdl/cards|hdl/devices|hdl/adapters) autocreate=1; subdir=$library;;
            (hdl/*) subdir=$library;;
            (*) subdir=components/$library;;
          esac
        elif [ -n "$platform" ] ; then
          [ -d "$hdlorrcc/platforms/$platform" ] ||
            bad the platform $platform does not exist \(in $hdlorrcc/platforms/$platform\)
          autocreate=1
          subdir=$hdlorrcc/platforms/$platform/devices
        elif [ "$libbase" == hdl ] ; then
          autocreate=1
          subdir=hdl/devices
        elif [ -n "$project" ] ; then
          subdir=.
        else
          check_components used
          subdir=components
        fi
        ;;
      (library)
        if [ -n "$liboptset" -o -n "$hdlliboptset" -o -n "$platform" -o -n "$project" ] ; then
          bad cannot specify \"-l, --hdl-library, -P, or -p\" \(library/platform/project\) from within a library.
        else
          subdir=.
        fi
        ;;
      (libraries)
        if [ -n "$library" ] ; then
          if [ -n "$platform" -o -n "$card" -o -n "$project" ] ; then
            bad cannot specify  \"-P, or -p\" \(platform/project\) within a libraries collection.
          fi
          subdir=$(basename $library)
        else
          bad must specify a library when operating from a libraries collection.
        fi
        ;;
      (hdl-platform)
        if [ -n "$library" -o -n "$platform" -o -n "$card" -o -n "$project" ] ; then
          bad cannot specify  \"-l, -h, -P, or -p\" \(library/platform/project\) within a platform.
        else
          autocreate=1
          subdir=devices
        fi
        ;;
      (hdl-platforms)
        if [ -n "$platform" ] ; then
          if [ -n "$library" -o -n "$card" -o -n "$project" ] ; then
            bad cannot specify  \"-l, -h, or -p\" \(library/project\) within a platform.
          fi
          [ -d "$platform" ] ||
            bad the platform $platform does not exist \(in hdl/platforms/$platform\)
          autocreate=1
          subdir=$platform/devices
        else
          bad must choose a platform \(-P\) when operating from the platforms directory.
        fi
        ;;
      (*) if [ -z "$dirtype" ] ; then
            bad cannot operate within unknown directory type.  Try returning to the top level of your project.
          else
            bad cannot operate within directory of type \"$dirtype\". Try returning to the top level of your project.
          fi ;;
    esac
    [ -d "$subdir" -o "$verb" != create ] || { if [ "$autocreate" == 1 ] ; then make_library $subdir $subdir ; fi }
    [ -d "$subdir" ] || bad the library for '"'$library'"' '('$subdir')' does not exist
    # Confirm that the subdirectory targeted is in fact a library
    get_dirtype $subdir
    [ -n "$standalone" -o "$dirtype" = library -o -n "$project" ] ||
      bad the directory for '"'$library'"' '('$libdir')' is not a library - it is of type '"'$dirtype'"'
    # Restore the desired dirtype (current directory) for use after this function call.
    get_dirtype .
  }
}

# Choose the delete command to recommend the user runs if a worker/device
# creation fails and the (-k) keep option is used (ie the partial results
# remain in the new worker/device dir).
function get_deletecmd {
  get_dirtype .
  if [ "$libbase" != hdl ] ; then
    deletecmd="-l $library delete $noun"
  else
    if [ "$noun" == worker ] ; then
      deletecmd="delete $noun"
    else
      deletecmd="delete hdl $noun "
    fi
    case "$dirtype" in
      (project|libraries)
        if [ -n "$platform" ] ; then
          deletecmd="-P $platform $deletecmd"
        elif [ -n "$library" ] ; then
          deletecmd="-h $(basename $library) $deletecmd"
        fi
        ;;
      (hdl-platform|library) #no change
        ;;
      (platforms)
        if [ -n "$platform" ] ; then
          deletecmd="-P $platform $deletecmd"
        else
          bad need to specify platform \"-P\" when operating from the platforms directory.
        fi
        ;;
      (*)
        bad a device can only be created from a project, library, or platform directory
        ;;
    esac
  fi
}

function ask {
  ans=''
  if [ -n "${JENKINS_HOME}" ]; then
    echo "OCPI:WARNING: Running under Jenkins: auto-answered 'yes' to '${*}?'"
    ans=y
  fi
  if [ -z "$force" ]; then
    until [[ "$ans" == [yY] || "$ans" == [nN] ]]; do
      read -p "Are you sure you want to $* (y or n)? " ans
    done
    [[ "$ans" == [Nn] ]] && exit 1
  fi
  return 0
}

function do_registry {
  if [ "$verb" == create ]; then
    if [ -z "$1" ]; then
      bad provide a registry directory to create
      exit 1
    elif [ -e "$1" ]; then
      bad the registry to create \(\"$1\"\) already exists. Use that one, or remove it and try again
    else
      [ -z "$verbose" ] || echo The project registry \"$1\" has been created
      mkdir $1
      echo "OCPI:WARNING:To use this registry, run the following command and add it to"\
           "your ~/.bashrc:" >&2
      echo "export OCPI_PROJECT_REGISTRY_DIR=$(ocpiReadLinkE $1)" >&2
    fi
  elif [ "$verb" == delete ]; then
    if [ -z "$1" ]; then
      registry_to_delete=$OCPI_PROJECT_REGISTRY_DIR
    else
      registry_to_delete=$1
    fi
    if [ -e "$registry_to_delete" ]; then
      if [ "$(ocpiReadLinkE $registry_to_delete)" == "$(ocpiReadLinkE $OCPI_CDK_DIR/../project-registry)" \
           -o "$(ocpiReadLinkE $registry_to_delete)" == "/opt/opencpi/project-registry" ] ; then
        bad cannot delete the default project registry \"$registry_to_delete\"
      fi
      ask delete the project registry at \"$registry_to_delete\"
      rm -rf "$1"
    else
      bad registry to delete \(\"$registry_to_delete\"\) does not exist
    fi
  elif [ "$verb" == set ]; then
    py_cmd="from _opencpi.assets import factory; factory.AssetFactory.factory(\"project\", \".\").set_registry(\"$1\")"
    if [ $(py_try_return_bool "$py_cmd" 1>&2; echo $?) -eq 1 ]; then
      # Error is printed in python
      exit 1
    fi
  elif [ "$verb" == unset ]; then
    py_cmd="from _opencpi.assets import factory; factory.AssetFactory.factory(\"project\", \".\").unset_registry()"
    if [ $(py_try_return_bool "$py_cmd" 1>&2; echo $?) -eq 1 ]; then
      # Error is printed in python
      exit 1
    fi
  else
    bad the registry noun is only valid after the create/delete or set/unset verbs
  fi
}

function do_project {
  set -e
  if [ "$verb" == register ]; then
    if [ $(register_project $1 1> /dev/null; echo $?) -eq 1 ]; then
      # Error message printed from python
      exit 1
    fi
    [ -z "$verbose" ] || echo "Successfully registered project \"$1\" in project registry."
    return 0
  elif [ "$verb" == unregister ]; then
    pj_title=\"$1\"
    [ -n "$1" ] || pj_title=current
    ask unregister the $pj_title project/package from its project registry
    if [ $(unregister_project $1 1> /dev/null; echo $?) -eq 1 ]; then
      # Error message printed from python
      exit 1
    fi
    [ -z "$verbose" ] || echo "Successfully unregistered project \"$1\" in project registry."
    return 0
  elif [ "$verb" == delete ]; then
    delete_project $1
    return $?
  elif [ "$verb" == refresh ]; then
    gpmd=$OCPI_CDK_DIR/scripts/genProjMetaData.py
       if [ -x $gpmd ] ; then
         $gpmd $(pwd)
       fi
       return 0
  elif [ "$verb" == build ]; then
    if [ -z "$buildRcc" -a -z "$buildHdl" -a -n "$buildClean" ]; then
      cleanTarget="clean"
    fi
    if [ -n "$buildRcc" -a -n "$buildClean" ]; then
      buildRcc=""
      cleanTarget+=" cleanrcc"
    fi
    if [ -n "$buildHdl" -a -n "$buildClean" ]; then
      buildHdl=""
      cleanTarget+=" cleanhdl"
    fi
    # Normally we build imports outside of any do_* function because
    # we always want to build imports when building. For project, we
    # do this here because
    make -C $subdir/$1 ${verbose:+AT=} imports
    make -C $subdir/$1 ${verbose:+AT=} ${cleanTarget:+$cleanTarget} ${buildRcc:+rcc} ${buildHdl:+hdl} \
            ${buildNoAssemblies:+Assemblies=} \
            ${assys:+Assemblies=" ${assys[@]}"} \
            ${hdlplats:+HdlPlatforms=" ${hdlplats[@]}"} \
            ${hdltargets:+HdlTargets=" ${hdltargets[@]}"} \
            ${swplats:+RccPlatforms=" ${swplats[@]}"} \
            ${hwswplats:+RccHdlPlatforms="${hwswplats[@]}"} \
            $OCPI_MAKE_OPTS
    if [ -n buildClean -a -z "$hardClean" ] ; then
      make -C $subdir/$1 ${verbose:+AT=} imports
      make -C $subdir/$1 ${verbose:+AT=} exports
    fi
    return 0
  fi
  [ -n "$dirtype" ] &&
     bad the directory \"$directory\" where the project directory would be created is inside an OpenCPI project
  [ -n "$top" -o -n "$platform" -o -n "$prebuilt" -o -n "$library" ] &&
    bad illegal options present for creating project
  noslash $1
  noexist $1
  mkdir $1
  cd $1
  if [ -z "$packagename" ]; then
    packagename=$1
  fi
  cat <<EOF > Project.exports
# This file specifies aspects of this project that are made available to users,
# by adding or subtracting from what is automatically exported based on the
# documented rules.
# Lines starting with + add to the exports
# Lines starting with - subtract from the exports
all
EOF
  cat <<EOF > Project.mk
# This Makefile fragment is for the "$1" project

# Package identifier is used in a hierarchical fashion from Project to Libraries....
# The PackageName, PackagePrefix and Package variables can optionally be set here:
# PackageName defaults to the name of the directory
# PackagePrefix defaults to local
# Package defaults to PackagePrefix.PackageName
#
# ***************** WARNING ********************
# When changing the PackageName or PackagePrefix of an existing project the
# project needs to be both unregistered and re-registered then cleaned and
# rebuilt. This also includes cleaning and rebuilding any projects that
# depend on this project.
# ***************** WARNING ********************
#
${packagename:+PackageName=$packagename}
${packageprefix:+PackagePrefix=$packageprefix}
${package:+Package=$package}
ProjectDependencies=${dependencies[@]}
${liblibs:+Libraries=${liblibs[@]}}
${includes:+IncludeDirs=${includes[@]}}
${xmlincludes:+XmlIncludeDirs=${xmlincludes[@]}}
${complibs:+ComponentLibraries=${complibs[@]}}
EOF
  cat <<EOF > Makefile
$CheckCDK
# This is the Makefile for the "$1" project.
include \$(OCPI_CDK_DIR)/include/project.mk
EOF

package_id=`python3 -c "\
import sys; sys.path.append(\"$OCPI_CDK_DIR/$OCPI_TOOL_PLATFORM/lib/\");
import _opencpi.util as ocpiutil; print(ocpiutil.get_project_package());"`

# Create IDE .project file (AV-1247)
  cat << EOF > .project
<?xml version="1.0" encoding="UTF-8"?>
<projectDescription>
  <name>$package_id</name>
  <comment></comment>
  <projects></projects>
  <buildSpec></buildSpec>
  <natures></natures>
</projectDescription>
EOF

cat <<EOF > .gitignore
# Lines starting with '#' are considered comments.
# Ignore (generated) html files,
#*.html
# except foo.html which is maintained by hand.
#!foo.html
# Ignore objects and archives.
*.rpm
*.obj
*.so
*~
*.o
target-*/
*.deps
gen/
*.old
*.hold
*.orig
*.log
lib/
#Texmaker artifacts
*.aux
*.synctex.gz
*.out
**/doc*/*.pdf
**/doc*/*.toc
**/doc*/*.lof
**/doc*/*.lot
run/
exports/
imports
*.pyc
EOF

cat <<EOF > .gitattributes
*.ngc -diff
*.edf -diff
*.bit -diff
EOF

  if [ -n "$register_enable" ]; then
    # Register a link to the project in the project registry
    if [ $(register_project 1> /dev/null; echo $?) -eq 1 ]; then
      if [ -n "$keep" ]; then
        echo The project directory has been created, but not registered \(which may be fine\).
        echo If the project must be registered, resolve the conflict and run:
        echo ocpidev register project $1
      else
        echo Removing the project directory \"$1\" due to errors.
        cd ..
        rm -r -f $1
      fi
      # Error message printed from python
      exit 1
    fi
  fi
  # Generate exports and imports
  make exports
  make imports

  [ -z "$verbose" ] || echo A new project named \"$1\" has been created in `pwd`.
}

function do_applications {
  case "$dirtype" in
    (project) subdir=applications/;;
    (applications) subdir=.;;
    (*) bad this command can only be issued in a project directory or an applications directory;;
  esac
  if [ "$verb" == build ]; then
    make -C $subdir ${verbose:+AT=} ${buildClean:+clean} ${swplats:+RccPlatforms=" ${swplats[@]}"} \
      ${hwswplats:+RccHdlPlatforms=" ${hwswplats[@]}"} \
      $OCPI_MAKE_OPTS
    return 0
  fi
}

# Create an application
# An application in a project might have:
# An xml file
# A c++ main program
# Its own private components (and thus act like a library)
# Its own private HDL assemblies
# Some data files
function do_application {
  set -e
  subdir=./
  [ -z "$standalone" ] && case "$dirtype" in
    (project) subdir=applications/;;
    (applications) subdir=./;;
    (*) bad this command can only be issued in a project directory or an applications directory;;
  esac
  adir=$subdir$1
  if [ "$verb" == build ]; then
    make -C $subdir$1 ${verbose:+AT=} ${buildClean:+clean} ${swplats:+RccPlatforms=" ${swplats[@]}"} \
      ${hwswplats:+RccHdlPlatforms=" ${hwswplats[@]}"} \
      $OCPI_MAKE_OPTS
    return 0
  fi
  if [ "$verb" == delete ]; then
    if [ -n "$xmlapp" ] ; then
      if [ -f "$adir.xml" ] ; then
        ask delete the application xml file \"$adir.xml\"
        rm -f $adir.xml
      else
        bad the application at \"$adir.xml\" does not exist
      fi
      return 0
    fi
    [ -e "$adir" ] || bad the application at \"$adir\" does not exist
    get_dirtype $adir
    [ "$dirtype" == application ] || bad the directory at $adir does not appear to be an application
    ask delete the application project in the \"$adir\" directory
    rm -r -f $adir
    [ -z "$verbose" ] || echo The application \"$1\" in the directory \"$adir\" has been deleted.
    return 0
  fi
  app=${1/%.xml/}
  if [ -n "$xmlapp" ] ; then
    if [ -e "$subdir/$app.xml" ] ; then
      bad the application \"$app\" already exists in ${topdir}$subdir$app.xml
    fi
  elif [ -d "$subdir$1" ] ; then
      bad the application \"$app\" already exists in ${topdir}$subdir$1
  fi

  if [ "$dirtype" == project -a ! -e applications ]; then
    mkdir applications
    cat <<EOF > applications/Makefile
$CheckCDK
# To restrict the applications that are built or run, you can set the Applications
# variable to the specific list of which ones you want to build and run, e.g.:
# Applications=app1 app3
# Otherwise all applications will be built and run
include \$(OCPI_CDK_DIR)/include/applications.mk
EOF
    [ -z "$verbose" ] || echo This is the first application in this project.  The \"applications\" directory has been created.
  fi

  appdir=""
  if [ -z "$xmlapp" ] ; then
    appdir=$1
    mkdir $subdir$1
    cat <<EOF > $subdir$1/Makefile
$CheckCDK
# This is the application Makefile for the "$1" application
# If there is a $1.cc (or $1.cxx) file, it will be assumed to be a C++ main program to build and run
# If there is a $1.xml file, it will be assumed to be an XML app that can be run with ocpirun.
# The RunArgs variable can be set to a standard set of arguments to use when executing either.
include \$(OCPI_CDK_DIR)/include/application.mk
EOF
    [ -z "$verbose" ] || {
      echo Application \"$1\" created in the directory \"$topdir$subdir$1\"
      echo You must create either a $1.xml or $1.cc file in the $topdir$subdir$1 directory\"
    }
  fi

  cat <<-EOF > $subdir$appdir/$app.xml
	<!-- The $1 application xml file -->
	<Application>
	  <Instance Component='ocpi.core.nothing' Name='nothing'/>
	</Application>
	EOF
  if [ -z "$xmlapp" -a -z "$xmldirapp" ] ; then
    cat <<EOF > $subdir$appdir/$app.cc
#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <cassert>
#include <string>
#include "OcpiApi.hh"

namespace OA = OCPI::API;

int main(/*int argc, char **argv*/) {
  // Reference https://opencpi.github.io/OpenCPI_Application_Development.pdf for
  // an explanation of the ACI.

  try {
    OA::Application app("$app.xml");
    app.initialize(); // all resources have been allocated
    app.start();      // execution is started

    // Do work here.

    // Must use either wait()/finish() or stop(). The finish() method must
    // always be called after wait(). The start() method can be called
    // again after stop().
    app.wait();       // wait until app is "done"
    app.finish();     // do end-of-run processing like dump properties
    // app.stop();

  } catch (std::string &e) {
    std::cerr << "app failed: " << e << std::endl;
    return 1;
  }
  return 0;
}
EOF
  fi
  [ -z "$verbose" ] || echo The XML application \"$app\" has been created in ${topdir}$subdir$appdir$app.xml

}

# Major issue is in which library or top level.
function do_protocol_or_spec {
  set -e
  subdir="$subdir/specs"
  if [ -n "$project" ] ; then
    subdir="specs"
  fi
  [ -z "$file" ] && case $1 in
    (*.xml)                    file=$1;;
    (*[-_]prot|*[-_]protocol)
      [ $noun != protocol ] && bad you cannot make a $noun file with a protocol suffix
      file=$1.xml
      ;;
    (*[-_]spec)
      [ $noun != spec ] && bad it is a bad idea to make a $noun file with a spec suffix
      file=$1.xml
      ;;
    (*[-_]props|*[-_]properties)
      [ $noun != properties ] && bad it is a bad idea to make a $noun file with a properties suffix
      file=$1.xml
      ;;
    (*[-_]sigs|*[-_]signals)
      [ $noun != signals ] && bad you cannot make a $noun file with a signals suffix
      ;;
    (*)
      case $noun in
        (spec) file=$1-spec.xml;;
        (protocol) file=$1-prot.xml;;
        (properties) file=$1-props.xml;;
        (signals) file=$1-signals.xml;;
        (card|slot) file=$1.xml;;
      esac
      ;;
  esac
  if [ "$verb" == delete ]; then
    [ -e "$subdir/$file" ] || bad the file \"$subdir/$file\" does not exist
    ask delete the file \"$subdir/$file\"
    rm $subdir/$file
    [ -z "$verbose" ] || echo The $2 \"$1\" in file $subdir/$file has been deleted.
    return 0
  fi
  [ -e "$subdir/$file" ] && bad the file \"$subdir/$file\" already exists
  if [ ! -d "$subdir" -a -n "$verbose" ] ; then
    if [ "$subdir" == specs ] ; then
      echo The \"specs\" directory does not exist yet and will be created.
    else
      echo The specs directory \"$subdir\" does not exist and will be created.
    fi
  fi
  mkdir -p $subdir
  # Make sure we record the package's prefix in the specs directory so that spec files used
  # by other projects (or this one) know what the package should be.
  if [ "$subdir" == specs -a ! -e specs/package-id ] ; then
    make specs/package-id
  fi
  case "$noun" in
    (protocol)
      cat <<EOF > $subdir/$file
<!-- This is the protocol spec file (OPS) for protocol: $1
     Add <operation> elements for message types.
     Add protocol summary attributes if necessary to override attributes
     inferred from operations/messages -->
<Protocol> <!-- add protocol summary attributes here if necessary -->
  <!-- Add operation elements here -->
</Protocol>
EOF
    ;;
    (spec)
      if [ -n "$nocontrol" ] ; then
        nocontrolstr=" NoControl='true' "
      fi
      cat <<EOF > $subdir/$file
<!-- This is the spec file (OCS) for: $1
     Add component spec attributes, like "protocol".
     Add property elements for spec properties.
     Add port elements for i/o ports -->
<ComponentSpec$nocontrolstr>
  <!-- Add property and port elements here -->
</ComponentSpec>
EOF
    ;;
    (properties)
      cat <<EOF > $subdir/$file
<!-- This is the properties file (OPS) initially named: $1
     Add <property> elements for each property in this set -->
<Properties>
  <!-- Add property elements here -->
</Properties>
EOF
    ;;
    (signals)
      cat <<EOF > $subdir/$file
<!-- This is the signals file (OSS) initially named: $1
     Add <signal> elements for each signal in this set -->
<Signals>
  <!-- Add signal elements here -->
</Signals>
EOF
    ;;
    (slot)
      cat <<EOF > $subdir/$file
<!-- This is the slot definition file for slots of type: $1
     Add <signal> elements for each signal in the slot -->
<SlotType>
  <!-- Add signal elements here -->
</SlotType>
EOF
      ;;
    (card)
      cat <<EOF > $subdir/$file
<!-- This is the card definition file for cards of type: $1
     Add <signal> elements for each signal in the slot -->
<Card>
  <!-- Add device elements here, with signal mappings to slot signals -->
</Card>
EOF
      ;;
  esac
  # If the parent directory is a library, update its links to specs to include this new one
  get_dirtype $subdir/..
  if [ "$dirtype" == library ]; then
    make speclinks -C $subdir/..
  fi
  if [ -n "$verbose" ]; then
    if [ "$dirtype" == library ]; then
      echo A new $2, \"$1\", has been created in library \"$(basename `ocpiReadLinkE $subdir/..`)\" in $subdir/$file
    else
      echo A new $2, \"$1\", has been created at the project level in $subdir/$file
    fi
  fi
  if [ -n "$createtest" ] ; then
    do_test $1
  fi
  return 0
}


function make_hdl_dir {
  if [ ! -d "hdl" ] ; then
    mkdir hdl
  fi
}

# make a library named $1, in the directory $2
function make_library {
  mkdir -p $2
  cat <<EOF > $2/Makefile
# This is the $1 library

# All workers created here in *.<model> will be built automatically
# All tests created here in *.test directories will be built/run automatically
# To limit the workers that actually get built, set the Workers= variable
# To limit the tests that actually get built/run, set the Tests= variable

# Any variable definitions that should apply for each individual worker/test
# in this library belong in Library.mk

include \$(OCPI_CDK_DIR)/include/library.mk
EOF

  cat <<EOF > $2/Library.mk
# This is the $1 library

# This makefile contains variable definitions that will apply when building each
# individual worker and test in the library

# Package identifier is used in a hierarchical fashion from Project to Libraries....
# The PackageName, PackagePrefix and Package variables can optionally be set here:
# PackageName defaults to the name of the directory
# PackagePrefix defaults to package of parent (project)
# Package defaults to PackagePrefix.PackageName
${packagename:+PackageName=$packagename}
${packageprefix:+PackagePrefix=$packageprefix}
${package:+Package=$package}
${liblibs:+Libraries=${liblibs[@]}}
${complibs:+ComponentLibraries=${complibs[@]}}
${includes:+IncludeDirs=${includes[@]}}
${xmlincludes:+XmlIncludeDirs=${xmlincludes[@]}}
EOF
  make --no-print-directory -C $2 ||
    bad library creation failed, you may want to do: ocpidev delete library $1
  [ -z "$verbose" ] || echo A new library named \"$1\" \(in directory \"$2/\"\) has been created.

}

function do_library {
  set -e
  if [ "$libbase" == "hdl" -a -n "$library" ]; then
    one=$(basename $library)
  else
    one=$1
  fi
  [ -z "$library" -o "$verb" != "delete" ] || bad The -l and --hdl_library options are invalid when deleting a library
  subdir=$one
  if [ -n "$standalone" ] ; then
    libbase=.
    subdir=$one
  #elif [  ]; then

  elif [ -z "$libbase" ] ; then
    libbase=components
  fi
  [ -z "$standalone" ] && case "$dirtype" in
    (project)
      [ "$libbase" != hdl -o -d "hdl" -o "$verb" != create ] || make_hdl_dir
      if [ "$1" == "$libbase" ] ; then
        subdir=$libbase #is this ever valid? I dont think so
      elif [ -n "$platform" ] ; then
        [ "$1" == devices ] || bad can only create the devices library under a platform
        subdir=hdl/platforms/$platform/devices
      else
        subdir=$libbase/$one
      fi
      ;;
    (libraries)
      [ -z "$platform" ] || bad cannot specify a platform from within a libraries collection.

      subdir=$one
      ;;
    (library)
      if [ "$(basename $subdir)" == "components" ] ; then
        bad this project already has a components library, you cannot create another library.
      else
        bad this command can only be issued in a project directory or a components directory
      fi
      ;;
    (*) bad this command can only be issued in a project directory or a components directory
  esac
  if [ "$verb" == "create" -a "$(ocpiReadLinkE $subdir)" == "hdl" ]; then
      1=$(basename $subdir)
  fi
  if [ "$verb" == delete ]; then
    [ "$subdir" == "$libbase" ] && check_components deleted $libbase
    [ -e "$subdir" ] || bad the library \(directory\) \"$subdir\" does not exist
    get_dirtype $subdir
    [ "$dirtype" == library ] ||
      bad the directory \"$subdir\" does not appear to be a library
    ask delete the library directory \"$subdir\"
    rm -r -f $subdir
    if [ "$odirtype" == project -a "$subdir" != "$libbase" -a "$(ls $libbase 2> /dev/null)" == Makefile ] ; then
      rm -r -f $libbase
    fi
    [ -z "$verbose" ] || echo The library named \"$one\" \(directory \"$subdir/\"\) has been deleted.
    return 0
  elif [ "$verb" == build ]; then
    [ -n "$one" ] || bad the library was not specified
    [ -d "$subdir" ] || bad the library \(directory\) \"$subdir\" does not exist
    if [ -z "$buildRcc" -a -z "$buildHdl" -a -n "$buildClean" ]; then
      cleanTarget="clean"
    fi
    if [ -n "$buildRcc" -a -n "$buildClean" ]; then
      buildRcc=""
      cleanTarget+=" cleanrcc"
    fi
    if [ -n "$buildHdl" -a -n "$buildClean" ]; then
      buildHdl=""
      cleanTarget+=" cleanhdl"
    fi
    make -C $subdir ${verbose:+AT=} ${cleanTarget:+$cleanTarget} ${verbose:+AT=} ${buildRcc:+rcc} ${buildHdl:+hdl}\
            ${hdlplats:+HdlPlatforms=" ${hdlplats[@]}"}\
            ${hdltargets:+HdlTargets=" ${hdltargets[@]}"}\
            ${swplats:+RccPlatforms=" ${swplats[@]}"}\
            ${workerList:+Workers=" ${workerList[@]}"}\
            ${hwswplats:+RccHdlPlatforms=" ${hwswplats[@]}"}\
            $OCPI_MAKE_OPTS
    return 0
  fi
  [ -e "$subdir" ] && bad the library \"$one\" \(directory $subdir/\) already exists
  get_dirtype .
  if [ "$dirtype" == "libraries" -a  "$(basename $(pwd))" == "components" -a "$subdir" == "components" ]; then
    bad a sub library named components is not supported, try another library name
  fi

  if [ -d "$libbase" ] ; then
    get_dirtype $libbase
    case "$dirtype" in
      (library|lib)
        if [ "$(basename $libbase)" == "components" ]; then
          bad "this project has a flattened components library. This components library can have
              workers/tests/specs, but cannot have sub-libraries. If sub-libraries are desired,
              the components library must be deleted at which point a sub-library can be directly
              created via 'ocpidev create library <sub-library>'."
        fi
        bad this project already has a \"$libbase\" library, you cannot create another library.
        ;;
      (libraries)
        ;;
      ("") ;; #standalone
      (*) bad unexpected contents of $libbase directory \($dirtype\);;
    esac
  fi
  if [ "$subdir" != "$libbase" -a ! -d "$libbase"  -a "$libbase" != "hdl" ] ; then
    if [ "$libbase" != "components" -o "$(basename $(pwd))" != "components" ] ; then
      mkdir -p $libbase
      cat <<EOF > $libbase/Makefile
$CheckCDK
# This is the Makefile for the $libbase directory when there are multiple
# libraries in their own directories underneath this $libbase directory
include \$(OCPI_CDK_DIR)/include/libraries.mk
EOF
    fi
  fi
  make_library $one $subdir
}



# Figure out what should be in the "spec" attribute
# get_spec <name> <spec>
# set "specelem" and "specattr" correctly
function get_spec {
  specelem= specattr= nameattr=
  # may want to store dirtype here so that this function does not change the dirtype global
  get_subdir
  case $2 in
    (none) s= ;;
    ("") # must be local spec with the same name
      if [ -e $subdir/specs/${1}-spec.xml -o \
           "$odirtype" == project -a -e specs/${1}-spec.xml ]; then
        s=${1}-spec
      elif [ -e $subdir/specs/${1}_spec.xml -o \
             "$odirtype" == project -a -e specs/${1}_spec.xml ]; then
	s=${1}_spec
      elif [ -n "$emulates" ] ; then
        s=emulator-spec
      elif [ "$odirtype" == project ]; then
        bad spec file not specified and ${1}'[-_]spec.xml' does not exist in library or project \"specs\" directory \($subdir/specs\)

      else
        bad spec file not specified and ${1}'[-_]spec.xml' does not exist in library \"specs\" directory \($subdir/specs\)
      fi
      ;;
    (specs/*) s=${2/specs\//} ;; # make the XML clearer since this is unnecessary
    (./*) s=${2/.\//} ;;
    (*) s=$2 ;;
  esac

  # Here we set some default xml. Subdevices are a special case with some additional default
  # XML. They are permitted to omit OCS, but are not required to.
  if [ -z "$s" ] ; then
    specelem="  <componentspec>
  </componentspec>"
  else
    specattr=" spec='$s'"
  fi
  if [ -n "$supported" ] ; then
    specelem="$specelem
$specopt  <!-- If this subdevice is sharing raw properties, include the following line:
    The count is the number of device workers that may share this subdevice
    The optional attribute is whether all the devices must be present
    <rawprop name='rprops' count='2' optional='true'/>
    For each device worker this subdevice supports, include lines like these:
    Note the index is relative to the rawprops counted above
    <supports worker='lime_tx'>
      <connect port=\"rawprops\" to=\"rprops\" index='1'/>
    </supports> -->
"
  fi
  [ -z "$3" ] || nameattr=" name='$3'"
}

# callers can preset libdir to bypass the location heuristics
function do_worker {
  odirtype=$dirtype
  libdir=$subdir
  if [ -n "$standalone" ] ; then
    libdir=.
  elif [ "$dirtype" == library ]; then
    [ -z "$library" ] || bad the -l or --hdl-library "(library)" option is not valid in a "library's" directory
  elif [ -z "$libdir" ]; then
    [ "$dirtype" == project ] || bad workers can only be created in project or library directories
  fi
  if [ "$verb" == delete ]; then
    [ -d $libdir/$1 ] || bad no worker named \"$1\" exists in the library
    get_dirtype $libdir/$1
    [ "$dirtype" == worker ] || bad the directory \"$libdir/$1\" does not contain a worker
    ask delete the worker named \"$1\" \(directory \"$libdir/$1\"\)
    rm -r -f $libdir/$1
    [ -z "$verbose" ] || echo The worker \"$1\" in the directory \"$libdir/$1\" has been deleted.
    return 0
  fi
  if [ "$verb" == build ]; then
    make -C $subdir/$1 ${verbose:+AT=} ${buildClean:+clean} \
            ${hdlplats:+HdlPlatforms=" ${hdlplats[@]}"} \
            ${hdltargets:+HdlTargets=" ${hdltargets[@]}"} \
            ${swplats:+RccPlatforms=" ${swplats[@]}"} \
            ${hwswplats:+RccHdlPlatforms=" ${hwswplats[@]}"} \
            $OCPI_MAKE_OPTS
    return 0
  fi
  words=(${1//./ })
  [ ${#words[*]} == 2 ] || bad the worker name \"$1\" is invalid, it must be '<name>.<model>'
  name=${words[0]}
  model=$(echo ${words[1]} | tr A-Z a-z)
  [ "$model" != "${words[1]}" ] && bad the authoring model part of the name \"$1\" must be lower case
  [ -d $OCPI_CDK_DIR/include/$model ] || bad the authoring model \"$model\" is not valid
  [ -n "$2" ] && {
     [ -n "$language" ] && bad language option specified \"$2\" when language argument after name is specified.
     language=$2
  }
  noexist $libdir/$1
  if [ -z "$language" ] ; then
    language=$(eval echo \${Language_$model})
    [ -z "$verbose" ] || echo Choosing default language \"$language\" for worker $1
    langattr=" language='$language'"
  else
    language=$(echo $language | tr A-Z a-z)
    for l in $(eval echo \${Languages_$model[*]}); do
      valid_languages+="\"${l/:*/}\" "
      [ "$language" = ${l/:*/} ] && suff=${l/*:/} langattr=" language='${l/:*/}'"
    done
    [ -z "$langattr" ] && bad Invalid language \"$language\" for model \"$model\". Available: ${valid_languages}
  fi
  [ -n "$emulates" ] && emuattr=" emulate='$emulates'"
  [ -n "$slave" ] && slaveattr=" slave='$slave'"
  [ -n "$version" ] && versionattr=" version='$version'"
  if [ "$OCPI_CREATE_BUILD_FILES" = 1 ]; then
    [ -n "${xmlincludes[*]}" ] && xmlincattr=" XmlIncludeDirs='${xmlincludes[@]}'"
    [ -n "${complibs[*]}" ] && complibattr=" ComponentLibraries='${complibs[@]}'"
  fi
  if [ \( -n "$rccstatprereqs" -o -n "$rccdynprereqs" \) -a "$model" != "rcc" ] ; then
    bad RccStatic/DynamicPrereqLibs is only valid for rcc workers
  fi
  elem=$(echo ${model:0:1} | tr a-z A-Z)${model:1}
  if [ "$noun" = worker ] ; then
     elem=${elem}Worker
  else
     # for devices
     elem=${elem}Device
  fi
  if [ -n "$workers" ] ; then
    for w in "${!workers[@]}"; do
      OFS="$IFS"
      IFS=: x=(${workers[$w]})
      IFS="$OFS"
      get_spec ${x[0]} ${x[1]} ${x[2]}
      wnames[$w]=${x[0]}
      specattrs[$w]="$specattr"
      specelems[$w]="$specelem"
      nameattrs[$w]="$nameattr"
      specs[$w]=${x[1]}
    done
#    for i in ${!wnames[@]}; do
#     echo MULTI $i: name:${wnames[$i]} attr:${specattrs[$i]} elem:${specelems[$i]}
#    done
    wvar="Workers=${wnames[@]}"
  else
    get_spec "$name" "$spec" "$wname"
    wnames=($name)
    specattrs=("$specattr")
    specelems=("$specelem")
    nameattrs=("$nameattr")
    specs=($spec)
  fi
  mkdir $libdir/$1
  if [ "$OCPI_CREATE_BUILD_FILES" = 1 ]; then
    [ -n "${xmlincludes[*]}" ] &&
      echo WARNING: $libdir/$1 needs XmlIncludeDirs attr in OWD for: ${xmlincludes[@]}
    [ -n "${complibs[*]}" ] &&
      echo WARNING: $libdir/$1 needs ComponentLibraries attr in OWD for: X${complibs[@]}X${#complibs[@]}
    cat <<-EOF >  $libdir/$1/Makefile
	# This is the Makefile for worker $1.
	# Unless you are doing something custom or unusual, you should not edit it.
	$CheckCDK
	$wvar
	include \$(OCPI_CDK_DIR)/include/worker.mk
	EOF
    if [ -n "${includes}${others}${cores}${rccstatprereqs}${rccdynprereqs}${targets}${extargets}${onlyplats}${explats}${liblibs}" ]; then
      cat <<-EOF  | sed '/^ *$/d' > $libdir/$1/${name}-build.xml
	<!-- The build file for worker $1, specifying how to build it -->
	<Build
	${includes:+  IncludeDirs='${includes[@]}'}
	${others:+  SourceFiles='${others[@]}'}
	${cores:+  Cores='${cores[@]}'}
	${rccstatprereqs:+  StaticPrereqLibs='${rccstatprereqs[@]}'}
	${rccdynprereqs:+  DynamicPrereqLibs='${rccdynprereqs[@]}'}
	${targets:+  OnlyTargets='${targets[@]}'}
	${extargets:+  ExcludeTargets='${extargets[@]}'}
	${onlyplats:+  OnlyPlatforms='${onlyplats[@]}'}
	${explats:+  ExcludePlatforms='${explats[@]}'}
	${liblibs:+  Libraries='${liblibs[@]}'}
	  >
	  <!-- Add any specific parameter settings using <parameter> elements -->
	  <!-- Add any specific build configurations using <configuration> elements -->
	</Build>
	EOF
    fi
  else
# Make one Makefile, even if multi-worker
  cat <<EOF | sed '/^ *$/d' >  $libdir/$1/Makefile
# This is the Makefile for worker $1
$wvar
${includes:+IncludeDirs=${includes[@]}}
${others:+SourceFiles=${others[@]}}
${cores:+Cores=${cores[@]}}
${rccstatprereqs:+RccStaticPrereqLibs=${rccstatprereqs[@]}}
${rccdynprereqs:+RccDynamicPrereqLibs=${rccdynprereqs[@]}}
${targets:+OnlyTargets=${targets[@]}}
${extargets:+ExcludeTargets=${extargets[@]}}
${onlyplats:+OnlyPlatforms=${onlyplats[@]}}
${explats:+ExcludePlatforms=${explats[@]}}
${liblibs:+Libraries=${liblibs[@]}}
${xmlincludes:+XmlIncludeDirs=${xmlincludes[@]}}
${complibs:+ComponentLibraries=${complibs[@]}}
include \$(OCPI_CDK_DIR)/include/worker.mk
EOF
fi
for s in "${supported[@]}" ; do
  s=${s/%.hdl/}
  supportselem=(${supportselem[@]} "
  <Supports Worker=\"$s\" >
     <!-- <Connect Port=\"rawprops\" To=\"rprops\" Index='0'/> -->
  </Supports>
")
done

  for w in "${!wnames[@]}"; do
    # Make one OWD file per worker
    cat <<EOF > $libdir/$1/${wnames[$w]}.xml
<$elem${nameattrs[$w]}${langattr}${specattrs[$w]}${emuattr}${slaveattr}${xmlincattr}${complibattr}${versionattr}>
${specelems[$w]}
${supportselem[@]}
</$elem>
EOF
    specfile=${specs[$w]}
    case "$specfile" in
      (*.xml) ;;
      (*-spec|*_spec) specfile=${specs[$w]}.xml ;;
      (*)
        if [ ! -e "$spec" -a -e ${specs[$w]}-spec.xml ] ; then
          specfile=${specfile}-spec.xml
        elif [ ! -e "$spec" -a -e ${specs[$w]}-spec.xml ] ; then
          specfile=${specfile}_spec.xml
        else
          specfile=${specfile}.xml
        fi
    esac
    [ -n "$verbose" -a "${specs[$w]}" != none -a ! -e "$specfile" ] && echo Warning:  spec file \"$specfile\" does not exist
  done
  [ -z "$verbose" ] || echo Running \"make skeleton\" to make initial skeleton for worker $1
  # FIXME: how do we get the project's or library's xmlincludepath etc.
  [ -z "$verbose" ] || echo "Command is: make --no-print-directory -C $libdir/$1 skeleton LibDir=../lib/$model genlinks"
  make --no-print-directory -C $libdir/$1 skeleton LibDir=../lib/$model genlinks || {
    if [ -n "$keep" ]; then
      echo The worker directory, Makefile and xml file have been created.
      get_deletecmd
      echo You may want to do: ocpidev ${standalone:+-s } $deletecmd $1
    else
      echo Removing the new worker directory \"$libdir/$1\" due to errors.
      rm -r -f $libdir/$1
    fi
    bad "Failed to build worker skeleton"
  }
  echo "Successfully built skeleton"
}

# A special version of creating a spec - just adjust the default locations
# slots are only in a top level spec or in hdl/cards (or standalone)
function do_card_or_slot {
  # If we are inside a library, then it must be cards
  # Otherwise, the subdir we are operating must be cards
  if [ "$dirtype" == library ] ; then
      [ "$(basename $(pwd))" != cards ] &&
        bad the only library where cards can be created is hdl/cards
  elif [ "$(basename $subdir)" != cards ] ; then
    bad \"$noun\" files can only be created from hdl/cards or at the project level
  fi
  [ "$verb" == delete -o "$(basename $subdir)" != cards -o -e "$subdir" ] ||
    make_library $subdir $subdir
  do_protocol_or_spec $1
}


# A special version of creating a worker - just adjust the default locations
function do_device {
  [ "${1/*./}" = hdl ] ||
    bad devices are only supported with an '".hdl"' suffix
  [ "$verb" != delete -a "$(basename $subdir)" == devices -a ! -e "$subdir" ] &&
    make_library hdl/devices $subdir
  do_worker $1
}


# Create a test directory corresponding to an OCS
# named <compname>.test with skeleton contents.
# Args: component to test
function do_test {
  odirtype=$dirtype
  if [ -z "$1" -a "$verb" == build ] ; then
    do_build_here test
    return $?
  fi
  if [ "${1/*./}" = test ] ; then
    testdir=$subdir/$1
  elif [[ ! "$testdir" == *.* ]] ; then
    testdir="$subdir/$1".test
  else
    bad test directories cannot end in \"${1/*./}\". You should use \".test\" or leave it blank and \".test\" will be appended.
  fi

  # delete:
  #   make sure dir exists
  #   if it does, delete it and its contents
  #     warn first/ask for confirmation
  if [ "$verb" == build ] ; then
    make -C $testdir  ${verbose:+AT=} ${buildClean:+clean}\
         ${hdlplats:+HdlPlatforms=" ${hdlplats[@]}"} \
         ${swplats:+RccPlatforms=" ${swplats[@]}"} \
         ${hwswplats:+RccHdlPlatforms=" ${hwswplats[@]}"} \
         $OCPI_MAKE_OPTS

  elif [ "$verb" == delete ] ; then
    echo "Deleting test dir: $testdir"
    if [ -d "$testdir" ] ; then
      get_dirtype $testdir
      [ "$dirtype" == "test" ] || bad "cannot delete test \"$testdir\". It is not a test directory."
      ask delete the test directory \"$testdir\"
      rm -rf $testdir
    else
      bad "cannot delete test \"$testdir\". Directory does not exist"
    fi

  # create:
  elif [ "$verb" == create ] ; then
    echo "Creating test dir: $testdir"
    #   make sure dir does not exist
    [ -e "$testdir" ] && bad "cannot create \"$testdir\". Directory/file already exists"
    testname=$(basename $1 .test)
    get_spec $testname $spec
    #   create dir
    mkdir -p $testdir
    #   create Makefile: include $(OCPI_CDK_DIR)/include/test.mk
    cat <<EOF > $testdir/Makefile
# This is the Makefile for the test directory which tests component "$testname"
include \$(OCPI_CDK_DIR)/include/test.mk
EOF
    #   create <comp>-test.xml:
    cat <<EOF > $testdir/$testname-test.xml
<!-- This is the test xml for testing component "$testname" -->
<Tests UseHDLFileIo='true'>
  <!-- Here are typical examples of generating for an input port and verifying results
       at an output port
  <Input Port='in' Script='generate.py'/>
  <Output Port='out' Script='verify.py' View='view.sh'/>
  -->
  <!-- Set properties here.
       Use Test='true' to create a test-exclusive property. -->
</Tests>
EOF
    #   create generate.py: #!/usr/bin/env python2
    cat <<EOF > $testdir/generate.py
#!/usr/bin/env python2

"""
Use this file to generate your input data.
Args: <list-of-user-defined-args> <input-file>
"""
EOF
    chmod +x $testdir/generate.py
    #   create verify.py: #!/usr/bin/env python2
    cat <<EOF > $testdir/verify.py
#!/usr/bin/env python2

"""
Use this script to validate your output data against your input data.
Args: <list-of-user-defined-args> <output-file> <input-files>
"""
EOF
    chmod +x $testdir/verify.py
    #   create view.sh: #!/bin/bash --noprofile
    cat <<EOF > $testdir/view.sh
#!/bin/bash --noprofile

# Use this script to view your input and output data.
# Args: <list-of-user-defined-args> <output-file> <input-files>
EOF
    chmod +x $testdir/view.sh
  fi
}

function do_hdl_platforms {
  if [ "$verb" ==  build ]; then
    cd $(get_project_top)
    if [ -n "$buildClean" ]; then
      make_target=clean
    else
      make_target=hdlplatforms
    fi
    # Note: If this implementation changes in the future, be sure to add logic to respect
    # missingOK; "make hdlprimitives" returns success even if no hdl/primitives present.
    make $make_target ${buildClean:+-C hdl/platforms} \
         ${hdlplats:+HdlPlatforms=" ${hdlplats[@]}"} \
         ${hdltargets:+HdlTargets=" ${hdltargets[@]}"} \
         $OCPI_MAKE_OPTS
    return 0;
  fi
  bad The only verb available for platforms is build. Did you mean platform
}

function do_hdl_platform {
  set -e
  case "$dirtype" in
    (project)
      subdir=hdl/platforms/
      [ -d "hdl" -o "$verb" != create ] || make_hdl_dir
      ;;
    (hdl-platforms)subdir=./;;
    (*) bad this command can only be issued in a project directory, hdl, or hdl/platforms directory;;
  esac
  pdir=$subdir$1
  if [ "$verb" ==  build ]; then
    make -C $pdir ${verbose:+AT=} ${buildClean:+clean} \
      $OCPI_MAKE_OPTS
    return 0;
  fi
  if [ "$verb" == delete ]; then
    [ -e "$pdir" ] || bad the HDL platform at \"$pdir\" does not exist
    get_dirtype $pdir
    [ "$dirtype" == "hdl-platform" ] || bad the directory at $pdir does not appear to be an HDL platform
    ask delete the HDL platform in the \"$pdir\" directory
    rm -r -f $pdir
    [ -z "$verbose" ] || echo The HDL platform \"$1\" in the directory \"$pdir\" has been deleted.
    return 0
  fi
  [ -e "$subdir$1" ] && bad the HDL platform \"$1\" in directory \"$topdir$subdir$1\" already exists
  libbase=hdl
  if [ "$dirtype" == project -a ! -e "$subdir" ]; then
    mkdir -p $subdir
    cat <<EOF > $subdir/Makefile
$CheckCDK
# To restrict the HDL platforms that are built, you can set the Platforms
# variable to the specific list of which ones you want to build, e.g.:
# Platforms=pf1 pf3
# Otherwise all platforms will be built
# Alternatively, you can set ExcludePlatforms to list the ones you want to exclude
include \$(OCPI_CDK_DIR)/include/hdl/hdl-platforms.mk
EOF
    [ -z "$verbose" ] || echo This is the first HDL platform in this project.  The \"hdl/platforms\" directory has been created.
  fi
  mkdir $subdir$1
  cat <<EOF > $subdir$1/Makefile
$CheckCDK
# This is an HDL platform Makefile for the "$1" platform
${includes:+IncludeDirs=${includes[@]}}
${xmlincludes:+XmlIncludeDirs=${xmlincludes[@]}}
${liblibs:+Libraries=${liblibs[@]}}
${others:+SourceFiles=${others[@]}}
${cores:+Cores=${cores[@]}}
${complibs:+ComponentLibraries=${complibs[@]}}
include \$(OCPI_CDK_DIR)/include/hdl/hdl-platform.mk
EOF
  if [ -z "$hdlpart" ] ; then
    hdlpart="xc7z020-1-clg484"
  fi
  cat<<EOF > $subdir/$1/$1.mk
# SET THIS VARIABLE to the part (die-speed-package, e.g. xc7z020-1-clg484) for the platform!
HdlPart_$1=$hdlpart
# Set this variable to the names of any other component libraries with devices defined in this
# platform. Do not use slashes.  If there is an hdl/devices library in this project, it will be
# searched automatically, as will "devices" in any projects this project depends on.
# An example might be something like "our_special_devices", which would exist in this or
# other projects.
# ComponentLibraries_$1=
${complibs:+ComponentLibraries_$1=${complibs[@]}}
EOF
  if [ -z "$timefreq" ] ; then
    timefreq=100e6
  fi
  if [ -z "$nosdp" ] ; then
    sdpxml="<SDP Name='sdp' Master='true'/>"
  else
    sdpxml=
  fi
  cat<<EOF > $subdir/$1/$1.xml
<!-- This file defines the $1 HDL platform -->
<HdlPlatform Language="VHDL" Spec="platform-spec">
  <SpecProperty Name='platform' Value='$1'/>
  <!-- These next two lines must be present in all platforms -->
  <MetaData Master="true"/>
  <TimeBase Master="true"/>
  $sdpxml
  <!-- Set your time server frequency -->
  <Device Worker='time_server'>
    <Property Name='frequency' Value='$timefreq'/>
  </Device>
  <!-- Put any additional platform-specific properties here using <Property> -->
  <!-- Put any built-in (always present) devices here using <device> -->
  <!-- Put any card slots here using <slot> -->
  <!-- Put ad hoc signals here using <signal> -->
</HdlPlatform>
EOF
  [ -z "$verbose" ] || echo The HDL platform \"$1\" has been created in ${topdir}$subdir/$1
}

function do_rcc_platform {
  case "$dirtype" in
    (project)subdir=rcc/platforms/;;
    (hdl-platforms)subdir=./;;
    (*) bad this command can only be issued in a project directory or an rcc/platforms directory;;
  esac
  pdir=$subdir$1
  if [ "$verb" == delete ]; then
    [ -e "$pdir" ] || bad the RCC platform at \"$pdir\" does not exist
    get_dirtype $pdir
    [ "$dirtype" == "rcc-platform" ] || bad the directory at $pdir does not appear to be an RCC platform
    ask delete the RCC platform in the \"$pdir\" directory
    rm -r -f $pdir
    [ -z "$verbose" ] || echo The RCC platform \"$1\" in the directory \"$pdir\" has been deleted.
    return 0
  fi
  [ -e "$pdir" ] && bad the RCC platform \"$1\" in directory \"$topdir$subdir$1\" already exists
  if [ "$dirtype" == project -a ! -e rcc/platforms ]; then
    mkdir -p rcc/platforms
    cat <<EOF > rcc/platforms/Makefile
$CheckCDK
# To restrict the RCC platforms that are built, you can set the Platforms
# variable to the specific list of which ones you want to build, e.g.:
# Platforms=pf1 pf3
# Otherwise all platforms will be built
# Alternatively, you can set ExcludePlatforms to list the ones you want to exclude
include \$(OCPI_CDK_DIR)/include/rcc/rcc-platforms.mk
EOF
    [ -z "$verbose" ] || echo This is the first RCC platform in this project.  The \"rcc/platforms\" directory has been created.
  fi
  mkdir $pdir
  cat <<EOF > $pdir/Makefile
$CheckCDK
# This is an RCC platform Makefile for the "$1" platform

include \$(OCPI_CDK_DIR)/include/rcc/rcc-platform.mk
EOF
  cat<<EOF > rcc/platforms/$1/$1-target.mk
# This file contains Makefile build settings for the $1 RCC platform.
# See the OpenCPI Platform Development document for complete documentation for the
# variables that can be set here.
# These variables are used for three purposes:
# 1. to build the OpenCPI core libraries
# 2. to build RCC workers
# 3. to build ACI executables.
# Some variables apply only to one of the above purposes, while others apply to several.
# Here is a preliminary list that may be out of date.
# OcpiLibraryPathEnv=LD_LIBRARY_PATH
# OcpiRpathOrigin=$${ORIGIN}
# OcpiDynamicSuffix=so
# OCPI_EXTRA_LIBS=rt dl pthread
# OCPI_EXPORT_DYNAMIC=-Xlinker --export-dynamic
# OcpiAsNeeded=-Xlinker --no-as-needed
# Basic compilation commands:
# CC = gcc
# CXX = c++
# LD = c++
# Cross-compilation variables (which typically change the CC/CXX/LD above).
# OCPI_CROSS_BUILD_BIN_DIR:=
# OCPI_CROSS_HOST:=
# OCPI_TARGET_KERNEL_DIR
EOF
}

function do_primitives {
  if [ "$verb" == build ]; then
    pjtop=$(get_project_top)
    if [ -n "$pjtop" ]; then
      [ -z "$verbose" ] || echo "Building from: $pj_top"
      cd $pjtop
    fi
    # echo $hdlplats
    # echo $hdltargets
    if [ -n "$buildClean" ]; then
      make_target=clean
    else
      make_target=hdlprimitives
    fi
    # Note: If this implementation changes in the future, be sure to add logic to respect
    # missingOK; "make hdlprimitives" returns success even if no hdl/primitives present.
    make $make_target ${buildClean:+-C hdl/primitives} \
                 ${hdlplats:+HdlPlatforms=" ${hdlplats[@]}"} \
                 ${hdltargets:+HdlTargets=" ${hdltargets[@]}"} \
                 $OCPI_MAKE_OPTS
    return 0
  fi
  bad The only verb available for primitives is build. Did you mean primitive
}

function do_primitive {
  [ "$2" = "" ] &&
    bad The argument after \"$verb hdl primitive $1\" must be the name of the primitive
  odirtype=$dirtype
  case "$dirtype" in
    (hdl-primitives) dir="$2"; mydir="this";;
    (project)
      dir=hdl/primitives/"$2"
      mydir="the hdl/primitives"
      [ -d "hdl" -o "$verb" != create ] || make_hdl_dir
      ;;
    (*) bad this command can only be executed in a project or hdl/primitives directory;;
  esac
  if [ "$verb" == build ]; then
    make -C $dir ${verbose:+AT=} ${buildClean:+clean} \
            ${hdlplats:+HdlPlatforms=" ${hdlplats[@]}"} \
            ${hdltargets:+HdlTargets=" ${hdltargets[@]}"} \
            $OCPI_MAKE_OPTS
    return 0
  fi
  if [ "$verb" == delete ]; then
    [ -d "$dir" ] || bad no $1 named \"$2\" exists in $mydir directory
    get_dirtype $dir
    case "$dirtype" in
      (hdl-library|hdl-lib) [ "$1" = library ] || bad the primitive in $dir is not a library;;
      (hdl-core) [ "$1" = core ] || bad the primitive in $dir is not a core;;
      (*) bad there is no primitive in the $dir directory;;
    esac
    ask delete the HDL primitive $1 named \"$2\" \(directory \"$dir\"\)
    rm -r -f $dir
    [ -z "$verbose" ] || echo The HDL primitive $1 \"$2\" in the directory \"$dir\" has been deleted.
    if [ "$odirtype" == project ] ; then
      if [ "$(ls hdl/primitives)" == Makefile ] ; then
        rm -r -f hdl/primitives
        [ -z "$verbose" ] || echo The hdl/primitives directory is now empty.  It has been removed.
      fi
    fi
    return 0
  fi
  [ -e "$dir" ] &&
    bad The primitive $2 already exists at $dir.
  if [ "$dirtype" == project -a ! -e hdl/primitives ]; then
    mkdir -p hdl/primitives
    cat <<EOF > hdl/primitives/Makefile
$CheckCDK
# To restrict the primitives that are built or run, you can set the Libraries or Cores
# variables to the specific list of which ones you want to build and run, e.g.:
# PrimitiveLibraries=lib1 lib2
# PrimitiveCores=core1 core2
# Otherwise all primitives will be built
include \$(OCPI_CDK_DIR)/include/hdl/hdl-primitives.mk
EOF
    [ -z "$verbose" ] || echo This is the first HDL primitive in this project.  The \"hdl/primitives\" directory has been created.
  fi
  mkdir -p $dir
  if [ "$1" = "library" ] ; then
    cat <<EOF > $dir/Makefile
# This Makefile is for the primitive library: $2

# Set this variable to any other primitive libraries that this $1 depends on.
# If they are remote from this project, use slashes in the name (relative or absolute)
# If they are in this project, they must be compiled first, and this requires that the
# PrimitiveLibraries variable be set in the hdl/primitives/Makefile such that the
# libraries are in dependency order.
#Libraries=
${liblibs:+Libraries=${liblibs[@]}}
${hdlnolib:+HdlNoLibraries=yes}
${hdlnoelab:+HdlNoElaboration=yes}
# Set this variable to the list of source files in dependency order
# If it is not set, all .vhd and .v files will be compiled in wildcard/random order,
# except that any *_pkg.vhd files will be compiled first
#SourceFiles=
${others:+SourceFiles=${others[@]}}

# Remember two rules for OpenCPI primitive libraries, in order to be usable with all tools:
# 1. Any entity (VHDL) or module (verilog) must have a VHDL component declaration in ${2}_pkg.vhd
# 2. Entities or modules to be used from outside the library must have the file name
#    be the same as the entity/module name, and one entity/module per file.


${cores:+Cores=${cores[@]}}
${targets:+OnlyTargets=${targets[@]}}
${extargets:+ExcludeTargets=${extargets[@]}}
${onlyplats:+OnlyPlatforms=${onlyplats[@]}}
${explats:+ExcludePlatforms=${explats[@]}}
include \$(OCPI_CDK_DIR)/include/hdl/hdl-library.mk
EOF
    cat <<EOF > $dir/$2_pkg.vhd
-- This package enables VHDL code to instantiate all entities and modules in this library
library ieee; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
package $2 is
-- put component declarations along with any related type definitions here
end package $2;
EOF
  else
    cat <<EOF > $dir/Makefile
# This Makefile is for the primitive core: $2

# Set this variable to any other primitive libraries that this core depends on.
# If they are remote from this project, use slashes in the name (relative or absolute)
#Libraries=
${liblibs:+Libraries=${liblibs[@]}}
# Set this variable to the list of source files in dependency order
# If it is not set, all .vhd and .v files will be compiled in wildcard/random order,
# except that any *_pkg.vhd will be compiled first, if present.
#SourceFiles=

# Set this variable if the top level module name for this core is different from the
# core name: $2
#Top=
${module:+Top=${module[@]}}

# Set this variable if this core is in fact a presynthesized/prebuilt core
# and thus does not have source files except perhaps for simulation
# The suffix is added for you (as appropriate for Xilinx or Altera etc.)
#PreBuiltCore=
${prebuilt:+PreBuiltCore=${prebuilt[@]}}

# Remember that verilog cores must have a black box empty module definition
# in a file named <top>_bb.v (where <top> is the of the core or the value of the Top
# variable).
${others:+SourceFiles=${others[@]}}
${cores:+Cores=${cores[@]}}
${targets:+OnlyTargets=${targets[@]}}
${extargets:+ExcludeTargets=${extargets[@]}}
${onlyplats:+OnlyPlatforms=${onlyplats[@]}}
${explats:+ExcludePlatforms=${explats[@]}}
include \$(OCPI_CDK_DIR)/include/hdl/hdl-core.mk
EOF
  fi
}

function do_assemblies {
  if [ "$verb" == build ]; then
    pjtop=$(get_project_top)
    if [ -n "$pjtop" ]; then
      cd $pjtop
    fi
    if [ -n "$buildClean" ]; then
      make_target=clean
    else
      make_target=hdlassemblies
    fi
    make $make_target ${buildClean:+-C hdl/assemblies} ${hdlplats:+HdlPlatforms=" ${hdlplats[@]}"} \
      $OCPI_MAKE_OPTS
    return 0
  fi
  bad The only verb available for primitives is build. Did you mean assembly
}

function do_build_here {
    buildTest="$1"
    get_dirtype .
    if [ -z "$buildRcc" -a -z "$buildHdl" -a -n "$buildClean" -a "$1" != "test" ]; then
      cleanTarget="clean"
    fi
    if [ -n "$buildClean" -a "$1" == "test" ]; then
      buildTest=""
      cleanTarget="cleantest"
    fi
    if [ -n "$buildRcc" -a -n "$buildClean" ]; then
      buildRcc=""
      cleanTarget+=" cleanrcc"
    fi
    if [ -n "$buildHdl" -a -n "$buildClean" ]; then
      buildHdl=""
      cleanTarget+=" cleanhdl"
    fi
  make ${cleanTarget:+$cleanTarget} ${verbose:+AT=} ${buildRcc:+rcc} ${buildHdl:+hdl} ${buildTest} \
       ${buildNoAssemblies:+Assemblies=} \
       ${assys:+Assemblies=" ${assys[@]}"} \
       ${hdlplats:+HdlPlatforms=" ${hdlplats[@]}"} \
       ${hdltargets:+HdlTargets=" ${hdltargets[@]}"} \
       ${swplats:+RccPlatforms=" ${swplats[@]}"} \
       ${hwswplats:+RccHdlPlatforms=" ${hwswplats[@]}"} \
       ${workerList:+Workers=" ${workerList[@]}"}\
       $OCPI_MAKE_OPTS
  if [ "$dirtype" == "project" -a -z "$hardClean" ] ; then
    make ${verbose:+AT=} imports
    make ${verbose:+AT=} exports
  fi
}

# Create an hdl assembly
# An assembly has its own xml file of the same name as its directory
# It may have container XML files or not.
function do_assembly {
  set -e
  case "$dirtype" in
    (project)
      [ -d "hdl" -o "$verb" != create ] || make_hdl_dir
      subdir=hdl/assemblies/
      ;;
    (hdl-assemblies)subdir=./;;
    (*) bad this command can only be issued in a project directory, hdl, or  hdl/assemblies directory;;
  esac
  adir=$subdir$1
  if [ "$verb" == build ]; then
    make -C $subdir/$1 ${verbose:+AT=} ${buildClean:+clean} ${hdlplats:+HdlPlatforms=" ${hdlplats[@]}"} \
      $OCPI_MAKE_OPTS
    return 0
  fi
  if [ "$verb" == delete ]; then
    [ -e "$adir" ] || bad the HDL assembly at \"$adir\" does not exist
    get_dirtype $adir
    [ "$dirtype" == "hdl-assembly" ] || bad the directory at $adir does not appear to be an HDL assembly
    ask delete the HDL assembly in the \"$adir\" directory
    rm -r -f $adir
    [ -z "$verbose" ] || echo The HDL assembly \"$1\" in the directory \"$adir\" has been deleted.
    return 0
  fi
  [ -e "$subdir$1" ] && bad the HDL assembly \"$1\" in directory \"$topdir$subdir$1\" already exists
  if [ "$dirtype" == project -a ! -e hdl/assemblies ]; then
    mkdir -p hdl/assemblies
    cat <<EOF > hdl/assemblies/Makefile
$CheckCDK
# To restrict the HDL assemblies that are built, you can set the Assemblies
# variable to the specific list of which ones you want to build, e.g.:
# Assemblies=assy1 assy3
# Otherwise all assemblies will be built
# Alternatively, you can set ExcludeAssemblies to list the ones you want to exclude
include \$(OCPI_CDK_DIR)/include/hdl/hdl-assemblies.mk
EOF
    [ -z "$verbose" ] || echo This is the first HDL assembly in this project.  The \"hdl/assemblies\" directory has been created.
  fi
  mkdir $subdir$1
  cat <<EOF > $subdir$1/Makefile
$CheckCDK
# This is the HDL assembly Makefile for the "$1" assembly
# The file "$1.xml" defines the assembly.
# The default container for all assemblies is one that connects all external ports to
# the devices interconnect to communicate with software workers or other FPGAs.
#
# Limit this assembly to certain platforms or targets with Exclude/Only and Targets/Platforms ie:
# ExcludePlatforms=
${targets:+OnlyTargets=${targets[@]}}
${extargets:+ExcludeTargets=${extargets[@]}}
${onlyplats:+OnlyPlatforms=${onlyplats[@]}}
${explats:+ExcludePlatforms=${explats[@]}}
#1752:
# If you want to modify the default MAP options for Xilinx bitstream builds, export
# the "OcpiXstMapOptions" variable here (the default options are shown below) e.g:
# export OcpiXstMapOptions=-detail -w -logic_opt on -xe c -mt 4 -register_duplication on -global_opt off -ir off -pr off -lc off -power off
# Note that the -t option should not be added in OcpiXstMapOptions as it is
# already handled by a different mechanism.
#
# If you want to modify the default PAR options for Xilinx bitstream builds, export
# the "OcpiXstParOptions" variable here (the default options are shown below) e.g:
# export OcpiXstParOptions=-mt 4 -w -xe n
#
# If you want to connect external ports of the assembly to local devices on the platform,
# you must define container XML files, and mention them in a "Containers" variable here, e.g.:
# Containers=take_input_from_local_ADC

include \$(OCPI_CDK_DIR)/include/hdl/hdl-assembly.mk
EOF
  [ -z "$verbose" ] || {
    echo HDL assembly \"$1\" created in the directory \"$topdir$subdir$1\"
    echo For non-default containers, an xml file is required and must be mentioned in the Makefile
  }
  assy=${1/%.xml/}
  cat <<EOF > $subdir$1/$assy.xml
<!-- The $1 HDL assembly xml file -->
<HdlAssembly>
  <!-- Remove this instance and replace it with real ones -->
  <Instance Worker='nothing' name='nothing'/>
</HdlAssembly>
EOF
  [ -z "$verbose" ] || echo The HDL assembly \"$assy\" has been created in ${topdir}hdl/assemblies/$assy/$assy.xml
}

function help_create {
  cat <<EOF | $pager

usage: ocpidev [options] create|delete <noun> <name>

'ocpidev create|delete' will create/delete OpenCPI assets of type specified by the noun(s) provided.

Choices:
  <noun>: project, registry, application, spec, protocol, test, library,
          worker, <model> <<model>_noun>

  <model>: hdl
  <hdl_noun>: assembly, card, slot, device, platform, primitive <hdl_primitive_noun>
  <hdl_primitive_noun>: library, core

---------------------------------------------------------------------------------------------------
Options for the create|delete verbs:
 == create project ==
  -D <project>       *Another project that this project depends on (use package-ID, e.g. ocpi.core)
                      (adds to "ProjectDependencies" in Project.mk)
  --register         Register the project in the project registry
                      (for other projects to optionally depend on)

 == create project|library ==
  -N <pkg-name>      Package name (default: name arg in 'ocpidev create project <name>' command)
  -F <pkg-prefix>    Package prefix (default: 'local' for projects, package-ID of parent otherwise)
                     Defaults to package-ID of parent for libraries.
  -K <package-ID>    Package-ID (default: pkg-prefix.pkg-name. Overrides -N and -F if set)

 == create|delete application ==
  -X                 Indicates that the application is simply an XML file, not a directory
  -x                 For use only without -X.  The application's directory has an
                     application XML file rather then the default (an ACI file e.g. <app>.cc)

 == create|delete component|protocol ==
  -p                 Place specs/protocols at project level, not in a library's \"specs\" directory

 == create component ==
  -t                 When creating a spec, also create a test skeleton: <component>.test.
                     This is invalid if '-p' is specified
  -n                 This spec has no lifecycle/configuration interface (no runtime properties)

 == create|delete component|protocol|test|worker|{hdl device} ==
  -l <library>                  Component library to add the asset to {when there is more than one
                                library}
  --hdl-library <hdl-library>   For BSPs: A library underneath hdl/ to add assets to.  Default is
                                "devices".  Valid <hdl-library> options are "devices", "cards" and
                                "adapters"

 == create worker|{hdl device} ==
  -S <spec>          The component spec that this worker implements (default is <worker>[-_]spec)
  -L <language>      Specify the language when creating a worker
  -V <slave>         Specify the slave worker when creating a proxy worker
  -E <emulates>      Specify the device (worker) when creating an emulator worker
  -P <platform>      Create this worker in the devices library underneath the specified platform
  --worker-version   Specify the worker API version
     <version>
  -W <worker>        *A worker to include (adds to "Workers" variable in worker Makefile)
                      Use <worker>:<spec> if spec name is different from worker name
  -R <rcc-static>    *Add an RCC static prerequisite library dependency to an RCC worker
                      (adds to "RccStaticPrereqLibs" in Makefile)
  -r <rcc-dynamic>   *Add an RCC dynamic prerequisite library dependency to an RCC worker
                      (adds to "RccDynamicPrereqLibs" in Makefile)

 == create worker|{hdl device|primitive|platform} ==
  -O <other>         *Other source files to include (adds to "SourceFiles" in Makefile)
  -C <core>          *Core to be included by this asset (adds to "Cores" in Makefile)

 == create project|library|worker|{hdl device|platform} ==
  -A <xml-include>   *A directory to search for XML files (adds to "XmlIncludeDirs" in Makefile)
  -I <include>       *A directory to search for language include files (e.g. C/C++ or Verilog)
                      (Adds to "IncludeDirs" in Makefile)
  -y <comp-library>  *Specify a component library to search for workers/devices/specs that this
                      asset references (adds to "ComponentLibraries" in Makefile)
  -Y <library>       *Specify a primitive library that this asset depends on
                      (adds to "Libraries" in Makefile)

 == create worker|{hdl device|primitive|platform|assemblies} ==
  -T <target>        *Specify one of the build-targets to limit this asset to
                      (adds to "OnlyTargets" in Makefile)
  -Z <target>        *Exclude a build-target for this asset (add to "ExcludeTargets" in Makefile)
  -G <platform>      *Specify one of the build-platforms to limit this asset to
                      (adds to "OnlyPlatforms" in Makefile)
  -Q <platform>      *Exclude a build-platform for this asset
                      (adds to "ExcludePlatforms" in Makefile)
  -U <supports>      *A device (worker) supported by the subdevice being created

 == create hdl primitive core ==
  -M <top-module>    Top level module name (default is name of the core) (sets "Top" in Makefile)
  -B <core>          Specify a prebuilt (not source) core (ie. *.ngc or *.edf file)
                     (sets "PreBuiltCore" in Makefile)

 == create hdl primitive library ==
  -H                 Primitive does not depend on any libraries (sets "HdlNoLibraries" in Makefile)
  -J                 Do not elaborate this HDL library (sets "HdlNoElaboration" in Makefile)
                     (limited tool support)

 == create hdl platform ==
  -g <hdl-part>      Specify the part (die-speed-package, e.g. xc7z020-1-clg484) for platform.
                     This sets the HdlPart_<platform> variable in <platform>.mk
  -q <time-freq>     Specify the time server frequency for this platform (i.e. "100e6").
                     If specified, this may need to be modified later in the platform OWD.
  -u                 Do not use SDP (legacy option for creating platforms that do not use SDP)

Create examples:
  ocpidev create project my-project
    (creates a project with name 'my-project'. This project's package-ID (and therefore its
     name in the registry) will be 'local.my-project')

  ocpidev create project my-project -F my.organization
    (creates a project with PackagePrefix 'my.organization' and name 'my-project'.
     This project's package-ID (and therefore its registry ID) will be 'my.organization.my-project')

  ocpidev -d ~/workspace create registry my-registry
    (creates a project-registry ~/workspace/my-registry with the CDK installed. Note that a new
     registry will not have a core project registered, so the next thing to do is locate and
     register a core project. Finally, 'ocpidev set registry ~/workspace/my-registry' can be used
     from within a project to tell that project to use the new registry. None of this is necessary
     if using the default project-registry)

  ocpidev create library components
    (create the default 'components' library for the project. If a name other than
     'components' is provided, it will be created underneath the 'components' directory)

  ocpidev create spec mycomp
    (create a component spec with name 'mycomp'. It will be placed in the default location,
     which is components/specs. Provide '-l <library-name>' if there are sub-libraries
     underneath 'components'. Use the -t option to generate a component unit test as well)

  ocpidev create test mycomp
    (create a component unit test for component spec 'mycomp'. It will be placed in the default
     location, which is components/mycomp.test. Provide '-l <library-name>' if there are
     sub-libraries underneath 'components')

  ocpidev create worker myworker.hdl -S mycomp-spec
    (create an HDL worker named myworker that implements the 'mycomp' spec. If the worker
     was instead named 'mycomp.hdl', the '-S mycomp-spec' argument can be omitted because the
     default spec is <worker-name>-spec.xml)

For a complete list of options and further documentation of ocpidev, please reference the
OpenCPI Component Development Guide.
EOF
exit 1
}

function help_build {
  cat <<EOF | $pager

usage: ocpidev [options] build|clean [<noun> [<name>]]

'ocpidev build|clean' will compile/clean the OpenCPI asset specified by the noun(s) provided.
If no nouns are provided, ocpidev will build/clean the current directory/asset.

Choices:
  <noun>: project, application, test, library, worker, <model> <<model>_noun>

  <model>: hdl
  <hdl_noun>: assembly, device, platform, primitive <hdl_primitive_noun>
  <hdl_primitive_noun>: library, core

---------------------------------------------------------------------------------------------------
Options for the build|clean verbs:
  --rcc              Build rcc workers (when building at the project or library level)
  --hdl              Build hdl workers/assets (when building at the project or library level)
  --worker <worker>  *Add a worker to be specificially built and don't build any other workers
  --clean-all        Force cleaning of all project registry related files (This option should
                     only be uses for SCM checkins)
  --no-assemblies    Do not build HDL assemblies (when building at the project level)
  --hdl-assembly <assembly>
                     *Add an assembly to be built instead of building all assemblies
  --hdl-target <target>
                     *Add an HDL target to build for
  --hdl-platform <platform>
                     *Add an HDL platform to build for
  --rcc-platform <platform>
                     *Add an RCC platform to build for (default: host RCC platform)
  --hdl-rcc-platform <hdl-platform>
                     *Add an RCC platform to build for based on the provided HDL platform
                      E.g. "zed" (HDL platform) evaluates to "xilinx13_3" (RCC platform)
Options with * may occur multiple times

Build examples:
  ocpidev build --hdl-platform zed --rcc-platform xilinx13_3
    (build the current asset/directory and those underneath it for the 'zed' HDL platform and
     'xilinx13_3' RCC platform)

  ocpidev build project assets --hdl-platform zed --rcc-platform xilinx13_3
    (builds the assets project for the 'zed' HDL platform and 'xilinx13_3' RCC platform.
     Omit the name 'assets' if inside the project)

  ocpidev -l dsp_comps build worker complex_mixer.hdl --hdl-platform zed --rcc-platform xilinx13_3
    (inside the assets project, builds the complex_mixer.hdl worker in the dsp_comps library
     for the 'zed' HDL platform and 'xilinx13_3' RCC platform. The '-l dsp_comps' can be omitted
     if operating in a project with only a single top-level 'components' library)

  ocpidev build library dsp_comps --hdl-platform zed --rcc-platform xilinx13_3
    (inside the assets project, builds the dsp_comps library located at components/dsp_comps
     for the 'zed' HDL platform and 'xilinx13_3' RCC platform. Note that the library NOUN
     differs from the '-l' OPTION in that the noun is used when building the library itself,
     and the option is used when building an asset inside the library)

  ocpidev build library dsp_comps  --worker complex_mixer.hdl --worker cic_dec.hdl --hdl-platform zed
    (inside the assets project, build the dsp_comps library, but limit the build to the
     complex_mixer.hdl and cic_dec.hdl workers. Note that the --worker OPTION differs from the
     worker NOUN in that the noun is used to build a single worker, while the --worker option is
     used when building a LIBRARY to limit the build to an individual worker (or a list of workers
     if --worker is specified multiple times). If building a library without the --worker option,
     all workers will be built)

Clean examples:
  ocpidev clean
    (cleans the current asset/directory and those underneath it)

  ocpidev clean project assets
    (cleans the assets project. Omit the name 'assets' if inside the project)

  ocpidev -l dsp_comps clean worker fir_real_sse.hdl
    (inside the assets project, this will clean the fir_real_sse.hdl worker in the
     dsp_comps library)

  ocpidev clean library dsp_comps
    (inside the assets project, this will clean the dsp_comps library
     located at components/dsp_comps)

For a complete list of options and further documentation of ocpidev, please reference the
OpenCPI Component Development Guide.
EOF
}

function help_reg {
  cat <<EOF | $pager

usage: ocpidev [options] [un]register project [<name>]

'[un]register' is used with the noun "project" to [un]register a project in the project-registry.

Omitting the <name> argument will assume that the [un]register command should be executed for the
current project.

Examples:
  ocpidev register project assets
    (register the assets project. From inside the assets project, the name 'assets' can be omitted)

  ocpidev unregister project assets
    (unregister the assets project. From inside the assets project, the name 'assets' can be
     omitted.)

  ocpidev unregister project ocpi.assets
    (unregister the project currently registered under the package-ID 'ocpi.assets'.
     You do not need to be in the same directory as that project to issue this command)

For a complete list of options and further documentation of ocpidev, please reference the
OpenCPI Component Development Guide.
EOF
}

function help_set {
  cat <<EOF | $pager

usage: ocpidev [options] [un]set registry [<name>]

'[un]set' is used with the noun "registry" to [un]set the registry used by the current project.

'ocpidev set registry' with no <name> argument will set the current project to use the default
project-registry as determined from your environment.

'ocpidev set registry' with a <name> argument will set the current project to use the registry
indicated by <name>. <name> should be a path to a valid project registry.

'ocpidev unset registry' does not require a <name> argument. This command removes any link from
this project to a project-registry. The next time this project is built, the default registry will
be assumed as determined from the environment.

Defaults:
  The default registry is determined as follows:
    OCPI_PROJECT_REGISTRY_DIR environment variable if set
    Otherwise: OCPI_CDK_DIR/../project-registry

  To change your environment's default project-registry, set the OCPI_PROJECT_REGISTRY_DIR
  environment variable.

Examples:
  ocpidev set registry
    (set the current project's registry to the default)

  ocpidev set registry ~/workspace/my-registry
    (set the current project's registry to the directory at ~/workspace/my-registry)

  ocpidev unset registry
    (unset the current project's registry link. Next time this project is built, the default
     registry will be used)

Notes:
  The 'set' verb is useful after creating a new project-registry. For example, a user might want
  to create his/her own sandbox environment using a new project-registry. To do this, a user would
  create a new registry using 'ocpidev create registry <new-registry>', and then point the current
  project to that registry by running 'ocpidev set registry <new-registry>' from with the project.
  The user would also need to create and register a core project into that new registry.

  To change the default project-registry, set the OCPI_PROJECT_REGISTRY_DIR environment variable.

For a complete list of options and further documentation of ocpidev, please reference the
OpenCPI Component Development Guide.
EOF
}

function help_refresh {
  cat <<EOF | $pager

usage: ocpidev [options] refresh project

'refresh' is used with the noun "project" to generate metadata about a project to be used by other framework tools.

Examples:
  ocpidev refresh project
    (refresh current project's metadata file(s))


Notes:
  The 'refresh' verb is useful if an asset is created outside of ocpidev and the metadata gets
  out of sync with what is in the actual project.

For a complete list of options and further documentation of ocpidev, please reference the
OpenCPI Component Development Guide.
EOF
}

function help {
  if [ -n "$PAGER" ]; then
    pager=$PAGER
  else
    pager="less"
  fi
  if [[   "$verb" == create   || "$verb" == delete     ]]; then
    help_create
  elif [[ "$verb" == build    || "$verb" == clean      ]]; then
    help_build
  elif [[ "$verb" == register || "$verb" == unregister ]]; then
    help_reg
  elif [[ "$verb" == set      || "$verb" == unset      ]]; then
    help_set
  elif [[ "$verb" == refresh                           ]]; then
    help_refresh
  else
    cat <<EOF | $pager

The ocpidev command performs developer functions on/in projects for OpenCPI.
---------------------------------------------------------------------------------------------------
usage: ocpidev [options] <verb> [<noun> [<name>]]

Choices:
  <verb>: create, delete, build, clean, [un]register, [un]set, show, refresh, run
  <noun>: project, registry, application, component, protocol, test, library,
          worker, <model> <<model>_noun>

  <model>: hdl
  <hdl_noun>: assembly, card, slot, device, platform, primitive <hdl_primitive_noun>
  <hdl_primitive_noun>: library, core

Verb descriptions:
  create         Create an OpenCPI asset for development
  delete         Delete an OpenCPI asset
  build          Compile an OpenCPI asset for the [optionally] specified platforms
  clean          Clean an OpenCPI asset
  [un]register   (Un)register a project so that other projects can(not) depend on its assets
  [un]set        (Un)set a project's registry to control which project-registry it references when
                 searching for assets
  show           Print listings or details of OpenCPI assets in projects in the current registry
  refresh        Regenerate any project metadata
  run            Run unit-tests and applications

'ocpidev --version' will show the released version of OpenCPI

'ocpidev --help <verb>' will show help for the specified verb

---------------------------------------------------------------------------------------------------
Common options (apply to all verb/ combinations):
  --help             Print the ocpidev help screen. Each verb also has its own help screen
  -v                 Be verbose, describing what is happening in more detail
  -f                 Force without asking. delete and unregister ask for confirmation without this
  -d <directory>     A directory in which to perform the function, e.g. 'cd' to a directory first

For a complete list of options and further documentation of ocpidev, please reference the
OpenCPI Component Development Guide.
EOF
  fi
# Hidden to avoid clutter. Only intended for advanced users:
#   nouns:properties, signals
# Unsupported:
#   -s                 Operate ocpidev in standalone mode (i.e. outside a project)
  exit 1
}

function myshift {
  unset argv[0]
  argv=(${argv[@]})
}
function takeval {
  [ -z "${argv[1]}" ] && bad missing value after ${argv[0]} option for $1
  [[ "${argv[1]}" == -* ]] && bad value after ${argv[0]} option: ${argv[1]}
  eval $1=\'${argv[1]}\'
  # eval echo ARGV:$1:\$\{$1\}
  myshift
}

set -e
[ "$#" == 0 -o "$1" == -help -o "$1" == --help -o \( "$1" == -h -a -z "$2" \) ] && help_screen=true

# Collect all flag arguments and do preliminary error checking that does not depend
# on other potential flags.
argv=($*)
# Keep an untouched argv to be passed to other executables
original_argv=($*)
while [[ "${argv[0]}" != "" ]] ; do
  if [[ "${argv[0]}" == show ]] ; then
    ocpishow_options=`sed -E 's/(^| )show( |$)/ /' <<< "${original_argv[@]}"`
    $OCPI_CDK_DIR/scripts/ocpishow.py $ocpishow_options
    exit $?
  elif [[ "${argv[0]}" == utilization ]] ; then
    ocpiutilization_options=`sed -E 's/(^| )utilization( |$)/ /' <<< "${original_argv[@]}"`
    $OCPI_CDK_DIR/scripts/ocpidev_utilization.py $ocpiutilization_options
    exit $?
  elif [[ "${argv[0]}" == run ]] ; then
    ocpidev_run_options=`sed -E 's/(^| )run( |$)/ /' <<< "${original_argv[@]}"`
    $OCPI_CDK_DIR/scripts/ocpidev_run.py $ocpidev_run_options
    exit $?
  elif [[ -n "$verb" && -n "$help_screen" ]]; then
    help
  elif [[ "${argv[0]}" == -* ]] ; then
    case "${argv[0]}" in
      (-v) verbose=1;;
      (-k) keep=1;;
      (-f) force=1;;
      (-d)
        takeval directory
        [ ! -d $directory ] && bad the directory \"$directory\" is not a directory or does not exist
        ;;
      (-p) project=1;;
      (-t) createtest=1;;
      (-n) nocontrol=1;;
      (-l)
        # if a library is mentioned, it must exist.
	takeval library # default is <components>
        liboptset=1    # the library variable can be set elsewhere (ie card option)
                       # This variable lets us keep track of whether the -l option
        ;;             # was used
      (-h | --hdl-library)
        libbase=hdl
        takeval hdllibrary
        library=hdl/$hdllibrary
        case "$hdllibrary" in
          (devices|cards|adapters) ;;
          (*)
            error_msg="the only valid libraries underneath hdl/ are devices, cards and adapters."
            if [ -n "$verb" ]; then
              bad "$error_msg"
            fi
            postponed_error="$error_msg"
            ;;
        esac
        hdlliboptset=1
        ;;
      (-s) standalone=1;;
      # project options
      (-N) takeval packagename; wname=$packagename ;; #internally defaults to dirname (except components)
      (-F) takeval packageprefix ;; #default is 'local' for projects (internally defaults to package-id of parent for libraries)
      (-K) takeval package ;; #default is empty. internally, this defaults to prefix.name
      (--register) register_enable=1 ;;
      (-D) takeval dependency; dependencies=(${dependencies[@]} ${dependency//:/ }) ;;
      # worker options
      (-S) takeval spec ;; # default for worker is <worker-name>-spec.xml
      (-P) takeval platform ;;
      (-L) takeval language ;;
      (-V) takeval slave ;;
      (-E) takeval emulates ;;
      (-W) takeval worker; workers=(${workers[@]} $worker) ;;
      (-I) takeval include; includes=(${includes[@]} $include) ;;
      (-A) takeval xmlinclude; xmlincludes=(${xmlincludes[@]} $xmlinclude) ;;
      (-Y) takeval liblib; liblibs=(${liblibs[@]} $liblib) ;;
      (-y) takeval complib; complibs=(${complibs[@]} $complib) ;;

      (-R) takeval rccstatprereq; rccstatprereqs=(${rccstatprereqs[@]} $rccstatprereq) ;;
      (-r) takeval rccdynprereq; rccdynprereqs=(${rccdynprereqs[@]} $rccdynprereq) ;;

      (-g) takeval hdlpart ;;
      (-q) takeval timefreq ;;
      (-u) nosdp=1 ;;
      (-ll) takeval loglevel; export OCPI_LOG_LEVEL=$loglevel;;

      (-O) takeval other; others=(${others[@]} $other) ;;
      (-C) takeval core; cores=(${cores[@]} $core) ;;

      (-T) takeval target; targets=(${targets[@]} $target) ;;
      (-Z) takeval extarget; extargets=(${extargets[@]} $extarget) ;;

      (-G) takeval onlyplat; onlyplats=(${onlyplats[@]} $onlyplat) ;;
      (-Q) takeval explat; explats=(${explats[@]} $explat) ;;

      (-U) takeval supports; supported=(${supported[@]} $supports) ;;
      # hdl primitive options
      (-M) takeval module ;;
      (-B) takeval prebuilt ;;
      # hdl primitive libraries
      (-H) hdlnolib=1 ;;
      (-J) hdlnoelab=1 ;;
      # for apps
      (-X) xmlapp=1;;
      (-x) xmldirapp=1;;
      (-help|--help)  help_screen=true;;
      # for building
      (--clean-all) hardClean=1;;
      (--build-rcc|--rcc) buildRcc=1;;
      (--build-hdl|--hdl) buildHdl=1;;
      (--worker) takeval curWorker; workerList="${workerList[@]} $curWorker";;
      (--build-no-assemblies|--no-assemblies) buildNoAssemblies=1;;
      (--build-hdl-assembly|--build--assembly|--hdl-assembly) takeval assy; assys="${assys[@]} $assy";;
      (--build-hdl-target|--hdl-target) takeval hdltarget; hdltargets="${hdltargets[@]} $hdltarget";;
      (--build-hdl-platform|--hdl-platform) takeval hdlplat; hdlplats="${hdlplats[@]} $hdlplat";;
      (--build-rcc-platform|--rcc-platform) takeval swplat; swplats="${swplats[@]} $swplat";;
      (--build-rcc-hdl-platform|--rcc-hdl-platform) takeval hwswplat; hwswplats="${hwswplats[@]} $hwswplat";;
      # OCPI_API_DEPRECATED 2.0 (AV-3457, specific search string)
      (--build-hdl-rcc-platform|--hdl-rcc-platform) warn "${argv[0]} is deprecated: use --rcc-hdl-platform to specify RCC platform using HDL name"; takeval hwswplat; hwswplats="${hwswplats[@]} $hwswplat";;
      (--create-build-files) OCPI_CREATE_BUILD_FILES=1;;
      (--version) ocpirun --version; exit 0;;
      (--worker-version) takeval version;;
      (*)
        error_msg="unknown option: ${argv[0]}"
        if [ -n "$verb" ]; then
          bad "$error_msg"
        fi
        postponed_error="$error_msg"
        ;;
    esac
  elif [ -z "$verb" ]; then
    case "${argv[0]}" in
      (create) verb=create;;
      (delete|rm) verb=delete;;
      (build) verb=build;;
      (clean) verb=build; buildClean=1;;
      (reg|register) verb=register;;
      (unreg|unregister) verb=unregister;;
      (set) verb="set";;
      (unset) verb="unset";;
      (show) verb=show;;
      (refresh) verb=refresh;;
      ("") bad missing command name;;
      (*) bad there is no command named \"${argv[0]}\";;
    esac
  elif [ -z "$noun" ]; then
    if [ -n "$hdl" ]; then
      libbase=hdl
      case "${argv[0]}" in
        (library)
          if [ "${verb}" != "build" ]; then
            echo "Warning: user creation of libraries underneath hdl/ is not fully supported."
            echo "         You probably just want to create a components library using 'ocpidev create library ...'"
          fi
          noun=${argv[0]}
          ;;
        (device|subdevice|platform|platforms|assembly|assemblies|signals) noun=${argv[0]} ;;

        (card|slot)
          noun=${argv[0]}
          card=1
          library=hdl/cards
          ;;
        (primitive)
          noun=${argv[0]}
	  myshift
	  case ${argv[0]} in
            (core|library)
              args=($args ${argv[0]});;
            (*) bad bad primitive type \"${argv[1]}\", must be \"library\" or \"core\";;
          esac
          ;;
        (primitives)
          noun=${argv[0]}
          ;;
        (*) bad invalid noun after \"hdl\": ${argv[0]} ;;
      esac
    elif [ "${argv[0]}" == hdl -o "${argv[0]}" = HDL ]; then
      hdl="hdl "
    elif [ "${argv[0]}" == rcc -o "${argv[0]}" = RCC ]; then
      rcc="rcc "
    elif [ -n "$rcc" ]; then
      case "${argv[0]}" in
        (platform|platforms) noun=${argv[0]} ;;
        (*) bad invalid argument \"${argv[0]}\" after rcc;;
      esac
    else
      case "${argv[0]}" in
        (device|platform|platforms|assembly|assemblies|card|slot|primitive|signals) bad the noun '"'${argv[0]}'"' must follow '"'hdl'"' ;;
        (*) noun=${argv[0]} ;;
      esac
    fi
  else
    args=($args ${argv[0]})
  fi
  myshift
done
#todo move this up where the other ones are when its not just project creation
if [ "$verb" == "create" -a "$noun" == "project" ]; then
  ocpidev_create_options=`sed -E 's/(^| )create( |$)/ /' <<< "${original_argv[@]}"`
  $OCPI_CDK_DIR/scripts/ocpicreate.py $ocpidev_create_options
  exit $?
fi
if [ -n "$help_screen" ]; then
  help
fi
if [ -n "$postponed_error" ]; then
  bad $postponed_error
fi
# option checking independent of noun and verb
[ -n "$project" -a -n "$library" ] && bad the -l and -p options are mutually exclusive.
[ -n "$standalone" -a -n "$library" ] && bad the -l and -s options are mutually exclusive.
[ -n "$card" -a \( -n "$liboptset" -o -n "$hdlliboptset" \) ] && bad the -l and -C and -h options are mutually exclusive
[ -n "$liboptset" -a -n "$hdlliboptset" ] && bad the -l and -h options are mutually exclusive
[ -n "$xmlapp" -a -n "$xmldirapp" ] && bad the -X and -x options are mutually exclusive
# Now error check the options when we have all the options
if [ -n "$directory" ]; then
  cd $directory
  topdir=$directory/
else
  directory=$(pwd)
  topdir=
fi
if [ "$noun" == "component" ]; then
  noun=spec
fi
[ -z "$verb" ] && bad missing verb, nothing to do
[ -z "$noun" -a "$verb" != build ] && help
# option testing dependent on noun and verb
[ -z "$args" -a "$noun" != primitives -a "$noun" != assemblies  -a "$noun" != applications -a "$noun" != platforms -a "$verb" != build -a "$verb" != register -a "$verb" != unregister -a "$verb" != refresh -a "$verb" != set -a "$verb" != unset -a \( "$verb" != delete -a "$noun" != registry \) -a \( "$verb" != create -a "$noun" != library -a \( -n "$liboptset" -o -n "$hdlliboptset" -o -n "$platform" \) \) ] && bad there must be a name argument after: $verb $hdl$noun
# force
[ -n "$force" -a "$verb" != delete -a "$verb" != unregister -a "$verb" != register ] &&
  bad the -f '(force)' option is only valid when deleting or \(un\)registering an asset
# project
[ -n "$project" -a \( "$noun" != spec -a "$noun" != protocol -a "$noun" != properties -a "$noun" != signals \) ] &&
  bad the -p '(project level)' option is only valid when creating a spec or a protocol
# create test
[ -n "$createtest" -a \( "$noun" != spec \) ] &&
  bad the -t '(create test)' option is only valid when creating a component spec
# nocontrol=true
[ -n "$nocontrol" -a \( "$noun" != spec \) ] &&
[ -n "$nocontrol" -a \( "$noun" != spec \) ] &&
  bad the -n '(nocontrol)' option is only valid when creating a component spec
# card
[ -n "$card" -a \( "$noun" != card -a "$noun" != slot -a "$noun" != worker -a "$noun" != spec -a "$noun" != protocol -a "$noun" != properties -a "$noun" != signals -a "$noun" != device -a "$noun" != test \) ] &&
  bad can only operate in hdl/cards when creating/deleting an HDL device worker, test, worker, spec, protocol, properties or signals
# library
[ \( -n "$liboptset" -o -n "$hdlliboptset" \) -a \( -n "$platform" -o \( "$noun" != worker -a "$noun" != spec -a "$noun" != protocol -a "$noun" != properties -a "$noun" != signals -a "$noun" != library -a "$noun" != device -a "$noun" != test \) \) ] &&
  bad the -l or --hdl-library '(library within project)' options are only valid when creating a spec, protocols, properties, signals, library, worker, device or test
# standalone
[ -n "$standalone" -a \( "$noun" != library -a "$noun" != worker -a "$noun" != device \) ] &&
  bad the -s '(standalone - outside a project)' option is only valid when creating a library, a worker or a device
# packagename
[ -n "$packagename" -a \( "$verb" != create -o "$noun" != project -a "$noun" != library -a "$noun" != worker -a "$noun" != device \) ] &&
  bad the -N '(packagename)' option is only valid when creating a project or library
# packageprefix
[ -n "$packageprefix" -a \( "$verb" != create -o "$noun" != project -a "$noun" != library \) ] &&
  bad the -F '(packageprefix)' option is only valid when creating a project or library
# package
[ -n "$package" -a \( "$verb" != create -o \( "$noun" != project -a "$noun" != library \) \) ] &&
  bad the -K '(package)' option is only valid when creating a project or library
# dependencies
[ -n "$dependencies" -a \( "$verb" != create -o "$noun" != project \) ] &&
  bad the -D '(dependencies)' option is only valid when creating a project
# spec
[ -n "$spec" -a \( "$verb" != create -o \( "$noun" != worker -a "$noun" != device -a "$noun" != test \) \) ] &&
  bad the -S '(spec)' option is only valid when creating a worker or device or test
# platform
[ -n "$platform" -a \( "$noun" != spec -a "$noun" != device -a "$noun" != worker -a "$noun" != test -a "$noun" != library \) ] &&
  bad the -P '(platform)' option is only valid when creating an HDL device worker, worker, test, or a platform\'s devices library
# language
[ -n "$language" -a \( "$verb" != create -o \( "$noun" != device -a "$noun" != worker \) \) ] &&
  bad the -L '(language)' option is only valid when creating a worker
# slave
[ -n "$slave" -a \( "$verb" != create -o "$noun" != worker \) ] &&
  bad the -V '(slave worker)' option is only valid when creating a worker
# workers
[ -n "$workers" -a \( "$verb" != create -o \( "$noun" != worker \) \) ] &&
  bad the -W '(one of multiple workers)' option is only valid when creating a worker
# emulate
[ -n "$emulates" -a \( "$verb" != create -o "$noun" != device \) ] &&
  bad the -E '(emulates)' option is only valid when creating an HDL device worker
[ -n "$supported" -a "$noun" != device ] &&
  bad the -U '(supports)' option is only valid when creating an HDL device worker
# module
[ -n "$module" -a \( "$verb" != create -o "$noun" != primitive -a "${args[0]}" != core \) ] &&
  bad the -M '(module)' option is only valid when creating an HDL primitive core
# prebuilt
[ -n "$prebuilt" -a \( "$verb" != create -o \( "$noun" != primitive -o "${args[0]}" != core \) \) ] &&
  bad the -B '(prebuilt)' option is only valid when creating an HDL primitive core
# targets/platforms limiting
[ \( -n "$targets" -o -n "$extargets" -o -n "$onlyplats" -o -n "$explats" \) -a \( "$verb" != create -o \( "$noun" != worker -a "$noun" != device -a "$noun" != primitive -a "$noun" != assembly \) \) ] &&
  bad the -T, -Z, -G or -Q, '(only/exclude targets/platforms)' options are only valid when creating a worker, device, primitive, or assembly
# platform options
[ \( -n "$hdlpart" -o -n "$timefreq" -o -n "$nosdp" \) -a \( "$verb" != create -o "$noun" != "platform" \) ] &&
  bad the -g, -q, or -u '(hdl-part, time-freq, no-sdp)' are only valid when creating an hdl platform.
# includes
[ \( -n "$includes" \) -a \( "$verb" != create -o \( "$noun" != "worker" -a "$noun" != "device" -a "$noun" != "platform" -a "$noun" != "library" -a "$noun" != "project" \) \) ] &&
  bad the -I '(includes)' option is only valid when creating a worker, device, platform, library, or project
# xml includes
[ \( -n "$xmlincludes" \) -a \( "$verb" != create -o \( "$noun" != "worker" -a "$noun" != "device" -a "$noun" != "platform" -a "$noun" != "library" -a "$noun" != "project" \) \) ] &&
  bad the -A '(xmlincludes)' option is only valid when creating a worker, device, platform, library, or project
# primitive library options
[ \( -n "$hdlnoelab" -o -n "$hdlnolib" \) -a \( "$verb" != create -o \( "$noun" != "primitive" -o  "${args[0]}" != library \) \) ] &&
  bad the -H or -J '(hdlnolib, hdlnoelab)' options are only valid when creating an hdl primitive library.
# rccprereqs FIXME: Should limit this makevariable to rcc workers only
[ \( -n "$rccstatprereqs" -o -n "$rccdynprereqs" \) -a \( "$verb" != create -o \( "$noun" != "worker" \) \) ] &&
  bad the -R or -r '(rcc static/dynamic prereq)' option is only valid when creating a proxy worker
# libraries include
[ \( -n "$liblibs" \) -a \( "$verb" != create -o \( "$noun" != "worker" -a "$noun" != "device" -a "$noun" != "platform" -a "$noun" != "primitive" -a "$noun" != "library" -a "$noun" != "project" \) \) ] &&
  bad the -Y, '(libraries)' option is only valid when creating a worker, device, platform, primitive, library, or project
# componentlibraries reference
[ \( -n "$complibs" \) -a \( "$verb" != create -o \( \( "$noun" != "worker" -o -z "$slave" \) -a \( \( "$noun" != "device" \) -o \( -z "$emulates" -a -z "$supported" \) \) -a "$noun" != "platform" -a "$noun" != "library" -a "$noun" != "project" \) \) ] &&
  bad the -y, '(component-libraries)' option is only valid when creating a proxy worker, device emulator, subdevice, platform, library, or project
# xml app
[ \( -n "$xmlapp" -o -n "$xmldirapp" \) -a \( \( "$verb" != create -a "$verb" != delete \) -o "$noun" != "application" \) ] &&
  bad the -X '(xml/simple)' or -x '(application directory with xml)' option is only valid when creating an application

# For future support of rcc platforms, primitives, and in general the rcc subtree
if [ -n "$rcc" ] ; then
  hdlorrcc=rcc
else #if [ -n "$hdl" ] ; then
  hdlorrcc=hdl
fi

# Get dir type unless creating something standalone
needname ${argv[0]} $verb $noun

# if verb is not build, if it is build and it has a noun, unless the noun is test with no argumnets
if [ \( "$verb" != "build" \) -o \( -n "$noun" -a ! \( "$noun" == "test" -a -z "$args" \) \) ] ; then
  get_subdir
fi

if [ "$verb" == "build" -a -z "$buildClean" ]; then
  pjtop=$(if [ "$noun" == "project" ]; then cd $subdir/${args[0]}; fi; get_project_top)
  if [ -n "$pjtop" ]; then
    (cd $pjtop && make imports)
  fi
fi
[ -z "$verbose" ] || echo Executing the \"$verb $hdl$noun\" command in a ${dirtype:-unknown type of} directory: $directory.
case $noun in
  (project)      do_project ${args[@]} ;;
  (registry)     do_registry ${args[@]} ;;
  (application)  do_application ${args[@]} ;;
  (applications) do_applications ${args[@]} ;;
  (protocol)     do_protocol_or_spec ${args[@]} ;;
  (spec)         do_protocol_or_spec ${args[@]} ;;
  (properties)   do_protocol_or_spec ${args[@]} ;;
  (library)      do_library ${args[@]} ;;
  (worker)       do_worker ${args[@]} ;;
  (test)         do_test ${args[@]} ;;
  (platforms)    do_hdl_platforms ${args[@]} ;;
  (platform)     if [ -n "$hdl" ] ; then
                   do_hdl_platform ${args[@]}
#                 else
#                   do_rcc_platform ${args[@]}
                 fi
                 ;;
  # Below are hdl exclusive commands
  (assembly)     do_assembly ${args[@]} ;;
  (assemblies)   do_assemblies ${args[@]} ;;
  (card)         do_card_or_slot ${args[@]} ;;
  (slot)         do_card_or_slot ${args[@]} ;;
  (device)       do_device ${args[@]} ;;
  (signals)      do_protocol_or_spec ${args[@]} ;;
  (primitive)    do_primitive ${args[@]} ;;
  (primitives)   do_primitives ${args[@]} ;;
  (*)
    if [ "$verb" == "build" ] ; then
      do_build_here ${args[@]}
    else
      bad the noun \"$noun\" is invalid after the verb \"$verb\"
    fi ;;
esac
gpmd=$OCPI_CDK_DIR/scripts/genProjMetaData.py
if [ -x $gpmd ] ; then
  if [ "$verb" == "create" -a "$noun" == "project" ] ; then
    $gpmd $(pwd) force
  elif [ \( "$verb" == create -a "$noun" != registry \) \
         -o \( "$verb" == "build" -o  "$verb" == "clean" -o "$verb" == "register" \) \
         -o \( "$verb" == delete -a "$noun" != project -a "$noun" != registry \) ] ; then
    if [ "$noun" == "project" -a -n "${args[0]}" ]; then
      $gpmd ${args[0]}
    else
      $gpmd $(pwd)
    fi
  fi
fi
exit 0

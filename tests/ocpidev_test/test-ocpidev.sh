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


if [[ "$1" == "-h" || "$1" == "-help" || "$1" == "--help" ]] ; then
echo "This script creates a project and every type of asset,
builds them, cleans them, and deletes them.
Optional \$1 is the test-project name to create
ONLY_CREATE=1 skips the build/clean/delete stages
ONLY_CREATE_BUILD=1 skips delete
NO_BUILD=1 skips build/clean"
exit
fi
set -e

# ocpidev verbosity
#V=" -v"
# Less verbose: redirect ocpidev output to /dev/null
#QUIET=1
# Trace commands executed
#set -x

case "$OCPI_LOG_LEVEL" in
  ([1-7]) V=" -v"; QUIET=1;;
  ([8-9]) V=" -v";;
  ([1][0-9]) V=" -v"; set -x;;
  (0|*) QUIET=1; echo "Valid Log Levels are 0-19. Using default=0";;
esac

OCPIBIN=$OCPI_CDK_DIR/$OCPI_TOOL_DIR/bin
OCPIDEV=$OCPIBIN/ocpidev
OCPIRUN=$OCPIBIN/ocpirun
# The caller needs to set the HDL platform if we are using it at all since they know
# which tools are installed.
if [ -z "$HDL_PLATFORM" ] ; then
  HDL_PLATFORM=isim_pf
  HDL_TARGET=isim
  if ! $OCPIRUN -v -C --only-platforms | grep '[^a-zA-Z]isim$'; then
    HDL_NO_BUILD=1
    RCC=--rcc
  fi
fi
# default the RCC platform to the one we are running on
[ -z "$RCC_PLATFORM" ] && RCC_PLATFORM=$OCPI_TOOL_PLATFORM
# strip trailing _pf from platform name
HDL_PLATFORM=${HDL_PLATFORM/%_pf/}
#echo HDL_PLATFORM:$HDL_PLATFORM RCC_PLATFORM:$RCC_PLATFORM HDL_NO_BUILD=$HDL_NO_BUILD RCC=$RCC
# This is the number of identical assets to create in EVERY library 
# including devices, platform/*/devices
if [ -z "$OCPI_NUM_ASSETS" ] ; then
  OCPI_NUM_ASSETS=1
fi
compnums1=$(seq 1 $OCPI_NUM_ASSETS)
compseq=()
for c in ${compnums1[@]} ; do
  compseq1=(${compseq1[@]} comp"$c")
done
compnums2=$(seq $(expr $OCPI_NUM_ASSETS + 1) $(expr $OCPI_NUM_ASSETS \* 2))
for c in ${compnums2[@]} ; do
  compseq2=(${compseq2[@]} comp"$c")
done

# Extract a base project, creating it at $1, using ocpidev to do everything we can.

function bad {
  echo Failed: $*
  exit 1
}

# devices in hdl/cards or hdl/platforms/*/devices may use 
# spec files or workers from hdl/devices. Therefore,
# we include directories in the search path for specs.
function do_ocpidev {
  if [ "$QUIET" == 1 ] ; then
    XmlIncludeDirs=$xmldirs $OCPIDEV $V $@ #&> /dev/null
  else
    echo "Calling: ocpidev $V $@"
    echo "         With: XmlIncludeDirs=$xmldirs" 
    XmlIncludeDirs=$xmldirs $OCPIDEV $V $@
  fi

}

pj=$1
if [ -z "$pj" ] ; then
  pj=test_project
fi

[ -n "$pj" -a ! -e "$pj" ] || bad Cannot create project \"$pj\". File/dir already exists

echo "========Creating project $pj"
do_ocpidev create project $(basename $pj) -d $(dirname $pj)
cd $pj


echo "========Creating top level specs"
for c in ${compseq1[@]}; do
  do_ocpidev create spec top_"$c" -p
  do_ocpidev create protocol top_"$c"_prot -p
done

echo "========Creating component libraries"
complibs=(dsp_comps comms_comps util_comps misc_comps)
for lib in ${complibs[@]} ; do
  do_ocpidev create library $lib

  do_ocpidev create worker "comp_$lib".hdl -l $lib -S none
  for c in ${compseq1[@]}; do
    do_ocpidev create spec $c -l $lib
    do_ocpidev create test $c -l $lib
    do_ocpidev create worker "$c".hdl -l $lib
  done
  for c in ${compseq1[@]}; do
    do_ocpidev create worker "$c".rcc -l $lib -S comp1-spec
  done
done

echo "========Creating HDL platforms"
platnames=(alst4_0 matchstiq_z1_0 ml605_0 picoflexor_0 zed_0)
for plat in ${platnames[@]} ; do
  do_ocpidev create hdl platform $plat -g $HDL_PLATFORM -q 200e6
done
#sed -ie "s/HdlPart_.*=.*/HdlPart_alst4_0=isim/g" hdl/platforms/alst4_0/alst4_0.mk

echo "========Creating primitive libraries"
for primlib in ${complibs[@]} ; do
  do_ocpidev create hdl primitive library $primlib
  do_ocpidev create hdl primitive core "$primlib"_core
done

echo "========Creating device libraries"
devlibs=(devices adapters cards)
for lib in ${devlibs[@]} ; do
  libopt=""
  if [ "$lib" != "devices" ] ; then
    libopt="-h $lib"
  fi
  do_ocpidev create hdl device "dev_$lib".hdl $libopt -S none
  Workers=" dev_$lib".hdl
  for c in ${compseq1[@]}; do
    do_ocpidev create spec $c -h $lib
    do_ocpidev create test $c -h $lib
    do_ocpidev create hdl device "$c".hdl $libopt
    Workers+=" $c".hdl
    do_ocpidev create hdl device "$c"_sub.hdl $libopt -U "$c".hdl -S none
    Workers+=" $c"_sub.hdl
    do_ocpidev create worker "$c"_proxy.rcc -h $lib -V "$c".hdl -S "$c"-spec
    Workers+=" $c"_proxy.rcc
    # This one file is copied so that this entire set of tests can run without depending
    # on built or exported projects - i.e. these tests can run in a virgin RPM installation
    [ -f specs/emulator-spec.xml ] ||
	cp $OCPI_CDK_DIR/../projects/core/specs/emulator-spec.xml specs
    do_ocpidev -v create hdl device "$c"_em.hdl $libopt -E "$c".hdl 
    Workers+=" $c"_em.hdl
  done
  # update the makefile beacuse order matters 
  cp /dev/null hdl/$lib/Makefile
  echo "Workers= $Workers" >> hdl/$lib/Makefile
  echo "include \$(OCPI_CDK_DIR)/include/library.mk" >> hdl/$lib/Makefile
done


echo "====Creating some cards, slots, and card devices"
do_ocpidev create hdl card card0
do_ocpidev create hdl slot slot0
do_ocpidev create spec comp6 -h cards
# this is broken an bug needs to be written for this
#do_ocpidev create test comp6 -h cards
do_ocpidev create hdl device comp6.hdl -h cards
do_ocpidev create hdl device comp6_sub.hdl -h cards -U "$c".hdl -S none
do_ocpidev create worker comp6_proxy.rcc -h cards -V "$c".hdl -S "$c"-spec
do_ocpidev create hdl device comp6_em.hdl -h cards -E "$c".hdl 


echo "========Creating platform device libraries"
for lib in ${platnames[@]} ; do
  do_ocpidev create hdl device "dev_$lib".hdl -P $lib -S none
  Workers=" dev_$lib".hdl
  for c in ${compseq1[@]}; do
    do_ocpidev create spec $c -P $lib
    do_ocpidev create test $c -P $lib
    do_ocpidev create hdl device "$c".hdl -P $lib
    Workers+=" $c".hdl
    do_ocpidev create hdl device "$c"_sub.hdl -P $lib -U "$c".hdl -S "$c"-spec
    Workers+=" $c"_sub.hdl
    do_ocpidev create worker "$c"_proxy.rcc -P $lib -V "$c".hdl -S "$c"-spec
    Workers+=" $c"_proxy.rcc
    do_ocpidev create hdl device "$c"_em.hdl -P $lib -E "$c".hdl 
    Workers+=" $c"_em.hdl
  done
  set -x
  cp /dev/null hdl/platforms/$lib/devices/Makefile
  echo "Workers= $Workers" >> hdl/platforms/$lib/devices/Makefile
  echo "include \$(OCPI_CDK_DIR)/include/library.mk" >> hdl/platforms/$lib/devices/Makefile
  set +x
done

echo "========Creating assemblies"
assemblies=(a1 a2)
for assemb in ${assemblies[@]} ; do
  do_ocpidev create hdl assembly $assemb
done


# Build at each possible directory-tree-level for applications
# since applications build quickly and we want to ensure the build
# works at each level.
echo "========Creating applications"
applications=(app1 app2)
for app in ${applications[@]} ; do
  do_ocpidev create application $app
  [ -n "$NO_BUILD" ] || [ -n "$ONLY_CREATE" ] || OCPI_TARGET_PLATFORM=$RCC_PLATFORM make -C applications/$app
  [ -n "$NO_BUILD" ] || [ -n "$ONLY_CREATE" ] || make clean -C applications/$app
  do_ocpidev create application "$app"_x -x
  [ -n "$NO_BUILD" ] || [ -n "$ONLY_CREATE" ] || OCPI_TARGET_PLATFORM=$RCC_PLATFORM make -C applications/"$app"_x
  [ -n "$NO_BUILD" ] || [ -n "$ONLY_CREATE" ] || make clean -C applications/"$app"_x
  do_ocpidev create application $app -X
  [ -n "$NO_BUILD" ] || [ -n "$ONLY_CREATE" ] || OCPI_TARGET_PLATFORM=$RCC_PLATFORM make -C applications
  [ -n "$NO_BUILD" ] || [ -n "$ONLY_CREATE" ] || make clean -C applications
done

echo "============OCPIDEVTEST:'show'ing HDL Platforms"
shownplats=`$OCPIDEV show hdl platforms`
echo "\"ocpidev show hdl platforms\" returned $shownplats"
for p in ${platnames[@]}; do
  echo "Confirming that \"$p\" is in the list [$shownplats]"
  [ -n "`echo $shownplats | grep \"$p\"`" ] || bad \"ocpidev show hdl platforms\" does not include platform \"$p\"
done

echo "============OCPIDEVTEST:'show' tests"
echo "ocpidev show rcc platforms"
do_ocpidev show rcc platforms
echo "ocpidev show rcc platforms --simple"
do_ocpidev show rcc platforms --simple
echo "ocpidev show rcc platforms --json"
do_ocpidev show rcc platforms --json
echo "ocpidev show hdl platforms"
do_ocpidev show hdl platforms
echo "ocpidev show hdl platforms --simple"
do_ocpidev show hdl platforms --simple 
echo "ocpidev show hdl platforms --json"
do_ocpidev show hdl platforms --json
echo "ocpidev show workers"
do_ocpidev show workers
echo "ocpidev show workers --simple"
do_ocpidev show workers --simple
echo "ocpidev show components"
do_ocpidev show components
echo "ocpidev show components --simple"
do_ocpidev show components --simple
echo "ocpidev show platforms"
do_ocpidev show platforms
echo "ocpidev show platforms --simple"
do_ocpidev show platforms --simple
echo "ocpidev show platforms --json"
do_ocpidev show platforms --json
echo "ocpidev show rcc targets"
do_ocpidev show rcc targets
echo "ocpidev show rcc targets --simple"
do_ocpidev show rcc targets --simple
echo "ocpidev show rcc targets --json"
do_ocpidev show rcc targets --json
echo "ocpidev show hdl targets"
do_ocpidev show hdl targets
echo "ocpidev show hdl targets --simple"
do_ocpidev show hdl targets --simple
echo "ocpidev show hdl targets --json"
do_ocpidev show hdl targets --json
echo "ocpidev show projects"
do_ocpidev show projects
echo "ocpidev show projects --simple"
do_ocpidev show projects --simple
echo "ocpidev show projects --simple"
do_ocpidev show projects --simple
echo "ocpidev show projects --json"
do_ocpidev show projects --json
echo "ocpidev show registry"
do_ocpidev show registry
echo "ocpidev show registry --simple"
do_ocpidev show registry --simple
echo "ocpidev show registry --json"
do_ocpidev show registry --json
echo "ocpidev show tests --json --local-scope"
do_ocpidev show tests --json --local-scope
echo "ocpidev show tests --local-scope"
do_ocpidev show tests --local-scope
echo "ocpidev show tests --local-scope --simple"
do_ocpidev show tests --local-scope --simple 
echo "ocpidev show project --local-scope"
do_ocpidev show project --local-scope
echo "ocpidev show project --local-scope --simple"
do_ocpidev show project --local-scope --simple
echo "ocpidev show project --json --local-scope"
do_ocpidev show project --json --local-scope
echo "ocpidev show libraries --json --local-scope"
do_ocpidev show libraries --json --local-scope
echo "ocpidev show libraries --local-scope"
do_ocpidev show libraries --local-scope
echo "ocpidev show libraries --local-scope --simple"
do_ocpidev show libraries --local-scope --simple

if [ "$ONLY_CREATE" == "1" ] ; then
  echo "Keeping the project and exiting before build or deletion"
  exit 0
fi

if [ -z "$NO_BUILD" ] ; then
  echo "============OCPIDEVTEST:Building applications "
  do_ocpidev build applications --build-rcc-platform $RCC_PLATFORM
  do_ocpidev build applications --rcc-platform $RCC_PLATFORM
  do_ocpidev clean applications
  echo "============OCPIDEVTEST:Building application app1 "
  do_ocpidev build application app1 --build-rcc-platform $RCC_PLATFORM
  do_ocpidev build application app1 --rcc-platform $RCC_PLATFORM
  do_ocpidev clean application app1
  echo "============OCPIDEVTEST:Building components rcc"
  do_ocpidev build library dsp_comps --build-rcc
  do_ocpidev build library dsp_comps --rcc
  do_ocpidev clean library dsp_comps --build-rcc
  do_ocpidev clean library dsp_comps --rcc
  do_ocpidev build library dsp_comps --rcc  --worker comp1.rcc
  do_ocpidev clean library dsp_comps --rcc
  echo "============OCPIDEVTEST:Building components hdl"
  do_ocpidev build library dsp_comps --build-hdl
  do_ocpidev build library dsp_comps --hdl
  do_ocpidev clean library dsp_comps --build-hdl
  do_ocpidev clean library dsp_comps --hdl
  echo "============OCPIDEVTEST:Building worker "
  do_ocpidev build worker comp1.rcc -l dsp_comps --build-rcc-platform $RCC_PLATFORM
  do_ocpidev build worker comp1.rcc -l dsp_comps --rcc-platform $RCC_PLATFORM
  do_ocpidev clean worker comp1.rcc -l dsp_comps --build-rcc-platform $RCC_PLATFORM
  do_ocpidev clean worker comp1.rcc -l dsp_comps --rcc-platform $RCC_PLATFORM
  echo "============OCPIDEVTEST:Building test rcc"
  do_ocpidev build library dsp_comps $RCC --build-rcc-platform $RCC_PLATFORM --build-hdl-platform $HDL_PLATFORM
  do_ocpidev build library dsp_comps $RCC --rcc-platform $RCC_PLATFORM --hdl-platform $HDL_PLATFORM
  do_ocpidev build test comp1.test -l dsp_comps --build-rcc-platform $RCC_PLATFORM
  do_ocpidev build test comp1.test -l dsp_comps --rcc-platform $RCC_PLATFORM
  do_ocpidev clean test comp1.test -l dsp_comps --build-rcc-platform $RCC_PLATFORM
  do_ocpidev clean test comp1.test -l dsp_comps --rcc-platform $RCC_PLATFORM
  do_ocpidev clean library dsp_comps --build-rcc
  do_ocpidev clean library dsp_comps --rcc
  echo "============OCPIDEVTEST:Building tests rcc"
  do_ocpidev build project . $RCC --build-rcc-platform $RCC_PLATFORM --build-hdl-platform $HDL_PLATFORM
  do_ocpidev build project . $RCC --rcc-platform $RCC_PLATFORM --hdl-platform $HDL_PLATFORM
  do_ocpidev build test --build-rcc-platform $RCC_PLATFORM
  do_ocpidev build test --rcc-platform $RCC_PLATFORM
  echo "============OCPIDEVTEST:Building tests rcc clean"
  do_ocpidev clean test --build-rcc-platform $RCC_PLATFORM
  do_ocpidev clean test --rcc-platform $RCC_PLATFORM
  # in this test project building platforms is not relevenat beacuse they are fake platforms
  #echo "============OCPIDEVTEST:Building platform "
  #do_ocpidev build hdl platform isim_0 
  #echo "============OCPIDEVTEST:Building platforms "
  #do_ocpidev build hdl platforms
if [ -z "$HDL_NO_BUILD" ]; then
  echo "============OCPIDEVTEST:Building primitive "
  do_ocpidev build hdl primitive library comms_comps --build-hdl-platform $HDL_PLATFORM
  do_ocpidev build hdl primitive library comms_comps --hdl-platform $HDL_PLATFORM
  do_ocpidev clean hdl primitive library comms_comps --build-hdl-platform $HDL_PLATFORM
  do_ocpidev clean hdl primitive library comms_comps --hdl-platform $HDL_PLATFORM
  echo "============OCPIDEVTEST:Building primitives "
  do_ocpidev build hdl primitives --build-hdl-platform $HDL_PLATFORM
  do_ocpidev build hdl primitives --hdl-platform $HDL_PLATFORM
  do_ocpidev clean hdl primitives --build-hdl-platform $HDL_PLATFORM
  do_ocpidev clean hdl primitives --hdl-platform $HDL_PLATFORM
  echo "============OCPIDEVTEST:Building project no assys1 "
  do_ocpidev build project . $RCC --build-hdl-platform $HDL_PLATFORM --build-no-assemblies
  do_ocpidev build project . $RCC --hdl-platform $HDL_PLATFORM --no-assemblies
  echo "============OCPIDEVTEST:Building test hdl"
  do_ocpidev build test comp1.test -l dsp_comps --build-hdl-platform $HDL_PLATFORM
  do_ocpidev build test comp1.test -l dsp_comps --hdl-platform $HDL_PLATFORM
  do_ocpidev clean test comp1.test -l dsp_comps --build-hdl-platform $HDL_PLATFORM
  do_ocpidev clean test comp1.test -l dsp_comps --hdl-platform $HDL_PLATFORM
  echo "============OCPIDEVTEST:Building tests hdl"
  do_ocpidev build test --build-hdl-platform $HDL_PLATFORM
  do_ocpidev build test --hdl-platform $HDL_PLATFORM
  do_ocpidev clean test --build-hdl-platform $HDL_PLATFORM
  do_ocpidev clean test --hdl-platform $HDL_PLATFORM
  echo "============OCPIDEVTEST:Building assys "
  do_ocpidev build hdl assemblies --build-hdl-platform $HDL_PLATFORM
  do_ocpidev build hdl assemblies --hdl-platform $HDL_PLATFORM
  do_ocpidev clean hdl assemblies --build-hdl-platform $HDL_PLATFORM
  do_ocpidev clean hdl assemblies --hdl-platform $HDL_PLATFORM
  do_ocpidev clean  
  echo "============OCPIDEVTEST:Building primitives target"
  do_ocpidev build hdl primitives --build-hdl-platform $HDL_TARGET
  do_ocpidev build hdl primitives --hdl-platform $HDL_TARGET
  do_ocpidev clean hdl primitives --build-hdl-platform $HDL_TARGET
  do_ocpidev clean hdl primitives --hdl-platform $HDL_TARGET
fi
  echo "============OCPIDEVTEST:Building project no assys2 "
  do_ocpidev build project . $RCC --build-hdl-platform $HDL_PLATFORM --build-no-assemblies
  do_ocpidev build project . $RCC --hdl-platform $HDL_PLATFORM --no-assemblies
if [ -z "$HDL_NO_BUILD" ]; then
  echo "============OCPIDEVTEST:Building assy "
  do_ocpidev build hdl assemblies --build-hdl-platform $HDL_PLATFORM
  do_ocpidev build hdl assemblies --hdl-platform $HDL_PLATFORM
fi
  echo "============OCPIDEVTEST:Clean "
  do_ocpidev clean  
  echo "============OCPIDEVTEST:Building project HP"
  do_ocpidev -v build project . $RCC --build-hdl-platform $HDL_PLATFORM
  do_ocpidev -v build project . $RCC --hdl-platform $HDL_PLATFORM
  do_ocpidev clean   
  echo "============OCPIDEVTEST:Building project HSP/HP"
  do_ocpidev build project . $RCC --build-hdl-rcc-platform $HDL_PLATFORM --build-hdl-platform $HDL_PLATFORM
  do_ocpidev build project . $RCC --hdl-rcc-platform $HDL_PLATFORM --hdl-platform $HDL_PLATFORM 
  fi  
  if [ "$ONLY_CREATE_BUILD" == 1 ] ; then
    echo "Exiting before project deletion."
    exit 0
  fi

function confirm_empty {
  echo "Should be empty of any assets (ie except Makefile, dirs):"
  ls $1 || echo "Good. Directory does not exist anymore."
}

echo "============ocpideleting the project and all of each of its assets individually"
echo "========Deleting applications"
for app in ${applications[@]} ; do
  do_ocpidev delete -f application $app
  do_ocpidev delete -f application "$app"_x
  do_ocpidev delete -f application $app -X
done
confirm_empty applications

echo "========Deleting assemblies"
for assemb in ${assemblies[@]} ; do
  do_ocpidev delete -f hdl assembly $assemb
done
echo "Should be empty:"
confirm_empty hdl/assemblies

echo "========Deleting platform device libraries"
for lib in ${platnames[@]} ; do
  do_ocpidev delete -f hdl device "dev_$lib".hdl -P $lib 
  for c in ${compseq1[@]}; do
    do_ocpidev delete -f spec $c -P $lib
    do_ocpidev delete -f test $c -P $lib
    do_ocpidev delete -f hdl device "$c".hdl -P $lib
    do_ocpidev delete -f hdl device "$c"_sub.hdl -P $lib
    do_ocpidev delete -f worker "$c"_proxy.rcc -P $lib 
    do_ocpidev delete -f hdl device "$c"_em.hdl -P $lib 
  done
  confirm_empty hdl/platforms/$lib/devices 
done

echo "====Deleting misc cards, slots, and card devices"
do_ocpidev delete -f hdl card card0
do_ocpidev delete -f hdl slot slot0
do_ocpidev delete -f spec comp6 -h cards
# this is broken an bug needs to be written for this 
#do_ocpidev delete -f test comp6 -h cards 
do_ocpidev delete -f hdl device comp6.hdl -h cards
do_ocpidev delete -f hdl device comp6_sub.hdl -h cards
do_ocpidev delete -f worker comp6_proxy.rcc -h cards
do_ocpidev delete -f hdl device comp6_em.hdl -h cards

echo "========Deleting device libraries"
for lib in ${devlibs[@]} ; do
  libopt=""
  if [ "$lib" != "devices" ] ; then
    libopt="-h $lib"
  fi
  do_ocpidev delete -f hdl device "dev_$lib".hdl $libopt
  for c in ${compseq1[@]}; do
    do_ocpidev delete -f spec $c -h $lib
    do_ocpidev delete -f test $c -h $lib
    do_ocpidev delete -f hdl device "$c".hdl $libopt
    do_ocpidev delete -f hdl device "$c"_sub.hdl $libopt
    do_ocpidev delete -f worker "$c"_proxy.rcc -h $lib
    do_ocpidev delete -f hdl device "$c"_em.hdl $libopt
  done
  confirm_empty hdl/$lib 
done

echo "========Deleting primitive libraries"
for primlib in ${complibs[@]} ; do
  do_ocpidev delete -f hdl primitive library $primlib
  do_ocpidev delete -f hdl primitive core "$primlib"_core
done
confirm_empty hdl/primitives

echo "========Deleting platforms"
for plat in ${platnames[@]} ; do
  do_ocpidev delete -f hdl platform $plat
done
confirm_empty hdl/platforms

echo "========Deleting component libraries"
for lib in ${complibs[@]} ; do
  do_ocpidev delete -f worker "comp_$lib".hdl -l $lib
  for c in ${compseq1[@]}; do
    do_ocpidev delete -f spec $c -l $lib
    do_ocpidev delete -f test $c -l $lib
    do_ocpidev delete -f worker "$c".hdl -l $lib
  done
  for c in ${compseq1[@]}; do
    do_ocpidev delete -f worker "$c".rcc -l $lib
  done
  confirm_empty components/$lib
  do_ocpidev delete -f library $lib
done

echo "========Deleting top level specs"
for c in ${compseq1[@]}; do
  do_ocpidev delete -f spec top_"$c" -p
  do_ocpidev delete -f protocol top_"$c"_prot -p
done

echo "========Deleting project $pj"
confirm_empty .
cd ..
do_ocpidev delete -f project $(basename $pj) -d $(dirname $pj)


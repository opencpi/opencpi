#!/bin/sh
set -e
OCPI_OSSIE_VERSION=embedded-01-10-2012
OCPI_OSSIE_USE_OPENCPI_FS=1
. setup_install.sh
mkdir -p ossie
cd ossie
sudo rm -r -f ossie*
curl -O http://ossie.wireless.vt.edu/download/ossie-embedded/ossie-$OCPI_OSSIE_VERSION.tar.bz2 
tar -xf ossie-$OCPI_OSSIE_VERSION.tar.bz2 
if [ -d ossie-embedded ]; then
  mv ossie-embedded ossie-$OCPI_OSSIE_VERSION
fi
cd ossie-$OCPI_OSSIE_VERSION
if test $OCPI_BUILD_OS == darwin; then
mkdir -p tmpbin
cat > tmpbin/libtoolize <<EOF
#!/bin/sh
exec glibtoolize $*
EOF
chmod a+x tmpbin/libtoolize
PATH=$PATH:`pwd`/tmpbin
fi
# Send in the patches:
echo $OCPI_BASE_DIR

#     platform/GPP/Makefile.am \
#     platform/nodes/default_GPP_node/Makefile.am \
#     system/nodebooter/Makefile.am \
#     system/ossie/framework/Makefile.am \

if test "$OCPI_BASE_DIR" != ""; then
   for f in \
     Makefile.am \
     system/ossie/framework/FileManager_impl.cpp \
     system/ossie/framework/FileSystem_impl.cpp \
     system/ossie/framework/File_impl.cpp \
     system/ossie/framework/helperFunctions.cpp \
     system/ossie/include/ossie/FileSystem_impl.h \
     system/ossie/parser/DASParser.cpp;
  do 
     cp $OCPI_BASE_DIR/ossie/$f $f
  done
fi
./bootstrap 

######################################################################
# These are the default OSSIE config parameters

OCPI_OSSIE_CPPFLAGS="-I$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/include"
OCPI_OSSIE_CXXFLAGS="-g"
# The -L options are for specifically mentioned libraries.
# The -Wl,-rpath is for indirectly used libraries and for the runtime library path
OCPI_OSSIE_LDFLAGS="\
-g \
-L$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_BUILD_TARGET/lib \
-Wl,-rpath,$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_BUILD_TARGET/lib \
"
OCPI_OSSIE_CONFIG_OPTIONS="--with-boost --with-boost-filesystem"



#######################################################################
# These are the changes when we want to use the OpenCPI SCA file system
# rather than OSSIE's boost-based one.

if [ "$OCPI_OSSIE_USE_OPENCPI_FS" == "1" ]; then
  # This adds the OpenCPI headers for the OpenCPI FS.
  OCPI_OSSIE_CPPFLAGS+="\
   -DUSE_OPENCPI_FS=1\
   -I$OCPI_BASE_DIR/core/local/util/vfs/include\
   -I$OCPI_BASE_DIR/core/local/util/filefs/include\
   -I$OCPI_BASE_DIR/core/sca/cf/cf222/idl\
   -I$OCPI_BASE_DIR/core/sca/cf_util/vfs/include\
   -I$OCPI_BASE_DIR/adapt/os/ocpios/interfaces/include\
  "
  # This adds the OpenCPI libraries for the OpenCPI FS.
  OCPI_OSSIE_LDFLAGS+="\
   -lcf_util\
   -L$OCPI_BASE_DIR/lib/$OCPI_BUILD_TARGET-bin\
   -Wl,-rpath,$OCPI_BASE_DIR/lib/$OCPI_BUILD_TARGET-bin\
  "
  OCPI_OSSIE_CONFIG_OPTIONS="--with-boost=no --with-boost-filesystem=no"
fi

# This is necessary for ./configure to run omniidl

PATH=$PATH:$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_BUILD_TARGET/bin

./configure \
  --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/ossie/$OCPI_BUILD_TARGET \
  --with-nbdir \
  $OCPI_OSSIE_CONFIG_OPTIONS \
  CXXFLAGS="$OCPI_OSSIE_CXXFLAGS" \
  CPPFLAGS="$OCPI_OSSIE_CPPFLAGS" \
  LDFLAGS="$OCPI_OSSIE_LDFLAGS" 

make

# no sudo since we're not putting it in a global place
make install

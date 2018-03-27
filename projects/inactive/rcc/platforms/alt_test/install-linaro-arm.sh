version=7.2.1-2017.11
minor=`sed 's/\([0-9]*\.[0-9]*\)\.[0-9]*\(-.*$\)/\1\2/' <<<$version`
dir=gcc-linaro-$version-x86_64_arm-linux-gnueabihf
me=gcc_linaro_arm_gnueabihf
[ -z "$OCPI_CDK_DIR" ] && echo Environment variable OCPI_CDK_DIR not set && exit 1
source $OCPI_CDK_DIR/scripts/setup-install.sh \
       "$1" \
       $me \
       "Tool chain for Altera SoC" \
       $dir.tar.xz \
       https://releases.linaro.org/components/toolchain/binaries/$minor/arm-linux-gnueabihf \
       $dir \
       0
dest=$OCPI_PREREQUISITES_INSTALL_DIR/$me
# Don't need a build dir since this is a binary distribution.
cd ..
# The tool chain finds ancillary files ok so all we need is bin.
relative_link bin $dest/$OCPI_TARGET_DIR

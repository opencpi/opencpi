#!/bin/sh --noprofile

# Fix up the simulation executable by converting xilinx-related library dependencies to
# be relative, not absolute
echo doing $1
set -o pipefail
set -e
[ -x "$1" ] || ( echo $1 is not executable; exit 1 )
ldd $1 > /dev/null || ( echo The ldd command does not work; exit 1 )
libs=`ldd $1 | sed -n '/^[ 	]*\//s/ *(.*$//p'`
changes=
for l in $libs; do
  [[ $l = */ISE/* ]] && changes+=" --replace-needed $l $(basename $l)"
done    
[ -n "$changes" ] && {
    echo Fixing absolute xilinx library dependencies for: $1. Doing:
    echo $OCPI_PREREQUISITES_DIR/patchelf/$OCPI_TOOL_DIR/bin/patchelf $changes $1    
    $OCPI_PREREQUISITES_DIR/patchelf/$OCPI_TOOL_DIR/bin/patchelf $changes $1    
}
exit 0

dnl 
dnl AC_CXX_COMPILE_OBJ( NAME, CODE, [ACTION-IF-OK [, ACTION-IF-NOT-OK] ] )
dnl
dnl Description
dnl
dnl  Compiles CODE into an object file NAME.
dnl  Any existing copy of NAME is deleted before we start.
dnl  ACTION-IF-OK is only run if NAME is created AND it hase size >0.
dnl  If NAME is successfully created, then it is up to the caller to delete it.
dnl
dnl Copyright (C) 2003, Alex Tingle <alex.autoconf@firetree.net>
dnl 
dnl License:
dnl GNU General Public License
dnl [http://www.gnu.org/software/ac-archive/htmldoc/COPYING.html]
dnl with this special exception
dnl [http://www.gnu.org/software/ac-archive/htmldoc/COPYING-Exception.html]. 
dnl 

AC_DEFUN([AC_CXX_COMPILE_OBJ],[
  cat > conftest.$ac_ext <<EOF
[#]line __oline__ "configure"
#include "confdefs.h"
[$2]
EOF
  ac_save_cxxflags="$CXXFLAGS"
  rm -f $1
  CXXFLAGS="$ac_save_cxxflags -o $1"
  if AC_TRY_EVAL(ac_compile) && test -s $1; then
    CXXFLAGS="$ac_save_cxxflags"
    rm -f conftest*
    ifelse([$3], , :, [$3])
  else
    echo "configure: failed program was:" >&AC_FD_CC
    cat conftest.$ac_ext >&AC_FD_CC
    CXXFLAGS="$ac_save_cxxflags"
    rm -f conftest* "$1"
    ifelse([$4], , , [$4])
  fi
])

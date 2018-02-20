dnl 
dnl AC_CXX_PIC_FLAG
dnl
dnl Description
dnl
dnl  Autodetects C++ `Position independant code' (PIC) flag.
dnl  Adds whatever CXXFLAGS are needed to ensure than C++ object code can
dnl  be linked into a dynamic shared library.
dnl 
dnl Copyright (C) 2003, Alex Tingle <alex.autoconf@firetree.net>
dnl 
dnl License:
dnl GNU General Public License
dnl [http://www.gnu.org/software/ac-archive/htmldoc/COPYING.html]
dnl with this special exception
dnl [http://www.gnu.org/software/ac-archive/htmldoc/COPYING-Exception.html]. 
dnl 

AC_DEFUN([AC_CXX_PIC_FLAG],[
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_REQUIRE([AC_CXX_IDENTITY])
  AC_CACHE_CHECK([for C++ position independent code flag],
    ac_cv_cxx_pic_flag,
    [
      ac_cxx_pic_save_cxxflags="$CXXFLAGS"
      ac_cv_cxx_pic_flag="none needed"

      case "$ac_cv_cxx_identity" in
        GNU-g++-*-*-*) ac_cv_cxx_pic_flag="-fPIC" ;;
         HP-aCC-*-*-*) ac_cv_cxx_pic_flag="+Z" ;;
        Kai-KCC-*-*-*) ac_cv_cxx_pic_flag="+Z" ;;
           *-CC-*-*-*) ac_cv_cxx_pic_flag="-KPIC" ;;
        DEC-cxx-*-*-*) ac_cv_cxx_pic_flag="none needed" ;;
        IBM-xlC-*-*-*) ac_cv_cxx_pic_flag="none needed" ;;
      esac

      # Finish up by adding the PIC flag to CXXFLAGS
      if test "$ac_cv_cxx_pic_flag" = "none needed"; then
        CXXFLAGS="$ac_cxx_pic_save_cxxflags"
      else
        CXXFLAGS="$ac_cxx_pic_save_cxxflags $ac_cv_cxx_pic_flag"
      fi
    ])
])

dnl
dnl AC_CXX_TEMPLATE_REPOSITORY
dnl
dnl Description
dnl 
dnl  Test whether the C++ compiler accepts the -ptr template
dnl  repository option. If so, sets the output variable `CXXFLAGS_PTR'.
dnl
dnl Copyright (C) 2003, Alex Tingle <alex.autoconf@firetree.net>
dnl 
dnl License:
dnl GNU General Public License
dnl [http://www.gnu.org/software/ac-archive/htmldoc/COPYING.html]
dnl with this special exception
dnl [http://www.gnu.org/software/ac-archive/htmldoc/COPYING-Exception.html]. 
dnl 

AC_DEFUN([AC_CXX_TEMPLATE_REPOSITORY],[
  AC_REQUIRE([AC_PROG_CXX])dnl 
  AC_CACHE_CHECK([whether C++ compiler accepts the -ptr option],
    ac_cv_cxx_ptr,[
      ac_cv_cxx_ptr=no
      if test "x$ac_cv_prog_gxx" != xyes; then
        AC_LANG_SAVE
        AC_LANG_CPLUSPLUS
        AC_CXX_CLEAN_TEMPLATE_REPOSITORY
        ac_cxx_ptr_save_cxxflags="$CXXFLAGS"
        ac_cxx_ptr_decl="template<class T> T m(T v){return v*2;}"
        ac_cxx_ptr_prog="int x=2; int y=m(x);"

        ac_cv_cxx_ptr="-ptr" #  Try it with no space after -ptr
        CXXFLAGS="$ac_cxx_ptr_save_cxxflags $ac_cv_cxx_ptr./confrepository.d"
        AC_TRY_COMPILE([$ac_cxx_ptr_decl],[$ac_cxx_ptr_prog],[:],[ac_cv_cxx_ptr=no])
        test -d confrepository.d || ac_cv_cxx_ptr=no

        if test "x$ac_cv_cxx_ptr" = xno; then
          ac_cv_cxx_ptr="-ptr " # Now try a space before the parameter.
          CXXFLAGS="$ac_cxx_ptr_save_cxxflags $ac_cv_cxx_ptr./confrepository.d"
          AC_TRY_COMPILE([$ac_cxx_ptr_decl],[$ac_cxx_ptr_prog],[:],[ac_cv_cxx_ptr=no])
          test -d confrepository.d || ac_cv_cxx_ptr=no
        fi

        # Clear away any confrepository.d that we might have made.
        rm -rf ./confrepository.d
        CXXFLAGS="$ac_cxx_ptr_save_cxxflags"
        AC_LANG_RESTORE
      fi
    ])
  if test "x$ac_cv_cxx_ptr" != xno
  then
    CXXFLAGS_PTR=$ac_cv_cxx_ptr
    AC_SUBST(CXXFLAGS_PTR)
  fi
])

dnl
dnl AC_PROG_CXX_AR
dnl 
dnl Description
dnl 
dnl C++ may require a special archiver to ensure that template code gets
dnl included with the rest of the objects. Sets the output variable CXX_AR.
dnl
dnl Copyright (C) 2003, Alex Tingle <alex.autoconf@firetree.net>
dnl 
dnl License:
dnl GNU General Public License
dnl [http://www.gnu.org/software/ac-archive/htmldoc/COPYING.html]
dnl with this special exception
dnl [http://www.gnu.org/software/ac-archive/htmldoc/COPYING-Exception.html]. 
dnl 

AC_DEFUN([AC_PROG_CXX_AR],[
  AC_REQUIRE([AC_CXX_IDENTITY])
  AC_CACHE_CHECK([for C++ static archiver],
    ac_cv_prog_cxx_ar,
    [
      ac_cv_prog_cxx_ar="no"
      ac_prog_cxx_ar_save_libs="$LIBS"
      AC_LANG_SAVE
      AC_CXX_CLEAN_TEMPLATE_REPOSITORY
      AC_LANG([C++])
      # Make object files a.o & b.o
      AC_CXX_COMPILE_OBJ([a.$ac_objext],[int a(){return 123;}],[
      AC_CXX_COMPILE_OBJ([b.$ac_objext],[int b(){return 456;}],[
        # Files a.o & b.o now exist, try to build them into a static library.
        # Try the more esoteric ones first, and then finally test for 'ar'.
        LIBS="$ac_prog_cxx_ar_save_libs -L. -lconftest"

        # Use the compiler's ID to make an initial guess.
        case "$ac_cv_cxx_identity" in
          GNU-g++-*-*-*) ac_prog_cxx_ar="ar cqs" ;;
           HP-aCC-*-*-*) ac_prog_cxx_ar="ar cqs" ;;
          Kai-KCC-*-*-*) ac_prog_cxx_ar="ar cqs" ;;
           Sun-CC-*-*-*) ac_prog_cxx_ar="${CXX-g++} -xar -o" ;;
           SGI-CC-*-*-*) ac_prog_cxx_ar="${CXX-g++} -ar -o" ;;
          DEC-cxx-*-*-*) ac_prog_cxx_ar="ar cqs" ;;
          IBM-xlC-*-*-*) ac_prog_cxx_ar="ar cqs" ;;
        esac
        if AC_TRY_COMMAND([$ac_prog_cxx_ar libconftest.a a.$ac_objext b.$ac_objext >/dev/null 2>/dev/null]); then
          AC_TRY_LINK([int b();],[return(b()==456?0:1)],
            [ac_cv_prog_cxx_ar="$ac_prog_cxx_ar"])
        fi
        rm -f libconftest.a
        
        # Hmmm...: ar cq (may be inefficient without a subsequent 'ranlib')
        if test "$ac_cv_prog_cxx_ar" = no; then
          if AC_TRY_COMMAND([ar cq libconftest.a a.$ac_objext b.$ac_objext >/dev/null 2>/dev/null]); then
            AC_TRY_LINK([int b();],[return(b()==456?0:1)],
              [ac_cv_prog_cxx_ar="ar cq"],[:])
          fi
          rm -f libconftest.a
        fi

      ]) ])
      rm -f a.$ac_objext b.$ac_objext
      AC_LANG_RESTORE
      LIBS="$ac_prog_cxx_ar_save_libs"
    ])
  if test "$ac_cv_prog_cxx_ar" != no
  then
    CXX_AR="$ac_cv_prog_cxx_ar"
    AC_SUBST(CXX_AR)
  fi
])


dnl
dnl AC_PROG_CXX_LINK_SO_LIB
dnl
dnl Description
dnl 
dnl Works out how to build a C++ shared object library.
dnl 
dnl If successful, the following output variables are set:
dnl  CXX_LINK_SO_LIB - Command to build a C++ shared object library.
dnl    Expects that the environment variable `$soname' is set to the name of
dnl    the library. The value of CXX_LINK_SO_LIB can only be used from
dnl    within a makefile (because is double-quotes the `$').
dnl  LD_LIBRARY_PATH_NAME - set to the name of the environment variable that
dnl    sets the run-time search path for shared libraries.
dnl  SOEXT - set to the system's filename extension for shared libraries.
dnl    E.g. `.so', `.a', `.sl'.
dnl
dnl Additionally, any options required to link a dynamic executable are
dnl added to `LDFLAGS'.
dnl 
dnl Copyright (C) 2003, Alex Tingle <alex.autoconf@firetree.net>
dnl 
dnl License:
dnl GNU General Public License
dnl [http://www.gnu.org/software/ac-archive/htmldoc/COPYING.html]
dnl with this special exception
dnl [http://www.gnu.org/software/ac-archive/htmldoc/COPYING-Exception.html]. 
dnl 

AC_DEFUN([AC_PROG_CXX_LINK_SO_LIB],[
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_REQUIRE([AC_CXX_IDENTITY])
  AC_REQUIRE([AC_CXX_PIC_FLAG])
  AC_CACHE_CHECK([for C++ shared library linker],
    ac_cv_prog_cxx_link_so_lib,
    [
      ac_cv_prog_cxx_link_so_lib="no"
      case "$host" in
        *-*-aix*)    ac_prog_cxx_soext=".a";;
        *-*-darwin*) ac_prog_cxx_soext=".dylib";;
        *-*-hpux*)   ac_prog_cxx_soext=".sl";;
        *-*-*)       ac_prog_cxx_soext=".so";;
      esac
      soname="libconftest$ac_prog_cxx_soext"

      AC_LANG_SAVE
      AC_CXX_CLEAN_TEMPLATE_REPOSITORY
      AC_LANG([C++])

      case "$ac_cv_cxx_identity" in
        GNU-g++-*-*-*)
                # GNU/g++ options vary with the platform:
                case "$host" in
                  *-*-darwin*)  ac_sobuild="${CXX-g++} -dynamiclib -install_name $soname -o " ;;
                  *-*-solaris*) ac_sobuild="${CXX-g++} -shared -Wl,-h,$soname -o " ;;
                  *-*-*)        ac_sobuild="${CXX-g++} -shared -Wl,-soname,$soname -o " ;;
                esac
                ;;
         HP-aCC-*-*-*) ac_sobuild="${CXX-g++} -b -Wl,+h$soname -o " ;;
        Kai-KCC-*-*-*) ac_sobuild="${CXX-g++} --soname $soname -o " ;;
           *-CC-*-*-*) ac_sobuild="${CXX-g++} -G -h $soname -o " ;;
         SGI-CC-*-*-*) ac_sobuild="${CXX-g++} -shared -soname $soname -o " ;;
        DEC-cxx-*-*-*) ac_sobuild="${CXX-g++} -shared -soname $soname -o " ;;
        IBM-xlC-*-*-*) ac_sobuild="${CXX-g++} -G -o " ;;
      esac

      case "$ac_cv_cxx_identity" in
        GNU-g++-*-*-*)
                # GNU/g++ options vary with the platform:
                case "$host" in
                  *-*-darwin*) ac_soflags="-dynamic" ;; # -dynamic is the default
                  *-*-*)       ac_soflags="-Wl,-Bdynamic" ;;
                esac
                ;;
         HP-aCC-*-*-*) ac_soflags="-Wl,-a,shared_archive" ;;
        Kai-KCC-*-*-*)  ;; # ??
           *-CC-*-*-*) ac_soflags="-Bdynamic" ;;
        DEC-cxx-*-*-*) ac_soflags="-call_shared" ;;
        IBM-xlC-*-*-*) ac_soflags="-bdynamic -brtl" ;;
      esac

      # Make object files a.o, b.o & c.o
      AC_CXX_COMPILE_OBJ([a.$ac_objext],[int a(){return 123;}],[:],[:])
      AC_CXX_COMPILE_OBJ([b.$ac_objext],[int b(){return 456;}],[:],[:])
      AC_CXX_COMPILE_OBJ([c.$ac_objext],[int b(){return 789;}],[:],[:])

      # Try various ways of building shared object libraries
      ac_sothere=""
      AC_CXX_TRY_LINK_SO([$ac_sobuild],$soname,[ac_sothere=yes])

      if test "$ac_sothere" = yes; then
        ac_save_libs="$LIBS"
        LIBS="$ac_save_libs -L. -lconftest"
        # Try various ways of running dynamic executables.
        ac_soworks=""
        if test -z "$ac_soworks"; then # Linux, solaris, SGI, HP-UX(64bit), Tru64, AIX
          ac_sopath="LD_LIBRARY_PATH"
          AC_CXX_TRY_RUN_DYNAMIC([$ac_soflags],[$ac_sopath],[ac_soworks=yes])
        fi
        if test -z "$ac_soworks"; then # solaris(64bit)
          ac_sopath="LD_LIBRARY_PATH_64"
          AC_CXX_TRY_RUN_DYNAMIC([$ac_soflags],[$ac_sopath],[ac_soworks=yes])
        fi

        if test -z "$ac_soworks"; then # HP-UX(32bit)
          ac_sopath="SHLIB_PATH"
          AC_CXX_TRY_RUN_DYNAMIC([$ac_soflags],[$ac_sopath],[ac_soworks=yes])
        fi
        if test -z "$ac_soworks"; then # Darwin/MacOSX
          ac_sopath="DYLD_LIBRARY_PATH"
          AC_CXX_TRY_RUN_DYNAMIC([$ac_soflags],[$ac_sopath],[ac_soworks=yes])
        fi
        if test -z "$ac_soworks"; then # SGI(32bit)
          ac_sopath="LD_LIBRARYN32_PATH"
          AC_CXX_TRY_RUN_DYNAMIC([$ac_soflags],[$ac_sopath],[ac_soworks=yes])
        fi
        if test -z "$ac_soworks"; then # SGI(64bit)
          ac_sopath="LD_LIBRARYN64_PATH"
          AC_CXX_TRY_RUN_DYNAMIC([$ac_soflags],[$ac_sopath],[ac_soworks=yes])
        fi
        LIBS="$ac_save_libs"
      fi

      if test "x$ac_soworks" = xyes; then
        ac_cv_prog_cxx_link_so_lib="`echo $ac_sobuild|sed s/$soname/\\$\\$soname/`"
        LDFLAGS="$LDFLAGS $ac_soflags"
      fi

      rm -f a.$ac_objext b.$ac_objext c.$ac_objext libconftest*
      AC_LANG_RESTORE
    ])
  if test "$ac_cv_prog_cxx_link_so_lib" != no
  then
    CXX_LINK_SO_LIB="$ac_cv_prog_cxx_link_so_lib"
    AC_SUBST(CXX_LINK_SO_LIB)
    LD_LIBRARY_PATH_NAME=$ac_sopath
    AC_SUBST(LD_LIBRARY_PATH_NAME)
    AC_SUBST(SOEXT,$ac_prog_cxx_soext)
  fi
])


dnl 
dnl AC_CXX_TRY_LINK_SO( SOLINK-COMMAND, SONAME-BASE,
dnl   [ACTION-IF-OK [, ACTION-IF-NOT-OK]])
dnl 

AC_DEFUN([AC_CXX_TRY_LINK_SO],[
  # We assume that a.o b.o & c.o already exist.
  rm -f $2 $3
  if AC_TRY_COMMAND([$1 $2.1 a.$ac_objext b.$ac_objext >/dev/null 2>/dev/null]) \
     && test -s $2.1
  then
    AC_TRY_COMMAND([$1 $2.2 a.$ac_objext c.$ac_objext >/dev/null 2>/dev/null])
    ifelse([$3], , :, [$3])
  else
    ifelse([$4], , :, [$4])
  fi
])
  

dnl 
dnl AC_CXX_TRY_RUN_DYNAMIC( EXE-LINK-FLAGS, LD_LIB_PATH-VARIABLE
dnl   [ACTION-IF-OK [, ACTION-IF-NOT-OK]])
dnl 

AC_DEFUN([AC_CXX_TRY_RUN_DYNAMIC],[
  ac_save_ldflags="$LDFLAGS"
  LDFLAGS="$ac_save_ldflags $1"
  ac_dollar_var="[\$]$2"
  ac_save_ldpath=`eval echo $ac_dollar_var`
  eval $2=".:$ac_save_ldpath"
  eval export $2
  ac_cxx_so_works=no
  rm -f $soname
  ln -s $soname.1 $soname
  AC_TRY_RUN([int b();int main(int,char**){return(b()==456?0:1);}],[
    rm -f $soname
    ln -s $soname.2 $soname
    if (./conftest$ac_exeext; exit); then
      :
    else
      # Success - ./conftest returned 1 this time, so changing the
      # library changed the behaviour of the exe!
      ac_cxx_so_works=yes
    fi
  ],[:])

  LDFLAGS="$ac_save_ldflags"
  eval $2="$ac_save_ldpath"
  eval export $2

  if test "$ac_cxx_so_works" = yes; then
    ifelse([$3], , :, [$3])
  else
    ifelse([$4], , :, [$4])
  fi
])


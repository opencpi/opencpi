AC_INIT(GPP, 0.6.2)
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
LT_INIT
AC_PREFIX_DEFAULT("/sdr/dev")

AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET

AC_HEADER_SYS_WAIT

AC_FUNC_FORK

AC_LANG_PUSH([C++])

AC_CHECK_LIB([omniORB4], [main], [], [AC_MSG_ERROR([cannot find omniORBi4 library])])
AC_CHECK_LIB([omnithread], [main], [], [AC_MSG_ERROR([cannot find omnithread library])])
AC_CHECK_LIB([omniDynamic4], [main], [], [AC_MSG_ERROR([cannot find omniDynamic4 library])])
AC_CHECK_HEADERS([omniORB4/CORBA.h], [], [AC_MSG_ERROR([cannot find omniORB4 header files])])

#AC_CHECK_HEADERS([boost/filesystem/operations.hpp], [], [AC_MSG_ERROR([Boost filesystem library headers not installed.])])
#AC_CHECK_LIB([boost_filesystem], [main], [], [AC_MSG_ERROR([cannot find boost_filesystem library.])])

AX_BOOST_BASE([1.35])
AX_BOOST_FILESYSTEM
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
CFLAGS="$CFLAGS $BOOST_CFLAGS"

AC_LANG_POP

export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig"
# PKG_CHECK_MODULES(OSSIE, ossie >= 0.6.0,,exit)
CXXFLAGS="$CXXFLAGS $OSSIE_CFLAGS"
LIBS="$LIBS $OSSIE_LIBS"
# IDL_FLAGS="$OSSIE_CFLAGS"
# AC_SUBST(IDL_FLAGS)

AC_CONFIG_FILES(Makefile)

AC_OUTPUT

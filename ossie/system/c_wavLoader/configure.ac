AC_INIT(c_wavLoader, 0.6.2)
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
LT_INIT
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET

AC_HEADER_SYS_WAIT

AC_FUNC_FORK

AC_LANG_PUSH([C++])

AC_CHECK_LIB([omniORB4], [main], [], [AC_MSG_ERROR([cannot find omniORB4 library])])
AC_CHECK_LIB([omnithread], [main], [], [AC_MSG_ERROR([cannot find omnithread library])])
AC_CHECK_LIB([omniDynamic4], [main], [], [AC_MSG_ERROR([cannot find omniDynamic4 library])])
AC_CHECK_HEADERS([omniORB4/CORBA.h], [], [AC_MSG_ERROR([cannot find omniORB4 header files])])
# AC_CHECK_HEADERS([ossie/tinyxml.h], [], [AC_MSG_ERROR([cannot find ossie/tinyxml.h header file])])

# AC_CHECK_LIB([ossieidl], [main], [], [AC_MSG_ERROR([cannot find OSSIE library])])

AC_LANG_POP

export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig"
# PKG_CHECK_MODULES(OSSIE, ossie >= 0.6.0,,exit)
CXXFLAGS="$CXXFLAGS $OSSIE_CFLAGS"
# LIBS="$LIBS $OSSIE_LIBS"
IDL_FLAGS="$OSSIE_CFLAGS"
# AC_SUBST(IDL_FLAGS)

AC_CONFIG_FILES(Makefile)

AC_OUTPUT

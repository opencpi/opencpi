AC_INIT(ChannelDemo, 0.6.2)
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
LT_INIT
AC_PREFIX_DEFAULT("/sdr/dom")

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

# AC_CHECK_LIB([standardInterfaces], [main], [], [AC_MSG_ERROR([cannot find standardInterfaces])])
# AC_CHECK_HEADERS([standardinterfaces/complexShort.h], [], [AC_MSG_ERROR([cannot find standardInterfaces header files])])

AC_LANG_POP

export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig"
# PKG_CHECK_MODULES(OSSIE, ossie >= 0.6.0,,exit)
CXXFLAGS="$CXXFLAGS $OSSIE_CFLAGS"
# IDL_FLAGS="$OSSIE_CFLAGS"
# AC_SUBST(IDL_FLAGS)

LIBS="$LIBS $OSSIE_LIBS"

AC_SUBST(SI_PATH)

AC_CONFIG_FILES(Makefile)

AC_OUTPUT

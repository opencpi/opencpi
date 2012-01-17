AC_INIT(standardInterfaces-metadata, 0.6.0)
AC_PREREQ(2.59)

AM_INIT_AUTOMAKE(nostdinc)

AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_LANG_PUSH([C++])

AC_CHECK_LIB([omniORB4], [main], [], [AC_MSG_ERROR([cannot find omniORB library])])
AC_CHECK_LIB([omnithread], [main], [], [AC_MSG_ERROR([cannot find omnithread library])])
AC_CHECK_HEADERS([omniORB4/CORBA.h], [], [AC_MSG_ERROR([cannot find omniORB4 header files])])
AC_CHECK_PROG([IDL], [omniidl], [omniidl], [AC_MSG_ERROR([cannot find omniidl prgram])])

AM_PATH_PYTHON([2.5])

AC_LANG_POP

export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig"
PKG_CHECK_MODULES(OSSIE, ossie >= 0.6.0,,exit)
CXXFLAGS="$CXXFLAGS $OSSIE_CFLAGS"
LIBS="$LIBS $OSSIE_LIBS"
IDL_FLAGS="$OSSIE_CFLAGS"
AC_SUBST(IDL_FLAGS)

AC_CONFIG_FILES(
	Makefile
	include/Makefile
	include/standardinterfaces-metadata/Makefile
	standardInterfaces-metadata.pc)
AC_OUTPUT


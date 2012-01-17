AC_PREREQ([2.61])
AC_INIT([ossie], [0.8.2])
AC_PREFIX_DEFAULT("/sdr")
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CXX

#AM_PATH_PYTHON([2.5])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SUBDIRS([system/ossie \
                   system/standardInterfaces \
                   system/nodebooter \
                   system/customInterfaces \
                   system/c_wavLoader \
                   system/SigProc \
                   platform/domain \
                   platform/dtd \
                   platform/nodes/default_GPP_node \
                   platform/GPP \
                   waveforms/ossie_demo \
                   components/amplifier \
                   components/ChannelDemo \
                   components/RxDemo \
                   components/TxDemo])

AC_ARG_WITH([nbdir],
            [AS_HELP_STRING([--with-nodebooter], [install nodeBooter in prefix/bin (default: /usr/local/bin)])],
            [nbdir="$prefix/bin"],
            [nbdir="/usr/local/bin"])

AM_CONDITIONAL([WITHNB], [test "$with_nbdir" = yes])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT


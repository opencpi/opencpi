AC_INIT(sigproc, 0.7.3)
AC_PREREQ(2.59)

AM_INIT_AUTOMAKE(nostdinc)

AC_CONFIG_MACRO_DIR([m4])
LT_INIT
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_ARG_ENABLE(fpm, AC_HELP_STRING([--enable-fpm=ARCH],
                   [use ARCH-specific fixed-point math routines
                    (one of: intel, arm, mips, sparc, ppc, 64bit, default)]),
[
    case "$enableval" in
        yes)                             ;;
        no|default|approx) FPM="DEFAULT" ;;
        intel|i?86)        FPM="INTEL"   ;;
        arm)               FPM="ARM"     ;;
        mips)              FPM="MIPS"    ;;
        sparc)             FPM="SPARC"   ;;
        ppc|powerpc)       FPM="PPC"     ;;
        64bit)             FPM="64BIT"   ;;
        float)             FPM="FLOAT"   ;;
        *)
            AC_MSG_RESULT(failed)
            AC_MSG_ERROR([bad --enable-fpm option])
            ;;
    esac

    FPM="-DFPM_$FPM"

])

AC_SUBST(FPM)

AC_CONFIG_FILES(Makefile sigproc.pc)
AC_OUTPUT

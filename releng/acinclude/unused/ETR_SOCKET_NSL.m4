dnl
dnl This file is protected by Copyright. Please refer to the COPYRIGHT file
dnl distributed with this source distribution.
dnl
dnl This file is part of REDHAWK core.
dnl
dnl REDHAWK core is free software: you can redistribute it and/or modify it under
dnl the terms of the GNU Lesser General Public License as published by the Free
dnl Software Foundation, either version 3 of the License, or (at your option) any
dnl later version.
dnl
dnl REDHAWK core is distributed in the hope that it will be useful, but WITHOUT
dnl ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
dnl FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
dnl details.
dnl
dnl You should have received a copy of the GNU Lesser General Public License
dnl along with this program.  If not, see http://www.gnu.org/licenses/.
dnl
dnl
dnl Description
dnl 
dnl This macro figures out what libraries are required on this platform to
dnl link sockets programs. It's usually -lsocket and/or -lnsl or neither. We
dnl test for all three combinations.
dnl 
dnl Version: 1.1 (last modified: 2001-06-07)
dnl Author: Warren Young <warren@etr-usa.com>
dnl 
dnl from http://www.gnu.org/software/ac-archive/htmldoc/index.html
dnl 
dnl License:
dnl GNU General Public License
dnl [http://www.gnu.org/software/ac-archive/htmldoc/COPYING.html]
dnl with this special exception
dnl [http://www.gnu.org/software/ac-archive/htmldoc/COPYING-Exception.html]. 
dnl 

AC_DEFUN([ETR_SOCKET_NSL],
[
AC_CACHE_CHECK(for libraries containing socket functions,
ac_cv_socket_libs, [
        oLIBS=$LIBS

        AC_TRY_LINK([
                        #include <sys/types.h>
                        #include <sys/socket.h>
                        #include <netinet/in.h>
                        #include <arpa/inet.h>
                ],
                [
                        struct in_addr add;
                        int sd = socket(AF_INET, SOCK_STREAM, 0);
                        inet_ntoa(add);
                ],
                ac_cv_socket_libs=-lc, ac_cv_socket_libs=no)

        if test x"$ac_cv_socket_libs" = "xno"
        then
                LIBS="$oLIBS -lsocket"
                AC_TRY_LINK([
                                #include <sys/types.h>
                                #include <sys/socket.h>
                                #include <netinet/in.h>
                                #include <arpa/inet.h>
                        ],
                        [
                                struct in_addr add;
                                int sd = socket(AF_INET, SOCK_STREAM, 0);
                                inet_ntoa(add);
                        ],
                        ac_cv_socket_libs=-lsocket, ac_cv_socket_libs=no)
        fi

        if test x"$ac_cv_socket_libs" = "xno"
        then
                LIBS="$oLIBS -lsocket -lnsl"
                AC_TRY_LINK([
                                #include <sys/types.h>
                                #include <sys/socket.h>
                                #include <netinet/in.h>
                                #include <arpa/inet.h>
                        ],
                        [
                                struct in_addr add;
                                int sd = socket(AF_INET, SOCK_STREAM, 0);
                                inet_ntoa(add);
                        ],
                        ac_cv_socket_libs="-lsocket -lnsl", ac_cv_socket_libs=no)
        fi

        LIBS=$oLIBS
])

        if test x"$ac_cv_socket_libs" = "xno"
        then
                AC_MSG_ERROR([Cannot find socket libraries])
        elif test x"$ac_cv_socket_libs" = "x-lc"
        then
                ETR_SOCKET_LIBS=""
        else
                ETR_SOCKET_LIBS="$ac_cv_socket_libs"
        fi

        AC_SUBST(ETR_SOCKET_LIBS)
]) dnl ETR_SOCKET_NSL

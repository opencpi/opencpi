#!/bin/sh -e
OCPI_PREREQUISITES_BUILD_DIR=/opt/opencpi/prerequisites
OCPI_PREREQUISITES_INSTALL_DIR=/opt/opencpi/prerequisites
OCPI_HOST_SYSTEM=linux-x86_64

#Download, but the web site download button doesn't have a precise URL for the downloads, so you have to click.
#So the first thing is to ensure that the file is there.
cd $OCPI_PREREQUISITES_BUILD_DIR
if test ! -d opensplice ; then
  mkdir opensplice
fi
OCPI_OPENSPLICE_VERSION=5.4.1
cd opensplice
TarFile=OpenSpliceDDSV$OCPI_OPENSPLICE_VERSION-src.tar
pwd
if test ! -f $TarFile ; then
  echo You must download the source tar file: $TarFile from:
  echo     http://www.prismtech.com/download-documents
  echo It has no download URL, so you must do it yourself and put it in: `pwd`
  echo Then you can run this $0 script again.
  exit 1
fi
tar xf $TarFile
DirName=OpenSplice-$OCPI_OPENSPLICE_VERSION
rm -r -f $DirName
mv OpenSplice $DirName
cd $DirName
if test `uname` == Darwin; then
  mkdir src/abstraction/os/include/sys
  mkdir src/abstraction/os/include/asm
  mkdir src/abstraction/os/include/linux
  mkdir src/abstraction/os/include/net
  cat > src/abstraction/os/include/sys/wait.h <<EOF
#include "/usr/include/sys/wait.h"
#ifdef _W_INT
#undef _W_INT
#define _W_INT(x) (x)
#endif
EOF
  cat > src/abstraction/os/include/linux/socket.h <<EOF
  /* Nothing here yet */
EOF
  cat > src/abstraction/os/include/linux/if.h <<EOF
  /* Nothing here yet */
EOF
  cat > src/abstraction/os/include/asm/param.h <<EOF
#include <sys/param.h>
EOF
  cat > src/abstraction/os/include/unistd.h <<EOF
#include </usr/include/unistd.h>
#ifndef _FAKE_UNISTD_H
#define _FAKE_UNISTD_H
#define execve(a,b,c) execv(a,b)
#endif
EOF
  cat > src/abstraction/os/include/pthread.h <<EOF
#include "/usr/include/pthread.h"
#ifdef PTHREAD_PROCESS_SHARED
#undef PTHREAD_PROCESS_SHARED
#define PTHREAD_PROCESS_SHARED PTHREAD_PROCESS_PRIVATE
#endif
EOF
  cat > src/abstraction/os/include/values.h <<EOF
/* nothing is needed but the file */
EOF
  cat > src/abstraction/os/include/sched.h <<EOF
#include </usr/include/sched.h>
#include <pthread.h>
#if defined(PTHREAD_ONCE_INIT) && !defined(_FAKE_SCHED_)
#define _FAKE_SCHED_
#include <errno.h>
#define sched_setscheduler(pid, policy, param) \
pthread_setschedparam(pthread_self(), policy, param)
static int sched_getscheduler(pid_t pid) {
   int policy, res;
   struct sched_param param;
   assert(pid == 0 || pid == getpid());
   if ((res = pthread_getschedparam(pthread_self(), &policy, &param))) {
     errno = res;
     return -1;
  }
  return policy;
}
static int sched_getparam(pid_t pid, struct sched_param *paramp) {
   int policy, res;
   assert(pid == 0 || pid == getpid());
   if ((res = pthread_getschedparam(pthread_self(), &policy, paramp))) {
     errno = res;
     return -1;
  }
  return 0;
}
#endif
EOF
  cat > src/abstraction/os/include/sys/mman.h<<EOF
#include "unistd.h"
#include </usr/include/sys/mman.h>
#ifndef mremap
#define mremap(__oldaddr__,__oldsize__,__newsize__,__flags__) \
(__flags__ != 0 ? MAP_FAILED : \
(munmap(__oldaddr__, __oldsize__) != 0 ? MAP_FAILED : \
mmap(__oldaddr__,__newsize__,PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED,mmfHandle->fd,0)))
#endif
EOF
  cat > src/abstraction/os/include/time.h<<EOF
#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#include </usr/include/time.h>
#ifndef _FAKE_TIME
#define _FAKE_TIME
#include </usr/include/sys/time.h>
#define CLOCK_REALTIME 0
static int clock_gettime(int id, struct timespec *tp) {
  struct timeval tv;
  (void)id;
  gettimeofday(&tv, NULL);
  TIMEVAL_TO_TIMESPEC(&tv, tp);
  return 0;
}
#endif
EOF
fi
sed 's/-D_XOPEN_SOURCE=500//;s/-lrt//;s/-static-libgcc//' < setup/x86_64.linux2.6-default.mak > temp.mak
mv temp.mak setup/x86_64.linux2.6-default.mak
sed s=endian.h=machine/endian.h= < src/abstraction/os/linux2.6/include/os_abstract.h > temp.h
mv temp.h src/abstraction/os/linux2.6/include/os_abstract.h
sed s=OS_MAP_ON_SEG=OS_MAP_ON_FILE= < src/abstraction/os/linux2.6/code/os_sharedmem.c > temp.c
mv temp.c src/abstraction/os/linux2.6/code/os_sharedmem.c
(export JAVA_HOME=.; . ./configure x86_64.linux2.6-dev && make clean && make -w CPP=gcc install)
BuiltDir=install/HDE/x86_64.linux2.6-dev
sed 's=@@INSTALLDIR@@.*"'=$OCPI_PREREQUISITES_INSTALL_DIR/opensplice/$OCPI_HOST_SYSTEM'"'= < $BuiltDir/release.com > temp.com
mv temp.com $BuiltDir/release.com
rm -f $OCPI_PREREQUISITES_INSTALL_DIR/opensplice/$OCPI_HOST_SYSTEM
ln -s $OCPI_PREREQUISITES_BUILD_DIR/opensplice/$DirName/$BuiltDir $OCPI_PREREQUISITES_INSTALL_DIR/opensplice/$OCPI_HOST_SYSTEM

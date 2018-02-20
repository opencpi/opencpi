dnl 
dnl AC_PROG_OMNIIDL
dnl
dnl Description
dnl 
dnl  AC_CXX_IDENTITY( [ ACTION-IF-OK [ ,ACTION-IF-FAILED ] ] )
dnl  Sets ac_cv_cxx_identity to a string that identifies the C++ compiler.
dnl  Format: vendor-compiler-major-minor-micro
dnl 
dnl Copyright (C) 2003, Alex Tingle <alex.autoconf@firetree.net>
dnl 
dnl License:
dnl GNU General Public License
dnl [http://www.gnu.org/software/ac-archive/htmldoc/COPYING.html]
dnl with this special exception
dnl [http://www.gnu.org/software/ac-archive/htmldoc/COPYING-Exception.html]. 
dnl 

AC_DEFUN([AC_CXX_IDENTITY],[
  AC_REQUIRE([AC_PROG_CXX])
  AC_CACHE_CHECK([C++ compiler identity],[ac_cv_cxx_identity],[
    ac_cv_cxx_identity="unknown"
    cat > conftest.$ac_ext <<EOF
[#]line __oline__ "configure"
#include "confdefs.h"
#include <stdio.h>

int _result=1;
void set(const char* vendor,const char* compiler,int major,int minor,int micro)
{
  switch(_result)
  {
    case 0:  _result=2; break;
    case 1:  _result=0; break;
    default: _result=2;
  }
  printf("%s-%s-%i-%i-%i\n",vendor,compiler,major,minor,micro);
}

#define HEXN(X,N) ((X>>(4*N))&0xF)

int main(int,char**)
{
#ifdef __GNUG__
  set("GNU","g++",__GNUC__,__GNUC_MINOR__,0);
#endif

#ifdef __SUNPRO_CC
  set("Sun","CC",HEXN(__SUNPRO_CC,2),HEXN(__SUNPRO_CC,1),HEXN(__SUNPRO_CC,0));
#endif

#ifdef __xlC__
  set("IBM","xlC",10*HEXN(__xlC__,3)+HEXN(__xlC__,2),10*HEXN(__xlC__,1)+HEXN(__xlC__,0),0);
#else
#  if defined(_AIX) && !defined(__GNUC__)
  set("IBM","xlC",0,0,0);
#  endif
#endif

#ifdef __DECCXX_VER
  set("DEC","cxx",__DECCXX_VER/10000000,__DECCXX_VER/100000%100,__DECCXX_VER%100);
#endif

#ifdef __HP_aCC
  set("HP","aCC",__HP_aCC/10000,__HP_aCC/100%100,__HP_aCC%100);
#endif

#ifdef __KCC_VERSION
  set("Kai","KCC",HEXN(__KCC_VERSION,3),HEXN(__KCC_VERSION,2),__KCC_VERSION&0xFF);
#endif

#ifdef _MSC_VER
  set("Microsoft","VC++",_MSC_VER>>8,_MSC_VER&0xFF,0);
#endif

  return _result;
}
EOF
    if AC_TRY_EVAL(ac_link) \
      && test -s conftest${ac_exeext} \
      && AC_TRY_COMMAND([./conftest${ac_exeext} >/dev/null])
    then
      ac_cv_cxx_identity=`./conftest${ac_exeext}`
    fi
    if test $ac_cv_cxx_identity = "unknown"; then
      echo "configure: failed program was:" >&AC_FD_CC
      cat conftest.$ac_ext >&AC_FD_CC
      ifelse([$2], , , [$2])
    else
      ifelse([$1], , :, [$1])
    fi
    rm -f conftest*
  ])
])
  

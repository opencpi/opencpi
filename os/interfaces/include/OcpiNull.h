// From stackoverflow.com, by Yaroslav, on Aug 22 '12 at 15:22
// http://stackoverflow.com/a/12076387
// For older C++ that does not support nullptr
#pragma once

/*
This is to support gcc on CentOS 6, gcc on CentOS 7, and clang on C7.
The MSC stuff in here is legacy, but no reason to remove it at this time.

clang declares itself to be gcc 4.2 when using __GNUC__ and __GNUC_MINOR__.
gcc on C6 declares __cplusplus to be "1" instead of "199711"
  ( https://gcc.gnu.org/bugzilla/show_bug.cgi?id=1773 ), even when told "c++0x"
gcc on C7 declares __cplusplus to be "199711" normally, "c++0x" and "c++11" both
  give "201103"
clang supports "nullptr" only if given "-std=c++0x", but defines __cplusplus
  to be "201103" for both c++0x and c++11, and "199711" with no options.

So, to make a long story short, if __cplusplus is > 199711, nullptr should be defined.

Note: This is only needed in the non-autoconf tree. The autoconf tree autodetects.
*/

#include "angryviper.h"
#ifndef HAVE_NULLPTR
#ifndef __APPLE__
#if __cplusplus <= 199711

namespace ocpi
{
    //based on SC22/WG21/N2431 = J16/07-0301
    struct nullptr_t
    {
        template<typename any> operator any * () const
    {
        return 0;
    }
    template<class any, typename T> operator T any:: * () const
    {
        return 0;
    }

#ifdef _MSC_VER
    struct pad {};
    pad __[sizeof(void*)/sizeof(pad)];
#else
    char __[sizeof(void*)];
#endif
private:
    //  nullptr_t();// {}
    //  nullptr_t(const nullptr_t&);
    //  void operator = (const nullptr_t&);
    void operator &() const;
    template<typename any> void operator +(any) const
    {
        /*I Love MSVC 2005!*/
    }
    template<typename any> void operator -(any) const
    {
        /*I Love MSVC 2005!*/
    }
    };
static const nullptr_t __nullptr = {};
}

#define nullptr ocpi::__nullptr

#endif
#endif
#endif

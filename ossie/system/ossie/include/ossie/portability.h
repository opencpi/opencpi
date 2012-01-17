/*******************************************************************************

Copyright 2008, Virginia Polytechnic Institute and State University

This file is part of the OSSIE Parser.

OSSIE Parser is free software; you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

OSSIE Parser is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with OSSIE Parser; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef PORTABILITY_H
#define PORTABILITY_H

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif

namespace ossieSupport
{
/**
   Sleep for integer number of seconds.
*/
inline unsigned int sleep(unsigned int seconds)
{
#ifdef HAVE_UNISTD_H
    return ::sleep(seconds);
#endif
}

/**
   Nano-second resolution sleep.
*/
inline int nsleep(unsigned long seconds, unsigned long nano_seconds)
{
#ifdef HAVE_TIME_H
    struct timespec t, rem;

    t.tv_sec = seconds;
    t.tv_nsec = nano_seconds;

    int result = nanosleep(&t, &rem);

    return result;
#endif
}
}

#endif


#include "OcpiTimer.h"

#include <sys/time.h>

namespace
{
  const double usecs_per_seconds = 1000000.0;

  const double msecs_per_usec = 1000.0;
}

void OCPI::Timer::start ( )
{
  struct timeval tv;

  gettimeofday ( &tv, 0 );

  d_start_time = tv.tv_usec + ( tv.tv_sec * usecs_per_seconds );
}


void OCPI::Timer::stop ( )
{
  struct timeval tv;

  gettimeofday ( &tv, 0 );

  double stop_time = tv.tv_usec + ( tv.tv_sec * usecs_per_seconds );

  d_elapsed_time = ( stop_time - d_start_time ) / usecs_per_seconds;
}


double OCPI::Timer::elapsedTime ( ) const
{
  // Return time in milliseconds
  return d_elapsed_time / msecs_per_usec;
}


#ifndef INCLUDED_OCPI_TIMER_H
#define INCLUDED_OCPI_TIMER_H

namespace OCPI
{
  class Timer
  {
    public:
      void start ( );

      void stop ( );

      double elapsedTime ( ) const;

    private:
      double d_start_time;

      double d_elapsed_time;
  };

} // End: namespace OCPI

#endif  // End: #ifndef INCLUDED_OCPI_TIMER_H

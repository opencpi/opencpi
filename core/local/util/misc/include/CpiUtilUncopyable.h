
/**
  @file

  @brief
    Provides a mix-in to make a class Uncopyable.

  Revision History:

    10/13/2008 - Michael Pepe
                 Initial version.

************************************************************************** */


#ifndef INCLUDED_CPI_UTIL_UNCOPYABLE_H
#define INCLUDED_CPI_UTIL_UNCOPYABLE_H

namespace CPI
{
  namespace Util
  {
    /**
      @class Uncopyable

      @brief
        Base class designed to prevent copying.

      Private inheritance from the Uncopyable class disallows copy
      construction or copy assignment.

    ********************************************************************* */

    class Uncopyable
    {
      protected:

        Uncopyable ( )
        throw ()
        {
          // Empty
        }

        ~Uncopyable ( )
        throw ()
        {
          // Empty
        }

      private:

        //! Not implemented
        Uncopyable ( const Uncopyable& );

        //! Not implemented
        const Uncopyable& operator= ( const Uncopyable& );
    };

  } // End: namespace Util

} // End: namespace CPI

#endif // End: #ifndef INCLUDED_CPI_UTIL_UNCOPYABLE_H


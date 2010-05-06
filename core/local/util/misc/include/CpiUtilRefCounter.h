// -*- c++ -*-

#ifndef CPIUTILREFCOUNTER_H__
#define CPIUTILREFCOUNTER_H__

/**
 * \file
 * \brief Utility base class used for reference counting an object.
 *
 * Revision History:
 *
 *     08/10/2005 - John Miller
 *                  Initial version.
 */

namespace CPI {
  namespace Util {
    namespace Misc {

      /**
       * \brief Utility base class used for reference counting an object.
       *
       * \note The implementation is not thread safe.
       */

      class RefCounter {
      public:
        RefCounter();
        virtual ~RefCounter();
        int incRef();
        int decRef();
      private:
        int refCount;
        
      };

    }
  }
}

#endif

// -*- c++ -*-

#ifndef CPI_CFUTIL_STRINGIFY_CF_EXCEPTION_H__
#define CPI_CFUTIL_STRINGIFY_CF_EXCEPTION_H__

/**
 * \file
 * \brief Map an SCA exception to a string.
 *
 * Revision History:
 *
 *     10/13/2008 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <string>
#include <CF.h>

namespace CPI {
  namespace CFUtil {

    /**
     * Map an SCA exception to a string.
     *
     * \param[in] ex An SCA exception.
     * \return A human-readable string describing the exception.
     */

    std::string stringifyCFException (const CORBA::Exception & ex)
      throw ();

  }
}

#endif

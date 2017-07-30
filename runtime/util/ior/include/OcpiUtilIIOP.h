/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// -*- c++ -*-

#ifndef OCPIUTILIIOP_H__
#define OCPIUTILIIOP_H__

/**
 * \file
 * \brief Introspecting and manipulating IIOP IOR profiles.
 *
 * Revision History:
 *
 *     05/24/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <OcpiOsDataTypes.h>
#include <string>
#include <vector>
#include "OcpiUtilIOP.h"

namespace OCPI {
  namespace Util {
    /**
     * \brief Introspecting and manipulating IIOP IOR profiles.
     */

    namespace IIOP {

      /**
       * \brief IIOP version
       *
       * IIOP version as specified in CORBA section 15.7.2,
       * <em>IIOP IOR Profiles</em>.
       */

      struct Version {
        unsigned char major;
        unsigned char minor;
      };

      /**
       * \brief IIOP ProfileBody
       *
       * IIOP ProfileBody as specified in CORBA section 15.7.2,
       * <em>IIOP IOR Profiles</em>.  A ProfileBody identifies
       * individual objects accessible through the IIOP protocol.
       */

      class ProfileBody {
      public:
        /**
         * Default constructor.
         */

        ProfileBody ()
          throw ();

        /**
         * Constructor from encapsulated data.
         *
         * \param[in] data A CDR-encapsulated IIOP ProfileBody, using
         *                 IIOP versions 1.0, 1.1, 1.2 or 1.3.
         *
         * \throw std::string If the ProfileBody can not be unmarshalled
         * from \a data.
         */

        explicit ProfileBody (const std::string & data)
          throw (std::string);

        /**
         * Copy constructor.
         *
         * \param[in] other Another ProfileBody.
         */

        ProfileBody (const ProfileBody & other)
          throw ();

        /**
         * Assignment operator.
         *
         * \param[in] other Another ProfileBody.
         * \return *this
         */

        ProfileBody & operator= (const ProfileBody & other)
          throw ();

        /**
         * Assign from CDR-encapsulated data.
         *
         * \param[in] data A CDR-encapsulated IIOP ProfileBody, using
         *                 IIOP versions 1.0, 1.1, 1.2 or 1.3.
         *
         * \throw std::string If the ProfileBody can not be unmarshalled
         * from \a data.
         *
         * \note Equivalent to operator= (ProfileBody (data)).
         */

        void decode (const std::string & data)
          throw (std::string);

        /**
         * Marshal this ProfileBody as CDR-encapsulated data.
         *
         * \return The CDR-encapsulated ProfileBody.
         *
         * \throw std::string If the IIOP version in #iiop_version
         * is not 1.0, 1.1, 1.2 or 1.3.
         */

        std::string encode () const
          throw (std::string);

        /**
         * \name Access to tagged components
         */

        //@{

        /**
         * Add a component to the set of tagged components.
         *
         * \param[in] tag  The tag value.
         * \param[in] data Pointer to the data associated with the tag.  The
         *                 format of the data is component-specific.
         * \param[in] len  The length of the data, in octets.
         *
         * \pre !#hasComponent(\a tag)
         */

        void addComponent (OCPI::Util::IOP::ComponentId tag, const void * data, unsigned long len)
          throw ();

        /**
         * Add a component to the set of tagged components.
         *
         * \param[in] tag  The tag value.
         * \param[in] data The data associated with the tag.  The format of
         *                 the data is component-specific.
         *
         * \pre !#hasComponent(\a tag)
         */

        void addComponent (OCPI::Util::IOP::ComponentId tag, const std::string & data)
          throw ();

        /**
         * Check if the profile contains a tagged component.
         *
         * \param[in] tag  The tag value to search.
         * \return         true if the profile contains a component with this
         *                 tag, false if not.
         */

        bool hasComponent (OCPI::Util::IOP::ComponentId tag)
          throw ();

        /**
         * Access the data associated with a component (non-const version).
         * The returned reference can be used to modify the tagged component
         * data.
         *
         * \param[in] tag  The tag value to search.
         * \return         The data associated with the tag.
         *
         * \throw std::string If the profile does not contain a component
         * tagged \a tag.
         *
         * \pre #hasComponent(\a tag).
         */

        std::string & componentData (OCPI::Util::IOP::ComponentId tag)
          throw (std::string);

        /**
         * Access the data associated with a component (const version).
         *
         * \param[in] tag  The tag value to search.
         * \return         The data associated with the tag.
         *
         * \throw std::string If the profile does not contain a component
         * tagged \a tag.
         *
         * \pre #hasComponent(\a tag).
         */

        const std::string & componentData (OCPI::Util::IOP::ComponentId tag) const
          throw (std::string);

        /**
         * The number of tagged components in this profile.
         *
         * \return The number of tagged components in this profile.
         */

        unsigned long numComponents () const
          throw ();

        /**
         * Access to a tagged component (non-const version).
         * The returned reference can be used to modify the tagged component
         * data.
         *
         * \param[in] idx The index of the component to access.
         * \return        The profile.
         *
         * \throw std::string If \a idx exceeds the number of components in
         * this profile.
         *
         * \pre \a idx < #numComponents().
         */

        OCPI::Util::IOP::TaggedComponent & getComponent (unsigned long idx)
          throw (std::string);

        /**
         * Access to a tagged component (const version).
         *
         * \param[in] idx The index of the component to access.
         * \return        The profile.
         *
         * \throw std::string If \a idx exceeds the number of components in
         * this profile.
         *
         * \pre \a idx < #numComponents().
         */

        const OCPI::Util::IOP::TaggedComponent & getComponent (unsigned long idx) const
          throw (std::string);

        //@}

      public:
        /**
         * \name Data members
         */

        //@{

        /**
         * The version of IIOP that the agent at the specified address
         * is prepared to receive.
         */

        Version iiop_version;

        /**
         * The internet host to which GIOP messages for the specified
         * object may be sent.
         */

        std::string host;

        /**
         * The TCP/IP port number (at the specified host) where the
         * target agent is listening for connection requests.
         */

        OCPI::OS::uint16_t port;

        /**
         * An opaque value supplied by the agent producing the IOR.
         * This value is used in request messages to identify the
         * object to which the request is directed.
         */

        std::string object_key;

        /**
         * A sequence of OCPI::Util::IOP::TaggedComponent which contains
         * additional information that may be used in making invocations
         * on the object described by this profile.  Not present in
         * IIOP version 1.0.
         */

        OCPI::Util::IOP::TaggedComponentSeq components;

        //@}
      };

    }
  }
}

#endif

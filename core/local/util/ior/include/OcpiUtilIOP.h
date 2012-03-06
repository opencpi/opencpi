
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */

// -*- c++ -*-

#ifndef OCPIUTILIOP_H__
#define OCPIUTILIOP_H__

/**
 * \file
 * \brief Introspecting and manipulating CORBA object references.
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

namespace OCPI {
  namespace Util {
    /**
     * \brief Introspecting and manipulating CORBA object references.
     */

    namespace IOP {

      /**
       * \name Tagged profiles
       */

      //@{

      /**
       * Tagged Profile tag value as specified in CORBA section 13.6.2,
       * <em>Interoperable Object References: IORs</em>.
       */

      typedef OCPI::OS::uint32_t ProfileId;

      /**
       * \brief CORBA IOR Tagged Profile
       *
       * Tagged Profile as specified in CORBA section 13.6.2,
       * <em>Interoperable Object References: IORs</em>.
       *
       * If <tt>tag == TAG_INTERNET_IOP</tt>, then <tt>profile_data</tt>
       * is an encapsulated OCPI::Util::IIOP::ProfileBody.
       *
       * If <tt>tag == TAG_MULTIPLE_COMPONENTS</tt>, then <tt>profile_data</tt>
       * is an encapsulated OCPI::Util::IOP::MultipleComponentProfile.
       */

      struct TaggedProfile {
        ProfileId tag;
        std::string profile_data;
      };

      /**
       * Sequence of tagged profiles as specified in CORBA section 13.6.2,
       * <em>Interoperable Object References: IORs</em>.
       */

      typedef std::vector<TaggedProfile> TaggedProfileSeq;

      /**
       * Identifies an IOR profile as an IIOP IOR Profile ProfileBody.
       * Use OCPI::Util::IIOP::ProfileBody.
       */

      extern const ProfileId TAG_INTERNET_IOP;

      /**
       * Identifies an IOR profile as a multiple component profile.
       * Use OCPI::Util::IOP::MultipleComponentProfile.
       */

      extern const ProfileId TAG_MULTIPLE_COMPONENTS;

      //@}

      /**
       * Tagged Component tag value as specified in CORBA section 13.6.2,
       * <em>Interoperable Object References: IORs</em>.
       */

      typedef OCPI::OS::uint32_t ComponentId;

      /**
       * \brief CORBA IOR Tagged Component
       *
       * Tagged Component as specified in CORBA section 13.6.2,
       * <em>Interoperable Object References: IORs</em>.
       */

      struct TaggedComponent {
        ComponentId tag;
        std::string component_data;
      };

      /**
       * Sequence of tagged components as specified in CORBA section 13.6.2,
       * <em>Interoperable Object References: IORs</em>.
       */

      typedef std::vector<TaggedComponent> TaggedComponentSeq;

      /**
       * \brief Interoperable Object Reference.
       *
       * IOR as specified in CORBA section 13.6.2,
       * <em>Interoperable Object References: IORs</em>.
       */

      class IOR {
      public:
        /**
         * Default constructor.
         *
         * Initializes the IOR to represent a <em>nil</em> object
         * reference, with an empty <em>type_id</em> field and an
         * empty set of profiles.
         */

        IOR ()
          throw ();

        /**
         * Constructor from encapsulated data.
         *
         * \param[in] data A CDR-encapsulated IOR.
         *
         * \throw std::string If the IOR can not be unmarshalled from
         * \a data.
         */

        explicit IOR (const std::string & data)
          throw (std::string);

        /**
         * Constructor from stringified IOR
         *
         * \param[in] data A stringified IOR.
         *
         * \throw std::string If the IOR can not be unmarshalled from
         * \a data.
         */
        explicit IOR (const char *data)
          throw (std::string);

        /**
         * Copy constructor.
         *
         * \param[in] other Another IOR.
         */

        IOR (const IOR & other)
          throw ();

        /**
         * Assignment operator.
         *
         * \param[in] other Another IOR.
         * \return *this
         */

        IOR & operator= (const IOR & other)
          throw ();

        /**
         * Assign from CDR-encapsulated data.
         *
         * \param[in] data A CDR-encapsulated IOR.
         *
         * \throw std::string If the ProfileBody can not be unmarshalled
         * from \a data.
         *
         * \note Equivalent to operator= (IOR (data)).
         */

        void decode (const std::string & data)
          throw (std::string);

        /**
         * Marshal this IOR as CDR-encapsulated data.
         *
         * \return The CDR-encapsulated IOR.
         */

        std::string encode () const
          throw ();

        /**
         * Access the IOR's <em>type_id</em> field.
         *
         * \return The IOR's <em>type_id</em>.
         */

        const std::string & type_id () const
          throw ();

        /**
         * Set the IOR's <em>type_id</em> field.
         *
         * \param[in] id The new value for the <em>type_id</em> field, i.e.,
         * the most derived type that the server wishes to publish.
         */

        void type_id (const std::string & id)
          throw ();

        /**
         * \name Access to tagged profiles
         */

        //@{

        /**
         * Add a profile to the IOR's set of profiles.
         *
         * \param[in] tag  The tag value.
         * \param[in] data Pointer to the data associated with the tag.  The
         *                 format of the data is profile-specific.
         * \param[in] len  The length of the data, in octets.
         *
         * \pre !#hasProfile(\a tag)
         */

        void addProfile (ProfileId tag, const void * data, unsigned long len)
          throw ();

        /**
         * Add a profile to the IOR's set of profiles.
         *
         * \param[in] tag  The tag value.
         * \param[in] data The data associated with the tag.  The format of
         *                 the data is profile-specific.
         *
         * \pre !#hasProfile(\a tag)
         */

        void addProfile (ProfileId tag, const std::string & data)
          throw ();

        /**
         * Check if the IOR contains a tagged profile.
         *
         * \param[in] tag  The tag value to search.
         * \return         true if the IOR contains a profile with this tag,
         *                 false if not.
         */

        bool hasProfile (ProfileId tag)
          throw ();

        /**
         * Access the data associated with a profile (non-const version).
         * The returned reference can be used to modify the profile data.
         *
         * \param[in] tag  The tag value to search.
         * \return         The data associated with the tag.
         *
         * \throw std::string if the IOR does not contain a profile
         * tagged \a tag.
         *
         * \pre #hasProfile(\a tag).
         */

        std::string & profileData (ProfileId tag)
          throw (std::string);

        /**
         * Access the data associated with a profile (const version).
         *
         * \param[in] tag  The tag value to search.
         * \return         The data associated with the tag.
         *
         * \throw std::string if the IOR does not contain a profile
         * tagged \a tag.
         *
         * \pre #hasProfile(\a tag).
         */

        const std::string & profileData (ProfileId tag) const
          throw (std::string);

        /**
         * The number of profiles in this IOR.
         *
         * \return The number of profiles in this IOR.
         */

        unsigned long numProfiles () const
          throw ();

        /**
         * Access to a profile (non-const version).
         * The returned reference can be used to modify the profile data.
         *
         * \param[in] idx The index of the profile to access.
         * \return        The profile.
         *
         * \throw std::string If \a idx exceeds the number of profiles
         * in this IOR.
         *
         * \pre \a idx < #numProfiles().
         */

        TaggedProfile & getProfile (unsigned long idx)
          throw (std::string);

        /**
         * Access to a profile (const version).
         *
         * \param[in] idx The index of the profile to access.
         * \return        The profile.
         *
         * \throw std::string If \a idx exceeds the number of profiles
         * in this IOR.
         *
         * \pre \a idx < #numProfiles().
         */

        const TaggedProfile & getProfile (unsigned long idx) const
          throw (std::string);

        //@}

        const std::string & corbaloc()
          throw (std::string);
      protected:
        std::string m_type_id;
	std::string m_corbaloc;
        TaggedProfileSeq m_profiles;
      private:
	void doKey(const std::string &key) throw();
	void doAddress(const std::string &addr) throw ();
	void doIPAddress(unsigned major, unsigned minor, const std::string &host, uint16_t port) throw ();
	void doComponent(TaggedComponent &tc) throw (std::string);
      };

      /**
       * \name Convert between IOR objects and stringified object references
       */

      //@{

      /**
       * Convert a stringified object reference to an IOR object.
       *
       * \param[in] hex A stringified object reference, matching the
       *                pattern <tt>IOR:[0-9a-fA-F]*</tt>.
       * \return        An IOR object.
       *
       * \throw std::string If the string is not a valid stringified IOR.
       */

      IOR string_to_ior (const std::string & hex)
        throw (std::string);

      /**
       * Produce a stringified IOR from an IOR object.
       *
       * \param[in] ior An IOR object.
       * \return        A stringified object reference.
       */

      std::string ior_to_string (const IOR & ior)
        throw ();

      //@}

      /**
       * \name Tagged Components
       */

      //@{


      /**
       * Identifies a tagged component as a <em>TAG_ORB_TYPE</em>
       * component.  Use OCPI::Util::IOP::ORBTypeComponent.
       */

      extern const ComponentId TAG_ORB_TYPE;

      /**
       * Identifies a tagged component as a <em>TAG_CODE_SETS</em>
       * component.  See CORBA section 13.10.2.4, <em>CodeSet Component
       * of IOR Multi-Component Profile</em>.
       */

      extern const ComponentId TAG_CODE_SETS;

      /**
       * Identifies a tagged component as a <em>TAG_POLICIES</em>
       * component.  See CORBA Messaging chapter 22.
       */

      extern const ComponentId TAG_POLICIES;

      /**
       * Identifies a tagged component as a <em>TAG_ALTERNATE_IIOP_ADDRESS</em>
       * component.  Use OCPI::Util::IOP::AlternateIIOPAddressComponent.
       */

      extern const ComponentId 
	TAG_ALTERNATE_IIOP_ADDRESS,
	TAG_OMNIORB_UNIX_TRANS,
	TAG_OMNIORB_PERSISTENT_ID,
	TAG_OMNIORB_RESTRICTED_CONNECTION,
	TAG_OMNIORB_OCPI_TRANS;

      /**
       * \brief A list of protocol components for an IOR.
       *
       * MultipleComponentProfile as specified in CORBA section 13.6.4,
       * <em>Standard IOR Profiles</em>.
       */

      class MultipleComponentProfile {
      public:
        /**
         * Default constructor.
         */

        MultipleComponentProfile ()
          throw ();

        /**
         * Constructor from encapsulated data.
         *
         * \param[in] data CDR-encapsulated data.
         *
         * \throw std::string If the MultipleComponentProfile can not be
         * unmarshalled from \a data.
         */

        explicit MultipleComponentProfile (const std::string & data)
          throw (std::string);

        /**
         * Copy constructor.
         *
         * \param[in] other Another MultipleComponentProfile.
         */

        MultipleComponentProfile (const MultipleComponentProfile & other)
          throw ();

        /**
         * Assignment operator.
         *
         * \param[in] other Another MultipleComponentProfile.
         * \return *this
         */

        MultipleComponentProfile & operator= (const MultipleComponentProfile & other)
          throw ();

        /**
         * Assign from CDR-encapsulated data.
         *
         * \param[in] data CDR-encapsulated data.
         *
         *
         * \throw std::string If the MultipleComponentProfile can not be
         * unmarshalled from \a data.
         *
         * \note Equivalent to operator= (MultipleComponentProfile (data)).
         */

        void decode (const std::string & data)
          throw (std::string);

        /*
         * Marshal this MultipleComponentProfile as CDR-encapsulated data.
         *
         * \return The CDR-encapsulated MultipleComponentProfile.
         */

        std::string encode () const
          throw ();

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

        void addComponent (ComponentId tag, const void * data, unsigned long len)
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

        void addComponent (ComponentId tag, const std::string & data)
          throw ();

        /**
         * Check if the profile contains a tagged component.
         *
         * \param[in] tag  The tag value to search.
         * \return         true if the profile contains a component with this
         *                 tag, false if not.
         */

        bool hasComponent (ComponentId tag)
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

        std::string & componentData (ComponentId tag)
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

        const std::string & componentData (ComponentId tag) const
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

        TaggedComponent & getComponent (unsigned long idx)
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

        const TaggedComponent & getComponent (unsigned long idx) const
          throw (std::string);

        //@}

      protected:
        TaggedComponentSeq m_components;
      };

      /**
       * \brief OCPI_CORBA_ORB Type Component
       *
       * The OCPI_CORBA_ORB Type Component is encapsulated in a tagged component
       * tagged OCPI::Util::IOP::TAG_ORB_TYPE.
       *
       * See CORBA section 13.6.6.1, <em>TAG_ORB_TYPE Component</em>.
       *
       * The <em>TAG_ORB_TYPE</em> component can appear at most once in
       * any IOR profile.  For profiles supporting IIOP 1.1 or greater,
       * it is optionally present.
       *
       * See also http://doc.omg.org/vendor-tags for a list of OCPI_CORBA_ORB type
       * descriptions and values.
       */

      class ORBTypeComponent {
      public:
        /**
         * Default constructor.
         */

        ORBTypeComponent ()
          throw ();

        /**
         * Constructor.
         *
         * \param[in] type The OCPI_CORBA_ORB type ID.
         */

        explicit ORBTypeComponent (OCPI::OS::uint32_t type)
          throw ();

        /**
         * Constructor from encapsulated data.
         *
         * \param[in] data A CDR-encapsulated <em>TAG_ORB_TYPE</em> component.
         *
         * \throw std::string If the data can not be unmarshalled as a
         * <em>TAG_ORB_TYPE</em> component.
         */

        explicit ORBTypeComponent (const std::string & data)
          throw (std::string);

        /**
         * Copy constructor.
         *
         * \param[in] other Another <em>TAG_ORB_TYPE</em> component.
         */

        ORBTypeComponent (const ORBTypeComponent & other)
          throw ();

        /**
         * Assignment operator.
         *
         * \param[in] type The OCPI_CORBA_ORB type ID.
         */

        ORBTypeComponent & operator= (OCPI::OS::uint32_t type)
          throw ();

        /**
         * Assignment operator.
         *
         * \param[in] other Another <em>TAG_ORB_TYPE</em> component.
         * \return *this
         */

        ORBTypeComponent & operator= (const ORBTypeComponent & other)
          throw ();

        /**
         * Assign from CDR-encapsulated data.
         *
         * \param[in] data A CDR-encapsulated <em>TAG_ORB_TYPE</em> component.
         *
         * \throw std::string If the data can not be unmarshalled as a
         * <em>TAG_ORB_TYPE</em> component.
         */

        void decode (const std::string & data)
          throw (std::string);

        /*
         * Marshal this component as CDR-encapsulated data.
         *
         * \return The CDR-encapsulated <em>TAG_ORB_TYPE</em> component.
         */

        std::string encode () const
          throw ();

      public:
        /**
         * The OCPI_CORBA_ORB type ID.
         */

        OCPI::OS::uint32_t orb_type;
      };

      /**
       * \brief Alternate IIOP Address Component
       *
       * The Alternate IIOP Address Component is encapsulated in a
       * tagged component tagged
       * OCPI::Util::IOP::TAG_ALTERNATE_IIOP_ADDRESS.
       *
       * See CORBA section 13.6.6.2, <em>TAG_ALTERNATE_IIOP_ADDRESS Component</em>.
       *
       * Zero or more instances of this component type may be included in
       * a version 1.2 <em>TAG_INTERNET_IOP</em> profile.  Each of these
       * alternative addresses may be used by the client OCPI_CORBA_ORB, in addition
       * to the host and port address expressed in the body of the Profile.
       */

      class AlternateIIOPAddressComponent {
      public:
        /**
         * Default constructor.
         */

        AlternateIIOPAddressComponent ()
          throw ();

        /**
         * Constructor from encapsulated data.
         *
         * \param[in] data A CDR-encapsulated
         *                 <em>TAG_ALTERNATE_IIOP_ADDRESS</em> component.
         *
         * \throw std::string If the data can not be unmarshalled as a
         * <em>TAG_ALTERNATE_IIOP_ADDRESS</em> component.
         */

        explicit AlternateIIOPAddressComponent (const std::string & data)
          throw (std::string);

        /**
         * Copy constructor.
         *
         * \param[in] other Another <em>TAG_ALTERNATE_IIOP_ADDRESS</em>
         *                  component.
         */

        AlternateIIOPAddressComponent (const AlternateIIOPAddressComponent & other)
          throw ();

        /**
         * Assignment operator.
         *
         * \param[in] other Another <em>TAG_ALTERNATE_IIOP_ADDRESS</em>
         *                  component.
         * \return *this
         */

        AlternateIIOPAddressComponent & operator= (const AlternateIIOPAddressComponent & other)
          throw ();

        /**
         * Assign from CDR-encapsulated data.
         *
         * \param[in] data A CDR-encapsulated
         *                 <em>TAG_ALTERNATE_IIOP_ADDRESS</em> component.
         *
         * \throw std::string If the data can not be unmarshalled as a
         * <em>TAG_ALTERNATE_IIOP_ADDRESS</em> component.
         */

        void decode (const std::string & data)
          throw (std::string);

        /**
         * Marshal this component as CDR-encapsulated data.
         *
         * \return The CDR-encapsulated
         * <em>TAG_ALTERNATE_IIOP_ADDRESS</em> component.
         */

        std::string encode () const
          throw ();

      public:
        /**
         * The alternate IIOP address' host.
         */

        std::string HostID;

        /**
         * The alternate IIOP address' port.
         */

        OCPI::OS::uint16_t port;
      };

    }
  }
}

#endif

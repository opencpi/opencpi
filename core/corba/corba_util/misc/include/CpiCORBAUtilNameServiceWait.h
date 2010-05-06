// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

// -*- c++ -*-

#ifndef CPI_CORBAUTIL_NAMESERVICEWAIT_H__
#define CPI_CORBAUTIL_NAMESERVICEWAIT_H__

/**
 * \file
 * \brief Wait for an object to be bound to the Name Service.
 *
 * Revision History:
 *
 *     09/19/2008 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <string>
#include <corba.h>
#include <CpiOsMutex.h>
#include <CosNaming.h>

namespace CPI {
  namespace CORBAUtil {

    /**
     * \brief Wait for an object to be bound to the Name Service.
     *
     * This class partially implements the CosNaming::NamingContext API
     * and waits for a client to bind an object using a well-known name.
     *
     * The motivation is that, when executing a worker using the SCA
     * Executable Device interface, the worker expects to bind with
     * the Naming Service, and there is no other way to get the worker's
     * object reference than via this binding.
     */

    class WaitForNameServiceBinding {
    public:
      WaitForNameServiceBinding (CORBA::ORB_ptr orb,
                                 PortableServer::POA_ptr poa,
                                 const CosNaming::Name & expectedBinding)
        throw (std::string);
      ~WaitForNameServiceBinding ()
        throw ();

      CosNaming::NamingContext_ptr getContext ()
        throw ();
      bool waitForBinding (unsigned long timeoutInSeconds)
        throw ();
      CORBA::Object_var getBinding ()
        throw ();

    public:
      /*
       * The implementation extends this base class with an implementation
       * of POA_CosNaming::NamingContext, but we don't want to expose that
       * here.
       */

      class NamingContextBase {
      public:
        NamingContextBase (const CosNaming::Name & expectedBinding)
          throw ();
        virtual ~NamingContextBase ()
          throw ();

        bool haveBinding ()
          throw ();
        CORBA::Object_ptr getBinding ()
          throw ();

      protected:
        static bool namesAreEqual (const CosNaming::Name & n1,
                                   const CosNaming::Name & n2)
          throw ();

      protected:
        bool m_haveBinding;
        CosNaming::Name m_expectedBinding;
        CORBA::Object_var m_boundObject;
        CPI::OS::Mutex m_mutex;
      };

    protected:
      CORBA::ORB_var m_orb;
      PortableServer::POA_var m_poa;
      PortableServer::ObjectId_var m_oid;
      CosNaming::NamingContext_var m_context;
      NamingContextBase * m_myNamingContext;
    };

  }
}

#endif

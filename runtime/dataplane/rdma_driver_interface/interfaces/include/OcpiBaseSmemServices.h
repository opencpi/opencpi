
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


//        BaseSmemServices is a base class used by most/all implementations

#ifndef OCPI_BASE_SMEM_SERVICES_H_
#define OCPI_BASE_SMEM_SERVICES_H_

#include <map>
#include <string>
#include "OcpiUtilMisc.h"
#include "DtSharedMemoryInterface.h"
#include "DtExceptions.h"

namespace DataTransfer {
  namespace OU = OCPI::Util;
  class BaseSmem
  {
  public:
    // Constructor stores arguments and inits other properties.
    BaseSmem (EndPoint* loc) 
      : m_location(loc), m_size(loc->size), m_refcnt (0), m_mapcnt (0), m_mappedva (0), 
	m_mappedoffset (0), m_reqoffset (0), m_mappedsize (0) {
      mapEndPoint(*loc, m_name);
    }
    virtual ~BaseSmem() {}
    static void mapEndPoint(EndPoint &ep, std::string &out) {
      const char *semi = strrchr(ep.end_point.c_str(), ';');
      assert(semi);
      OU::format(out, "/%.*s", (int)(semi - ep.end_point.c_str()), ep.end_point.c_str());
      for (unsigned n = 1; n < out.length(); n++)
	if (out[n] == '/')
	  out[n] = '~';
    }

      // Properties of an BaseSmem instance
  public:
    std::string                         m_name;         // Name of this shared memory area
    EndPoint                           *m_location;     // Location of this shared memory area
    size_t                              m_size;         // Size of area
    int                                 m_refcnt;       // Reference count of create/attach calls
    int                                 m_mapcnt;       // Reference count of number of successful map calls
    void*                               m_mappedva;     // Virtual address of mapped area
    uint64_t                            m_mappedoffset; // Offset of where mapping was actually done (what mappedva points to)
    uint64_t                            m_reqoffset;    // Offset that was request in last Map call
    size_t                              m_mappedsize;   // Size of area that was mapped
  };

  class BaseSmemServices : public DataTransfer::SmemServices
  {
    // Public methods available to clients
  public:

    // THIS IS THE NEW CREATE !!
    //    virtual void create (EndPoint* loc) = 0;

    // Close shared memory object.
    virtual void close () = 0;

    // Attach to an existing shared memory object by name.
    virtual OCPI::OS::int32_t attach (EndPoint*) = 0;

    // Detach from shared memory object
    virtual OCPI::OS::int32_t detach () = 0;

    // Map a view of the shared memory area at some offset/size and return the virtual address.
    virtual void* map(DtOsDataTypes::Offset offset, size_t size ) = 0;

    // Unmap the current mapped view.
    virtual OCPI::OS::int32_t unMap () = 0;

    // Enable mapping
    virtual void* enable () = 0;

    // Disable mapping
    virtual OCPI::OS::int32_t disable () = 0;

    //        GetName - the name of the shared memory object
    virtual const char* getName ()
    {
      if (m_pSmem == 0)
        {
          OCPI_THROWNULL( DataTransferEx (RESOURCE_EXCEPTION, "BaseSmemServices::GetName: No active instance") );
        }
      return m_pSmem->m_name.c_str();
    }

    //        getEndPoint - the location of the shared area as an enumeration
    virtual EndPoint* getEndPoint ()
    {
      if (m_pSmem == 0)
        {
          OCPI_THROWNULL( DataTransferEx (RESOURCE_EXCEPTION, "BaseSmemServices::getEndPoint: No active instance") );
        }
      return m_pSmem->m_location;
    }

    //        GetHandle - platform dependent opaque handle for current mapping
    virtual void* getHandle () = 0;

  public:

    // Ctor/dtor
    BaseSmemServices (EndPoint &loc ) 
      : DataTransfer::SmemServices(loc ), m_pSmem (0) 
      {
      }
      virtual ~BaseSmemServices () {}

      // Derived class helpers
  protected:
      // Add a new name->BaseSmem dictionary entry
      static void add (BaseSmem* pSmem)
        {
          s_cache[pSmem->m_name] = pSmem;
          pSmem->m_refcnt++;
        }

      // Remove an instance from the dictionary and initialize the state.
      void remove (BaseSmem* pSmem)
      {
        if (pSmem->m_refcnt)
          {
            OCPI_THROWVOID( DataTransferEx (RESOURCE_EXCEPTION, "BaseSmemServices::Remove: Active references exist"));
          }
        std::map<std::string, BaseSmem*>::iterator pos = s_cache.find(pSmem->m_name);
        if (pos != s_cache.end ())
          {
            s_cache.erase (pos);
          }
      }

      // Lookup an existing named shared memory object.
    BaseSmem* lookup(EndPoint &ep) {
	  std::string name;
	  BaseSmem::mapEndPoint(ep, name);
          BaseSmem* pSmem = 0;
          std::map<std::string, BaseSmem*>::iterator pos = s_cache.find (name);
          if (pos != s_cache.end ())
            {
              // Successful Lookup bumps the reference count
              pSmem = pos->second;
              pSmem->m_refcnt++;
            }

          return pSmem;
        }

  protected:
      // The currently attached (via Create or Attach) instance.
      BaseSmem*        m_pSmem;

      // Our location
      EndPoint *m_location;

  private:
        
      static std::map<std::string, BaseSmem*>        s_cache;
        
  };

};


#endif


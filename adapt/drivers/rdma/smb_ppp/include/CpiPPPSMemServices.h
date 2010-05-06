//        BaseSmemServices is a base class used by most/all implementations

#ifndef CPI_BASE_SMEM_SERVICES_H
#define CPI_BASE_SMEM_SERVICES_H


#include <map>
#include <vector>
#include <string>
#include <CpiOsMutex.h>
#include <DtSharedMemoryInterface.h>
#include "rose/rose_defs.h"
#include "rose/rose_user_if.h"

namespace DataTransfer {

  /**********************************
   * This is our PPP shared memory location implementation.  The format of the 
   * address is as follows:
   *     "[rio]://tid,name:300000.1.2"
   *  Where:
   *      tid:  is the target id
   *      name: is the name of the endpoint
   *********************************/
  class  PPPEndPoint : public EndPoint 
  {
  public:

    // Constructors
  PPPEndPoint(long s=0)
    :EndPoint(s){};
    virtual ~PPPEndPoint();
  PPPEndPoint( std::string& ep, CPI::OS::uint32_t size=0)
    :EndPoint(ep,size){setEndpoint(ep);};

    // Sets smem location data based upon the specified endpoint
    virtual CPI::OS::int32_t setEndpoint( std::string& ep );

    // Get the address from the endpoint
    virtual const char* getAddress(){return p_virt_addr;}

    size_t             n_bytes;
    std::string        ep_name;
    char*              p_virt_addr;

    RoseUint32Type     target_id;
    RoseUserOffsetType paddr;   
    RoseEndpointInfo   ep_info;
    RoseMemoryInfo     mem_info;
    RoseUint32Type     actual_n_bytes;

  };

  class PPPSmemServices : public DataTransfer::SmemServices
    {
      // Public methods available to clients
    public:

      // Create the service
      void create (EndPoint* loc, CPI::OS::uint32_t size);

      // Close shared memory object.
      void close ();

      // Attach to an existing shared memory object by name.
      CPI::OS::int32_t attach (EndPoint*);

      // Detach from shared memory object
      CPI::OS::int32_t detach ();

      // Map a view of the shared memory area at some offset/size and return the virtual address.
      void* map (CPI::OS::uint64_t offset, CPI::OS::uint32_t size );

      //  Externalize an offset into this SMB so that it is "mappable" from outside 
      CPI::OS::uint32_t externalizeOffset( CPI::OS::uint32_t offset );

      // Unmap the current mapped view.
      CPI::OS::int32_t unMap ();

      // Enable mapping
      void* enable ();

      // Disable mapping
      CPI::OS::int32_t disable ();

      //        GetName - the name of the shared memory object
      const char* getName ()
      {
        return m_location->getAddress();
      }

      //        getEndPoint - the location of the shared area as an enumeration
      EndPoint* getEndPoint ()
      {
        return m_location;
      }

      //        GetHandle - platform dependent opaque handle for current mapping
      void* getHandle ();

      // Ctor/dtor
      PPPSmemServices ( EndPoint* loc ) 
        : DataTransfer::SmemServices( loc ), m_init(false)
        {
          m_location = dynamic_cast<PPPEndPoint*>(loc);
          create( loc, loc->size);

        }
      PPPSmemServices ();
      virtual ~PPPSmemServices ();
        
    protected:
        
      //  Our thread safe mutex
      static CPI::OS::Mutex m_threadSafeMutex;

      // Our location
      PPPEndPoint      *m_location;
      bool              m_init;
      CPI::OS::uint64_t m_last_offset;
      unsigned int      m_size;
                
      struct Map {
        RoseMapHandle   rio_handle;
        CPI::OS::uint64_t offset;
        unsigned int size;
        void*        vaddr;
      Map( RoseMapHandle h,CPI::OS::uint64_t o,unsigned int s, void* v ):rio_handle(h),offset(o),size(s),vaddr(v){};
      };
      std::vector<Map> m_map;

      // Rose handle to the shared memory block
      RosePmemHandle  m_handle;
      RoseMapHandle   m_rio_handle;

      // Our virtual address
      RoseVirtAddrType m_virt_addr;

    private:


    };

};


#endif


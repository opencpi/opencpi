#include <CpiOsDataTypes.h>
#include <cstdlib>
#include <memory>

namespace CPI {
  namespace Util {
    typedef CPI::OS::uint64_t ResAddrType;
    struct ResPool;
    class MemBlockMgr
    {
    public:
      MemBlockMgr(ResAddrType start, CPI::OS::uint64_t size)
	throw( std::bad_alloc );
      ~MemBlockMgr()
	throw();
      int alloc( CPI::OS::uint64_t nbytes, unsigned int alignment, ResAddrType& req_addr)
	throw( std::bad_alloc );
      int free(ResAddrType  addr )
	throw( std::bad_alloc );


    private:
      ResPool *m_pool;

    };
  }
}








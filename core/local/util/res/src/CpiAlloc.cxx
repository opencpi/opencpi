#define NDEBUG


#include <CpiOsDataTypes.h>
#include <CpiRes.h>
#include <list>


namespace CPI {
  namespace Util {
    struct Block {
      ResPool*               rp;
      CPI::OS::uint64_t       size;
      CPI::Util::ResAddrType  addr;
      CPI::Util::ResAddrType  aligned_addr;      
      Block( ResPool* rpp, CPI::OS::uint64_t s, CPI::OS::uint64_t adr,  CPI::OS::uint64_t a_adr)
        :rp(rpp),size(s),addr(adr),aligned_addr(a_adr){}
      int operator<(const Block& rhs)const;
    };
    struct ResPool {
      CPI::Util::ResAddrType  start_off;
      CPI::OS::uint64_t  total_size;
      std::list<Block> free_list;
      std::list<Block> alloc_list;
      bool             sort_by_size;
      ResPool():sort_by_size(true){}
      void defrag();
    };
    int Block::operator<(const Block& rhs)const
    {
      if ( rp->sort_by_size ) {
        if ( this->size < rhs.size ) return 1;
      }
      else {
        if ( this->addr < rhs.addr ) return 1;
      }
      return 0;
    }
  }
}

#ifndef NDEBUG
static void dumpList( std::list<CPI::Util::Block>& list )
{
  std::list<CPI::Util::Block>::iterator it;
  for ( it=list.begin(); it !=list.end(); it++ ) {
    printf("addr = %d, aligned_addr = %d, size = %lld\n", (*it).addr, (*it).aligned_addr, (long long)(*it).size );
  }
}
#endif


static CPI::Util::ResAddrType ALIGN( CPI::Util::ResAddrType addr, unsigned int alignment ) 
{
  if ( alignment == 0 ) {
    return addr;
  }
  CPI::Util::ResAddrType mask = alignment -1;
  return (addr & ~mask);
}


void CPI::Util::ResPool::defrag()
{
  sort_by_size = false;
  bool not_done;
  do {
    not_done=false;
    free_list.sort();
    std::list<Block>::iterator it1, it2;
    it1=it2=free_list.begin();
    it2++;
    for ( ; it2 !=free_list.end(); it2++, it1++ ) {
      if ( ((*it1).addr+(*it1).size) == (*it2).addr ) {
        (*it1).size += (*it2).size;
        free_list.erase(it2);
        not_done=true;
        break;
      }
    }
  }
  while ( not_done );
  sort_by_size = true;  

#ifndef NDEBUG
  printf("Alloc list\n");
  dumpList(alloc_list);
  printf("Free list\n");
  dumpList(free_list);
#endif

}


int CPI::Util::MemBlockMgr::alloc(CPI::OS::uint64_t nbytes, unsigned int alignment, CPI::Util::ResAddrType& req_addr)
  throw( std::bad_alloc ) 
{
  
  m_pool->free_list.sort();
  std::list<Block>::iterator it;
  int retry=2;
  nbytes += alignment/8;

  do {
    for ( it=m_pool->free_list.begin(); it !=m_pool->free_list.end(); it++ ) {
      if ( ((*it).size > nbytes) ) {
        req_addr = ALIGN((*it).addr, alignment);
        CPI::Util::ResAddrType taddr = (*it).addr;
        (*it).addr += nbytes;
        (*it).size -= nbytes;      
        m_pool->alloc_list.push_back( Block(m_pool, nbytes, taddr, ALIGN(req_addr, alignment)) );
#ifndef NDEBUG
        printf("**** Alloc Returning address = %d, %d\n", taddr, req_addr );
#endif
        return 0;
      }
      else if (  ((*it).size == nbytes) ) {
        req_addr = ALIGN((*it).addr, alignment);
        (*it).aligned_addr = req_addr;
        m_pool->alloc_list.push_back( *it );
        m_pool->free_list.erase( it );      
#ifndef NDEBUG
        printf("**** Alloc Returning address = %d, %d\n", (*it).addr, req_addr );
#endif
        return 0;
      }
    }
    retry--;
    m_pool->defrag();  
  } while ( retry );



#ifndef NDEBUG
  printf("Alloc list\n");
  dumpList(m_pool->alloc_list);
  printf("Free list\n");
  dumpList(m_pool->free_list);
#endif

  return -1;
}


int CPI::Util::MemBlockMgr::free( CPI::Util::ResAddrType addr )
  throw( std::bad_alloc ) 
{
  std::list<Block>::iterator it;
  for ( it=m_pool->alloc_list.begin(); it !=m_pool->alloc_list.end(); it++ ) {
    if ( (*it).aligned_addr == addr )  {
      m_pool->free_list.push_back( Block( m_pool, (*it).size, addr, addr ) );
      m_pool->alloc_list.erase( it );
      m_pool->defrag();  
      return 0;
    }
  }
  return -1;
}

CPI::Util::MemBlockMgr::MemBlockMgr(CPI::Util::ResAddrType start, CPI::OS::uint64_t size )
  throw( std::bad_alloc ) 
{
  
  m_pool = new CPI::Util::ResPool;
  m_pool->free_list.push_back( Block( m_pool, size, start, start ) );
}


CPI::Util::MemBlockMgr::~MemBlockMgr()
  throw()
{
  delete m_pool;
}



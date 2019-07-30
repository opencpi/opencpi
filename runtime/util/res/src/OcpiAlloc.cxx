//#define  DEBUG_LISTS 1
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

#include <inttypes.h>
#include <list>
#include <cstdio>
#include "OcpiOsDataTypes.h"
#include "OcpiOsAssert.h"
#include "OcpiUtilMisc.h"
#include "OcpiRes.h"

namespace OCPI {
  namespace Util {
    struct Block {
      ResPool*                 rp;
      size_t                   size;
      ResAddrType  addr;
      ResAddrType  aligned_addr;      
      Block( ResPool* rpp, size_t s, ResAddrType adr,  ResAddrType a_adr)
        :rp(rpp),size(s),addr(adr),aligned_addr(a_adr){}
      int operator<(const Block& rhs)const;
    };
    struct ResPool {
      OCPI::Util::ResAddrType  start_off;
      size_t  total_size, used;
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
static void dumpList( std::list<OCPI::Util::Block>& list )
{

#ifdef DEBUG_LISTS
  std::list<OCPI::Util::Block>::iterator it;
  for ( it=list.begin(); it !=list.end(); it++ ) {
    ocpiDebug("addr = %" PRIx32 ", aligned_addr = %" PRIx32 ", size = %zu", 
	      (*it).addr, 
	      (*it).aligned_addr, 
	      (*it).size );
  }
#else
  (void)list;
#endif

}
#endif

// Round the address UP to the next aligned boundary
static OCPI::Util::ResAddrType 
ALIGN(OCPI::Util::ResAddrType addr, unsigned int alignment) {
  return alignment ? (addr + alignment - 1) & ~(alignment - 1) : addr;
}


void OCPI::Util::ResPool::defrag()
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

#ifdef DEBUG_LISTS
  ocpiDebug("defrag: Alloc list");
  dumpList(alloc_list);
  ocpiDebug("defrag: Free list");
  dumpList(free_list);
#endif

}


int OCPI::Util::MemBlockMgr::
alloc(size_t nbytes, unsigned int alignment, OCPI::Util::ResAddrType& req_addr)
  throw(std::bad_alloc) 
{

  if (nbytes > 2000000) {
    ocpiInfo("Allocating large mem %zuK in %p %zu of %zu used",
	     nbytes/1024, this, m_pool->used, m_pool->total_size);
    // OCPI::OS::dumpStack(std::cerr);
  }
#ifdef DEBUG_LISTS
  ocpiDebug("alloc: Alloc list");
  dumpList(m_pool->alloc_list);
  ocpiDebug("alloc: Free list");
  dumpList(m_pool->free_list);
#endif

  m_pool->free_list.sort();
  for (unsigned retry = 2; retry; retry--) {
    for (std::list<Block>::iterator it = m_pool->free_list.begin(); it !=m_pool->free_list.end();
	 ++it) {
      ResAddr taddr = (*it).addr;
      req_addr = ALIGN(taddr, alignment);
      size_t skip = req_addr - taddr;
      if (skip >= (*it).size)
	continue;
      size_t size = (*it).size - skip; // size net of alignment
      if (size < nbytes)
	continue;
      if (size > nbytes) {
        (*it).addr = OCPI_UTRUNCATE(ResAddrType, req_addr + nbytes);
	nbytes += skip; // number of bytes taken out of this block
        (*it).size -= nbytes;
        m_pool->alloc_list.push_back(Block(m_pool, nbytes, taddr, req_addr));
      } else { // size matches
	nbytes = (*it).size;
        (*it).aligned_addr = req_addr;
        m_pool->alloc_list.push_back(*it);
        m_pool->free_list.erase(it);      
      }
      m_pool->used += nbytes;
      //ocpiDebug("**** Alloc of %zu Returning address = %" OCPI_UTIL_RESADDR_PRIx ", %" 
      //          OCPI_UTIL_RESADDR_PRIx " used %zu", nbytes, taddr, req_addr, m_pool->used);
      return 0;
    }
    m_pool->defrag();  
  }

#ifndef NDEBUG
  ocpiDebug("alloc2: Alloc list");
  dumpList(m_pool->alloc_list);
  ocpiDebug("alloc2: xoFree list");
  dumpList(m_pool->free_list);
#endif

  return -1;
}


int OCPI::Util::MemBlockMgr::free( OCPI::Util::ResAddrType addr )
  throw( std::bad_alloc ) 
{

  std::list<Block>::iterator it;
  for ( it=m_pool->alloc_list.begin(); it !=m_pool->alloc_list.end(); it++ ) {
    if ( (*it).aligned_addr == addr )  {

#ifndef NDEBUG
      if ( (*it).size > 2000000 ) {
	ocpiInfo("Freeing large mem %zuK in %p %zu of %zu used",
		 (*it).size/1024, this, m_pool->used, m_pool->total_size);
	//        OCPI::OS::dumpStack (std::cerr);
      }
#ifdef DEBUG_LISTS
      ocpiDebug("free: Alloc list");
      dumpList(m_pool->alloc_list);
      ocpiDebug("free: Free list");
      dumpList(m_pool->free_list);
#endif
#endif


      m_pool->used -= (*it).size;
      m_pool->free_list.push_back(Block(m_pool, (*it).size, (*it).addr, addr));
      m_pool->alloc_list.erase( it );
      m_pool->defrag();  
      // ocpiDebug("**** Free of = %" OCPI_UTIL_RESADDR_PRIx " used %zu", addr, m_pool->used);
      return 0;
    }
  }

  ocpiAssert(0);
return -1;
}

OCPI::Util::MemBlockMgr::MemBlockMgr(OCPI::Util::ResAddrType start, size_t size )
  throw( std::bad_alloc ) 
{
  
  m_pool = new OCPI::Util::ResPool;
  m_pool->total_size = size;
  m_pool->used = 0;
  m_pool->free_list.push_back( Block( m_pool, size, start, start ) );
}


OCPI::Util::MemBlockMgr::~MemBlockMgr()
  throw()
{
  ocpiDebug("Memory pool is using %zu of %zu on deletion", m_pool->used, m_pool->total_size);
  delete m_pool;
}



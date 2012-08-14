//#define  DEBUG_LISTS 1
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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <list>
#include <cstdio>
#include <OcpiOsDataTypes.h>
#include <OcpiOsAssert.h>
#include <OcpiRes.h>

namespace OCPI {
  namespace Util {
    struct Block {
      ResPool*               rp;
      OCPI::OS::uint64_t       size;
      OCPI::Util::ResAddrType  addr;
      OCPI::Util::ResAddrType  aligned_addr;      
      Block( ResPool* rpp, OCPI::OS::uint64_t s, OCPI::OS::uint64_t adr,  OCPI::OS::uint64_t a_adr)
        :rp(rpp),size(s),addr(adr),aligned_addr(a_adr){}
      int operator<(const Block& rhs)const;
    };
    struct ResPool {
      OCPI::Util::ResAddrType  start_off;
      OCPI::OS::uint64_t  total_size;
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
    ocpiDebug("addr = %" PRIx64 ", aligned_addr = %" PRIx64 ", size = %" PRIu64 "", 
	      (*it).addr, 
	      (*it).aligned_addr, 
	      (*it).size );
  }
#else
  (void)list;
#endif

}
#endif


static OCPI::Util::ResAddrType ALIGN( OCPI::Util::ResAddrType addr, unsigned int alignment ) 
{
  if ( alignment == 0 ) {
    return addr;
  }
  OCPI::Util::ResAddrType mask = alignment -1;
  return (addr & ~mask);
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
  ocpiDebug("Alloc list");
  dumpList(alloc_list);
  ocpiDebug("Free list");
  dumpList(free_list);
#endif

}


int OCPI::Util::MemBlockMgr::alloc(OCPI::OS::uint64_t nbytes, unsigned int alignment, OCPI::Util::ResAddrType& req_addr)
  throw( std::bad_alloc ) 
{

  if ( nbytes > 2000000 ) {
    ocpiInfo("Allocating large mem %" PRIu64 "K", nbytes/1024);
    //    OCPI::OS::dumpStack (std::cerr);
  }
#ifdef DEBUG_LISTS
  ocpiDebug("Alloc list");
  dumpList(m_pool->alloc_list);
  ocpiDebug("Free list");
  dumpList(m_pool->free_list);
#endif

  
  m_pool->free_list.sort();
  std::list<Block>::iterator it;
  int retry=2;
  nbytes += alignment/8;

  do {
    for ( it=m_pool->free_list.begin(); it !=m_pool->free_list.end(); it++ ) {
      if ( ((*it).size > nbytes) ) {
        req_addr = ALIGN((*it).addr, alignment);
        OCPI::Util::ResAddrType taddr = (*it).addr;
        (*it).addr += nbytes;
        (*it).size -= nbytes;      
        m_pool->alloc_list.push_back( Block(m_pool, nbytes, taddr, ALIGN(req_addr, alignment)) );
        ocpiDebug("**** Alloc Returning address = %" PRIx64 ", %" PRIx64 "", 
               taddr, req_addr );
        return 0;
      }
      else if (  ((*it).size == nbytes) ) {
        req_addr = ALIGN((*it).addr, alignment);
        (*it).aligned_addr = req_addr;
        m_pool->alloc_list.push_back( *it );
        ocpiDebug("**** Alloc Returning address = %" PRIx64 ", %" PRIx64 "", 
               (*it).addr, req_addr );
        m_pool->free_list.erase( it );      
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


int OCPI::Util::MemBlockMgr::free( OCPI::Util::ResAddrType addr )
  throw( std::bad_alloc ) 
{

  std::list<Block>::iterator it;
  for ( it=m_pool->alloc_list.begin(); it !=m_pool->alloc_list.end(); it++ ) {
    if ( (*it).aligned_addr == addr )  {

#ifndef NDEBUG
      if ( (*it).size > 2000000 ) {
	ocpiInfo("Freeing large mem %" PRIu64 "K", (*it).size/1024);
      }
#ifdef DEBUG_LISTS
      printf("Alloc list\n");
      dumpList(m_pool->alloc_list);
      printf("Free list\n");
      dumpList(m_pool->free_list);
#endif
#endif


      m_pool->free_list.push_back( Block( m_pool, (*it).size, addr, addr ) );
      m_pool->alloc_list.erase( it );
      m_pool->defrag();  
      return 0;
    }
  }

  ocpiAssert(0);
return -1;
}

OCPI::Util::MemBlockMgr::MemBlockMgr(OCPI::Util::ResAddrType start, OCPI::OS::uint64_t size )
  throw( std::bad_alloc ) 
{
  
  m_pool = new OCPI::Util::ResPool;
  m_pool->free_list.push_back( Block( m_pool, size, start, start ) );
}


OCPI::Util::MemBlockMgr::~MemBlockMgr()
  throw()
{
  delete m_pool;
}



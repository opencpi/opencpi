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

/**
   @file

   @brief
   The file contains the declaration of a simple list class.

   Revision History:

   05/20/2009 - John Miller
   Added a constructor flag that allows the list elements to remain in the same
   position in the list.  

   10/13/2008 - John Miller
   Initial version.

************************************************************************** */


#ifndef LIST_IF_H
#define LIST_IF_H

struct list_
{
  void **entries;  /* Array of pointers to list entries */
  unsigned nentries;   /* Number of active entries in the list */
  unsigned nspaces;    /* Number of allocated slots for entries */
  int no_shuffle;

  inline list_(){entries=0;nentries=nspaces=0;no_shuffle=0;};
};

typedef struct list_ List;


/* External functions */
extern int insert_to_list(List *, void *, unsigned, unsigned);
extern int insert_to_list(List *, void *);
extern int prepend_to_list(List *, void *, unsigned, unsigned);
extern int remove_from_list(List *, void *);
extern unsigned get_nentries(List *);
extern int validate(List *, void *);
extern void destroy_list(List *);
extern void *get_entry(List *, unsigned);
extern void insert_to_position(List*, void*, unsigned pos);


namespace OCPI {
  namespace Util {

    class VList {

    public: 
      VList(int no_shuffle=0);
      ~VList();
      void  noShuffle();
      void  shuffle(int s);
      void  destroyList();
      void  insert(void* element);
      void  insertToPosition( void* element, unsigned pos);
      void  push_back(void* element);
      void  prepend(void* element);
      void  remove(void* element);
      unsigned int getElementCount();
      unsigned int size();
      void* getEntry(unsigned index);
      void* operator[](unsigned);
    private:
      List m_list;
    };

    // Inlines
    inline VList::VList(int shuf){m_list.no_shuffle=shuf;}
    inline void VList::noShuffle(){m_list.no_shuffle=1;}
    inline VList::~VList(){destroy_list(&m_list);}
    inline void VList::destroyList(){destroy_list(&m_list);}
    inline void VList::insert(void* element){insert_to_list(&m_list,element);}
    inline void VList::insertToPosition( void* element, unsigned pos)
    {insert_to_position(&m_list, element, pos);}
    inline void VList::push_back(void* element){insert(element);}
    inline void VList::prepend(void* element){prepend_to_list(&m_list,element,64,8);}
    inline void VList::remove(void* element){remove_from_list(&m_list,element);}
    inline unsigned int VList::getElementCount(){return get_nentries( &m_list );}
    inline unsigned int VList::size(){return get_nentries( &m_list );}
    inline void* VList::getEntry(unsigned index){return get_entry(&m_list, index);}
    inline         void* VList::operator[](unsigned n){return getEntry(n);}

  }

}












#endif /* !defined LIST_IF_H */

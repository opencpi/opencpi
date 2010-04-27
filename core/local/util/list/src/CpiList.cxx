/* @MERCURY.COPYRIGHT.C@ */
/* @MERCURY.PROPRIETARY.C@ */

/**
   @file

   @brief
   The file contains the implementation for the list class.

   Revision History:

   05/20/2009 - John Miller
   Added a constructor flag that allows the list elements to remain in the same
   position in the list.  

   10/13/2008 - John Miller
   Initial version.

   ************************************************************************** */


/* Operating System Includes */
#include <stdio.h>
#include <stdlib.h>

/* Facility Interface Includes */
#include <CpiList.h>
#include <CpiOsAssert.h>



// This wiil either replace the element at "pos" or create a list that is at least "pos"
// int to add it, filling in all the extra pointers to NULL;
void insert_to_position(List* list, void* element, int pos)
{

  cpiAssert( list->no_shuffle==1 );

  int length = get_nentries(list);
  if ( pos < length ) {
    list->entries[pos] = element;
  }
  else {
    for ( int n=0; n<=(pos-length); n++ ) {
      insert_to_list(list,NULL);
    }
    list->entries[pos] = element;
  }
}


int
insert_to_list(List *list, void *item )
{
  return insert_to_list(list,item,64,8);
}

int
insert_to_list(List *list, void *item, int isize, int incr)
{
  /* Check to see if the list has been initialized */
  if (list->nspaces == 0) {
    /* Need to initialize the list */
    if (!(list->entries = (void **)malloc(isize * sizeof(void *)))) {
      /* Allocation failed */
      return 1;
    }
    /* Initialize the list size */
    list->nspaces = isize;
  }
  /* Check to see if we have space in the list */
  else if (list->nentries == list->nspaces) {
    /* Need to expand the list */
    /* Calculate the new size for the list */
    int size = (list->nspaces + incr) * sizeof(void *);
    /* Reallocate the list */
    if (!(list->entries = (void **)realloc(list->entries, size))) {
      /* Allocation failed */
      return 1;
    }
    /* Update the list size */
    list->nspaces += incr;
  }
  /* Insert the item into the list */
  list->entries[list->nentries++] = item;

  /* Completed successfully */
  return 0;
}


int
prepend_to_list(List *list, void *item, int isize, int incr)
{
  /* Check to see if the list has been initialized */
  if (list->nspaces == 0) {
    /* Need to initialize the list */
    if (!(list->entries = (void **)malloc(isize * sizeof(void *)))) {
      /* Allocation failed */
      return 1;
    }
    /* Initialize the list size */
    list->nspaces = isize;
  }
  /* Check to see if we have space in the list */
  else if (list->nentries == list->nspaces) {
    /* Need to expand the list */
    /* Calculate the new size for the list */
    int size = (list->nspaces + incr) * sizeof(void *);
    /* Reallocate the list */
    if (!(list->entries = (void **)realloc(list->entries, size))) {
      /* Allocation failed */
      return 1;
    }
    /* Update the list size */
    list->nspaces += incr;
  }

  // Shift everyone down
  for ( int u=0; u<list->nentries; u++ ) {
    list->entries[(list->nentries-1)-u+1] = list->entries[(list->nentries-1)-u];
  }

  /* Insert the item into the list */
  list->entries[0] = item;
  list->nentries++;

  /* Completed successfully */
  return 0;
}



int
remove_from_list(List *list, void *item)
{
  /* Check to see if the list is empty */
  if (list->nentries == 0)
    return 1;

  /* Check to see if the item is in the list */
  for (int i=0; i < list->nentries; i++) {
    /* Check for the item */
    if (list->entries[i] == item) { /* Found it */

      if ( list->no_shuffle == 0 ) {
	/* See if we need to shuffle the rest of the list */
	if (i != (list->nentries - 1)) {
	  /* Shuffle the rest of the list */
	  for (int j=i; j < (list->nentries - 1); j++) {
	    list->entries[j] = list->entries[j+1];
	  }
	}
	/* Decrement the entry count */
	list->nentries--;
	/* Reprocess the list */
	i--;
	continue;
      }
      else {
	list->entries[i] = NULL;
      }
    }
  }

  return 0;
}

int
validate(List *list, void *entity)
{
  for (int i=0; i < list->nentries; i++) {
    if (list->entries[i] == entity)
      return 0;
  }

  return 1;
}

int
get_nentries(List *list)
{
  return list->nentries;
}

void *
get_entry(List *list, int index)
{
  if ( list && list->nentries ) {
    return list->entries[index];
  }
  else {
    return NULL;
  }
}

void
destroy_list(List *list)
{
  /* Free the entry space */
  free(list->entries);

  /* Reinitialize the list */
  list->entries = 0;
  list->nentries = 0;
  list->nspaces = 0;

  return;
}


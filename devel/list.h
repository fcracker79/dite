#ifndef __LIST__H
#include "common.h"

struct list;

/* This function inserts a node into our list. It requires the head of the
   list, the data to be inserted and a function that can determine the
   order of the nodes.
   It returns -1 on any error.
 */
int l_insert (struct list **l, void *data, unsigned int size,
	      int (*gt) (void *, void *));

/* This function removes the i-th node from the list.
   It returns -1 on any error.
 */
int l_remove (struct list **l, unsigned int i);

/* This function finds a node into our list.
   It returns the node number.
   If -1, there was an error.
   if -2, we haven't found the element.
   if 0, the element hasn't been found.
   It has to be tested. TODO
 */
int l_find (struct list *l, void *data, unsigned int size);

/* This function prints the list.
   It returns the number of elements.
 */
int print_list (FILE * stream, struct list *l,
		void (*pr) (FILE * stream, void *data));

/* This function clears all the list.
 */

int clear_list (struct list *l);
#endif

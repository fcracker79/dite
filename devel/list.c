#include "list.h"

struct list {
   struct list *next;
   void *data;
};

int
l_insert (struct list **l, void *data, unsigned int size,
	  int (*gt) (void *, void *))
{
   struct list *tmp, *mine;
   void *tmpdata;

   if (l == NULL)
      return -1;
   if (data == NULL)
      return -1;
   if (gt == NULL)
      return -1;
   if (size == 0)
      return -1;

   PRINTDEBUG ("l_insert: the parameters are ok.\n");

   tmp = (struct list *) malloc (sizeof (struct list));
   tmpdata = (void *) malloc (size);
   bcopy (data, tmpdata, size);
   tmp->data = tmpdata;

/* Case of empty list.
 */
   if (*l == NULL) {
      *l = tmp;
      tmp->next = NULL;
      return 1;
   }

/* Case of head insertion.
 */
   if (gt (tmp->data, (*l)->data)) {
      tmp->next = *l;
      *l = tmp;
      return 1;
   }

/* Take the list.
 */
   mine = *l;

/* Now we find the node that is not greater than the new one.
 */
   while ((mine->next != NULL) && (gt (mine->next->data, tmp->data)))
      mine = mine->next;

/* OK we have found it, or we are at the end of the list. Let's insert the
   node!
 */
   tmp->next = mine->next;
   mine->next = tmp;

   return 1;
}

int
l_remove (struct list **l, unsigned int i)
{
   struct list *tmp, *tmp2;
   unsigned int k;

   if (l == NULL)
      return -1;
/* The list is empty?
 */
   if (*l == NULL)
      return 0;

   if (i == 0) {
      PRINTDEBUG ("Delete head.\n");
      tmp = *l;
      *l = (*l)->next;
      free (tmp->data);
      free ((void *) tmp);
      return 1;
   }

/* Here we know that i >= 1.
 */
   tmp = *l;
   for (k = 0; k < i - 1; k++) {
      tmp = tmp->next;
      /* We may have found the end of the list.
       */
      if (tmp == NULL)
	 return 0;
   }
   PRINTDEBUG ("Found correct node.\n");
/* Here we have found the correct node. tmp2 will be deleted.
 */
   tmp2 = tmp->next;
   if (tmp2 == NULL)
      tmp->next = NULL;
   else
      tmp->next = tmp2->next;

   if (tmp2 != NULL) {
      free (tmp2->data);
      free ((void *) tmp2);
   }

   return 1;
}

int
l_find (struct list *l, void *data, unsigned int size)
{
   struct list *tmp;
   unsigned int i;

   if (l == NULL)
      return -1;
   if (data == NULL)
      return -1;
   if (size == 0)
      return -1;

   tmp = l;
   i = 0;
   while ((tmp != NULL)) {
      if (memcmp (tmp->data, data, size) == 0)
	 break;
      i++;
      tmp = tmp->next;
   }

   if (tmp == NULL)
      return -2;
   return i;
}

int
print_list (FILE * stream, struct list *l,
	    void (*pr) (FILE * stream, void *data))
{
   struct list *tmp;
   unsigned int i;

   if (l == NULL)
      return 0;

   tmp = l;
   i = 0;
   while (tmp != NULL) {
      pr (stream, tmp->data);
      fprintf (stream, "\n");
      i++;
      tmp = tmp->next;
   }
   return i;
}

int
clear_list (struct list *l)
{
   struct list *tmp;

   tmp = l;
   while (l != NULL) {
      l = l->next;
      PRINTDEBUG ("Deleting...\n");
      free (tmp->data);
      free ((void *) tmp);
      PRINTDEBUG ("OK.\n");
      tmp = l;
   }
   return 1;
}

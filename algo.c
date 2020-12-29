#include "algo.h"

int
sort (void *array, unsigned int num, unsigned int size,
      int (*gt) (void *, void *))
{
   unsigned int i, j;
   void *buff;

   if (array == NULL)
      return -1;
   if (gt == NULL)
      return -1;

   buff = malloc (size);
   for (i = 0; i < num; i++)
      for (j = i; j < num; j++)
	 if (gt (array + j * size, array + i * size) > 0) {
	    memcpy (buff, array + i * size, size);
	    memcpy (array + i * size, array + j * size, size);
	    memcpy (array + j * size, buff, size);
	 }
   free (buff);
   return 1;
}

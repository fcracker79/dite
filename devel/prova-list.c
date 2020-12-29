#include "list.h"

struct numero {
   unsigned int v;
};

int
gt (struct numero *a, struct numero *b)
{
   return (a->v > b->v);
}

void
pr (FILE * stream, struct numero *x)
{
   fprintf (stream, "%u", x->v);
}

int
main ()
{
   struct list *l;
   struct numero t;
   int i;

   l = NULL;
   t.v = 0;
   printf ("Lista vuota:\n");
   print_list (stdout, l, pr);
   printf ("OK\n");

   do {
      printf ("Dammi un numero. 0 per terminare:\n");
      scanf ("%d", &t.v);
      printf ("pd\n");
      l_insert (&l, (void *) &t, sizeof (unsigned int), gt);
      printf ("\nLa lista ora e'\n");
      print_list (stdout, l, pr);
      printf ("OK\n");
   } while (t.v > 0);

   do {
      printf
	 ("Dammi l'indice dell'elemento da eliminare. >= 10000 per terminare:\n ");
      scanf ("%d", &i);
      l_remove (&l, i);
      printf ("\nLa lista ora e'\n");
      print_list (stdout, l, pr);
      printf ("OK\n");
   } while (i < 10000);
   printf ("Ora cancello l'intera lista.\n");
   clear_list (l);
   return 0;
}

#include "parse.h"

int
parse_file (const char *filename, struct parsing_entry **list_top)
{
   FILE *fileconf;
   char linebuf[MAX_PARSING_LINE_LENGTH];
   int desc_num;
   struct parsing_entry *list_current;

   fileconf = fopen (filename, "r");
   if (!fileconf || !list_top)
      return -1;

   /*
      file parsing
      put all non blank and non commented lines into entry list
    */
   *list_top = NULL;
   list_current = NULL;
   desc_num = 0;
   while (!feof (fileconf)) {
      int len, i, j, old_i, k;
      char argbuf[MAX_PARSING_COMMAND_ARGUMENT + 1];

      memset (linebuf, 0, sizeof (linebuf));
      /* get next line */
      if (!fgets (linebuf, sizeof (linebuf), fileconf))
	 break;

      /* check for line overflow, skip if occurs */
      if (linebuf[sizeof (linebuf) - 2] != 0 &&
	  linebuf[sizeof (linebuf) - 2] != '\n') {
	 /* reach end of line, then restart */
	 int c;
	 char b;

	 for (;;) {
	    c = fgetc (fileconf);
	    if (c == '\n') {
	       b = 0;
	       break;
	    }
	    if (c == EOF) {
	       b = 1;
	       break;
	    }
	 }
	 if (b)
	    break;
	 else
	    continue;
      }

      len = strlen (linebuf) - 1;	/* newline character */

      i = 0;
      /* eat white spaces */
      while ((linebuf[i] == ' ' || linebuf[i] == '\t') && i < len)
	 ++i;

      if (i >= len)
	 continue;

      /* checking for commented line */
      if (linebuf[i] == '#')
	 continue;

      /* command expected */

      /* allocating new element memory */
      if (!list_current) {
	 /* create first list element */
	 *list_top = Malloc (sizeof (struct parsing_entry));
	 list_current = *list_top;
      }
      else {
	 /* create next element at the back of the list */
	 list_current->next = Malloc (sizeof (struct parsing_entry));
	 list_current = list_current->next;
      }

      j = 0;	/* command index */
      while (linebuf[i] != ' ' && linebuf[i] != '\t' &&
	     j < MAX_PARSING_COMMAND_LENGTH) {
	 list_current->command[j] = linebuf[i];
	 ++j;
	 ++i;
      }
      /* null terminate command string */
      list_current->command[j] = '\0';

      /* check for command overflow */
      if (j == MAX_PARSING_COMMAND_LENGTH && linebuf[i] != ' '
	  && linebuf[i] != '\t') {
	 /* skip to first argument, if any */
	 while (linebuf[i] != ' ' && linebuf[i] != '\t' && i < len)
	    ++i;
      }

      /* looking for arguments */

      /* store i value */
      old_i = i;

      /* count arguments number */
      for (; i < len;) {
	 /* eat white spaces */
	 while ((linebuf[i] == ' ' || linebuf[i] == '\t') && i < len)
	    ++i;

	 /* end of line */
	 if (i >= len)
	    break;

	 /* argument found */
	 ++list_current->argc;

	 while (linebuf[i] != ' ' && linebuf[i] != '\t' && i < len)
	    ++i;
	 if (i >= len)
	    break;
      }

      /* allocate argument pointers array */
      list_current->argv = Malloc (list_current->argc * sizeof (char *));

      /* restore previously stored i value */
      i = old_i;

      /* insert arguments into argv array */
      /* k is argument number */
      for (k = 0; i < len; k++) {
	 int p;

	 memset (argbuf, 0, sizeof (argbuf));

	 /* eat white spaces */
	 while ((linebuf[i] == ' ' || linebuf[i] == '\t') && i < len)
	    ++i;

	 /* end of line */
	 if (i >= len)
	    break;

	 /* argument found */
	 p = 0;
	 while (linebuf[i] != ' ' && linebuf[i] != '\t' &&
		i < len && p < MAX_PARSING_COMMAND_ARGUMENT) {
	    argbuf[p] = linebuf[i];
	    ++i;
	    ++p;
	 }
	 if (p == MAX_PARSING_COMMAND_ARGUMENT) {
	    while (linebuf[i] != ' ' && linebuf[i] != '\t' && i < len)
	       ++i;
	 }

	 /* allocate single argument memory space */
	 list_current->argv[k] = Malloc (strlen (argbuf) + 1);

	 /* copy argument buffer into arguments array */
	 strcpy (list_current->argv[k], argbuf);

	 if (i >= len)
	    break;
      }
   }

   fclose (fileconf);

   return 1;
}

int
parse_destroy_list (struct parsing_entry *list)
{
   struct parsing_entry *current, *next;

   if (!list)
      return 0;

   current = list;
   while (current) {
      int i;

      next = current->next;
      for (i = 0; i < current->argc; i++)
	 if (current->argv)
	    free (current->argv[i]);
      free (current->argv);

      free (current);
      current = next;
   }

   return 1;
}

int
parse_print_list (struct parsing_entry *list)
{
   struct parsing_entry *current;

   if (!list)
      return 0;
   current = list;
   while (current) {
      int i;

      fprintf (stderr, "%s (%d): ", current->command, current->argc);
      for (i = 0; i < current->argc; i++)
	 if (current->argv)
	    fprintf (stderr, "%s ", current->argv[i]);
      fprintf (stderr, "\n");
      fflush (stderr);

      current = current->next;
   }

   return 1;
}

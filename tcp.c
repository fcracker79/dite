#include "tcp.h"

#define SIZE_ADDR 32
extern unsigned short int server_port;

FILE *
startNetTCP (void)
{
   FILE *tcp;
   unsigned int c;

   tcp = fopen ("/proc/net/tcp", "r");
   if (tcp == NULL)
      return NULL;

/* Here we skip a line. Ugly, but I'm lamer.
 */
   do
      c = fgetc (tcp);
   while (c != '\n' && c != EOF);

   return tcp;
}

int
getNetTCP (FILE * tcp, unsigned int *addr)
{
   char saddr[SIZE_ADDR];
   char daddr[SIZE_ADDR];
   unsigned int port;

   char *temp;

   if (tcp == NULL)
      return -1;
   if (addr == NULL)
      return -1;

   bzero ((void *) saddr, SIZE_ADDR);
   bzero ((void *) daddr, SIZE_ADDR);

/* Here we read a line of the /proc/net/tcp file and we take only the address.
 */
   if (fscanf (tcp,
	       "%*s %s %s %*x %*s %*s %*x %*x %*x %*x %*x %*x %*x %*x %*x %*x %*x %*d",
	       saddr, daddr) != EOF) {

      /* The destination address and port MUST be zero, otherwise the line we have
         found is referred to an effective connection. Note that every address is
         in the foll. form:
         address:port
       */
      temp = strstr (daddr, ":");
      *temp = '\0';
      if (strtol (daddr, NULL, 16) != 0)
	 return 0;
      temp++;
      /* Now temp points to a string with the dest. port number.
       */
      if (strtol (temp, NULL, 16) != 0)
	 return 0;

      PRINTDEBUG (saddr);
      PRINTDEBUG ("\n");
      temp = strstr (saddr, ":");
      *temp = '\0';
      *addr = strtol (saddr, NULL, 16);

		/* For now, we do not log bind ports. */
		if (*addr != 0) return 0;
		
      /* The number is represented on base 16.
       */
      port = strtol (temp + 1, NULL, 16);
      return port;
   }
   return -1;
}

int
endNetTCP (FILE * tcp)
{
   if (tcp == NULL)
      return -1;
   fclose (tcp);
   return 1;
}

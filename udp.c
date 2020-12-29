#include "udp.h"

#define SIZE_ADDR 32

FILE *
startNetUDP (void)
{
   FILE *udp;
   unsigned int c;

   udp = fopen ("/proc/net/udp", "r");
   if (udp == NULL)
      return NULL;

/* Here we skip a line. Ugly, but I'm lamer.
 */
   do
      c = fgetc (udp);
   while (c != '\n' && c != EOF);

   return udp;
}

int
getNetUDP (FILE * udp, unsigned int *addr)
{
   char saddr[SIZE_ADDR];
   char daddr[SIZE_ADDR];
   unsigned int port;

   char *temp;

   if (udp == NULL)
      return -1;

   bzero ((void *) saddr, SIZE_ADDR);
   bzero ((void *) daddr, SIZE_ADDR);

/* Here we read a line of the /proc/net/udp file and we take only the address.
 */
   if (fscanf (udp,
	       "%*s %s %s %*x %*s %*s %*x %*x %*x %*x %*x %*x",
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

		if (*addr != 0) return 0;

      port = strtol (temp + 1, NULL, 16);

      return port;
   }
   return -1;
}

int
endNetUDP (FILE * udp)
{
   if (udp == NULL)
      return -1;
   fclose (udp);
   return 1;
}

#include "net.h"
int
main ()
{
   open_port *tcp, *udp;
   unsigned int lt, lu;

   getOpenTCP (&tcp, &lt);
   getOpenUDP (&udp, &lu);
   fprintf (stderr, "TCP ports:\n");
   print_open_port (tcp, lt);
   fprintf (stderr, "\nUDP ports:\n");
   print_open_port (udp, lu);
   free ((void *) tcp);
   free ((void *) udp);
   return 1;
}

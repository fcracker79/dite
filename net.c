#include "net.h"
#include "tcp.h"
#include "udp.h"

static int min_port_TCP = 1;
static int max_port_TCP = 65535;

static int min_port_UDP = 1;
static int max_port_UDP = 65535;

int set_range_TCP (int min, int max) {
   if (min < 1) return -1;
   if (max > 65535) return -1;
   if (min > max ) return -1;
   max_port_TCP = max;
   min_port_TCP = min;
   return 1;
}

int set_range_UDP (int min, int max) {
   if (min < 1) return -1;
   if (max > 65535) return -1;
   if (min > max ) return -1;
   max_port_UDP = max;
   min_port_UDP = min;
   return 1;
}

int set_range (int min, int max) {
   if (min < 1) return -1;
   if (max > 65535) return -1;
   if (min > max ) return -1;
   
   set_range_TCP (min, max);
   set_range_UDP (min, max);

   return 1;
}

	 
int
gt_port (void *a, void *b)
{
   open_port p1, p2;

   memcpy (&p1, a, sizeof (open_port));
   memcpy (&p2, b, sizeof (open_port));
   if (p1.port > p2.port)
      return 1;
   if (p1.port == p2.port) {
      if (p1.bind_addr > p2.bind_addr)
	 return 1;
      else if (p1.bind_addr == p2.bind_addr)
	 return 0;
      else
	 return -1;
   }
   return -1;
}

int
getOpenTCP (open_port ** op, unsigned int *l)
{
   int i, j;
   FILE *tcp;
   unsigned int addr;

   if (l == NULL)
      return -1;
   if (op == NULL)
      return -1;
   *l = 0;

   tcp = startNetTCP ();
   if (tcp == NULL)
      return -1;
   while ((i = getNetTCP (tcp, &addr)) != -1)
      if (i != 0 && i >= min_port_TCP && i <= max_port_TCP)
	 (*l)++;
   endNetTCP (tcp);

   *op = (open_port *) Malloc (sizeof (open_port) * (*l));
   tcp = startNetTCP ();
   if (tcp == NULL)
      return -1;
   j = 0;
   while ((i = getNetTCP (tcp, &addr)) != -1)
      if (i != 0 && i >= min_port_TCP && i <= max_port_TCP) {
	 (*op)[j].port = i;
	 (*op)[j].bind_addr = addr;
	 j++;
      }
   endNetTCP (tcp);
   sort (*op, *l, sizeof (open_port), gt_port);
   *l *= sizeof (open_port);
   return 1;
}

int
getOpenUDP (open_port ** op, unsigned int *l)
{
   int i, j;
   FILE *udp;
   unsigned int addr;

   if (l == NULL)
      return -1;
   if (op == NULL)
      return -1;
   *l = 0;

   udp = startNetUDP ();
   if (udp == NULL)
      return -1;
   while ((i = getNetUDP (udp, &addr)) != -1)
      if (i != 0 && i >= min_port_UDP && i <= max_port_UDP)
	 (*l)++;
   endNetUDP (udp);

   *op = (open_port *) Malloc (sizeof (open_port) * (*l));
   udp = startNetUDP ();
   if (udp == NULL)
      return -1;
   j = 0;
   while ((i = getNetUDP (udp, &addr)) != -1)
      if (i != 0 && i >= min_port_UDP && i <= max_port_UDP) {
	 (*op)[j].port = i;
	 (*op)[j].bind_addr = addr;
	 j++;
      }
   endNetUDP (udp);
   sort (*op, *l, sizeof (open_port), gt_port);
   *l *= sizeof (open_port);
   return 1;
}

int
print_open_port (open_port * op, unsigned int l)
{
   unsigned int i;
   struct in_addr s;

   if (op == NULL)
      return -1;
   for (i = 0; i < l; i++) {
      s.s_addr = op[i].bind_addr;
      fprintf (stderr, "Port %d. Bind addr: %s\n", op[i].port, inet_ntoa (s));
   }
   return 1;
}

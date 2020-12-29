#include "server.h"
#include "client.h"

int
main (int argc, char *argv[])
{
   parse_server ();
   print_server_conf (stdout);
   reset_server ();
   print_server_conf (stdout);

   parse_client ();
   print_client_conf (stdout);
   reset_client ();
   print_client_conf (stdout);
   exit (0);
}

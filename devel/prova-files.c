#include "files.h"

int
main ()
{
   fstree_init ();
   getPathCRC ("/home/claudio/code/security/intruder/prova");
   fstree_print_file ();
   fstree_print_dir ();
   fstree_destroy ();
   return 0;
}

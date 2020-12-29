#include "kernel.h"
#include <errno.h>

int
main ()
{
   void *buff;
   void *buffk;
   unsigned int l, lk;

   getKernelStatus (&buffk, &lk);
   print_kernel_status (buffk);
   getLoadedModules (&buff, &l);
   print_kernel_modules (buff, l);
   free (buff);
   free (buffk);
   return 1;
}

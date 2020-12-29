#include "compression.h"
#define IN_SIZE 1024
#define OUT_SIZE 2048

int
main ()
{
   FILE *s, *d;
   int num, ret;
   void *in, *out;
   c_stream *c;

   in = malloc (IN_SIZE);
   out = malloc (OUT_SIZE);

   s = fopen ("messages", "r");
   d = fopen ("messages.compression", "w");
/* Compression initialization.
 */
   c = c_init (in, IN_SIZE, out, OUT_SIZE, COMPRESSION);
   print_status (c, stderr);
/* For each block of read data...
 */
   while ((num = fread (in, 1, IN_SIZE, s)) != 0) {
      /* ...we run the compression routine as many times as we need to manage
         all input data.
       */
      new_data (c, NULL, num);
      print_status (c, stderr);

      while ((ret = de_compress (c)) == OUT_SIZE)
	 fwrite (out, 1, OUT_SIZE, d);
      /* If there was an error, exit.
       */
      if (ret < 0) {
	 print_status (c, stderr);
	 fprintf (stderr, "Error %d occured.\n", ret);
	 exit (ret);
      }
   }

/* This is the close function. First we flush all pending data.
*/
   while ((ret = c_free (c)) != END_FLUSHING)
      fwrite (out, 1, ret, d);
   fclose (s);
   fclose (d);
   free (c);

   s = fopen ("messages.compression", "r");
   d = fopen ("messages.check", "w");
/* Compression initialization.
 */
   c = c_init (in, IN_SIZE, out, OUT_SIZE, DECOMPRESSION);
   print_status (c, stderr);
/* For each block of read data...
 */
   while ((num = fread (in, 1, IN_SIZE, s)) != 0) {
      /* ...we run the decompression routine as many times as we need to manage
         all input data.
       */
      new_data (c, NULL, num);
      print_status (c, stderr);
      while ((ret = de_compress (c)) == OUT_SIZE)
	 fwrite (out, 1, ret, d);
      /* If there was an error, exit.
       */
      if (ret < 0) {
	 print_status (c, stderr);
	 fprintf (stderr, "Error %d occured.\n", ret);
	 exit (ret);
      }
   }

   PRINTDEBUG ("D - End of cycle.\n");
/* This is the close function. First we flush all pending data.
*/
   while ((ret = c_free (c)) != END_FLUSHING)
      fwrite (out, 1, ret, d);
   fclose (s);
   fclose (d);
   free (c);

   return 1;
}

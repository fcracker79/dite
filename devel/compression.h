#ifndef __COMPRESSION__H

#include "common.h"
#include <zlib.h>

#define COMPRESSION 0
#define DECOMPRESSION 1
#define END_FLUSHING -20

typedef struct {
   z_stream z;
   void *in;
   unsigned int isize;
   void *out;
   unsigned int osize;
   unsigned short int type;
   unsigned short int flush;
} c_stream;

/* This function initialize the process of compression. The data to be
   de/compressed are pointed by in, isize bytes long. The destination will be
   pointed by out, osize bytes long.
   The "type" variable specifies if we want to do compression or decompression.
   It can assume only values COMPRESSION or DECOMPRESSION.
   The function returns a c_stream* or NULL on an error. The pointer must be
   freed by the user.
 */
c_stream *c_init (void *in, const unsigned int isize,
		  void *out, const unsigned int osize,
		  const unsigned short int type);

/* This function de/compresses as data as it can, according to the parameters
   sent to the c_init() function.
   The function returns the used output buffer size. Note that value is
   cumulative, so if you call two or more times this function with more input
   data for each call, the return value will grow up until the whole size of
   the output buffer.

   Example:
   ...
   new_data (c, NULL, size1);
   n = de_compress (c);  <- here we obtain a return value, say n.
   new_data (c, NULL, size2);
   m = de_compress (c);  <- here we obtain another return value, say m.

   In this case, the input buffer size1 long and the buffer size2 long are 
   supposed to be compressed using only one output buffer. The data is stored
   in this way:

   Output buffer
   -------------------------------------------------------
   |  Compressed buffer 1 |  Compressed buffer 2 |       |
   -------------------------------------------------------
                          ^                      ^       ^ 
                          |                      |       |
	                  n                      m    output buffer size

   Obviously, n < m.
   
   For this reason, it is a good idea giving the stream as input data as you
   can and consuming the output data only when the output buffer is full.
   When the output buffer is full, the function reinitializes it to the
   beginning, so, before another call, you must consume the output data, or
   the current output buffer data will be overwritten by the new ones.
   Note that if the return value is < output buffer size, this means that
   all input data is consumed, so you have to call the new_data() function.
   Also note that an input consumed is NOT an output compressed, because some
   output data are internally buffered to permit a better compression.
   See also c_free() function.

   The function returns < 0 on error.
 */
int de_compress (c_stream * c);


/* This function permits to give other input data instead of compressing it 
   with another session. This performs better compression instead of managing 
   apart the new input data.
   If in == NULL the input buffer pointer isn't changed.
   If isize == 0 the buffer size isn't changed.
   The function returns -1 on error, > 0 otherwise.
*/
int new_data (c_stream * c, void *in, const unsigned int isize);

/* This function deallocates all dynamic structures of the compression
   process, but input and output buffer.
   Before finishing, it flushes all data. For this reason, you MUST call
   this function at the end of the de/compression operation, or some output
   data won't be written.
   The function returns the number of byte to be read from the output buffer.
   Unlike the de_compress() function, here you must read the output buffer data
   for each c_free() call until it returns END_FLUSHING.

   Note: this difference from the de_compress() function is justified by the
   consideration that every de_compress() function call produces internal
   buffer data, but often no output data. Every c_free() call, instead,
   always produces output data.
 */
int c_free (c_stream * c);

#ifdef DEBUG
/* This function prints some information on the compression status. This is
   only for debugging purpose.
 */
int print_status (c_stream * c, FILE * f);
#endif

#define __COMPRESSION__H
#endif

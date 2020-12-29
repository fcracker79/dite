#include "compression.h"
#define COMPR_LEVEL 9
#define OPAQUE_SIZE 65536
#define NOT_FLUSHED 0
#define FLUSHED 1
c_stream *
c_init (void *in, const unsigned int isize,
	void *out, const unsigned int osize, const unsigned short int type)
{
   c_stream *c;
   unsigned short int what;
   int error;

   if (in == NULL)
      return NULL;
   if (out == NULL)
      return NULL;
   if (isize == 0)
      return NULL;
   if (osize == 0)
      return NULL;
   if (type > 0)
      what = DECOMPRESSION;
   else
      what = COMPRESSION;

/* Initializing structure.
 */
   c = (c_stream *) malloc (sizeof (c_stream));

/* Here we save the input buffer if we need to read from the same buffer
   more times. Also, we save the output buffer, so, when it is full, we can
   reinitialize it.
 */
   c->in = in;
   c->out = out;
   c->isize = isize;
   c->osize = osize;

/* Here we specify the initial input and output location and size.
 */
   (c->z).next_in = in;
   (c->z).next_out = out;
   (c->z).avail_in = isize;
   (c->z).avail_out = osize;

/* The allocation and deallocation functions are the default ones.
 */
   (c->z).zalloc = NULL;
   (c->z).zfree = NULL;

/* Internal buffer.
 */
   (c->z).opaque = malloc (OPAQUE_SIZE);

/* Compression or decompression operation.
 */
   c->type = what;

/* This variable is used for the flushing operation.
 */
   c->flush = NOT_FLUSHED;

/* Here we set the operation type. According to it, we will call the correct
   initialization function.
 */
   if (what == COMPRESSION)
      error = deflateInit (&(c->z), COMPR_LEVEL);
   else
      error = inflateInit (&(c->z));
   PRINTDEBUG ("c_init: error %d\n", error);
   return c;
}

int
de_compress (c_stream * c)
{

/* This one will be a compression or decompression function, according to the
   "what" value of the c_init() function.
 */
   int (*go) (z_streamp, int);
   int ret;

   if (c == NULL)
      return -1;

/* Here the function go() will be a compression or decompression function.
 */
   if (c->type == COMPRESSION)
      go = deflate;
   else
      go = inflate;

/* Here the compression starts. Note the Z_NO_FLUSH value: the output buffer
   is not flushed to permit a better compression.
 */
   ret = go (&(c->z), Z_NO_FLUSH);

/* If there was an error, returns it.
 */
   if (ret < 0) {
      PRINTDEBUG ("De-compress: error %d\n", ret);
      return ret;
   }

   PRINTDEBUG ("de_compress OK.\n");

/* If the buffer is full, we reinitialize the output pointers and return the
   size of the output buffer.
   When the user recalls this function, the output buffer will be overwritten
   from its beginning.
 */
   if ((c->z).avail_out == 0) {
      PRINTDEBUG ("Empty the output buffer.\n");
      (c->z).next_out = c->out;
      (c->z).avail_out = c->osize;
      return c->osize;
   }

/* Returns the num. of bytes available in the output buffer.
 */
   return c->osize - (c->z).avail_out;
}

int
new_data (c_stream * c, void *in, const unsigned int isize)
{
   if (c == NULL)
      return -1;

/* First, we update the input buffer with the original values.
 */
   (c->z).next_in = c->in;
   (c->z).avail_in = c->isize;

/* But if we specify a new pointer, we completely change it. The old input
   buffer is lost.
 */
   if (in != NULL) {
      c->in = in;
      (c->z).next_in = in;
   }

/* If we specify a new size, we completely change it. The old size is lost.
 */
   if (isize != 0) {
      c->isize = isize;
      (c->z).avail_in = isize;
   }

   return 1;
}

int
c_free (c_stream * c)
{

/* This pointer will point to the deallocation function, specifically for the
   compression or decompression operation.
 */
   int (*end) (z_streamp);

/* This pointer will point to the flushing function, specifically for the
   compression or decompression operation.
 */
   int (*go) (z_streamp, int);
   int ret;

   if (c == NULL)
      return -1;

/* Here, if we are doing a compression operation.
 */
   if (c->type == COMPRESSION) {
      go = deflate;
      end = deflateEnd;
   }

/* Here, if we are doing a decompression operation. The actual zlib
   implementation, during decompression operation, flushes as many data as it
   can. So a flushing operation should never be necessary. However, for
   possible changes, we call the flushing function. In this case we will have
   a stream error, which will means that the flush is complete.
 */
   else {
      //return END_FLUSHING;
      go = inflate;
      end = inflateEnd;
   }

/* We call the flushing function.
 */
   ret = go (&(c->z), Z_FINISH);
   PRINTDEBUG ("c_free: ret=%d\n", ret);

/* If there was an error, we suppose that the flushing is complete.
 */
   if (ret < 0) {
      PRINTDEBUG ("c-free: error %d. This should not be a problem.\n", ret);
#ifdef DEBUG
      print_status (c, stderr);
#endif
      return END_FLUSHING;
   }

/* If we have already got ret == Z_STREAM_END, a next call will returns
   END_FLUSHING (see below).
 */
   if (c->flush == FLUSHED)
      return END_FLUSHING;

/* Ok. The output buffer is enough to contain all buffered data. So we mark
   that we have terminated the flushing operation. The next call we will
   return END_FLUSHING (see above). For now, we return the last output buffer
   data size to be read.
 */
   if (ret == Z_STREAM_END) {
      c->flush = FLUSHED;
      return c->osize - (c->z).avail_out;
   }

/* On any other case, we have got ret == Z_OK. In this case, the function has
   flushed some data, but the output buffer got full. For this reason, we
   return the output buffer size.
   The user will have to call this function until it returns END_FLUSHING.
*/
   ret = c->osize - (c->z).avail_out;
   (c->z).next_out = c->out;
   (c->z).avail_out = c->osize;

   return ret;
}

#ifdef DEBUG
/* Only for debugging purpose.
 */
int
print_status (c_stream * c, FILE * f)
{
   if (c == NULL)
      return -1;
   if (f == NULL)
      return -1;

   if (c->type == COMPRESSION)
      fprintf (f, "Operation: compression\n");
   else
      fprintf (f, "Operation: decompression\n");
   fprintf (f, "Original Input buffer: %p\n Size: %d\n", c->in, c->isize);
   fprintf (f, "Input buffer: %p\n Size: %d\n", (c->z).next_in,
	    (c->z).avail_in);
   fprintf (f, "Original Output buffer: %p\n Size: %d\n", c->out, c->osize);
   fprintf (f, "Output buffer: %p\n Size: %d\n", (c->z).next_out,
	    (c->z).avail_out);
   return 1;
}
#endif

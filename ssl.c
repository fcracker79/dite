#include "ssl.h"

int
print_digest (FILE * stream, const unsigned char *digest,
	      const unsigned int size)
{
   unsigned int i;

   if (stream == NULL)
      return -1;
   if (digest == NULL)
      return -1;

   for (i = 0; i < size; i++)
      fprintf (stream, "%02x", digest[i]);
   return 1;
}

int
get_digest (const char *msg, const unsigned int size, char *md5_digest,
	    char *sha1_digest)
{

   BIO *bio, *mdtmp;
   EVP_MD *md;
   unsigned char *dest;

   if (msg == NULL)
      return -1;
   if (md5_digest == NULL)
      return -1;
   if (sha1_digest == NULL)
      return -1;

/* The encryption functions can crypt only short messages. So we crypt the
   digest of our message. This is the pgp idea. Boh...
   In this case we use MD5/SHA1 digest.
   We build a chain which will be our filter for the data to be digested.

   BIO(SHA1) -- BIO(MD5) -- NULL
   The last element of the chain throws away every data. It is necessary for
   chain termination.
 */
   bio = BIO_new (BIO_s_null ());
   mdtmp = BIO_new (BIO_f_md ());

/* SHA1 digest.
 */
   BIO_set_md (mdtmp, EVP_sha1 ());
   bio = BIO_push (mdtmp, bio);

   mdtmp = BIO_new (BIO_f_md ());

/* MD5 digest.
 */
   BIO_set_md (mdtmp, EVP_md5 ());
   bio = BIO_push (mdtmp, bio);

/* And finally, we write our data into it.
 */
   BIO_write (bio, msg, size);

/* Now we have both the MD5 and SHA1 digest. We have only to put it in plain
   text.
 */
   mdtmp = bio;
   do {
      /* Search the first bio structure of the type BIO_TYPE_MD into the passed BIO      chain.
       */
      mdtmp = BIO_find_type (mdtmp, BIO_TYPE_MD);

      /* If we only find sink BIO, we can exit.
       */
      if (!mdtmp)
	 break;

      /* the documentation lacks... Maybe here we take the message digest.
       */
      BIO_get_md (mdtmp, &md);

      /* We know that we have used only MD5 and SHA1. We recover the two digests.
       */
      switch (EVP_MD_type (md)) {
      case NID_sha1:{
	    dest = sha1_digest;
	    break;
	 }
      case NID_md5:{
	    dest = md5_digest;
	    break;
	 }
      }

      /* We write into md5_digest or sha1_digest.
       */
      BIO_gets (mdtmp, dest, EVP_MAX_MD_SIZE);

      mdtmp = BIO_next (mdtmp);

   } while (mdtmp);

   BIO_free_all (bio);
   return 1;
}

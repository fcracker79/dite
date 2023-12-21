#include "rsa.h"

RSA *
generate_key (void)
{
   RSA *prv;

   prv = RSA_generate_key (NUM_BITS, PUB_EXP, NULL, NULL);
   if (RSA_check_key (prv) == 1)
      return prv;
   return NULL;
}

int
encrypt (unsigned int size, unsigned char *src, unsigned char **dst,
	 RSA * prv)
{
   if (src == NULL)
      return -1;
   if (dst == NULL)
      return -1;
   if (prv == NULL)
      return -1;
   if (RSA_size (prv) - 11 <= size)
      return -2;

   *dst = (char *) Malloc (RSA_size (prv));
/* Returns RSA_size (prv).
 */
   return RSA_private_encrypt (size, src, *dst, prv, RSA_PKCS1_PADDING);
}

int
decrypt (unsigned int size, unsigned char *src, unsigned char **dst,
	 RSA * pub)
{
   if (src == NULL)
      return -1;
   if (dst == NULL)
      return -1;
   if (pub == NULL)
      return -1;

/* This is the largest size for a RSA decrypted message.
 */
   *dst = (char *) Malloc (RSA_size (pub) - 11);
   
   return RSA_public_decrypt (size, src, *dst, pub, RSA_PKCS1_PADDING);
}

int
get_modulus (RSA * rsa, char **bin)
{
   unsigned int size;
   

   if (rsa == NULL)
      return -1;
   if (bin == NULL)
      return -1;

   PRINTDEBUG ("Getting modulus.\n");
   const BIGNUM *n = RSA_get0_n(rsa);

   PRINTDEBUG ("Now get the size of the RSA number.\n");
   size = RSA_size (rsa);

   PRINTDEBUG ("Size = %d\n", size);

   PRINTDEBUG ("Let's allocate the memory.\n");
   *bin = (char *) malloc (size);

   PRINTDEBUG ("Now we call BN_bn2bin().\n");
   BN_bn2bin (n, *bin);
   return size;
}

int
get_pub_exp (RSA * rsa, char **bin)
{
   unsigned int size;
   
   if (rsa == NULL)
      return -1;
   if (bin == NULL)
      return -1;

   PRINTDEBUG ("Getting modulus.\n");
   const BIGNUM *e = RSA_get0_e(rsa);

   PRINTDEBUG ("Now get the size of the RSA number.\n");
   size = RSA_size (rsa);

   PRINTDEBUG ("Size = %d\n", size);

   PRINTDEBUG ("Let's allocate the memory.\n");
   *bin = (char *) malloc (size);

   PRINTDEBUG ("Now we call BN_bn2bin().\n");
   size = BN_bn2bin (e, *bin);
   return size;
}

int
rebuild_pubkey (RSA ** rsa,
		const char *bin_mod,
		const unsigned int mod_size,
		const char *bin_exp, const unsigned int exp_size)
{

   BIGNUM *n, *e;

   if (rsa == NULL)
      return -1;
   if (bin_mod == NULL)
      return -1;
   if (bin_exp == NULL)
      return -1;
   if (mod_size * exp_size == 0)
      return -1;

   *rsa = RSA_new ();
   n = BN_new ();
   e = BN_new ();
   BN_bin2bn (bin_mod, mod_size, n);
   BN_bin2bn (bin_exp, exp_size, e);

   // BN_clear_free (r->n);
   // BN_clear_free (r->e);
   // BN_clear_free (r->dmp1);
   // BN_clear_free (r->dmq1);
   // BN_clear_free (r->iqmp);
   RSA_set0_key(*rsa, n, e, NULL);
   return 1;
}

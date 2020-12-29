#include "ssl.h"
#include "rsa.h"
#define MSG_SIZE 36
int
main ()
{
   RSA *prv, *pub;
   char *mod, *exp;
   unsigned int mod_size, exp_size, i, crypt_size, buf_size;
   unsigned char *buf;
   unsigned char *crypt;

   printf ("Inizio\n");
   buf = (char *) malloc (MSG_SIZE + 1);
   bzero ((void *) buf, MSG_SIZE + 1);

   printf ("Genero chiave\n");
   prv = generate_key ();

   printf ("Tiro fuori modulo ed esponente\n");
   mod_size = get_modulus (prv, &mod);
   exp_size = get_pub_exp (prv, &exp);

   printf ("Ricostruisco la chiave\n");
   rebuild_pubkey (&pub, mod, mod_size, exp, exp_size);

   printf ("Private key:\n");
   RSA_print_fp (stdout, prv, 1);
   printf ("\nPublic key:\n");
   RSA_print_fp (stdout, pub, 1);
   printf ("\n");

   fgets (buf, MSG_SIZE + 1, stdin);

   printf ("Crittografazione...\n");
   crypt_size = encrypt (MSG_SIZE, buf, &crypt, prv);
   printf ("OK! %d\n", crypt_size);

   printf ("Messaggio crittografato:");
   for (i = 0; i < RSA_size (prv); i++)
      printf ("%02x ", crypt[i]);

   printf ("\nDimensioni: %d\n", crypt_size);

   bzero ((void *) buf, MSG_SIZE);
   free ((void *) buf);

   printf ("Decrittografazione...\n");
   buf_size = decrypt (crypt_size, crypt, &buf, pub);
   printf ("OK!\n");

   printf ("Stringa crittografata e decrittografata:");
   for (i = 0; i < buf_size; i++)
      printf ("%c", buf[i]);
   printf ("\n");

   return 1;
}

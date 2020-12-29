#include "common.h"
#include "rsa.h"
#include "ssl.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

int
main ()
{
   RSA *rsa;
   FILE *f;
   char *mod, *exp;
   int mod_size, exp_size, i;
   int sock, type, size;
   char buffer[1024];
   char file_buffer[775];
   char digest[36];
   char *temp;
   char dig_file_buffer[860457];
   struct sockaddr_in saddr;
   time_t tempo;
   ack ack_buf;
   header h;

   tempo = 333;
   bzero ((void *) &saddr, sizeof (saddr));
   rsa = generate_key ();
   mod_size = get_modulus (rsa, &mod);
   exp_size = get_pub_exp (rsa, &exp);
   printf ("Mod_size: %d\nExp_size=%d\n", mod_size, exp_size);
   sock = socket (AF_INET, SOCK_STREAM, 0);
   inet_pton (AF_INET, "127.0.0.1", (void *) &saddr.sin_addr);
   saddr.sin_family = AF_INET;
   saddr.sin_port = htons (8001);
   connect (sock, (struct sockaddr *) &saddr, sizeof (saddr));
   read (sock, &ack_buf, sizeof (ack_buf));
   printf ("Got response %d\n", ack_buf.type);
   type = 1;
   temp = buffer;
   memcpy (temp, &type, sizeof (tempo));
   temp += sizeof (type);
   memcpy (temp, &tempo, sizeof (type));
   temp += sizeof (tempo);
   memcpy (temp, &mod_size, sizeof (mod_size));
   temp += sizeof (mod_size);
   memcpy (temp, mod, mod_size);
   temp += mod_size;
   memcpy (temp, &exp_size, sizeof (exp_size));
   temp += sizeof (exp_size);
   memcpy (temp, exp, exp_size);
   temp += exp_size;
   size = temp - buffer;
   printf ("Packet size: %d\n", size);
   write (sock, buffer, size);
   read (sock, &ack_buf, sizeof (ack_buf));
   printf ("Got response %d\n", ack_buf.type);
   f = fopen ("prova_tx", "r");
   h.type = STATUS_PORT;
   h.length = 860457;
   fread (dig_file_buffer, 860457, 1, f);
   rewind (f);
   get_digest (dig_file_buffer, 860457, digest, digest + 16);
   printf ("Encrypt: %d",
	   RSA_private_encrypt (36, digest, h.digest, rsa,
				RSA_PKCS1_PADDING));
   write (sock, &h, sizeof (h));

   while ((i = fread (file_buffer, 1, sizeof (file_buffer), f)) > 0) {
      write (sock, file_buffer, i);
   }
   close (sock);
   sleep (1);
   sock = socket (AF_INET, SOCK_STREAM, 0);
   connect (sock, (struct sockaddr *) &saddr, sizeof (saddr));
   printf ("%s", strerror (errno));
   read (sock, &ack_buf, sizeof (ack_buf));

   fseek (f, 0, SEEK_SET);
   write (sock, &h, sizeof (h));
   while ((i = fread (file_buffer, 1, sizeof (file_buffer), f)) > 0) {
      write (sock, file_buffer, i);
   }
   close (sock);
   sleep (1);
   sock = socket (AF_INET, SOCK_STREAM, 0);
   connect (sock, (struct sockaddr *) &saddr, sizeof (saddr));
   printf ("%s", strerror (errno));
   read (sock, &ack_buf, sizeof (ack_buf));

   fseek (f, 0, SEEK_SET);
   write (sock, buffer, size);
   read (sock, &ack_buf, sizeof (ack_buf));
   printf ("Got response %d\n", ack_buf.type);
   write (sock, &h, sizeof (h));
   while ((i = fread (file_buffer, 1, sizeof (file_buffer), f)) > 0) {
      write (sock, file_buffer, i);
   }
   while (1);
   close (sock);
   exit (0);
}

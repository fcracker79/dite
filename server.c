#include "server.h"

#include <openssl/md5.h>
#include <openssl/sha.h>

/**************************************************************/
/*                     global variables                       */
/**************************************************************/

static int status_type[] = {
   STATUS_PORT,
   STATUS_KERNEL,
   STATUS_FILES
};

static char *status_string[] = {
   "_ports",
   "_kernel",
   "_files"
};

#define NUM_STATUS sizeof (status_type) / sizeof (int)

/*
	client descriptors array
 */
static client_desc *client_desc_array = NULL;
static unsigned int num_desc = 0;

/* socket read/write timeout
	real socket timeout is maximum value between rw_timeout and
	a client specified value
 */
static struct timeval *rw_timeout = NULL;

static unsigned short int server_port = DEFAULT_SERVER_PORT; /* host order */
static int listen_socket = 0; /* global listening socket */
static int verbose = 0;		/* quiet */
static int foreground = 0;	/* background */
static pthread_t listen_tid = 0;	/* listen thread id */

/* HUP synchronization */
static sem_t sighup_sync;
static int server_gonna_update = 0;
static pthread_t real_sighup_handler_tid = 0;

/* SIGQUIT catching */
static sem_t sigquit_sync;
static pthread_t real_sigquit_handler_tid = 0;

/*
	server configuration full path file name
 */
static char *server_conf_filename = NULL;
static char *server_pid_filename = NULL;
static char *server_prefix = NULL;
static char *server_conf_prefix = NULL;
static char *server_logfile_name = NULL;
static int   server_logfile_fd = -1;

/*
	configuration file parsing time
 */
static time_t server_parse_time = 0;

/**************************************************************/
/*                   functions definitions                    */
/**************************************************************/

void
sigquit_handler (int i) {
   sem_post (&sigquit_sync);
}

void
sighup_handler (int i) {
	sem_post (&sighup_sync);
}

int
string_error (char *dest, unsigned int errnum)
{
   if (dest == NULL)
      return -1;

   switch (errnum) {
   case S_OK:
      strcpy (dest, "success");
      break;

   case S_READ_ERR:
   case S_WRITE_ERR:
   case S_ERR_ACK:
   case S_ERR_NAK:
      strcpy (dest, "socket error");
      break;

   case S_DIFF_SIZE:
   case S_CMP_ERR:
   case S_SHORT_DATA:
   case S_TOO_LARGE:
   case S_FILE_TOO_LRG:
   case S_DIG_CMP_ERR:
      strcpy (dest, "comparison error");
      break;

   case S_DIG_SIZE_ERR:
      strcpy (dest, "ugly error");
      break;
   default:
      strcpy (dest, "unknown error");
      return -1;
   }
   return 1;
}

int
parse_server ()
{
   struct parsing_entry *list;
   struct parsing_entry *p;
   int ret;
   int i;

   list = NULL;
   if (server_conf_filename != NULL)
      ret = parse_file (server_conf_filename, &list);
   else
      ret = parse_file (DEFAULT_SERVER_CONF_FILE_NAME, &list);

	if (ret < 0)
		goto parse_server_error;

   /*
      process parsing list to fit global variables
    */

   if (!list)
      return 0;

   server_prefix = Malloc (strlen (DEFAULT_SERVER_PREFIX) + 1);
   strcpy (server_prefix, DEFAULT_SERVER_PREFIX);

   server_conf_prefix = Malloc (strlen (DEFAULT_SERVER_CONF_PREFIX) + 1);
   strcpy (server_conf_prefix, DEFAULT_SERVER_CONF_PREFIX);

   /* set logfilename to default */
   server_logfile_name = Malloc (1 + strlen(DEFAULT_SERVER_LOGFILE));
   strcpy (server_logfile_name, DEFAULT_SERVER_LOGFILE);

   /* count client descriptors */
   p = list;
   while (p) {
      if (!strcmp (p->command, "client") && p->argc == 2)
	      ++num_desc;
      p = p->next;
   }
   /* allocate client descriptors array memory
      and initialize sync semaphores to 0 */
   if (num_desc > 0) {
      client_desc_array = Malloc (sizeof (client_desc) * num_desc);
      for (i = 0; i < num_desc; i++)
	 sem_init (&client_desc_array[i].sync, 0, 0);
   }

   /* set pid filename to default */
   server_pid_filename = Malloc (1 + strlen(DEFAULT_SERVER_PID_FILENAME));
   strcpy (server_pid_filename, DEFAULT_SERVER_PID_FILENAME);

   /* allocate timeout structure and set to default value */
   rw_timeout = Malloc (sizeof (struct timeval));
   rw_timeout->tv_sec = DEFAULT_RW_TIMEOUT / 1000;
   rw_timeout->tv_usec = (DEFAULT_RW_TIMEOUT % 1000) * 1000;

   i = 0;	/* point to current client descriptor, if any */
   p = list;
   while (p) {

      /* client descriptor */
      if (!strcmp (p->command, "client") && p->argc == 2) {
	 int n;			/* minimum between max lengths */

	 n = (MAX_NICK_LENGTH > MAX_PARSING_COMMAND_ARGUMENT) ?
	    MAX_PARSING_COMMAND_ARGUMENT : MAX_NICK_LENGTH;

	 /* copy nick */
	 strncpy (client_desc_array[i].nick, p->argv[0], n);

	 /* force null termination */
	 client_desc_array[i].nick[n] = '\0';

	 /* copy address */
	 n = inet_aton (p->argv[1], &client_desc_array[i].addr);
	 if (!n) {
	    client_desc_array[i].addr.s_addr = htonl (INADDR_ANY);
		 goto parse_server_error;
	 }
	 ++i;
      }

      /* port number */
      else if (!strcmp (p->command, "port") && p->argc == 1) {
	 server_port = (atoi (p->argv[0]) > MAX_PORT_NUMBER ||
			atoi (p->argv[0]) <=
			0) ? server_port : atoi (p->argv[0]);
      }

      /* server prefix */
      else if (!strcmp (p->command, "prefix") && p->argc == 1) {
	 free (server_prefix);
	 server_prefix = Malloc (strlen (p->argv[0]) + 1);
	 strcpy (server_prefix, p->argv[0]);
      }
      
      /* server configuration prefix */      
		else if (!strcmp (p->command, "conf_prefix") && p->argc == 1) {
			free (server_conf_prefix);
			server_conf_prefix = Malloc (strlen (p->argv[0]) + 1);
			strcpy (server_conf_prefix, p->argv[0]);
		}

      /* logfile name */
      else if (!strcmp (p->command, "logfile") && p->argc == 1) {
         free (server_logfile_name);
         server_logfile_name = Malloc (1 + strlen(p->argv[0]));
         strcpy (server_logfile_name, p->argv[0]);
      }

      /* read/write timeout */
      else if (!strcmp (p->command, "rw_timeout") && p->argc == 1) {
	 int t;

	 t = (atoi (p->argv[0]) > 0) ? atoi (p->argv[0]) : 0;
	 rw_timeout->tv_sec = t / 1000;
	 rw_timeout->tv_usec = (t % 1000) * 1000;
      }

      /* pid filename */
      else if (!strcmp (p->command, "pidfile") && p->argc == 1) {
         free (server_pid_filename);
         server_pid_filename = Malloc (1 + strlen(p->argv[0]));
         strcpy (server_pid_filename, p->argv[0]);
      }

      p = p->next;
   }

   /* destroy parsing list */
   parse_destroy_list (list);

   /* update parsing time */
   server_parse_time = time (NULL);

   return 1;
parse_server_error:
	return -1;
}

int
reset_server ()
{

	sem_destroy (&sighup_sync);
	sem_destroy (&sigquit_sync);
	
   if (client_desc_array) {
      int i;

      for (i = 0; i < num_desc; i++) {
			/* TODO garbage collection (rsa and so on) */
	      sem_destroy (&client_desc_array[i].sync);
		}

      free (client_desc_array);
      client_desc_array = NULL;
      num_desc = 0;
   }

   if (server_prefix) {
      free (server_prefix);
      server_prefix = NULL;
   }

	if (server_conf_prefix) {
		free (server_conf_prefix);
		server_conf_prefix = NULL;
	}

	if (server_pid_filename) {
		free (server_pid_filename);
		server_pid_filename = NULL;
	}

   if (rw_timeout) {
      free (rw_timeout);
      rw_timeout = NULL;
   }

   if (server_logfile_name) {
      free (server_logfile_name);
      server_logfile_name = NULL;
   }


   server_port = DEFAULT_SERVER_PORT;

   server_parse_time = time (NULL);

   /* close logfile */
   if (server_logfile_fd >= 0)
      close (server_logfile_fd);

   return 1;
}

int
print_server_conf (FILE * f)
{
   int i;

   fprintf (f, "SERVER CONFIGURATION\n");
   fprintf (f, "parsed at %.24s\n", ctime (&server_parse_time));
   if (server_conf_filename)
      fprintf (f, "configuration file: %s\n", server_conf_filename);
   else
      fprintf (f, "configuration file: %s\n", DEFAULT_SERVER_CONF_FILE_NAME);
   if (verbose)
      fprintf (f, "verbosity on\n");
   else
      fprintf (f, "verbosity off\n");
   if (foreground)
      fprintf (f, "foreground run mode\n");
   else
      fprintf (f, "background run mode\n");
	fprintf (f, "pid filename %s\n", server_pid_filename);
   fprintf (f, "port number %d\n", server_port);
   fprintf (f, "read/write timeout: %f\n",
	    rw_timeout->tv_sec + (float) rw_timeout->tv_usec / 1000000.0);
   fprintf (f, "prefix %s\n", server_prefix);
	fprintf (f, "conf prefix %s\n", server_conf_prefix);
   fprintf (f, "logfile %s\n", server_logfile_name);
   fprintf (f, "clients\n");
   for (i = 0; i < num_desc; i++)
      fprintf (f, "\t%s at %s\n", client_desc_array[i].nick,
	       inet_ntoa (client_desc_array[i].addr));
   fprintf (f, "\n");

   return 1;
}

void
print_usage ()
{
   printf (PROGRAM_NAME " version " VERSION_MAJOR "." VERSION_MINOR "\n"
	   "usage: " SERVER_PROGRAM_NAME " [-hvF] [-c conf]\n"
	   "   -c conf    use file conf as configuration file\n"
	   "   -F         run in foreground\n"
	   "   -v         verbose (useful only if -F)\n"
	   "   -h         print this help\n");
}

/**************************************************************/
/*                       threads section                      */
/**************************************************************/

void *
listen_thread (void *v)
{
   int i;
   struct sockaddr_in s;
   struct sockaddr_in c;
   socklen_t csize;
   int sock_conn, found;
   int x;

   found = 0;

   bzero ((void *) &s, sizeof (struct sockaddr_in));
   s.sin_family = AF_INET;
   s.sin_addr.s_addr = htonl (INADDR_ANY);
   s.sin_port = htons (server_port);

   listen_socket = socket (AF_INET, SOCK_STREAM, 0);
   if (listen_socket < 0)
      goto listen_thread_error;

   x = 1;
   setsockopt (listen_socket, SOL_SOCKET, SO_REUSEADDR, &x, sizeof (x));

   if (bind (listen_socket, (struct sockaddr *) &s, sizeof (s)) < 0)
      goto listen_thread_error;

   if (listen (listen_socket, 0) < 0)
      goto listen_thread_error;

   while ((sock_conn = accept (listen_socket, (struct sockaddr *) &c, &csize)) >= 0) {
      MOREVERBOSE ("connection from %s", inet_ntoa (c.sin_addr));
      for (i = 0; i < num_desc; i++) {
	 if (memcmp (&client_desc_array[i].addr, &c.sin_addr,
		     sizeof (struct in_addr)) == 0) {
	    if ((client_desc_array[i].status != LISTEN)) {
	       VERBOSE ("connection from %s refused", inet_ntoa (c.sin_addr));
	       close (sock_conn);
	    }
	    else {
	       /* make connected socket non blocking */
	       fcntl (sock_conn, F_SETFL, O_NONBLOCK);

	       /* unlock thread waiting for its client connection */
	       client_desc_array[i].socket = sock_conn;
	       client_desc_array[i].status = SEND_OK;
	       sem_post (&client_desc_array[i].sync);
	    }
	    found = 1;
	    break;
	 }
      }
      if (!found)
	 close (sock_conn);
   }
   return NULL;

 listen_thread_error:
   fprintf (stderr, "listen_thread: %s\n", strerror (errno));
   exit (errno);
}

void *
server_thread (void *v)
{
	/* Buffer for log print. */
	char printlog_buf[MAX_LOGFILE_LINE_LENGTH-33];
	
	/* Specific client descriptor. */
   client_desc *desc;

	/* Sructures needed for digest calculation. */
   MD5_CTX md5;
   SHA_CTX sha1;
	
   /* Temporary file handler to write on disk what we receive from client. */
	FILE *f;
	
   /* File name string for the temporary file. */
	char *filename;

	/* Buffer used to build an ACK packet. */
   ack ack_buf;
	
   char *p_buffer;

	/* error reports, on the server, the internal status. */
   int type, error;

	/* Structures needed for public key management. */
   char pubmod[NUM_BITS / 8];
   int mod_size;
   char pubexp[NUM_BITS / 8];
   int exp_size;

	/* Buffer used to manage the header part of a packet. */
   header header_buf;

	/* Buffer used to put the local digest calculation. */
   unsigned char digest[MD5_DIGEST_LENGTH + SHA_DIGEST_LENGTH];

	/* Buffer used to decrypt the remote calculated digest. */
   unsigned char *digest_header;
	
   int digest_header_len;
   int type_buf;

	/* Temporary buffer. Used for partial read from the socket. */
   unsigned char buffer[BUFFER_SIZE];

	/* Temporary buffer. Used for partial read from the files. */
   unsigned char file_buffer[BUFFER_SIZE];

	/* Return values. */
   int read_size, write_size;

	/* Num. of status packet being managed. */
   int which_packet;

	/* Buffer for self-thread printing error strings. */
   char error_buf[MAX_STRING_ERR_LEN];

   desc = (client_desc *) v;	/* alias */
   /* zeroing buffers */
   desc->rsa = NULL;
   digest_header = NULL;

   MOREVERBOSE ("Starting server thread %ld, nick %s", pthread_self (),
		  desc->nick);
	snprintf (printlog_buf, sizeof(printlog_buf),
			"starting server thread %s", desc->nick);
	printlog (printlog_buf, server_logfile_fd);
	
   desc->socket_timeout = NULL;

     /* Now we check if the server was killed or crashed. */
   filename = Malloc ( strlen (server_conf_prefix) + strlen (desc->nick) + 2 +
         strlen (CONF_POSTFIX) );
   strcpy (filename, server_conf_prefix);
   strcat (filename, "/");
   strcat (filename, desc->nick);
   strcat (filename, CONF_POSTFIX);
   f = fopen (filename, "r");
   free (filename);
   filename = NULL;

   if (f) {
      VERBOSE ("recovering saved data on %s", desc->nick);
      printlog ("recovering saved data", server_logfile_fd);
		
      desc->socket_timeout = Malloc (sizeof (struct timeval));

      read_size = fread (&mod_size, 1 , sizeof (mod_size), f);
      read_size += fread (&exp_size, 1, sizeof (exp_size), f);
      read_size += fread (pubmod, 1, mod_size, f);
      read_size += fread (pubexp, 1, exp_size, f);
      read_size += fread (desc->socket_timeout, 1,
            sizeof (struct timeval), f);
      fclose (f);

		/* If there was a problem on crash recovery, we expect a new configuation
			from the client. In fact, if we could not read the configuration
			file, it is possible that the client could not send correctly its
			configuration, so it is still trying to do it.
       */
      if ( read_size < (sizeof (mod_size) + sizeof (exp_size) +
               mod_size + exp_size +
               sizeof (struct timeval)) ) {
         VERBOSE ("unable to recover config on %s", desc->nick);
			printlog ("unable to recover config", server_logfile_fd);
			
         free (desc->socket_timeout);
         desc->socket_timeout = NULL;
      }

		/* If all recovery information is retrieved, we suppose that it is
			correct. The client could not change it because the server was down.
		 */	
      else {
			int z;
         MOREVERBOSE ("All recovery information retrieved on %s", desc->nick);
         rebuild_pubkey (&desc->rsa, pubmod, mod_size, pubexp, exp_size);
			
         desc->got_conf = 1;
		   
		   /* We assume we have a status, that is the common case. But if we
			   have no status, we assume it no more.
			 */	
			desc->got_status = 1;
			
			/* Now we have to check if we also have a status system */
			for (z = 0; z < NUM_STATUS; z++) {
				filename = Malloc ( strlen (server_prefix) + 2 + 
						strlen (desc->nick) + strlen (status_string[z]));
				strcpy (filename, server_prefix);
				strcat (filename, "/");
				strcat (filename, desc->nick);
				strcat (filename, status_string[z]);
				
				f = fopen (filename, "r");
				MOREVERBOSE ("checking file %s on %s", filename, desc->nick);
				/* We have no status. We will accept any new one */
				if (f == NULL) desc->got_status = 0;
			   else fclose (f);
				free (filename);
				filename = NULL;

				if (!desc->got_status) break;
			}
			
			if (desc->got_status) {
				MOREVERBOSE ("status information also available on %s", desc->nick);
			}
			else {
				MOREVERBOSE ("status information not available on %s", desc->nick);
			}
      } /* if (read_size < (...)) ... else */
   } /* if (f) */

	/* following cycle will terminate only if a SIGHUP is caught */
	/* Cycle A */
   while (1) {
      desc->status = LISTEN;
      VERBOSE ("status LISTEN %s", desc->nick);

      /* waiting for listen thread to post sync semaphore */
      sem_wait (&desc->sync);

		if (server_gonna_update) {
			MOREVERBOSE ("%s sighup caught", desc->nick);
			break;
		}

		desc->status = SEND_OK;
		VERBOSE ("status SEND_OK %s", desc->nick);
		ack_buf.type = SERVER_CARRIES_TIMEOUT;
		memcpy (&ack_buf.payload.sock_timeout, rw_timeout,
			sizeof (struct timeval));
		write_size = Write (desc->socket, &ack_buf,
				sizeof (ack_buf), desc->socket_timeout);

      if (write_size < sizeof (ack_buf)) {
	      VERBOSE ("error on SEND_OK %s", desc->nick);
	      close (desc->socket);
			/* From here we go to the Cycle A beginning. */
	      continue;
      }

		/* Client connected */

      /* We read the packet type */
      if (Read (desc->socket, &type_buf, sizeof (type_buf), 
					desc->socket_timeout) < sizeof (type_buf)) {
	      VERBOSE ("error on getting packet type %s", desc->nick)
	      close (desc->socket);
			/* From here we go to the Cycle A beginning. */
	      continue;
      }

      /* If we get a PKT_CONF packet and we have already the configuration, there
         may be something wrong.
       */
      if (desc->got_conf && type_buf == PKT_CONF) {
	    VERBOSE ("warning: waiting for packet, got conf")
		 syslog (LOG_ALERT, "waiting for packet, got conf from %s", desc->nick);
	    printlog ("got unrequested configuration", server_logfile_fd);
      }

      /* However, if we have not the status or the packet is a PKT_CONF, we try to
         get it.
       */
      if (!desc->got_conf || type_buf == PKT_CONF) {
	 int x, y;

	 desc->status = WAIT_FOR_CONF;
	 VERBOSE ("status WAIT_FOR_CONF %s", desc->nick);
	 memcpy (buffer, &type_buf, sizeof (type_buf));

	 /* Now we read the rest of the header, or the rest of the status */
	 read_size = Read (desc->socket, buffer + sizeof (header_buf.type),
			   sizeof (buffer) - sizeof (header_buf.type),
			   desc->socket_timeout);
	 if (read_size < 0) {
	    VERBOSE ("error on WAIT_FOR_CONF %s", desc->nick)
	       close (desc->socket);
	    continue;
	 }

	 MOREVERBOSE ("read %d bytes from %s", read_size, desc->nick)
	    memcpy (&type, buffer, sizeof (type));
	 if (type != PKT_CONF) {
	    VERBOSE ("error: waiting type PKT_CONF, got %d on %s",
		     type, desc->nick)
	       close (desc->socket);
	    continue;
	 }

	 MOREVERBOSE ("got PKT_CONF packet on %s", desc->nick)
	    p_buffer = buffer;

	 /* skip type bytes */
	 p_buffer += sizeof (type);
	 read_size -= sizeof (type);

	 /* get timeout period */
	 memcpy (&desc->period, p_buffer, sizeof (desc->period));
	 p_buffer += sizeof (desc->period);
	 read_size -= sizeof (desc->period);

	 /* get socket timeout from client */
	 memcpy (&desc->rw_timeout, p_buffer, sizeof (struct timeval));
	 p_buffer += sizeof (struct timeval);
	 read_size -= sizeof (struct timeval);

	 /* setting *real* socket timeout to maximum value between
	    newly got and global one */
	 if (desc->socket_timeout)
	    free (desc->socket_timeout);
	 desc->socket_timeout = Malloc (sizeof (struct timeval));
	 x = desc->rw_timeout.tv_sec * 1000000 + desc->rw_timeout.tv_usec;
	 y = rw_timeout->tv_sec * 1000000 + rw_timeout->tv_usec;
	 if (x > y)
	    memcpy (desc->socket_timeout, &desc->rw_timeout,
		    sizeof (struct timeval));
	 else
	    memcpy (desc->socket_timeout, rw_timeout, sizeof (struct timeval));

	 /* get modulus size */
	 memcpy (&mod_size, p_buffer, sizeof (mod_size));
	 p_buffer += sizeof (mod_size);
	 read_size -= sizeof (mod_size);

	 /* get public modulus */
	 memcpy (pubmod, p_buffer, mod_size);
	 p_buffer += mod_size;
	 read_size -= mod_size;

	 /* get exponent size */
	 memcpy (&exp_size, p_buffer, sizeof (exp_size));
	 p_buffer += sizeof (exp_size);
	 read_size -= sizeof (exp_size);

	 /* get public exponent */
	 memcpy (pubexp, p_buffer, exp_size);
	 /* p_buffer += exp_size */
	 read_size -= exp_size;

	 /* check for configuration packet length consistency */
	 read_size += sizeof (type_buf);
	 if (read_size < 0) {
	    VERBOSE ("error: packet PKT_CONF too short on %s", desc->nick)
	       close (desc->socket);
	    continue;
	 }

	 if (desc->rsa) {
	    RSA_free (desc->rsa);
	    desc->rsa = NULL;
	 }
	 rebuild_pubkey (&desc->rsa, pubmod, mod_size, pubexp, exp_size);

	 /* Now write on a file the configuration informations */
	 filename = Malloc ( strlen (server_conf_prefix) + 2 + strlen (desc->nick) +
			 strlen (CONF_POSTFIX) );
	 strcpy (filename, server_conf_prefix);
	 strcat (filename, "/");
	 strcat (filename, desc->nick);
	 strcat (filename, CONF_POSTFIX);
    f = fopen (filename, "w");
    if (f == NULL) {
      VERBOSE ("error opening %s on %s", filename, desc->nick);
      free (filename);
      continue;
    }
    free (filename);

    write_size = fwrite (&mod_size, 1, sizeof (mod_size), f);
    write_size += fwrite (&exp_size, 1, sizeof (exp_size), f);
    write_size += fwrite (pubmod, 1, mod_size, f);
    write_size += fwrite (pubexp, 1, exp_size, f);
    write_size += fwrite (desc->socket_timeout, 1, sizeof (struct timeval), f);

    if (write_size < (sizeof (mod_size) + sizeof (exp_size) +
             mod_size + exp_size +
             sizeof (struct timeval)) ) {
       VERBOSE ("error writing file on %s", desc->nick);
       fclose (f);

       ack_buf.type = SERVER_NAK;
       Write (desc->socket, &ack_buf, sizeof (ack_buf), desc->socket_timeout);
		 fclose (f);
       continue;
    }
    fclose (f);

	 /* send ack to client */
	 ack_buf.type = SERVER_OK;
	 Write (desc->socket, &ack_buf, sizeof (ack_buf), desc->socket_timeout);
	 desc->got_conf = 1;

	 /* read next packet */
	 read_size = Read (desc->socket, &type_buf,
			   sizeof (type_buf), desc->socket_timeout);
	 if (read_size < sizeof (type_buf)) {
	    VERBOSE ("error: connection terminated abruptly on %s",
		     desc->nick);
	    close (desc->socket);
	    continue;
	 }
      } /*if (!desc->got_conf || type_buf == PKT_CONF) */
      which_packet = 0;

      /* Here we have only the packet type on type_buf. It will be a tragedy.
       */
      while (which_packet < NUM_STATUS) {
	 header_buf.type = type_buf;
	 read_size =
	    Read (desc->socket, (void *) &header_buf + sizeof (type_buf),
		  sizeof (header_buf) - sizeof (type_buf), desc->socket_timeout);

	 if (read_size <= 0) {
	    VERBOSE ("error on reading from %s", desc->nick);
	    break;
	 }

	 read_size += sizeof (type_buf);
	 if (read_size < sizeof (header_buf)) {
	    VERBOSE ("error: header corrupted on %s", desc->nick);
	    MOREVERBOSE ("Waiting for %d bytes, got %d bytes",
			 sizeof (header_buf), read_size);
	    close (desc->socket);
	    continue;
	 }

	 if ((header_buf.type == POWER_DOWN) ||
			(header_buf.type == UPDATE) ) {
	    error = S_OK;

	    /* We get the digest of the header_buf.type. */
	    if (get_digest ((char *) &header_buf.type, sizeof (header_buf.type),
			    digest, digest + MD5_DIGEST_LENGTH) < 0) {
	       DIE ("Error on creating MD5/SHA1 digest on %s", desc->nick);
	    }

	    if (!digest_header)
	       digest_header = Malloc (RSA_size (desc->rsa));

	    digest_header_len =
	       RSA_public_decrypt (sizeof (header_buf.digest),
				   header_buf.digest, digest_header, desc->rsa,
				   RSA_PKCS1_PADDING);
		 
	    if (digest_header_len != sizeof (digest)) {
	       PRINTDEBUG ("digest_header_len = %d, sizeof (digest) = %d",
			   digest_header_len, sizeof (digest));
	       error = S_DIG_SIZE_ERR;
	    }
       else
			 if (memcmp (digest, digest_header, sizeof (digest)) != 0) {
             VERBOSE ("error comparing digest message on %s",
						desc->nick);
             error = S_DIG_CMP_ERR;
			 }
		 ack_buf.type = (error) ? SERVER_NAK : SERVER_OK;
		 
		 if (Write (desc->socket, &ack_buf, sizeof (ack_buf), 
					 desc->socket_timeout) < 0) {
			 
			 switch (header_buf.type) {
				 case POWER_DOWN:
			       VERBOSE("error sending Power Down ACK on %s", desc->nick);
					 break;
				 case UPDATE:
					 VERBOSE ("error sending Update ACK on %s", desc->nick);
				 default:
					 /* For future developement. */
					 ;
			 }
			 
			 error = S_WRITE_ERR;
		 }

		 if (!error) {
			 switch (header_buf.type) {
				 case POWER_DOWN:
					 desc->got_conf = 0;
			       MOREVERBOSE ("sent Power Down ACK on %s", desc->nick);
			       VERBOSE ("POWER DOWN %s", desc->nick);
					 printlog ("power down", server_logfile_fd);
					 break;
				 case UPDATE:
					 desc->got_conf = 0;
					 desc->got_status = 0;
					 MOREVERBOSE ("sent Update ACK on %s", desc->nick);
					 VERBOSE ("UPDATE %s", desc->nick);
					 printlog ("update", server_logfile_fd);
					 break;
				 default:
					 /* For future developement. */
					 ;
			 }
		 }
	    break;
	 }
	 
	 /* In this case, the packet type must be equal to the current status
	    type (ports, kernel or files). Otherwise, there is a problem.
	  */
	 if (header_buf.type != status_type[which_packet]) {
	    VERBOSE ("error: header type incorrect on %s", desc->nick);
	    MOREVERBOSE ("Waiting for %d, got %d", status_type[which_packet],
			 header_buf.type);
	    close (desc->socket);
	    continue;
	 }

	 desc->status = status_type[which_packet];
	 VERBOSE ("status WAIT_FOR%s %s", status_string[which_packet],
		  desc->nick);
	 error = S_OK;
	 MD5_Init (&md5);
	 SHA1_Init (&sha1);

	 /* filename size = server prefix size + nick length + postfix +
	    "/" between server prefix and nick + '\0' */
	 filename = Malloc (strlen (server_prefix) + 2 +
			    strlen (desc->nick) +
			    strlen (status_string[which_packet]));

	 strcpy (filename, server_prefix);
	 strcat (filename, "/");
	 strcat (filename, desc->nick);
	 strcat (filename, status_string[which_packet]);
	 if (desc->got_status)
	    f = fopen (filename, "r");
	 else
	    f = fopen (filename, "w");
	 if (f == NULL) {
	    VERBOSE ("file %s not found on %s", filename, desc->nick);
	    free (filename);
	    close (desc->socket);
	    continue;
	 }
	 free (filename);
	 /* Here we read all data. If we have already a state, we compare the
	    received data with the old ones, else we write it. At the same time,
	    we calculate the MD5/SHA1 digest, so we will be able to compare it
	    with the sent one.
	  */
	 while (1) {	/* Cycle B */
	    read_size = Read (desc->socket, buffer, sizeof (buffer),
			      desc->socket_timeout);
	    if (read_size < 0) {
	       VERBOSE ("error reading data on %s", desc->nick);
	       error = S_READ_ERR;
	       break;
	    }
	    MD5_Update (&md5, buffer, read_size);
	    SHA1_Update (&sha1, buffer, read_size);
	    if (desc->got_status) {
	       int fread_size;

	       /* Comparison.
	        */
	       fread_size = fread (file_buffer, 1, read_size, f);
	       if (fread_size != read_size) {
		  VERBOSE ("error on reading data from the file on %s",
			   desc->nick);
		  error = S_DIFF_SIZE;
		  break;
	       }
	       if (memcmp (file_buffer, buffer, read_size) != 0) {
		  VERBOSE ("error comparing data on %s", desc->nick);
		  error = S_CMP_ERR;
		  break;
	       }
	    }
	    else {
	       if (fwrite (buffer, 1, read_size, f) < 0) {
		  VERBOSE ("error writing file on %s", desc->nick);
		  error = S_WRITE_ERR;
		  break;
	       }
	    }
	    header_buf.length -= read_size;
	    if (header_buf.length < 0) {
	       VERBOSE ("data sent too short on %s", desc->nick);
	       error = S_SHORT_DATA;
	       break;
	    }
	    if (header_buf.length == 0) {
	       MOREVERBOSE ("OK getting data on %s", desc->nick);
	       break;
	    }

	 } /* End of Cycle B */
	 /* If we have already a status and the comparison is ok, we must 
	    check that the old one isn't too long.
	  */
	 if (desc->got_status) {
	    long int cur_pos, last_pos;

	    cur_pos = ftell (f);
	    fseek (f, 0, SEEK_END);
	    last_pos = ftell (f);
	    if (cur_pos != last_pos) {
	       VERBOSE ("file too large on %s", desc->nick);
	       error = S_FILE_TOO_LRG;
	    }
	 }
	 fclose (f);

	 /* Ok. Now we have a status or we have got an error. Everything is
	    written on file if it was the first status */
	 if (!error) {
	    MOREVERBOSE ("Start decrypting digest message on %s", desc->nick);

	    if (digest_header)
	       free (digest_header);
	    digest_header = Malloc (RSA_size (desc->rsa));

	    digest_header_len =
	       RSA_public_decrypt (sizeof (header_buf.digest),
				   header_buf.digest, digest_header, desc->rsa,
				   RSA_PKCS1_PADDING);

	    if (digest_header_len != sizeof (digest)) {
	       PRINTDEBUG ("digest_header_len = %d, sizeof (digest) = %d",
			   digest_header_len, sizeof (digest));
	       error = S_DIG_SIZE_ERR;
	    }
	    /* The decrypted digest size is correct */
	    else {
	       MD5_Final (digest, &md5);
	       SHA1_Final (digest + MD5_DIGEST_LENGTH, &sha1);
	       /* Let's check that our digest message is identical to the sent 
	          one.
	        */
	       MOREVERBOSE ("Comparing digest message on %s", desc->nick);
	       if (memcmp (digest, digest_header, sizeof (digest)) != 0) {
		  VERBOSE ("error comparing digest message on %s",
			   desc->nick);
		  error = S_DIG_CMP_ERR;
	       }
	       else {
		  /* Well. The comparison is ok, the MD5/SHA1 is ok, so we 
		     will have to send the OK to the client.
		   */
		  MOREVERBOSE ("Digest comparison OK on %s", desc->nick);
		  ack_buf.type = SERVER_OK;
		  if (Write (desc->socket, &ack_buf,
			     sizeof (ack_buf), desc->socket_timeout) < 0) {
		     VERBOSE ("error sending ACK message on %s", desc->nick);
		     error = S_ERR_ACK;
		  }
		  MOREVERBOSE ("Sent ACK on %s", desc->nick);
	       } /* if (memcmp (digest, digest_header, sizeof (digest)) != 0)...
					else */
	    } /*if (digest_header_len != sizeof (digest)) ... else */
	 } /* if (!error) */

	 if (digest_header) {
	    free (digest_header);
	    digest_header = NULL;
	 }

	 if (error) {
	    ack_buf.type = SERVER_NAK;

	    /* On any error, we send a NAK message, so the client knows
	       something was wrong.
	       For future developement, the ack structure will also contain
	       the reason NAK.
	       TODO
	     */
	    if (Write (desc->socket, &ack_buf,
		       sizeof (ack_buf), desc->socket_timeout) < 0) {
	       VERBOSE ("error sending NAK message on %s", desc->nick);
	       error = S_ERR_NAK;
	    }
	    else {
	       MOREVERBOSE ("sent NAK on %s", desc->nick);

	    }
	    break;
	 }
	 which_packet++;
	 if (which_packet < NUM_STATUS) {
	    read_size = Read (desc->socket, &type_buf, sizeof (type_buf),
			      desc->socket_timeout);
	    /* Now it's time to read the next packet type.
	       On any error, everybody dies...
	     */
	    if (read_size < sizeof (type_buf)) {
	       VERBOSE ("error: got less data that expected %s",
			desc->nick);
	       error = S_READ_ERR;
	       break;
	    }
	 } /* if (which_packet < NUM_STATUS) */
      }	/* while (which_packet < NUM_STATUS) */

      close (desc->socket);

		/* Here we are finishing the inner cycle. But on Power down
			we have to return on LISTEN status.
		 */	
      if ( (header_buf.type == POWER_DOWN) ||
				(header_buf.type == UPDATE) ) continue;

      if (!error) {
	      MOREVERBOSE ("Got status on %s", desc->nick);
			printlog ("mark", server_logfile_fd);
	      desc->got_status = 1;
      }
      else {
	      string_error (error_buf, error);
	      syslog (LOG_ERR, error_buf);
			printlog ("error", server_logfile_fd);
      }
   } /* while (1). Cycle A */

	/* TODO
		? ? ? ? ? ? ? ? ? ? garbage collection ? ? ? ? ? ? ? ? ? ?  
		ON VARIABLES LOCAL TO THIS FUNCTION (NOT CLIENT DESCRIPTOR)
	 */

	MOREVERBOSE ("%s quitting", desc->nick);
   printlog ("quit", server_logfile_fd);

   return NULL;
}

void
start_threads (void)
{
   unsigned int i;

	sem_init (&sighup_sync, 0, 0);
	sem_init (&sigquit_sync, 0, 0);

   syslog (LOG_INFO, SERVER_PROGRAM_NAME" started");

   /* open logfile */
   server_logfile_fd = open (server_logfile_name,
         O_WRONLY | O_CREAT | O_APPEND, 0644);

   if (server_logfile_fd < 0)
      syslog (LOG_WARNING, "could not open log file");

   printlog ("start", server_logfile_fd);

   for (i = 0; i < num_desc; i++)
      pthread_create (&client_desc_array[i].tid, NULL, server_thread,
		      &client_desc_array[i]);

   pthread_create (&listen_tid, NULL, listen_thread, NULL);
}

void*
real_sighup_handler (void* v) {
   int i;

   /* cycle MUST never end */
   while (1) {
      /* wait for SIGHUP to be caught */
      sem_wait (&sighup_sync);
		VERBOSE ("sighup caught");
		MOREVERBOSE ("server is NOT running");
		syslog (LOG_WARNING, "sighup caught");
      printlog ("sigterm caught", server_logfile_fd);
		
		server_gonna_update = 1;

		/* trigger threads */
		for (i = 0; i < num_desc; i++)
			sem_post (&client_desc_array[i].sync);

      /* wait for threads termination */
      for (i = 0; i < num_desc; i++)
         pthread_join (client_desc_array[i].tid, NULL);

		server_gonna_update = 0;

      /* Claudio think that's a safe way to terminate listen thread */
      pthread_cancel (listen_tid);
		close (listen_socket);

      reset_server ();
      parse_server ();
      start_threads ();
		MOREVERBOSE ("server is up again");
   } /* while (1) */

   return NULL;
}

void*
real_sigquit_handler (void* v) {
	int i;

	sem_wait (&sigquit_sync);
	VERBOSE ("sigquit caught");
   syslog (LOG_WARNING, "sigquit caught");	
   printlog ("sigquit caught", server_logfile_fd);

	server_gonna_update = 1;
	/* trigger threads */
	for (i = 0; i < num_desc; i++)
		sem_post (&client_desc_array[i].sync);

	/* wait for threads termination */
	for (i = 0; i < num_desc; i++)
		pthread_join (client_desc_array[i].tid, NULL);

	unlink (server_pid_filename);

	return NULL;
}

/**************************************************************/
/*                         main                               */
/**************************************************************/

int
main (int argc, char *argv[])
{
   int ch;
   int i, ret;
   char* program_name;
   int conffile_fd;
   int n;
   pid_t p;

   /* check for root privilege, if not exit */
   if (getuid () != 0) {
      printf ("you *must* be root\n");
      exit (1);
   }

   /* strip program name from full path */
   for (i = strlen(argv[0]) -1 ; i >= 0; i--) {
      if (argv[0][i] == '/') break;
   }

   if (i != 0) {
      program_name = Malloc (strlen(argv[0]) - i);
      strncpy (program_name, argv[0] + i + 1, strlen(argv[0]) - i - 1);
   }
   else {
      /* no '/' found */
      program_name = argv[0];
   }

   /* parse arguments */
   while ((ch = getopt (argc, argv, "hvFc:")) != -1) {
      switch (ch) {
      case 'h':
	 print_usage ();
	 exit (1);
      case 'v':
	 verbose++;
	 break;
      case 'F':
	 foreground++;
	 break;
      case 'c':
	 server_conf_filename = Malloc (strlen (optarg));
	 strcpy (server_conf_filename, optarg);
	 break;
      default:
	 print_usage ();
	 exit (1);
      }
   }

   argc -= optind;
   argv += optind;

   /* check for runaway arguments, if any exit */
   if (argc > 0) {
      print_usage ();
      exit (1);
   }

   /* parse configuration file */
   ret = parse_server ();

   /* check for runaway arguments, if any exit */
   if (argc > 0) {
      print_usage ();
      exit (1);
   }

   if ( ret < 0 ) {
      printf ("configuration parse error\n");
      exit (1);
   }

   if ( !strcmp (program_name, SERVER_PROGRAM_TESTCONF) ) {
      if ( ret >= 0 )
         print_server_conf (stdout);
      exit (0);
   }
   else if ( !strcmp (program_name, SERVER_PROGRAM_HUP) ) {
      conffile_fd = open (server_pid_filename, O_RDONLY);
      if (conffile_fd < 0) {
         printf (SERVER_PROGRAM_NAME" is not running\n");
         exit (1);
      }
      n = read (conffile_fd, &p, sizeof(p));
      if (n < sizeof(p)) {
         printf ("bad pid file\n");
         exit (1);
      }
      close (conffile_fd);
      kill (p, SIGHUP);
      exit (0);
   }
   else if ( !strcmp (program_name, SERVER_PROGRAM_QUIT) ) {
      conffile_fd = open (server_pid_filename, O_RDONLY);
      if (conffile_fd < 0) {
         printf (SERVER_PROGRAM_NAME" is not running\n");
         exit (1);
      }
      n = read (conffile_fd, &p, sizeof(p));
      if (n < sizeof(p)) {
         printf ("bad pid file\n");
         exit (1);
      }
      close (conffile_fd);
      kill (p, SIGQUIT);
      exit (0);
   }
   else if ( strcmp (program_name, SERVER_PROGRAM_NAME) ) {
      printf ("bad program name\n");
      exit (1);
   }

   conffile_fd = open (server_pid_filename, O_RDONLY);
   if ( conffile_fd >= 0 ) {
      printf (SERVER_PROGRAM_NAME" is already running\n");
      exit (1);
   }
   close (conffile_fd);

   conffile_fd = open (server_pid_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
   p = getpid ();
   n = write (conffile_fd, &p, sizeof(p));
   if (n < sizeof(p)) {
      printf ("could not write to pid file\n");
      exit (1);
   }
   close (conffile_fd);

   /* capture signals */
   signal (SIGTERM, SIG_IGN);
   signal (SIGHUP, sighup_handler);
   signal (SIGPIPE, SIG_IGN);
   signal (SIGQUIT, sigquit_handler);
   signal (SIGINT, SIG_IGN);

   /* run in background */
   if (!foreground) {
      daemon (0, 0);

      /* overwriting old value of process id stored in pid file */
      p = getpid ();
      conffile_fd = open (server_pid_filename,
            O_WRONLY | O_CREAT | O_TRUNC, 0644);
      write (conffile_fd, &p, sizeof(p));
      close (conffile_fd);
   }

   srand (time (0) ^ p);

   openlog (SERVER_PROGRAM_NAME, LOG_PID, LOG_DAEMON);

	start_threads ();

	pthread_create (&real_sighup_handler_tid, NULL, real_sighup_handler, NULL);
	pthread_create (&real_sigquit_handler_tid, NULL, real_sigquit_handler, NULL);

   pthread_join (real_sigquit_handler_tid, NULL);

   closelog ();

   printlog ("quit", server_logfile_fd);
   exit (0);
}

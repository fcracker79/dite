#include "client.h"

#include <openssl/sha.h>

/* client_has_to_quit == QUIT_SHUTDOWN ==>  the client has not to be updated.
   client_has_to_quit == QUIT_UPDATE ==> the client may be updated. 
 */
#define QUIT_SHUTDOWN 1
#define QUIT_UPDATE 2

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
	timer thread id
 */
pthread_t timer_tid = 0;

/*
   server descriptors array
 */
static server_desc *server_desc_array = NULL;
static unsigned long int server_num_desc = 0;

/*
	time we last built client status
 */
static time_t last_status_built = 0;

/*
	interval time after which we consider client status obsolete (sec)
 */
static time_t status_expire_time = DEFAULT_STATUS_EXPIRE_TIME;

/*
	read/write socket timeout
 */
static struct timeval *rw_timeout = NULL;

/* 
	client status variables
 */

/* buffers and corresponding sizes of client data status */
static void *client_ports_buf = NULL;
static unsigned int client_ports_len = 0;

static void *client_kernel_buf = NULL;
static unsigned int client_kernel_len = 0;

static void *client_files_buf = NULL;
static unsigned int client_files_len = 0;

/* read/write muteces */
static pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t read_mutex = PTHREAD_MUTEX_INITIALIZER;
static int num_readers = 0;

/* update/shutdown global variables */

static int client_has_to_quit = 0;
static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t sigquit_caught;
sem_t sigterm_caught;
time_queue timer_queue;
static int unpriv_port = DEFAULT_UNPRIV_PORT;

/*
   directory array
 */
static char **directory_array = NULL;
static unsigned int directory_num = 0;

/*
	client configuration
 */
static int verbose = 0;		/* quiet */
static int foreground = 0;	/* background */

/*
   server configuration full path file name & pid filename
 */
static char *client_conf_filename = NULL;
static char *client_pid_filename = NULL;
static char *client_logfile_name = NULL;
static int   client_logfile_fd = -1;

/*
   configuration file parsing time
 */
static time_t client_parse_time = 0;

static pthread_t sigquit_tid = 0;
static pthread_t sigterm_tid = 0;
static int timer_forever = 1;

/**************************************************************/
/*                   functions definitions                    */
/**************************************************************/

void sigquit_handler (int i) {
	sem_post (&sigquit_caught);
}

void sigterm_handler (int i) {
	sem_post (&sigterm_caught);
}

void* sigquit_real_handler (void* v) {
	int i;
	
	sem_wait (&sigquit_caught);

	VERBOSE ("SIGQUIT caught");
	syslog (LOG_INFO, "sigquit caught");
	printlog ("sigquit caught", client_logfile_fd);
	
	pthread_mutex_lock (&timer_mutex);
	client_has_to_quit = QUIT_SHUTDOWN;
	for (i = 0; i < server_num_desc; i++)
		sem_post (&server_desc_array[i].sync);
	pthread_mutex_unlock (&timer_mutex);

   /* We join to the client threads, so we will be able to cancell timer
	   thread.
	 */	
	for (i = 0; i < server_num_desc; i++)
		pthread_join (server_desc_array[i].tid, 0);

	MOREVERBOSE ("SIGQUIT function: all client threads died");

	/* Now the timer thread can die. */
	timer_forever = 0;
	return NULL;
}

void* sigterm_real_handler (void* v) {
	int i;
	sem_wait (&sigterm_caught);

   VERBOSE ("SIGTERM caught");
   syslog (LOG_INFO, "sigterm caught");
	printlog ("sigterm caught", client_logfile_fd);

	pthread_mutex_lock (&timer_mutex);
	client_has_to_quit = QUIT_UPDATE;
	for (i = 0; i < server_num_desc; i++)
		sem_post (&server_desc_array[i].sync);
	pthread_mutex_unlock (&timer_mutex);

   /* We join to the client threads, so we will be able to cancell timer
      thread.
    */
   for (i = 0; i < server_num_desc; i++)
      pthread_join (server_desc_array[i].tid, 0);
   
	MOREVERBOSE ("SIGTERM function: all client threads died");
   /* Now the timer thread can die. */
	timer_forever = 0;
	return NULL;
}

int
parse_client ()
{
   struct parsing_entry *list;
   struct parsing_entry *p;
   int ret;
   int i, j;

   list = NULL;
   if (client_conf_filename != NULL)
      ret = parse_file (client_conf_filename, &list);
   else
      ret = parse_file (DEFAULT_CLIENT_CONF_NAME, &list);

	if (ret < 0)
		goto parse_client_error;

   /*
      process parsing list to fit global variables
    */

   if (!list)
      return 0;

   /* count server descriptors & directories */
   p = list;
   while (p) {
      if (!strcmp (p->command, "server") && p->argc == 4)
	 ++server_num_desc;
      if (!strcmp (p->command, "directory") && p->argc == 1)
	 ++directory_num;
      p = p->next;
   }

	/* set pid filename to default */
	client_pid_filename = Malloc (1 + strlen(DEFAULT_CLIENT_PID_FILENAME));
	strcpy (client_pid_filename, DEFAULT_CLIENT_PID_FILENAME);

	/* set logfilename to default */
	client_logfile_name = Malloc (1 + strlen(DEFAULT_CLIENT_LOGFILE));
	strcpy (client_logfile_name, DEFAULT_CLIENT_LOGFILE);

   /* allocate timeout structure and set to default value */
   rw_timeout = Malloc (sizeof (struct timeval));
   rw_timeout->tv_sec = DEFAULT_RW_TIMEOUT / 1000;
   rw_timeout->tv_usec = (DEFAULT_RW_TIMEOUT % 1000) * 1000;

   /* allocate server descriptors array & directory array */
   if (server_num_desc > 0)
      server_desc_array = Malloc (sizeof (server_desc) * server_num_desc);
   if (directory_num > 0)
      directory_array = Malloc (sizeof (char *) * directory_num);

   i = 0;	/* point to current server descriptor, if any */
   j = 0;	/* point to current directory, if any */
   p = list;	/* point to current element in the parsing list */

   while (p) {
      /* server descriptor */
      if (!strcmp (p->command, "server") && p->argc == 4) {
	 int n;			/* minimum between max lengths */

	 n = (MAX_NICK_LENGTH > MAX_PARSING_COMMAND_ARGUMENT) ?
	    MAX_PARSING_COMMAND_ARGUMENT : MAX_NICK_LENGTH;

	 /* copy nick */
	 strncpy (server_desc_array[i].nick, p->argv[0], n);

	 /* force null termination */
	 server_desc_array[i].nick[n] = '\0';

	 /* copy address (family, port, ipv4 address) */
	 server_desc_array[i].addr.sin_family = AF_INET;
	 server_desc_array[i].addr.sin_port = htons (atoi (p->argv[2]));

	 n = inet_aton (p->argv[1], &server_desc_array[i].addr.sin_addr);
	 if (n == 0)
	    goto parse_client_error;

	 /* get timeout period (seconds) */
	 if (atoi (p->argv[3]) <= 0)
	    goto parse_client_error;
	 else
	    server_desc_array[i].timeout_period = atoi (p->argv[3]);

	 ++i;
      }

      /* directory */
      else if (!strcmp (p->command, "directory") && p->argc == 1) {
	 directory_array[j] = Malloc (strlen (p->argv[0]) + 1);
	 strcpy (directory_array[j], p->argv[0]);
	 ++j;
      }

      /* expire time (sec) */
      else if (!strcmp (p->command, "expire_time") && p->argc == 1) {
	 if (atoi (p->argv[0]) < 0)
	    goto parse_client_error;
	 status_expire_time = atoi (p->argv[0]);
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
			free (client_pid_filename);
			client_pid_filename = Malloc (1 + strlen(p->argv[0]));
			strcpy (client_pid_filename, p->argv[0]);
		}

		/* logfile name */
		else if (!strcmp (p->command, "logfile") && p->argc == 1) {
			free (client_logfile_name);
			client_logfile_name = Malloc (1 + strlen(p->argv[0]));
			strcpy (client_logfile_name, p->argv[0]);
		}

		/* unprivileged ports monitoring */
		else if (!strcmp (p->command, "unpriv_port") && p->argc == 1) {
			if (!strcmp (p->argv[0], "on")) {
				set_range (1, 65535);
				unpriv_port = 1;
			}
			else if (!strcmp (p->argv[0], "off")) {
				set_range (1, 1023);
				unpriv_port = 0;
			}
			else {
				if (DEFAULT_UNPRIV_PORT == 1) set_range (1, 65535);
				else set_range (1, 1023);
				unpriv_port = DEFAULT_UNPRIV_PORT;
         }				
		}

      p = p->next;
   }

   /* destroy parsing list */
   parse_destroy_list (list);

   /* update parsing time */
   client_parse_time = time (NULL);

   return 1;

 parse_client_error:
   return -1;
}

int
print_client_conf (FILE * f)
{
   int i;

   fprintf (f, "CLIENT CONFIGURATION\n");
   fprintf (f, "parsed at %.24s\n", ctime (&client_parse_time));
	fprintf (f, "pid filename %s\n", client_pid_filename);
   fprintf (f, "status expire time %d\n", (int) status_expire_time);
	fprintf (f, "logfile %s\n", client_logfile_name);
	if (unpriv_port)
		fprintf (f, "monitor unprivileged ports on\n");
	else
		fprintf (f, "monitor unprivileged ports off\n");
   fprintf (f, "read/write timeout: %f\n",
	    rw_timeout->tv_sec + (float) rw_timeout->tv_usec / 1000000.0);
   fprintf (f, "servers\n");
   for (i = 0; i < server_num_desc; i++)
      fprintf (f, "\t%s at %s:%d [%d sec]\n",
	       server_desc_array[i].nick,
	       inet_ntoa (server_desc_array[i].addr.sin_addr),
	       ntohs (server_desc_array[i].addr.sin_port),
	       (int) server_desc_array[i].timeout_period);
   fprintf (f, "directories\n");
   for (i = 0; i < directory_num; i++)
      fprintf (f, "\t%s\n", directory_array[i]);
   fprintf (f, "\n");

   return 1;
}

int
reset_client ()
{
   if (server_desc_array) {
      free (server_desc_array);
      server_desc_array = NULL;
      server_num_desc = 0;
   }

   if (directory_array) {
      free (directory_array);
      directory_array = NULL;
      directory_num = 0;
   }

   if (rw_timeout) {
      free (rw_timeout);
      rw_timeout = NULL;
   }

	if (client_pid_filename) {
		free (client_pid_filename);
		client_pid_filename = NULL;
	}

	if (client_logfile_name) {
		free (client_logfile_name);
		client_logfile_name = NULL;
	}

   status_expire_time = DEFAULT_STATUS_EXPIRE_TIME;
	unpriv_port = DEFAULT_UNPRIV_PORT;

   client_parse_time = time (NULL);

	/* close logfile */
	if (client_logfile_fd >= 0)
		close (client_logfile_fd);
	
   return 1;
}

void
print_usage ()
{
   printf (PROGRAM_NAME " version " VERSION_MAJOR "." VERSION_MINOR "\n"
	   "usage: " CLIENT_PROGRAM_NAME " [-hvF] [-c conf]\n"
	   "   -c conf    use file conf as configuration file\n"
	   "   -F         run in foreground\n"
	   "   -v         verbose (useful only if -F)\n"
	   "   -h         print this help\n"
		"see man "CLIENT_PROGRAM_NAME" (8)\n");
}

/**************************************************************/
/*                      Threads section                       */
/**************************************************************/

int
build_status ()
{
   void *buf, *files_buf_temp;
   int len, i, files_len_temp, current;
   int ret;

   MOREVERBOSE ("Going to build client status [%ld]", pthread_self ());

   /* cleans up previously allocated buffers */
   if (client_ports_buf)
      free (client_ports_buf);
   if (client_kernel_buf)
      free (client_kernel_buf);
   if (client_files_buf)
      free (client_files_buf);

   MOREVERBOSE ("Loading modules [%ld]", pthread_self ());
   ret = getLoadedModules (&client_kernel_buf, &client_kernel_len);
   if (ret < 0)
      goto error_build;
   buf = NULL;
   ret = getKernelStatus (&buf, &len);
   if (ret < 0)
      goto error_build;
   client_kernel_buf = realloc (client_kernel_buf, client_kernel_len + len);
   memcpy (client_kernel_buf + client_kernel_len, buf, len);
   if (buf)
      free (buf);
   client_kernel_len += len;

   MOREVERBOSE ("Loading ports [%ld]", pthread_self ());
   ret = getOpenTCP ((open_port **) & client_ports_buf, &client_ports_len);
   if (ret < 0)
      goto error_build;
   buf = NULL;
   ret = getOpenUDP ((open_port **) & buf, &len);
   if (ret < 0)
      goto error_build;
   client_ports_buf = realloc (client_ports_buf, client_ports_len + len);
   memcpy (client_ports_buf + client_ports_len, buf, len);
   if (buf)
      free (buf);
   client_ports_len += len;

   MOREVERBOSE ("Loading files [%ld]", pthread_self ());

   client_files_len = 0;
   client_files_buf = NULL;
   files_len_temp = 0;
   files_buf_temp = NULL;
   current = 0;
/* For each directory entry...
 */
   for (i = 0; i < directory_num; i++) {
      MOREVERBOSE ("Building directory %s, %d", directory_array[i], i);

      /* We initialize the fstree structure. It must be done for every directory
         entry.
       */
      fstree_init ();
      MOREVERBOSE ("Fstree initialized");
      /* Then we build the structure for each directory entry.
       */
      getPathCRC (directory_array[i]);
      MOREVERBOSE ("Fstree built");
      fstree_print_file ();
		fstree_print_dir ();
		fflush (stdout);
      /* Here we get the binary data for each directory entry.
       */
      ret = getFiles (&files_buf_temp, &files_len_temp);
      MOREVERBOSE ("Binary data build. Return value %d", ret);
      MOREVERBOSE ("The new block data is %p %d", files_buf_temp,
		   files_len_temp);

      /* Before building another fstree, it must be destroyed.
       */
      fstree_destroy ();
      MOREVERBOSE ("Fstree destroyed");
      if (ret < 0)
	 goto error_build;

      /* Now the client_files_buf is enlarged, so it can contain the new
         directory data.
       */
      client_files_buf = realloc (client_files_buf, current + files_len_temp);

      /* Now we append the new data to the enlarged buffer. Note that at this
         point "current" does not indicate the "client_files_buf" size, but
         its old size. So current now points at the first free buffer location.
       */
      memcpy (client_files_buf + current, files_buf_temp, files_len_temp);

      /* "current" is the current buffer size.
       */
      MOREVERBOSE ("Current pointer at %d", current);
      current += files_len_temp;
      free (files_buf_temp);
   }

   MOREVERBOSE ("Success on building client status [%ld]", pthread_self ());
   client_files_len = current;
   return 1;

 error_build:
   MOREVERBOSE ("Error on building client status [%ld]", pthread_self ());
   if (client_ports_buf) {
      free (client_ports_buf);
      client_ports_buf = NULL;
   }
   if (client_kernel_buf) {
      free (client_kernel_buf);
      client_kernel_buf = NULL;
   }
   if (client_files_buf) {
      free (client_files_buf);
      client_files_buf = NULL;
   }
   if (buf)
      free (buf);
   return -1;
}

int
create_status ()
{
   time_t now;
   int ret;

   now = time (0);
	/* force return value to be ok if we did not create status */
	ret = 1;
   if ( (now - last_status_built) > status_expire_time) {
      MOREVERBOSE ("write_mutex locked [%ld]", pthread_self ());
      pthread_mutex_lock (&write_mutex);
      now = time (0);
      if ( (now - last_status_built) > status_expire_time) {
	 VERBOSE ("real build of status [%ld]", pthread_self ());
	 ret = build_status ();
	 last_status_built = time (0);
	 if (ret > 0) printlog ("status created", client_logfile_fd);
	 else printlog ("error on status creation", client_logfile_fd);
      }
      pthread_mutex_unlock (&write_mutex);
      MOREVERBOSE ("write_mutex unlocked [%ld]", pthread_self ());
   }
   MOREVERBOSE ("The status creation returned %d", ret);
   return ret;
}

void *
client_thread (void *v)
{
#ifdef CLIENT_DEBUG
   FILE* f;
	char* filename;
#endif

	/* pointer to server descriptor */
   server_desc *desc;
	int id;

	/* buffer containing packet header */
   header header_buf;

	/* write buffer (such as configuration packet one) */
   char buffer[BUFFER_SIZE];

	/* useful pointers */
   char *temp, *mod, *exp;

	/* public key */
   RSA *rsa;

	/* digest of packet sent to server */
   char digest[MD5_DIGEST_LENGTH + SHA_DIGEST_LENGTH];

	/* printlog buffer */
	char printlog_buf[MAX_LOGFILE_LINE_LENGTH-33];

	/* type of packet sent to server */
   int type;

	/* client sent configuration packet to server */
	int conf_sent;

	/* useful indices and sizes */
   int size, mod_size, exp_size, i, n, first_cycle;

	/* return value */
   int ret;

	/* integers variables to compute maximum between rw_timeout
		system wide available from client and server one */
   int x, y;

	/* buffer to accomodate ack from server */
   ack ack_buf;

	/* real socket timeout, maximum between x and y */
   struct timeval *socket_timeout;

	/* number and names of status packet that client must send to server */
   int status_size[NUM_STATUS];
   void *status_pnt[NUM_STATUS];

	/* actual packet to be sent to server, between 0 and NUM_STATUS-1 */
   int which_packet;

	/* flag that indicate if client has to stop */
	int error;

   desc = (server_desc *) v;
/* This is the server_desc_array[] index.
 */
	id = desc - server_desc_array;

   desc->status = CLIENT_INIT;
   VERBOSE ("status INIT %s [%ld]", desc->nick, pthread_self ());
	snprintf (printlog_buf, sizeof(printlog_buf),
			"thread %s start", desc->nick);
	printlog (printlog_buf, client_logfile_fd);

   /* zeroing buffers */
   bzero (&header_buf, sizeof (header_buf));
   bzero (&buffer, BUFFER_SIZE);
   socket_timeout = NULL;
	error = C_OK;
   sem_init (&desc->sync, 0, 0);
	conf_sent = 0;

   /* defining useful aliases */
   temp = buffer;

   /* generate key */
   rsa = generate_key ();
   mod_size = get_modulus (rsa, &mod);
   exp_size = get_pub_exp (rsa, &exp);
   /* put packet type (configuration) into send buffer */
   type = PKT_CONF;
   memcpy (temp, &type, sizeof (type));
   temp += sizeof (type);

   /* put timeout period */
   memcpy (temp, &desc->timeout_period, sizeof (desc->timeout_period));
   temp += sizeof (desc->timeout_period);

   /* put socket timeout */
   memcpy (temp, rw_timeout, sizeof (struct timeval));
   temp += sizeof (struct timeval);

   /* put modulus size */
   memcpy (temp, &mod_size, sizeof (mod_size));
   temp += sizeof (mod_size);

   /* put public modulus */
   memcpy (temp, mod, mod_size);
   temp += mod_size;

   /* put exponent size */
   memcpy (temp, &exp_size, sizeof (exp_size));
   temp += sizeof (exp_size);

   /* put public exponent */
   memcpy (temp, exp, exp_size);
   temp += exp_size;
   size = temp - buffer;

	first_cycle = 1;
	/* forever cycle if everything goes fine */
	while (1) {

		if ( !client_has_to_quit ) {

         if (!error) printlog ("mark", client_logfile_fd);
	      else if (error == C_CONN_ERR)
				printlog ("connection error", client_logfile_fd);
			else printlog ("error", client_logfile_fd);

			if ( !first_cycle ) {
				desc->status = CLIENT_WAIT;
				VERBOSE ("status CLIENT_WAIT %s", desc->nick);

				pthread_mutex_lock (&timer_mutex);
				tq_push (&timer_queue, desc->timeout_period, id);
				pthread_mutex_unlock (&timer_mutex);

				/* Wait for timer synchronization */
				sem_wait (&desc->sync);
			}

			first_cycle = 0;

			if (!client_has_to_quit && create_status () <= 0) {
				DIE ("%s: error on status building", desc->nick);
			}

			/* connect to server */
			MOREVERBOSE ("Creating socket %s", desc->nick);
			desc->socket = socket (AF_INET, SOCK_STREAM, 0);
			if (desc->socket < 0) {
				DIE ("%s: error creating socket", desc->nick);
			}

			MOREVERBOSE ("Connecting to server %s", desc->nick);
			if (connect (desc->socket, (struct sockaddr *) &desc->addr,
				sizeof (desc->addr)) < 0) {
					VERBOSE ("error connecting to %s on socket %d (%s)", desc->nick,
					desc->socket, inet_ntoa (desc->addr.sin_addr));
					close (desc->socket);
					error = C_CONN_ERR;
					continue;
			}
			error = C_OK;

			/* set desc->socket nonblocking */
			fcntl (desc->socket, F_SETFL, O_NONBLOCK);

			/* wait for server ack + socket timeout */
			memset (&ack_buf, 0, sizeof (ack_buf));
			MOREVERBOSE ("Waiting for server ack %s", desc->nick);
			ret = Read (desc->socket, &ack_buf, sizeof (ack_buf), socket_timeout);
			if (ret < sizeof (ack_buf)) {
				VERBOSE ("no acknowledgment from server %s", desc->nick);
				error = C_ACK_ERR;
				close (desc->socket);
				continue;
			}

		} /* if ( !client_has_to_quit ) */

      if (client_has_to_quit) {
         switch (client_has_to_quit) {
            case QUIT_SHUTDOWN:
               header_buf.type = POWER_DOWN;
               header_buf.length = 0;
               break;
            case QUIT_UPDATE:
               header_buf.type = UPDATE;
               header_buf.length = 0;
               break;
            default:
               /* For future developement. */
               break;
         }

      /* The POWER_DOWN packet has no payload, so we (sigh) calculate
         the MD5/SHA1 for the type.
       */
         if (get_digest ((char*)&header_buf.type, sizeof (header_buf.type),
               digest, digest + MD5_DIGEST_LENGTH) < 0) {
            DIE ("Error on creating MD5/SHA1 digest on %s", desc->nick);
         }
         if (RSA_private_encrypt
               (MD5_DIGEST_LENGTH + SHA_DIGEST_LENGTH, digest,
               header_buf.digest, rsa, RSA_PKCS1_PADDING) < 0) {
            DIE ("Error %s on crypting MD5/SHA1 digest on %s",
                 ERR_error_string (ERR_get_error (), NULL), desc->nick);
         }
         if (Write (desc->socket, &header_buf,
               sizeof (header_buf), socket_timeout) < 0) {
            MOREVERBOSE ("error writing on %s", desc->nick);
            error = C_WRITE_ERR;
            break;
         }

         ret = Read (desc->socket, &ack_buf, sizeof (ack_buf), socket_timeout);
         if (ret < 0 || ack_buf.type != SERVER_OK) {
            MOREVERBOSE ("error reading on %s", desc->nick);
            error = C_READ_ERR;
         }
         if (!error) {
            switch (client_has_to_quit) {
               case QUIT_SHUTDOWN:
                  VERBOSE ("POWER DOWN %s", desc->nick);
                  break;
               case QUIT_UPDATE:
                  VERBOSE ("UPDATE %s", desc->nick);
                  break;
               default:
                  /* For future developement. */
				  ;
            }
         }
         else {
				printf ("error %d\n", error);
				fflush (stdout);	
            switch (client_has_to_quit) {
               case QUIT_SHUTDOWN:
                  VERBOSE ("error on POWER DOWN %s", desc->nick);
                  break;
               case QUIT_UPDATE:
                  VERBOSE ("error on UPDATE %s", desc->nick);
                  break;
               default:
                  /* For future developement. */
				  ;
            }
         }
         close (desc->socket);
         break;
      } /* if (client_has_to_quit) */

		if (ack_buf.type != SERVER_CARRIES_TIMEOUT &&
				ack_buf.type != SERVER_OK) {
			VERBOSE ("bad acknoledgment from server %s", desc->nick);
			error = C_ACK_ERR;
			close (desc->socket);
			continue;
		}

		/* get server socket timeout */
		memcpy (&desc->rw_timeout, &ack_buf.payload.sock_timeout,
			sizeof (struct timeval));

		/* setting *real* socket timeout to maximum value between
			newly got and global one */
		if (socket_timeout)
			free (socket_timeout);
		socket_timeout = Malloc (sizeof (struct timeval));
		x = desc->rw_timeout.tv_sec * 1000000 + desc->rw_timeout.tv_usec;
		y = rw_timeout->tv_sec * 1000000 + rw_timeout->tv_usec;
		if (x > y)
			memcpy (socket_timeout, &desc->rw_timeout, sizeof (struct timeval));
		else
			memcpy (socket_timeout, rw_timeout, sizeof (struct timeval));

		if ( !conf_sent ) {
			desc->status = CLIENT_SEND_CONF;
			VERBOSE ("status SEND_CONF %s", desc->nick);

			/* send configuration packet */
			MOREVERBOSE ("Sending configuration packet %s", desc->nick);
			ret = Write (desc->socket, buffer, size, socket_timeout);
			PRINTDEBUG ("%d bytes sent on %s", ret, desc->nick);
			if (ret < 0) {
				VERBOSE ("server %s not responding", desc->nick);
				error = C_WRITE_ERR;
				close (desc->socket);
				continue;
			}

			/* wait for server ack */
			MOREVERBOSE ("Waiting for server ack %s", desc->nick);
			ret = Read (desc->socket, &ack_buf, sizeof (ack_buf), socket_timeout);
			if (ret < sizeof (ack_buf)) {
				VERBOSE ("no acknowledgment from server %s", desc->nick);
				error = C_ACK_ERR;
				close (desc->socket);
				continue;
			}
			if (ack_buf.type != SERVER_OK) {
				VERBOSE ("bad acknoledgment from server %s", desc->nick);
				error = C_ACK_ERR;
				close (desc->socket);
				continue;
			}
			
			if (!error)
				printlog ("server acknowledged configuration", client_logfile_fd);
			else printlog ("server rejected configuration ", client_logfile_fd);

         conf_sent = 1;
		} /* if ( !conf_sent ) */

		/* Critical section PROLOGUE */
		pthread_mutex_lock (&read_mutex);
		num_readers++;
		if (num_readers == 1)
			pthread_mutex_lock (&write_mutex);
		pthread_mutex_unlock (&read_mutex);

      status_pnt[0] = client_ports_buf;
      status_pnt[1] = client_kernel_buf;
      status_pnt[2] = client_files_buf;
      status_size[0] = client_ports_len;
      status_size[1] = client_kernel_len;
      status_size[2] = client_files_len;
      which_packet = 0;

      while (which_packet < NUM_STATUS) {
	 /* status send */
	 desc->status = CLIENT_SEND_STATUS;
	 VERBOSE ("status SEND_STATUS%s %s", status_string[which_packet],
		  desc->nick);

	 /* Here we get the packet type we have to send */
	 MOREVERBOSE ("header_buf.type = %d", status_type[which_packet]);
	 header_buf.type = status_type[which_packet];

	 /* Now we get the packet size */
	 MOREVERBOSE ("header_buf_len = %d", status_size[which_packet]);
	 header_buf.length = status_size[which_packet];

	 /* And now, for that packet, we calculate its MD5/SHA1 digest message */
	 if (get_digest (status_pnt[which_packet], status_size[which_packet],
			 digest, digest + MD5_DIGEST_LENGTH) < 0) {
	    DIE ("Error on creating MD5/SHA1 digest on %s", desc->nick);
	 }

	 /* Now it's time to crypt the digest message */
	 
	 if (RSA_private_encrypt
	     (MD5_DIGEST_LENGTH + SHA_DIGEST_LENGTH, digest,
	      header_buf.digest, rsa, RSA_PKCS1_PADDING) < 0) {
	    DIE ("Error %s on crypting MD5/SHA1 digest on %s",
		 ERR_error_string (ERR_get_error (), NULL), desc->nick);

	 }
	 
	 /* Let's send to the server the packet header */
	 if (Write (desc->socket, &header_buf,
		    sizeof (header_buf), socket_timeout) < 0) {
	    VERBOSE ("error writing on %s", desc->nick);
		 error = C_WRITE_ERR;
		 break;
	 }

	 temp = status_pnt[which_packet];
	 n = status_size[which_packet];
#ifdef CLIENT_DEBUG
	 filename = (char *) Malloc (strlen (desc->nick) +
				     strlen (status_string[which_packet]) +
				     strlen ("_temp") + 1);
	 strcpy (filename, desc->nick);
	 strcat (filename, status_string[which_packet]);
	 strcat (filename, "_temp");
	 f = fopen (filename, "w");
	 free (filename);
#endif

	 while (1) {
	    i = (n < BUFFER_SIZE) ? n : BUFFER_SIZE;
	    i = Write (desc->socket, temp, i, socket_timeout);
#ifdef CLIENT_DEBUG
	    fwrite (temp, 1, i, f);
#endif
	    if (i < 0) {
	       VERBOSE ("error sending packet to %s", desc->nick);
			 error = C_WRITE_ERR;
			 break;
	    }
	    n -= i;
	    temp += i;
	    if (n == 0)
	       break;
	 }
#ifdef CLIENT_DEBUG
	 fclose (f);
#endif
	 if (error) break;
#ifdef CLIENT_DEBUG
	 MOREVERBOSE ("Local file correctly written on %s", desc->nick);
#endif

	 /* wait for server ack */
	 ret = Read (desc->socket, &ack_buf, sizeof (ack_buf),
		     socket_timeout);
	 if (ret < sizeof (ack_buf)) {
	    VERBOSE ("error getting ACK message from %s", desc->nick);
		 error = C_ACK_ERR;
		 break;
	 }
	 if (ack_buf.type == SERVER_OK) {
	    MOREVERBOSE ("ACK message received from %s", desc->nick);
	 }
	 else if (ack_buf.type == SERVER_NAK) {
	    VERBOSE ("warning: NAK message received from %s", desc->nick);
		 syslog (LOG_WARNING, "status failed %s", desc->nick);
		 error = C_NAK;
		 break;
	 }
	 else {
	    VERBOSE ("error getting ACK message from %s", desc->nick);
		 error = C_ACK_ERR;
		 break;
	 }
	 which_packet++;
      } /* while (which_packet < NUM_STATUS) */

    /* Critical section EPILOGUE */
    pthread_mutex_lock (&read_mutex);
    num_readers--;
    if (num_readers == 0)
       pthread_mutex_unlock (&write_mutex);
    pthread_mutex_unlock (&read_mutex);

	 close (desc->socket);

   }	/* while (1) */

	/* if we exited from previous forever cycle, something bad happened,
		so we should clean up previously allocated resource and quit */
	free (socket_timeout);
	RSA_free (rsa);
   MOREVERBOSE ("%s dies", desc->nick);
   return NULL;
}

/*
	thread that weaks up sleeping client threads
	this function should never return
 */
void *
timer_thread (void *p)
{
	int i;
   while (timer_forever) {
		sleep (1);
		pthread_mutex_lock (&timer_mutex);
		tq_tick (&timer_queue);
		while ( tq_test ( &timer_queue ) == 1 ) {
			i = tq_pop ( &timer_queue );
			sem_post (&server_desc_array [i].sync);
		}
		pthread_mutex_unlock (&timer_mutex);
   }
   return NULL;
}

void
start_threads (void)
{
   unsigned int i;

	sem_init (&sigquit_caught, 0, 0);
	sem_init (&sigterm_caught, 0, 0);
	
	tq_init (&timer_queue);

   syslog (LOG_INFO, CLIENT_PROGRAM_NAME" started");
	
   /* open logfile */
   client_logfile_fd = open (client_logfile_name,
         O_WRONLY | O_CREAT | O_APPEND, 0644);

   if (client_logfile_fd < 0)
      syslog (LOG_WARNING, "could not open log file");

	printlog ("start", client_logfile_fd);

   for (i = 0; i < server_num_desc; i++)
      pthread_create (&server_desc_array[i].tid, NULL, client_thread,
		      &server_desc_array[i]);

	/* create sigquit thread */
	pthread_create (&sigquit_tid, NULL, sigquit_real_handler, NULL);

	/* create sigterm thread */
	pthread_create (&sigterm_tid, NULL, sigterm_real_handler, NULL);
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
	 client_conf_filename = Malloc (strlen (optarg));
	 strcpy (client_conf_filename, optarg);
	 break;
      default:
	 print_usage ();
	 exit (1);
      }
   }

   argc -= optind;
   argv += optind;

   /* parse configuration file */
	ret = parse_client ();

   /* check for runaway arguments, if any exit */
   if (argc > 0) {
      print_usage ();
		exit (1);
   }

	if ( ret < 0 ) {
		printf ("configuration parse error\n");
		exit (1);
	}

	if ( !strcmp (program_name, CLIENT_PROGRAM_TESTCONF) ) {
		if ( ret >= 0 )
			print_client_conf (stdout);
		exit (0);
	}
	else if ( !strcmp (program_name, CLIENT_PROGRAM_QUIT) ) {
		conffile_fd = open (client_pid_filename, O_RDONLY);
		if (conffile_fd < 0) {
			printf (CLIENT_PROGRAM_NAME" is not running\n");
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
	else if ( !strcmp (program_name, CLIENT_PROGRAM_TERM) ) {
      conffile_fd = open (client_pid_filename, O_RDONLY);
      if (conffile_fd < 0) {
         printf (CLIENT_PROGRAM_NAME" is not running\n");
         exit (1);
      }
      n = read (conffile_fd, &p, sizeof(p));
      if (n < sizeof(p)) {
         printf ("bad pid file\n");
         exit (1);
      }
		close (conffile_fd);
      kill (p, SIGTERM);
      exit (0);
	}
	else if ( strcmp (program_name, CLIENT_PROGRAM_NAME) ) {
		printf ("bad program name\n");
		exit (1);
	}

	conffile_fd = open (client_pid_filename, O_RDONLY);
	if ( conffile_fd >= 0 ) {
		printf (CLIENT_PROGRAM_NAME" is already running\n");
		exit (1);
	}
	close (conffile_fd);

	conffile_fd = open (client_pid_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	p = getpid ();
	n = write (conffile_fd, &p, sizeof(p));
	if (n < sizeof(p)) {
		printf ("could not write to pid file\n");
		exit (1);
	}
	close (conffile_fd);

   /* capture signals */
   signal (SIGHUP, SIG_IGN);
   signal (SIGQUIT, sigquit_handler);
	signal (SIGTERM, sigterm_handler);
   signal (SIGINT, SIG_IGN);

	/* run in background */
   if (!foreground) {
		daemon (0, 0);

		/* overwriting old value of process id stored in pid file */
		p = getpid ();
		conffile_fd = open (client_pid_filename,
				O_WRONLY | O_CREAT | O_TRUNC, 0644);
		write (conffile_fd, &p, sizeof(p));
		close (conffile_fd);
	}

   srand (time (0) ^ p);

	openlog (CLIENT_PROGRAM_NAME, LOG_PID, LOG_DAEMON);

   /* create timer thread */
   pthread_create (&timer_tid, NULL, timer_thread, NULL);

   start_threads ();

   pthread_join (timer_tid, NULL);

	unlink (client_pid_filename);

	printlog ("quit", client_logfile_fd);

   exit (0);
}

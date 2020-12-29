/*
	Dite. Distributed linux intrusion detection system.
   Copyright (C) 2002 Bonasorte M., Cicconetti C.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef __SERVER__H

#include "common.h"
#include "parse.h"
#include "rsa.h"
#include "ssl.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_NICK_LENGTH 16
#define DEFAULT_SERVER_PORT 1001
/* #define DEFAULT_SERVER_CONF_FILE_NAME "/etc/cocito.conf" */
/* #define DEFAULT_SERVER_PID_FILENAME "/var/run/cocito.pid" */
#define MAX_SERVER_CONF_LINE_LENGTH 1024
/* #define DEFAULT_SERVER_PREFIX "/var/log/cocito" */
/* #define DEFAULT_SERVER_CONF_PREFIX "/var/log/cocito" */
/* #define DEFAULT_SERVER_LOGFILE "/var/log/cocito.log" */
#define CONF_POSTFIX ".cnf"
#define DEFAULT_RW_TIMEOUT 10000
#define MAX_STRING_ERR_LEN 64

/* List of possible server status */
#define INIT            0x00000000
#define LISTEN_RSA      0x00000001
#define LISTEN          0x00000002
#define KILL            0x00000004
#define ALERT           0x00000008
#define SEND_OK         0x00000010
#define WAIT_FOR_CONF   0x00000020
#define WAIT_FOR_PORT   0x00000040
/* End of client status list */

/* List of internal server error codes */
#define S_OK            0x00000000
#define S_READ_ERR      0x00000001
#define S_DIFF_SIZE     0x00000002
#define S_CMP_ERR       0x00000004
#define S_WRITE_ERR     0x00000008
#define S_SHORT_DATA    0x00000010
#define S_TOO_LARGE     0x00000020
#define S_DIG_SIZE_ERR  0x00000040
#define S_DIG_CMP_ERR   0x00000080
#define S_ERR_ACK       0x00000100
#define S_FILE_TOO_LRG  0x00000200
#define S_ERR_NAK       0x00000400
/* End of internal server error codes */

typedef struct {

	/* Connection related variables */
   struct in_addr addr;
   int socket;
   char nick[MAX_NICK_LENGTH];

	/* Internal status variables */
   unsigned long int status;
   int got_conf;
   int got_status;
	pthread_t tid;

	/* Timer variables */
   int active_timer;
   time_t period;
   struct timeval rw_timeout;
	struct timeval* socket_timeout;

	/* Synchronization variables */
   sem_t sync;

	/* Other stuff */
	RSA* rsa;
} client_desc;

/*
	Parse server configuration file name (one pass parse)
	Allocate server data structures memory
   Warning: file name is stored in a static pointer global available,
	   if pointer is NULL then default value is used
   BE CAREFUL: parse_server DOES NOT deallocate memory occupied by
	   previousely allocated server data structures, it is up to you
		to free up memory BEFORE you call this function
 */
int parse_server ();

/*
   Reset server configuration to before parsing state
 */
int reset_server ();

/*
	Print server configuration informations in a human readable format
 */
int print_server_conf (FILE * f);

/*
	Print to stdout usage informations
 */
void print_usage ();

#define __SERVER__H
#endif

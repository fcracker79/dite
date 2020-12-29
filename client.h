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

#ifndef __CLIENT__H

#include "common.h"
#include "parse.h"
#include "net.h"
#include "kernel.h"
#include "files.h"
#include "comms.h"
#include "rsa.h"
#include "ssl.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <pthread.h>

/* #define CLIENT_DEBUG */

#define MAX_NICK_LENGTH 16
/* #define DEFAULT_CLIENT_CONF_NAME "/etc/dite.conf" */
/* #define DEFAULT_CLIENT_PID_FILENAME "/var/run/dite.pid" */
/* #define DEFAULT_CLIENT_LOGFILE "/var/log/dite.log" */
#define MAX_CLIENT_CONF_LINE_LENGTH 1024
#define DEFAULT_STATUS_EXPIRE_TIME 600
#define DEFAULT_RW_TIMEOUT 10000
#define DEFAULT_UNPRIV_PORT 1

/* List of possible client status */
#define CLIENT_INIT         0x00000000
#define CLIENT_SEND_CONF    0x00000001
#define CLIENT_SEND_STATUS  0x00000002
#define CLIENT_CONNECT      0x00000004
#define CLIENT_STATUS_BUILD 0x00000008
#define CLIENT_ALERT        0x00000010
#define CLIENT_WAIT         0x00000020
#define CLIENT_UPDATE       0x00000040
#define CLIENT_SHUTDOWN     0x00000080
/* End of list of client status */

/* Internal client error codes */
#define C_OK                0x00000000
#define C_ACK_ERR           0x00000001
#define C_WRITE_ERR         0x00000002
#define C_READ_ERR          0x00000004
#define C_NAK               0x00000008
#define C_CONN_ERR          0x00000010
/* End of internal client error codes */

/* magic tag to send if no status to be send from client to server */
#define MAGIC_STATUS "ok"

typedef struct {
   struct sockaddr_in addr;
   int socket;
   char nick[MAX_NICK_LENGTH];
   unsigned long int status;
   pthread_t tid;
   time_t timeout_period;
   struct timeval rw_timeout;
	sem_t sync;
} server_desc;

/*
   Parse client configuration file name (one pass parse)
   Allocate client data structures memory
   Warning: file name is stored in a static pointer global available,
      if pointer is NULL then default value is used
   BE CAREFUL: parse_server DOES NOT deallocate memory occupied by
      previousely allocated client data structures, it is up to you
      to free up memory BEFORE you call this function
 */
int parse_client ();

/*
	Reset client configuration to before parsing state
 */
int reset_client ();

/*
   Print client configuration informations in a human readable format
 */
int print_client_conf (FILE * f);

/*
   Print to stdout usage informations
 */
void print_usage ();

#define __CLIENT__H
#endif

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

#ifndef __COMMON__H

/* start of configuration section */
#define SERVER_PROGRAM_NAME "cocito"
#define CLIENT_PROGRAM_NAME "dite"
#define PROGRAM_NAME    "Dite"
#define VERSION_MAJOR   "0"
#define VERSION_MINOR   "1c"

#define SERVER_PROGRAM_HUP      "cocito_hup"
#define SERVER_PROGRAM_QUIT     "cocito_quit"
#define SERVER_PROGRAM_TESTCONF "cocito_testconf"
#define CLIENT_PROGRAM_QUIT     "dite_quit"
#define CLIENT_PROGRAM_TERM     "dite_update"
#define CLIENT_PROGRAM_TESTCONF "dite_testconf"
/* end of configuration section */

#define NUM_BITS 2048
#define PUB_EXP 65535

/* #define DEBUG 1 */
#define moreverbose 0

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <semaphore.h>
#include <syslog.h>
#include <pthread.h>
#include <time.h>
#include "algo.h"
#include "time_queue.h"

/* Packet types */
#define CONF            0x00000001
#define POWER_DOWN      0x00000002
#define UPDATE          0x00000004

#define STATUS_PORT     0x00000010
#define STATUS_KERNEL   0x00000020
#define STATUS_FILES    0x00000040

/* List of server acks */
#define SERVER_OK      	0x00000001
#define SERVER_ERROR    0x00000002
#define SERVER_CARRIES_TIMEOUT	0x00000004
#define SERVER_NAK      0x00000008

/* client/server global defines */
#define MAX_PORT_NUMBER 65535
#define MAX_LOGFILE_LINE_LENGTH 80
#define BUFFER_SIZE sizeof (header) * 4 + 1

#ifdef DEBUG
#define PRINTDEBUG(args...) fprintf(stderr, args); fflush (stderr)
#else
#define PRINTDEBUG(args...)
#endif

#define VERBOSE(args...) if (verbose) { \
	time_t now; \
	now = time(0); \
   printf ("%.24s ", ctime (&now)); \
   printf (args); \
	putchar ('\n'); \
	fflush (stdout); \
   }

#define MOREVERBOSE(args...) if (moreverbose) { \
	time_t now; \
	now = time(0); \
	printf ("%.24s ", ctime (&now)); \
   printf (args); \
   putchar ('\n'); \
   fflush (stdout); \
   }

#define DIE(args...) printf(args); \
   putchar ('\n'); \
	fflush (stdout); \
	sleep (1); \
	exit (1);

/*
	some general purpose wrapped functions
 */

#define READ_CLOSECONN      0
#define READ_TIMEOUT		   -1
#define READ_ERROR         -2
#define READ_SELECT_ERROR  -3
#define WRITE_CLOSECONN     0
#define WRITE_TIMEOUT      -1
#define WRITE_ERROR        -2
#define WRITE_SELECT_ERROR -3

/*
	malloc & bzero
	return value: on success a pointer to the newly allocated memory
	              exit(1) if malloc did not success
 */
void *Malloc (size_t size);

/*
	select & read
	return value: on success =>  the number of bytes read
	              if timeout expires => READ_TIMEOUT
					  if our partner closes connection => READ_CLOSECONN
					  if a read error occurs => READ_ERROR
					  if a select error occurs => READ_SELECT_ERROR
   Read does never modifies timeout value
 */
ssize_t Read (int fd, void *buf, size_t count, struct timeval *timeout);

/*
   select & write
   return value: on success =>  the number of bytes written
                 if timeout expires => WRITE_TIMEOUT
                 if our partner closes connection => WRITE_CLOSECONN
                 if a read error occurs => WRITE_ERROR
                 if a select error occurs => WRITE_SELECT_ERROR
   Write does never modifies timeout value
 */
ssize_t Write (int fd, void *buf, size_t count, struct timeval *timeout);

/*
	print string s to descriptor fd
	adds a timestamp on top and terminates with a newline character
	line is truncated at MAX_LOGFILE_LINE_LENGTH
 */
void printlog (const char* s, int fd);

/*
	we intend as 'header packet', the first one of a client/server
	transaction, the only one containing payload digest
	each transaction from client to server has a header packet, except
	from the 'configuration packet', that brings the public key that
	permits to decrypt future digest in header packets coming

	header packet

	                                                  offset (bytes)
	        3	 2	   1           0		  
	 |-----------------------------------------|
         |                  type                   | 0
	 |-----------------------------------------|
	 |             length (in bytes)           | 4
	 |-----------------------------------------|
	 |                                         | 8
	 |                 digest                  |
	 |                                         | 4 + NUM_BITS/8
	 |-----------------------------------------|

   configuration packet

                                                     offset (bytes)
           3        2         1           0
    |-----------------------------------------|
    |                  type                   | 0
    |-----------------------------------------|
 	 |           timeout  period (sec)         | 4
	 |-----------------------------------------|
	 |           socket timeout (msec)         | 8
	 |-----------------------------------------|
	 |              modulus size               | 12
	 |-----------------------------------------|
	 |                                         | 16
	 |                 modulus                 |
	 |                                         | 12 + NUM_BITS/8
	 |-----------------------------------------|
	 |          public exponent size           | 16 + NUM_BITS/8
 	 |-----------------------------------------|
	 |                                         | 20 + NUM_BITS/8
	 |              public exponent            |
	 |                     ____________________|
	 |--------------------|(not 4-bytes aligned) 16 + 2 NUM_BITS/8 [MAX]

   to avoid missing client/server synchronization, server replies to
	each client write with a 'ack packet'; this packet is also used to
	tell client to stop sending data (because something went wrong) and
	to let him know (at the end of each transaction) whether sent status
	is ok or not

                                                     offset (bytes)	
                3        2         1           0	    
	 |-----------------------------------------|
	 |                  type                   | 0
	 |-----------------------------------------|
 */

typedef struct {
   int type;
   int length;
   /* The crypted MD5/SHA1 msg has the same length of the RSA key. */
   unsigned char digest[NUM_BITS / 8];
} header;

typedef struct {
   int type;
   union {
      struct timeval sock_timeout;
   } payload;
} ack;

#define __COMMON__H
#endif

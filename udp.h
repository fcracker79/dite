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

#ifndef __UDP__H

#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* This function starts the port check. The descriptor returned is associated
   to the /proc/net/udp and the opened file is pointed to the 2th line.
 */
FILE *startNetUDP (void);

/* This function returns the next opened UDP port. It returns the port number,
   or -1 if we have found no more opened ports.
	The unsigned int addr will contain the bind address.
   The function also returns -1 on any other error.
 */
int getNetUDP (FILE *, unsigned int *addr);

/* This function closes the UDP ports check. It is assumed to be called after
   a startNetUDP() and some getNetUDP().
 */
int endNetUDP (FILE *);

#define __UDP__H
#endif

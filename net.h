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

#ifndef __NET__H

#include "common.h"
#include "comms.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int getOpenTCP (open_port ** op, unsigned int *l);
int getOpenUDP (open_port ** op, unsigned int *l);

int print_open_port (open_port * op, unsigned int l);

int set_range_TCP (int min, int max);
int set_range_UDP (int min, int max);
int set_range (int min, int max);
#define __NET__H
#endif

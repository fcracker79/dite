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

#ifndef __TIME_QUEUE__H
#include <stdlib.h>

/* Timer Queue
	single linked ordered list on incremental time-to-live field
	
	data structure as follows

	          ------------       ------------               ------------
	top --->  |   next   | --->  |   next   | ---> ... ---> |   next   | ---X
	          ------------       ------------               ------------
				 |  ttl_a   |       |  ttl_b   |               |  ttl_n   |
				 ------------       ------------               ------------
				 |   id_a   |       |   id_b   |               |   id_n   |
				 ------------       ------------               ------------

   operations:

	* init:  initialize queue
	* tick:  decrement ttl_a by one
	* push:  insert a new element with id_i, ttl_i into the list, and care
	         that when done ttl_i = sum (ttl_j) where j = 1,2,...,i-1
   * pop:   delete top element and return id_a
	* test:  check ttl_a to be null, in which case return 1 (0 if not)
	* clear: delete each element
	* empty: test emptiness of queue, if empty return 1 (0 if not)

	access functions require a pointer to a time_queue to be passed as
	argument; if by error you give to any of preceding functions a NULL
	time_queue, it does nothing and, if possible, returns -1

	users SHOULD use only 'time_queue' data type and let access functions
	do all work, without hand checking/modifying queue data structures

	I think that would be trivial to modify functions to allow storing
	more data into one tq_elem, eventually dynamically allocating memory
	space to accomodate it
 */

#ifdef TQ_DEBUG
#include <stdio.h>
#endif

struct tq_elem {
	struct tq_elem* next;
	int ttl;
	int id;
};

typedef struct tq_elem tq_elem;
typedef tq_elem* time_queue;

void
tq_init ( tq_elem** top );

void
tq_tick ( tq_elem** top );

void
tq_push ( tq_elem** top, int ttl, int id );

int
tq_pop ( tq_elem** top );

int
tq_test ( tq_elem** top );

void
tq_clear ( tq_elem** top );

int
tq_empty ( tq_elem** top );

#define __TIME_QUEUE__H
#endif /* __TIME_QUEUE__H */

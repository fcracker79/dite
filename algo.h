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

#ifndef __ALGO__H
#include <stdlib.h>
/* This function sorts an array, which contains num elements. Each element
	is size bytes long. The array's length is num * size bytes.
	The function gt(a, b) compares two elements of the array. It returns > 0 if
	a > b, 0 if a == b, < 0 otherwise.
 */
int sort (void *array, unsigned int num, unsigned int size,
	  int (*gt) (void *, void *));
#define __ALGO__H
#endif

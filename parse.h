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

#ifndef __PARSE__H

#include "common.h"

#define MAX_PARSING_COMMAND_LENGTH 32
#define MAX_PARSING_COMMAND_ARGUMENT 64
#define MAX_PARSING_LINE_LENGTH 1024

/*
	Parsing is done by scanning once given file and putting data into a list,
	   which top pointer is given by user, whose elements are done as such
		as follows:

		          --------------
			------>|    next    |------> 
					 --------------
					 |  command   |
					 |    argc    |
					 |    argv    |--\
					 --------------    \
					                     \
											 -------------------
											 |     argv[0]     |
											 |     argv[1]     |
											 |       ...       |
											 |   argv[argc-1]  |
											 -------------------
   Rules:
		(1) comments are lines starting with a #
		(2) blank spaces (' ' and '\t') between words doesn't matter
		(3) a word is a sequence of (printable) character
		(4) first word in a line is put into command field
		(5) following words into the same line are arguments
		(6) if a line is too long ( > MAX_PARSING_LINE_LENGTH ) or if it does
		    not have any word it's discarded
		(7) if a command or argument is too long ( > MAX_PARSING_COMMAND_LENGTH
		    > MAX_PARSING_COMMAND_ARGUMENT ) it is truncated and overflow
			 characters are discarded

   Notes:
	   (1) you can change MAX_PARSING_LINE_LENGTH, MAX_PARSING_COMMAND_LENGTH,
		       MAX_PARSING_COMMAND_ARGUMENT as you prefere
	
 */

struct parsing_entry {
   struct parsing_entry *next;
   char command[MAX_PARSING_COMMAND_LENGTH + 1];
   int argc;
   char **argv;			/* null terminated argument list */
};

/*
	Parse filename into a list which elements are parsing_entry structures
	Memory allocation is done during parsing. Refuse to start if
		list_top pointer is not-null
   Refer to parsing list structure at the top of this file
 */
int parse_file (const char *filename, struct parsing_entry **list_top);

/*
	Destroy parsing list allocated space
	MUST be used after a call to parse_file
 */
int parse_destroy_list (struct parsing_entry *list);

/*
	Debug function that prints out parsing list
	Does not allocate memory
 */
int parse_print_list (struct parsing_entry *list);

#define __PARSE__H
#endif

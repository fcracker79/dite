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

#ifndef __INTRUDER__H

#include "common.h"
#include <glob.h>
#include <limits.h>
#include <time.h>
#include <libgen.h>
#include <openssl/md5.h>

/******************************************************************/
/*                      fstree structure                          */
/******************************************************************/
/*
	make it there is following directory structure

	/abs-path/
	   dir_a/
		   file_a_0
			file_a_1
			file_a_2
      dir_b/
		   dir_b0/
			   file_b0_0
				file_b0_1
         dir_b1/
			   file_b1_0
				file_b1_1
         dir_b2/
            file_b2_0
            file_b2_1
         file_b_0
			file_b_1
			file_b_2

   fstree structure of it is contained in a generic tree structure 
	whose nodes (leaf/non-leaf) are directory:

	                           abs-path
                               /   \
                             /       \
                          dir_a     dir_b
                                    / | \
                                  /   |   \
                                /     |     \
                           dir_b0  dir_b1    dir_b2
   
   each node store following informations:
		* node (directory) name
      * pointers to its children
      * pointers to loose files

   -> a pointer to a node of fstree is an index of the array that store entire
      structure, index is double-word-aligned and entries are zero-filled
   -> a pointer to a file is a double-word-aligned index of the files array
      that stores filenames; entries are zero-filled.

   generic entry of files array:
           -------------------------------------
   i-0000  |   f    |    i   |    l   |   e    |
   i-0004  |   n    |    a   |    m   |   e    |
   i-0008  |  NULL  |   CRC  |   CRC  |  CRC   |  <- NULL-terminated string
   i-000c  |  CRC   |   CRC  |   CRC  |  CRC   |
   i-0010  |  CRC   |   CRC  |   CRC  |  CRC   |
   i-0014  |  CRC   |   CRC  |   CRC  |  CRC   |
   i-0018  |  CRC   |  mode  |  mode  | mode   |
   i-001c  |  mode  |   uid  |   uid  |  uid   |
   i-0020  |  uid   |   gid  |   gid  |  gid   |
   i-0024  |  gid   | m_time | m_time | m_time |
   i-0028  | m_time | c_time | c_time | c_time |
   i-002c  | c_time |  NULL  |  NULL  |  NULL  |  <- dword alignment
           -------------------------------------

   generic entry of directory tree:

           -------------------------------------
	j-0004  |   /    |    a   |    b   |   s    |
   j-0008  |   -    |    p   |    a   |   t    |
   j-000c  |   h    |    /   |    d   |   i    |
   j-0010  |   r    |   n    |    a   |   m    |
   j-0014  |   e    |  NULL  |  NULL  |  NULL  |  <- dword alignment
   j-0018  |          first-file-index         |  <- (*)
   j-001c  |           last-file-index         |  <- 
   j-0020  |          sub-dir-k-index          |  <- first child index
   j-0024  |         sub-dir-k+1-index         |  <- second
   j-0028  |         sub-dir-k+2-index         |  <- third
           -------------------------------------

   (*) all files belonging to a directory are stored contiguosly in
	    files array, so it's unnecessary to store each pointer to them:
		 we only put first and last files indeces. If there is only one
		 file into a directory
		           (first-file-index) - (last-file-index) = 4
       if there is none of them
		 			  (first-file-index) == (last-file-index)

	To allow dynamic growth of fstree and files array, we store each of
	them into a single-linked list, pointed by
				first_block_dir
				first_block_file
   a block consisting of BLOCK_SIZE entries (i.e. BLOCK_SIZE * 4 bytes),
	at the very end of each block (BLOCK_SIZE index) we store the pointer
	to the next block.
 */
typedef struct {
   unsigned int abs;
   unsigned int rel;
   void *current_block;
} entry;

int getPathCRC (char *path);

int getCRC (FILE * stream, char *filename);

/* This function finds the MD5 CRC for the specified file.
 */
int crc (const char *filename, unsigned char *msg);

/* This function prints the MD5 CRC in human readable format.
 */
int writeCRC (FILE * stream, const unsigned char *msg);

/* This function prints some stat informations from s on the specified stream.
 */
int stat_print (FILE * stream, const struct stat *s);

int fstree_init (void);
int fstree_destroy (void);
void fstree_print_file (void);
void fstree_print_dir (void);

int getFiles (void **data, unsigned int *l);

#define __INTRUDER__H
#endif

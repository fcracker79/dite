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

#ifndef __SSL__H

#include "common.h"
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

/* This function gets the MD5/SHA1 digest of the message msg. The size of the
   message is specified by the parameter SIZE. The result is put into
   md5_digest and sha1_digest.
   The function returns -1 on error, 1 otherwise.
   Remember that the size of a MD5 digest is MD5_DIGEST_SIZE and for SHA1 is
   SHA_DIGEST_SIZE.
 */
int get_digest (const char *msg, const unsigned int size, char *md5_digest,
		char *sha1_digest);

/* This function prints in human-readable format the digest.
 */
int print_digest (FILE * stream, const unsigned char *digest,
		  const unsigned int size);

#define __SSL__H
#endif

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

#ifndef __RSA__H

#include "common.h"
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/err.h>
/* This function generates a random key pair. It is assumed that the pseudo-
   random numbers generator has been previously initialized.
 */
RSA *generate_key (void);

/* This function crypts a block of data. The function returns the size of
   the destination buffer. Note that prv MUST be a private key.
   The function returns -1 on any error.
   Note that the size of the source msg MUST BE lesser than 
   RSA_size (prv) - 11, otherwise the function returns -2.
*/
int encrypt (unsigned int size, unsigned char *src, unsigned char **dst,
	     RSA * prv);

/* This function decrypts a block of data. The function returns the size of
   the destination buffer. Note that pub SHOULD be a public key.
   The function returns -1 on any error.
 */
int decrypt (unsigned int size, unsigned char *src, unsigned char **dst,
	     RSA * pub);

/* This function gets the modulus of the rsa structure. It returns the size
   of the array.
 */
int get_modulus (RSA * rsa, char **bin);

/* Just like get_modulus (), but for the public exponent.
 */
int get_pub_exp (RSA * rsa, char **bin);

/* This function rebuilds the public key from the binary representation of
   its modulus and its public exponent. I hope this will be -gulp- portable.
   This function is necessary for the preliminar transmission of the public
   key to the other PC.
 */
int rebuild_pubkey (RSA ** rsa,
		    const char *bin_mod,
		    const unsigned int mod_size,
		    const char *bin_exp, const unsigned int exp_size);
#endif

#include "files.h"

/* Don't touch this if you want to have a happy day.
 */
#define BLOCK_SIZE 2048

/* This is the length of the "'/'*" string. Two chars. :)
 */
#define SIZE_AST 2

/* Size of the buffer used for the CRC function. It must be a multiple of 4.
 */
#define CRC_BUF_SIZE 65536

/* Our global data.
 */
entry p_dir;
void *first_block_dir;
entry p_file;
void *first_block_file;

void
fstree_print_dir (void)
{
   unsigned int iabs, irel;
   unsigned char *temp;

   PRINTDEBUG ("Printing dir block\n");
   temp = (char *) first_block_dir;
   iabs = irel = 0;
   while (1) {
      if (iabs >= p_dir.abs)
	 break;
      PRINTDEBUG ("%d %02x %02x %02x %02x", iabs, *(temp + irel * 4),
		  *(temp + irel * 4 + 1), *(temp + irel * 4 + 2),
		  *(temp + irel * 4 + 3));
      PRINTDEBUG ("\t%c %c %c %c\n", *(temp + irel * 4),
		  *(temp + irel * 4 + 1), *(temp + irel * 4 + 2),
		  *(temp + irel * 4 + 3));
      iabs++;
      irel++;
      if (irel >= BLOCK_SIZE) {
	 irel = 0;
	 memcpy ((void *) &temp, (void *) (temp + BLOCK_SIZE * 4), 4);
      }
   }
}

void
fstree_print_file (void)
{
   unsigned int iabs, irel;
   unsigned char *temp;

   PRINTDEBUG ("Printing file block\n");
   temp = (char *) first_block_file;
   iabs = irel = 0;
   while (1) {
      if (iabs >= p_file.abs)
	 break;
      PRINTDEBUG ("%d %02x %02x %02x %02x", iabs, *(temp + irel * 4),
		  *(temp + irel * 4 + 1), *(temp + irel * 4 + 2),
		  *(temp + irel * 4 + 3));
      PRINTDEBUG ("\t%c %c %c %c\n", *(temp + irel * 4),
		  *(temp + irel * 4 + 1), *(temp + irel * 4 + 2),
		  *(temp + irel * 4 + 3));
      iabs++;
      irel++;
      if (irel >= BLOCK_SIZE) {
	 irel = 0;
	 memcpy ((void *) &temp, (void *) (temp + BLOCK_SIZE * 4), 4);
      }
   }
}

int
fstree_init (void)
{
   p_dir.abs = p_dir.rel = 0;

/* We will allocate BLOCK_SIZE bytes + 4 bytes for a pointer to the next
   block.
 */
   p_dir.current_block = first_block_dir = Malloc (4 * BLOCK_SIZE + 4);
   bzero (first_block_dir, BLOCK_SIZE * 4 + 4);

   p_file.abs = p_file.rel = 0;
   p_file.current_block = first_block_file = Malloc (4 * BLOCK_SIZE + 4);
   bzero (first_block_file, 4 * BLOCK_SIZE + 4);
   return 1;
}

int
fstree_destroy (void)
{
   void *temp, *temp2;

   temp = p_dir.current_block;
   while (temp != NULL) {
      memcpy ((void *) &temp2, (void *) (temp + BLOCK_SIZE * 4), 4);
      free (temp);
      temp = temp2;
   }

   temp = p_file.current_block;
   while (temp != NULL) {
      memcpy ((void *) &temp2, (void *) (temp + BLOCK_SIZE * 4), 4);
      free (temp);
      temp = temp2;
   }

   p_dir.current_block = first_block_dir = NULL;
   p_file.current_block = first_block_file = NULL;
   return 1;
}

int
fstree_insert (unsigned int location, unsigned int data)
{
   unsigned int num_block, i;
   void *temp;

   num_block = location / BLOCK_SIZE;
   temp = first_block_dir;
   for (i = 0; i < num_block; i++)
      memcpy (&temp, temp + BLOCK_SIZE * 4, 4);
   memcpy (temp + 4 * (location % BLOCK_SIZE), (void *) &data, 4);
   return 1;
}

int
fstree_insert_dir (void *data, unsigned int size)
{
   void *new_block;

   if (data == NULL)
      return -1;
   if (size > BLOCK_SIZE)
      return -1;
   if (size == 0)
      return 1;

   if ((BLOCK_SIZE - p_dir.rel) < size) {
      new_block = Malloc (BLOCK_SIZE * 4 + 4);
      bzero (new_block, BLOCK_SIZE * 4 + 4);
      memcpy (p_dir.current_block + BLOCK_SIZE * 4, &new_block, 4);
      p_dir.current_block = new_block;
      p_dir.abs += BLOCK_SIZE - p_dir.rel;
      p_dir.rel = 0;
   }

   memcpy (p_dir.current_block + p_dir.rel * 4, data, size * 4);

   p_dir.abs += size;
   p_dir.rel += size;

   return 1;
}

int
fstree_insert_file (void *data, unsigned int size)
{
   void *new_block;

   if (data == NULL)
      return -1;
   if (size > BLOCK_SIZE)
      return -1;
   if (size == 0)
      return 1;

   if ((BLOCK_SIZE - p_file.rel) < size) {
      new_block = Malloc (BLOCK_SIZE * 4 + 4);
      bzero (new_block, BLOCK_SIZE * 4 + 4);
      memcpy (p_file.current_block + BLOCK_SIZE * 4, &new_block, 4);
      p_file.current_block = new_block;
      p_file.abs += BLOCK_SIZE - p_file.rel;
      p_file.rel = 0;
   }

   memcpy (p_file.current_block + p_file.rel * 4, data, size * 4);

   p_file.abs += size;
   p_file.rel += size;

   return 1;
}

int
stat_print (FILE * stream, const struct stat *s)
{
   if (stream == NULL)
      return -1;
   if (s == NULL)
      return -1;
   fprintf (stream, "S mode: %x\n", s->st_mode);
   fprintf (stream, "S uid: %u\n", s->st_uid);
   fprintf (stream, "S gid: %u\n", s->st_gid);
   fprintf (stream, "S Last mod: %s", ctime (&(s->st_mtime)));
   fprintf (stream, "S Last chg: %s", ctime (&(s->st_ctime)));
   return 1;
}

int
getCRC (FILE * stream, char *filename)
{
   struct stat s;
   char msg[CRC_BUF_SIZE];

   bzero ((void *) msg, CRC_BUF_SIZE);

   if (stream == NULL)
      return -1;
   if (filename == NULL)
      return -1;

   fprintf (stream, "X %s\n", filename);
   crc (filename, msg);
   writeCRC (stream, msg);
   fprintf (stream, "\n");
   stat (filename, &s);
   stat_print (stream, &s);
   return 1;
}

int
getPathCRC (char *path)
{
   unsigned int i, dir_found, old_pointer, buf_size;
   char *strtemp, *cur_path, *bname;
   char real_path[PATH_MAX];

   //char md5_msg[MD5_DIGEST_LENGTH];
   glob_t glob_buf;
   struct stat status_file;
   char buff[BLOCK_SIZE * 4];

   if (path == NULL)
      return -1;

   dir_found = 0;
   buf_size = 0;
   strtemp = Malloc (strlen (path) + SIZE_AST + 1);

   bzero (strtemp, strlen (path) + SIZE_AST + 1);

   strncpy (strtemp, path, strlen (path));

   /* Now strtemp has the form:
      path'/'*
    */
   strncat (strtemp, "/*", SIZE_AST);

   PRINTDEBUG ("Parsing path %s\n", strtemp);

/* Get all entries into the path specified by strtemp.
 */
   glob (strtemp, GLOB_MARK, NULL, &glob_buf);

   PRINTDEBUG ("Got glob structure\n");

   //real_path = Malloc (strlen (path) + 1);
   realpath (path, real_path);

   /* Here we will insert a directory entry.
      TODO
    */
   bzero (buff, 4 * BLOCK_SIZE);
   memcpy (buff, real_path, strlen (real_path));
   fstree_insert_dir ((void *) buff, (strlen (real_path)) / 4 + 1);
   fstree_insert_dir ((void *) &p_file.abs, 1);
   //free (real_path);

   for (i = 0; i < glob_buf.gl_pathc; i++) {
      cur_path = (glob_buf.gl_pathv)[i];
      PRINTDEBUG ("Found ");
      PRINTDEBUG (cur_path);
      PRINTDEBUG ("\n");

      /* Ok we have found a file!.
       */
      if (cur_path[strlen (cur_path) - 1] != '/') {

	 /* First, we print only the file name, without the base dir.
	  */
	 bname = basename (cur_path);

	 /* Here we will insert all the file names.
	  */
	 bzero ((void *) buff, sizeof (buff));
	 memcpy ((void *) buff, (void *) bname, strlen (bname));
	 buf_size = strlen (bname) + 1;

	 /* Here we will insert the CRC
	  */
	 crc (cur_path, buff + buf_size);
	 buf_size += MD5_DIGEST_LENGTH;



	 /* And now we print all the stats of the file.
	  */
	 stat (cur_path, &status_file);
	 memcpy (buff + buf_size, &status_file.st_mode, sizeof (mode_t));
	 buf_size += sizeof (mode_t);
	 memcpy (buff + buf_size, &status_file.st_uid, sizeof (uid_t));
	 buf_size += sizeof (uid_t);
	 memcpy (buff + buf_size, &status_file.st_gid, sizeof (gid_t));
	 buf_size += sizeof (gid_t);
	 memcpy (buff + buf_size, &status_file.st_mtime, sizeof (time_t));
	 buf_size += sizeof (time_t);
	 memcpy (buff + buf_size, &status_file.st_ctime, sizeof (time_t));
	 buf_size += sizeof (time_t);

	 fstree_insert_file ((void *) buff, (buf_size - 1) / 4 + 1);

      }
      else
	 dir_found++;
   }

/* Here we insert into the dir tree the pointer to the first empty entry of
   the file tree.	
 */
   fstree_insert_dir ((void *) &p_file.abs, 1);

/* We save the pointer of the dir tree, so that we can write back the index
   for the file list.
 */
   old_pointer = p_dir.abs;

/* Note that buff contains non-coherent data. It is only used to allocate
   space shit that will be overwritten.
 */
   fstree_insert_dir ((void *) buff, dir_found);

/* Now we take care of the sub directories...
 */
   for (i = 0; i < glob_buf.gl_pathc; i++) {
      cur_path = (glob_buf.gl_pathv)[i];
      PRINTDEBUG ("Found ");
      PRINTDEBUG (cur_path);
      PRINTDEBUG ("\n");

      /* Ok we have found a directory!.
       */
      if (cur_path[strlen (cur_path) - 1] == '/') {
	 fstree_insert (old_pointer, p_dir.abs);
	 getPathCRC (cur_path);
	 old_pointer++;
      }
   }

   globfree (&glob_buf);

   /* Deallocate resources. 
    */
   free (strtemp);
   return 1;
}

int
crc (const char *filename, unsigned char *msg)
{
   int fd, numbytes;
   char *buffer;
   MD5_CTX md5;

   if (filename == NULL)
      return -1;
   fd = open (filename, O_RDONLY);
   if (fd < 0)
      return 0;

   buffer = Malloc (CRC_BUF_SIZE);
   bzero (buffer, CRC_BUF_SIZE);

/* Start the MD5 calculations.
 */
   MD5_Init (&md5);

   while (1) {
      /* For every block of data...
       */
      numbytes = read (fd, buffer, CRC_BUF_SIZE);

      /* ...we update the partial result for MD5.
       */
      MD5_Update (&md5, (void *) buffer, numbytes);

      /* If the least block was the last, we can stop.
       */
      if (numbytes < CRC_BUF_SIZE)
	 break;
   }

/* Save the final MD5 message into msg.
 */
   MD5_Final (msg, &md5);

   close (fd);
   free (buffer);
   return 1;
}

int
writeCRC (FILE * stream, const unsigned char *msg)
{
   int i;

   if (msg == NULL)
      return -1;
   if (stream == 0)
      return -1;
   for (i = 0; i < MD5_DIGEST_LENGTH; i++)
      fprintf (stream, "%02x", msg[i]);
   return 1;
}


int
getFiles (void **data, unsigned int *l)
{
   void *p, *u;
   void *temp;
   int i, j;

   if (first_block_dir == NULL)
      return -1;
   if (first_block_file == NULL)
      return -2;

   p = first_block_dir;
   memcpy (&u, (p + BLOCK_SIZE * 4), sizeof (u));
   i = 0;

   while (u != NULL) {
      p = u;
      memcpy (&u, (void *) (p + BLOCK_SIZE * 4), sizeof (p));
      i += BLOCK_SIZE * 4;
   }
	
   i += p_dir.rel * 4;

   p = first_block_file;
   memcpy (&u, (p + BLOCK_SIZE * 4), sizeof (u));
   j = 0;

   while (u != NULL) {
      p = u;
      memcpy (&u, (void *) (p + BLOCK_SIZE * 4), sizeof (p));
      j += BLOCK_SIZE * 4;
   }

   j += p_file.rel * 4;

   *data = Malloc (i + j);
   temp = *data;

   p = first_block_dir;
   memcpy (&u, (p + BLOCK_SIZE * 4), sizeof (u));

   while (u != NULL) {
      memcpy (temp, p, BLOCK_SIZE * 4);
      temp += BLOCK_SIZE * 4;
      p = u;
      memcpy (&u, (void *) (p + BLOCK_SIZE * 4), sizeof (p));
   }

   memcpy (temp, p, p_dir.rel * 4);
   temp += p_dir.rel * 4;

   p = first_block_file;
   memcpy (&u, (p + BLOCK_SIZE * 4), sizeof (u));

   while (u != NULL) {
      memcpy (temp, p, BLOCK_SIZE * 4);
      temp += BLOCK_SIZE * 4;
      p = u;
      memcpy (&u, (void *) (p + BLOCK_SIZE * 4), sizeof (p) );
   }

   memcpy (temp, p, p_file.rel * 4);
   temp += p_file.rel * 4;

   *l = i + j;
   return 1;
}

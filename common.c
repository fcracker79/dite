#include "common.h"

void *
Malloc (size_t size)
{
   void *t;

   t = malloc (size);
   /* fatal error occurred */
   if (!t)
      exit (1);
   memset (t, 0, size);
   return t;
}

ssize_t
Read (int fd, void *buf, size_t count, struct timeval * timeout)
{
   fd_set rdfds;
   int sel_ret, n;
   struct timeval t;

   FD_ZERO (&rdfds);
   FD_SET (fd, &rdfds);

   if (timeout) {
      t.tv_sec = timeout->tv_sec;
      t.tv_usec = timeout->tv_usec;
      sel_ret = select (1 + fd, &rdfds, NULL, NULL, &t);
   }
   else
      sel_ret = select (1 + fd, &rdfds, NULL, NULL, NULL);

   if (sel_ret < 0)
      return READ_SELECT_ERROR;
   if (sel_ret == 0)
      return READ_TIMEOUT;

   n = read (fd, buf, count);

   if (n < 0)
      return READ_ERROR;
   if (n == 0)
      return READ_CLOSECONN;

   return n;
}

ssize_t
Write (int fd, void *buf, size_t count, struct timeval * timeout)
{
   fd_set wrfds;
   int sel_ret, n;
   struct timeval t;

   FD_ZERO (&wrfds);
   FD_SET (fd, &wrfds);

   if (timeout) {
      t.tv_sec = timeout->tv_sec;
      t.tv_usec = timeout->tv_usec;
      sel_ret = select (1 + fd, NULL, &wrfds, NULL, &t);
   }
   else
      sel_ret = select (1 + fd, NULL, &wrfds, NULL, NULL);

   if (sel_ret < 0)
      return WRITE_SELECT_ERROR;
   if (sel_ret == 0)
      return WRITE_TIMEOUT;

   n = write (fd, buf, count);

   if (n < 0)
      return WRITE_ERROR;
   if (n == 0)
      return WRITE_CLOSECONN;

   return n;
}

void printlog (const char* s, int fd) {
   time_t now;
   char line_buf[MAX_LOGFILE_LINE_LENGTH];
   int n;

   now = time (NULL);
   ctime_r (&now, line_buf);
   n = snprintf (line_buf + 24, sizeof(line_buf) - 25,
         " [%ld]: %s", pthread_self(), s);
   if (n < (MAX_LOGFILE_LINE_LENGTH-24)) {
      line_buf[24+n] = '\n';
      write (fd, line_buf, n + 25);
   }
   else {
      line_buf[sizeof(line_buf)-1] = '\n';
      write (fd, line_buf, sizeof(line_buf));
   }
}


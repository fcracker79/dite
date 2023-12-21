#include "kernel.h"

#include <linux/module.h>

int
getKernelStatus (void **data, unsigned int *size)
{
   struct utsname uts;
   void *temp;

   if (data == NULL)
      return -1;
   if (size == NULL)
      return -1;
   if (uname (&uts) == -1)
      return -1;
   *data = temp = Malloc (sizeof (uts));
   memcpy (temp, &uts, sizeof (uts));
   *size = sizeof (uts);
   return 1;
}

int
print_kernel_status (void *data)
{
   struct utsname *u;

   if (data == NULL)
      return -1;
   u = (struct utsname *) data;
   fprintf (stderr, "Sysname: %s\nNodename: %s\nRelease: %s\nVersion: %s\n"
	    "Machine: %s\n", u->sysname, u->nodename, u->release, u->version,
	    u->machine);
   return 1;
}

/* Alert: the module structure is hard-coded.
 */
int
gt_mod (void *a, void *b)
{
   char *s1, *s2;

   memcpy (&s1, a + 4, 4);
   memcpy (&s2, b + 4, 4);
   return strcmp (s1, s2);
}

int
getModuleStatus (module * buff, const char *modname)
{
   struct module_info m;
   unsigned long int size;

   if (buff == NULL)
      return -1;
   if (modname == NULL)
      return -1;

   /* Require some information on the specified module. */
   // query_module removed in Linux 2.6
   return -1;
   // if (query_module (modname, 5, &m, sizeof (struct module_info),
	// 	     (size_t *) & size)
   //     == -1)
   //    return -1;

   buff->length = strlen (modname);
   buff->name = (char *) Malloc (strlen (modname) + 1);
   strncpy (buff->name, modname, strlen (modname));
   buff->size = m.size;
   buff->flags = m.flags;
   return 1;
}

int
print_kernel_modules (void *buff, unsigned int l)
{
   char *name;
   unsigned long int length, flags, size, i;

   if (buff == NULL)
      return -1;

   i = 0;
   while (i < l) {
      memcpy (&length, buff, 4);
      buff += 4;
      i += 4;
      name = (char *) Malloc (length + 1);
      name[length] = 0;
      memcpy (name, buff, length);
      buff += length;
      i += length;
      memcpy (&size, buff, 4);
      i += 4;
      buff += 4;
      memcpy (&flags, buff, 4);
      i += 4;
      buff += 4;
      fprintf (stderr, "Mod. %s, size %ld, flags %lx\n", name, size, flags);
   }
   return 1;
}

int
getLoadedModules (void **mod_array, unsigned int *l)
{
   char *buffer;
   size_t size;
   size_t num;
   char *temp;
   unsigned int i = 0;
   module *mod_temp;
   void *p;

   if (mod_array == NULL)
      return -1;
   if (l == NULL)
      return -1;

/* First we need to know the needed size of the buffer. So we make the
   request with an empty buffer. The function will returns an error and the
   requested size of the buffer.
 */

   // query_module removed in Linux 2.6
   // query_module (NULL, QM_MODULES, NULL, 0, &size);
   buffer = (char *) Malloc (size);

/* The kernel mantains the ENOSPC error, which was intentionally raised by the
   previous query_module() call. So we clean errno. Ugly work...
 */
   errno = 0;
   temp = buffer;
   // query_module (NULL, QM_MODULES, buffer, size, &num);

   mod_temp = (module *) Malloc (sizeof (module) * num);

   *l = num * 12 + size - num;
   *mod_array = Malloc (*l);

/* The structure of the buffer is a set of concatenated null-terminated
   strings. So, now we have to parse this structure.
 */

   for (i = 0; i < num; i++) {
      getModuleStatus (&mod_temp[i], temp);
      temp += strlen (temp) + 1;
   }
   sort (mod_temp, num, sizeof (module), gt_mod);
   p = *mod_array;
   for (i = 0; i < num; i++) {
      memcpy (p, &mod_temp[i].length, 4);
      p += 4;
      memcpy (p, mod_temp[i].name, mod_temp[i].length);
      p += mod_temp[i].length;
      memcpy (p, &mod_temp[i].size, 4);
      p += 4;
      memcpy (p, &mod_temp[i].flags, 4);
      p += 4;
   }
   free ((char *) buffer);
   return 1;
}

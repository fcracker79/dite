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

#ifndef __KERNEL__H

#include "common.h"
#include "comms.h"
#include <sys/utsname.h>

struct module_info {
  unsigned long address;
  unsigned long size;
  unsigned long flags;
};

int query_module(
    const char* name, int which, void* buf, size_t bufsize, size_t* ret);

int getModuleStatus(module* buff, const char* modname);

int getLoadedModules(void** mod_array, unsigned int* l);

int print_kernel_modules(void* buff, unsigned int l);

int getKernelStatus(void** data, unsigned int* size);

int print_kernel_status(void* data);

#define __KERNEL__H
#endif

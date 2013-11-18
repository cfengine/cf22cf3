/* cfengine for GNU
 
        Copyright (C) 1995
        Free Software Foundation, Inc.
 
   This file is part of GNU cfengine - written and maintained 
   by Mark Burgess, Dept of Computing and Engineering, Oslo College,
   Dept. of Theoretical physics, University of Oslo
 
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

*/
 

#include "cf.defs.h"
#include "cf.extern.h"

#ifdef DARWIN
#include <sys/attr.h>
#endif

/*********************************************************************/

int IsHomeDir(char *name)

      /* This assumes that the dir lies under mountpattern */

{ char *sp;
  struct Item *ip;
  int slashes;

if (name == NULL || strlen(name) == 0)
   {
   return false;
   }
  
if (VMOUNTLIST == NULL)
   {
   return (false);
   } 

for (ip = VHOMEPATLIST; ip != NULL; ip=ip->next)
   {
   slashes = 0;

   for (sp = ip->name; *sp != '\0'; sp++)
      {
      if (*sp == '/')
         {
         slashes++;
         }
      }
   
   for (sp = name+strlen(name); (*(sp-1) != '/') && (sp >= name) && (slashes >= 0); sp--)
      {
      if (*sp == '/')
         {
         slashes--;
         }
      }
   
   /* Full comparison */
   
   if (WildMatch(ip->name,sp))
      {
      Debug("IsHomeDir(true)\n");
      return(true);
      }
   }

Debug("IsHomeDir(%s,false)\n",name);
return(false);
}


/*********************************************************************/

int EmptyDir(char *path)

{ DIR *dirh;
  struct dirent *dirp;
  int count = 0;
  
Debug2("cfengine: EmptyDir(%s)\n",path);

if ((dirh = opendir(path)) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open directory %s\n",path);
   CfLog(cfverbose,OUTPUT,"opendir");
   return true;
   }

for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
   {
   if (!SensibleFile(dirp->d_name,path,NULL))
      {
      continue;
      }

   count++;
   }

closedir(dirh);

return (!count);
}


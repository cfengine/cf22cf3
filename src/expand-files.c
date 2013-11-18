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

/*****************************************************************************/
/*                                                                           */
/* File: expand-files.c                                                      */
/*                                                                           */
/*****************************************************************************/

int RecursiveCheck(char *name,int recurse,int rlevel,struct File *ptr,struct stat *sb)

{ DIR *dirh;
  int goback; 
  struct dirent *dirp;
  char pcwd[CF_BUFSIZE];
  struct stat statbuf;
  
if (recurse == -1)
   {
   return false;
   }

if (rlevel > CF_RECURSION_LIMIT)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"WARNING: Very deep nesting of directories (>%d deep): %s (Aborting files)",rlevel,name);
   CfLog(cferror,OUTPUT,"");
   return false;
   }
 
memset(pcwd,0,CF_BUFSIZE); 

Debug("RecursiveCheck(%s,+%o,-%o)\n",name,ptr->plus,ptr->minus);

if (!DirPush(name,sb))
   {
   return false;
   }
 
if ((dirh = opendir(".")) == NULL)
   {
   if (lstat(name,&statbuf) != -1)
      {
      CheckExistingFile("*",name,&statbuf,ptr);
      }
   return true;
   }

for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
   {
   if (!SensibleFile(dirp->d_name,name,NULL))
      {
      continue;
      }

   if (IgnoreFile(name,dirp->d_name,ptr->ignores))
      {
      continue;
      }
   
   strcpy(pcwd,name);                                   /* Assemble pathname */
   AddSlash(pcwd);

   if (BufferOverflow(pcwd,dirp->d_name))
      {
      closedir(dirh);
      return true;
      }

   strcat(pcwd,dirp->d_name);

   if (lstat(dirp->d_name,&statbuf) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"RecursiveCheck was looking at %s when this happened:\n",pcwd);
      CfLog(cfverbose,OUTPUT,"lstat");
      continue;
      }
   
   if (TRAVLINKS)
      {
      if (lstat(dirp->d_name,&statbuf) == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Can't stat %s\n",pcwd);
         CfLog(cfinform,OUTPUT,"stat");
         continue;
         }
      
      if (S_ISLNK(statbuf.st_mode) && (statbuf.st_mode != getuid()))   
         {
         snprintf(OUTPUT,CF_BUFSIZE,"File %s is an untrusted link. cfagent will not follow it with a destructive operation (tidy)",pcwd);
         continue;
         }
      
      if (stat(dirp->d_name,&statbuf) == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"RecursiveCheck was working on %s when this happened:\n",pcwd);
         CfLog(cferror,OUTPUT,"stat");
         continue;
         }
      }
   
   if (ptr->xdev =='y' && DeviceChanged(statbuf.st_dev))
      {
      Verbose("Skipping %s on different device\n",pcwd);
      continue;
      }
   
   if (S_ISLNK(statbuf.st_mode))            /* should we ignore links? */
      {
      CheckExistingFile("*",pcwd,&statbuf,ptr);
      continue;
      }
   
   if (S_ISDIR(statbuf.st_mode))
      {
      if (IsMountedFileSystem(&statbuf,pcwd,rlevel))
         {
         continue;
         }
      else
         {
         if ((ptr->recurse > 1) || (ptr->recurse == CF_INF_RECURSE))
            {
            CheckExistingFile("*",pcwd,&statbuf,ptr);
            goback = RecursiveCheck(pcwd,recurse-1,rlevel+1,ptr,&statbuf);
            DirPop(goback,name,sb);
            }
         else
            {
            CheckExistingFile("*",pcwd,&statbuf,ptr);
            }
         }
      }
   else
      {
      CheckExistingFile("*",pcwd,&statbuf,ptr);
      }
   }

closedir(dirh);
return true; 
}






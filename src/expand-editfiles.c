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
/* File: expand-editfiles.c                                                  */
/*                                                                           */
/*****************************************************************************/


int DoRecursiveEditFiles(char *name,int level,struct Edit *ptr,struct stat *sb)

{ DIR *dirh;
  struct dirent *dirp;
  char pcwd[CF_BUFSIZE];
  struct stat statbuf;
  int goback;

if (level == -1)
   {
   return false;
   }

Debug("RecursiveEditFiles(%s)\n",name);

if (!DirPush(name,sb))
   {
   return false;
   }
 
if ((dirh = opendir(".")) == NULL)
   {
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
      return true;
      }

   strcat(pcwd,dirp->d_name);

   if (!FileObjectFilter(pcwd,&statbuf,ptr->filters,editfiles))
      {
      Verbose("Skipping filtered file %s\n",pcwd);
      continue;
      }
      
   if (TRAVLINKS)
      {
      if (lstat(dirp->d_name,&statbuf) == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Can't stat %s\n",pcwd);
         CfLog(cferror,OUTPUT,"stat");
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
   else
      {
      if (lstat(dirp->d_name,&statbuf) == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"RecursiveCheck was working in %s when this happened:\n",pcwd);
         CfLog(cferror,OUTPUT,"lstat");
         continue;
         }
      }
   
   if (S_ISDIR(statbuf.st_mode))
      {
      if (IsMountedFileSystem(&statbuf,pcwd,level))
         {
         continue;
         }
      else
         {
         if ((ptr->recurse > 1) || (ptr->recurse == CF_INF_RECURSE))
            {
            goback = DoRecursiveEditFiles(pcwd,level-1,ptr,&statbuf);
            DirPop(goback,name,sb);
            }
         else
            {
            WrapDoEditFile(ptr,pcwd);
            }
         }
      }
   else
      {
      WrapDoEditFile(ptr,pcwd);
      }
   }

closedir(dirh);
return true; 
}

/********************************************************************/

void DoEditHomeFiles(struct Edit *ptr)

{ DIR *dirh, *dirh2;
  struct dirent *dirp, *dirp2;
  char *sp,homedir[CF_BUFSIZE],dest[CF_BUFSIZE];
  struct passwd *pw;
  struct stat statbuf;
  struct Item *ip;
  uid_t uid;
  
if (!MountPathDefined())
   {
   CfLog(cfinform,"Mountpattern is undefined\n","");
   return;
   }

for (ip = VMOUNTLIST; ip != NULL; ip=ip->next)
   {
   if (IsExcluded(ip->classes))
      {
      continue;
      }
   
   if ((dirh = opendir(ip->name)) == NULL)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open directory %s\n",ip->name);
      CfLog(cferror,OUTPUT,"opendir");
      return;
      }

   for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
      {
      if (!SensibleFile(dirp->d_name,ip->name,NULL))
         {
         continue;
         }

      strcpy(homedir,ip->name);
      AddSlash(homedir);
      strcat(homedir,dirp->d_name);

      if (! IsHomeDir(homedir))
         {
         continue;
         }

      if ((dirh2 = opendir(homedir)) == NULL)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Can't open directory%s\n",homedir);
         CfLog(cferror,OUTPUT,"opendir");
         return;
         }

      for (dirp2 = readdir(dirh2); dirp2 != NULL; dirp2 = readdir(dirh2))
         {
         if (!SensibleFile(dirp2->d_name,homedir,NULL))
            {
            continue;
            }
         
         strcpy(dest,homedir);
         AddSlash(dest);
         strcat(dest,dirp2->d_name);
         AddSlash(dest);
         sp = ptr->fname + strlen("home/");
         strcat(dest,sp);
         
         if (stat(dest,&statbuf))
            {
            EditVerbose("File %s doesn't exist for editing, skipping\n",dest);
            continue;
            }
      
         if ((pw = getpwnam(dirp2->d_name)) == NULL)
            {
            Debug2("cfengine: directory corresponds to no user %s - ignoring\n",dirp2->d_name);
            continue;
            }
         else
            {
            Debug2("(Setting user id to %s)\n",dirp2->d_name);
            }

         uid = statbuf.st_uid;

         WrapDoEditFile(ptr,dest);
      
         chown(dest,uid,CF_SAME_OWNER);
         }
      closedir(dirh2);
      }
   closedir(dirh);
   }
}

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
 

/*********************************************************************/
/*                                                                   */
/*  TOOLKITS: "rotate" library                                       */
/*                                                                   */
/*********************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"
#include "../pub/global.h"

/*********************************************************************/

void RotateFiles(char *name,int number)

 /* Rotates file extensions like in free bsd, messages, syslog */
 /* etc. Note that this doesn't check for failure, since the   */
 /* aim is to disable the files anyway                         */

{ int i, fd;
  struct Image dummy;
  struct stat statbuf;
  char filename[CF_BUFSIZE];
  
if (stat(name,&statbuf) == -1)
   {
   Verbose("No access to file %s\n",name);
   return;
   }

for (i = number-1; i > 0; i--)
   {
   if (BufferOverflow(name,"1"))
      {
      CfLog(cferror,"Culprit: RotateFiles in Disable:\n","");
      return;
      }
   
   snprintf(filename,CF_BUFSIZE,"%s.%d",name,i);
   snprintf(VBUFF,CF_BUFSIZE,"%s.%d",name, i+1);
   
   if (rename(filename,VBUFF) == -1)
      {
      Debug("Rename failed in RotateFiles %s -> %s\n",name,filename);
      }

   snprintf(filename,CF_BUFSIZE,"%s.%d.gz",name,i);
   snprintf(VBUFF,CF_BUFSIZE,"%s.%d.gz",name, i+1);
   
   if (rename(filename,VBUFF) == -1)
      {
      Debug("Rename failed in RotateFiles %s -> %s\n",name,filename);
      }

   snprintf(filename,CF_BUFSIZE,"%s.%d.Z",name,i);
   snprintf(VBUFF,CF_BUFSIZE,"%s.%d.Z",name, i+1);
   
   if (rename(filename,VBUFF) == -1)
      {
      Debug("Rename failed in RotateFiles %s -> %s\n",name,filename);
      }   

   snprintf(filename,CF_BUFSIZE,"%s.%d.bz",name,i);
   snprintf(VBUFF,CF_BUFSIZE,"%s.%d.bz",name, i+1);
   
   if (rename(filename,VBUFF) == -1)
      {
      Debug("Rename failed in RotateFiles %s -> %s\n",name,filename);
      }   

   snprintf(filename,CF_BUFSIZE,"%s.%d.bz2",name,i);
   snprintf(VBUFF,CF_BUFSIZE,"%s.%d.bz2",name, i+1);
   
   if (rename(filename,VBUFF) == -1)
      {
      Debug("Rename failed in RotateFiles %s -> %s\n",name,filename);
      }   

   }

snprintf(VBUFF,CF_BUFSIZE,"%s.1",name);

dummy.server = "localhost";
dummy.inode_cache = NULL;
dummy.cache = NULL;
dummy.stealth = 'n';
dummy.encrypt = 'n'; 
dummy.preservetimes = 'n';
 
if (CopyRegDisk(name,VBUFF,&dummy) == -1)
   {
   Debug2("cfengine: copy failed in RotateFiles %s -> %s\n",name,VBUFF);
   }

chmod(VBUFF,statbuf.st_mode);
chown(VBUFF,statbuf.st_uid,statbuf.st_gid); 
chmod(name,0600);                 /* File must be writable to empty ..*/
 
if ((fd = creat(name,statbuf.st_mode)) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Failed to create new %s in disable(rotate)\n",name);
   CfLog(cferror,OUTPUT,"creat");
   }
else
   {
   chown(name,statbuf.st_uid,statbuf.st_gid); /* NT doesn't have fchown */
   fchmod(fd,statbuf.st_mode);
   close(fd);
   }
}

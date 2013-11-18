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
/*  Repository handler                                               */
/*                                                                   */
/*********************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"
#include "../pub/global.h"

/*********************************************************************/

int Repository(char *file,char *repository)

 /* Returns true if the file was backup up and false if not */

{ char buffer[CF_BUFSIZE];
  char localrepository[CF_BUFSIZE]; 
  char node[CF_BUFSIZE];
  struct stat sstat, dstat;
  char *sp;
  struct Image dummy;
  short imagecopy;

if (repository == NULL)
   {
   strncpy(localrepository,VREPOSITORY,CF_BUFSIZE);
   }
else
   {
   if (strcmp(repository,"none") == 0 || strcmp(repository,"off") == 0)
      {
      return false;
      }
   
   strncpy(localrepository,repository,CF_BUFSIZE);
   }

if (IMAGEBACKUP == 'n')
   {
   return true;
   }

if (IsItemIn(VREPOSLIST,file))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"The file %s has already been moved to the repository once.",file);
   CfLog(cfinform,OUTPUT,"");
   snprintf(OUTPUT,CF_BUFSIZE,"Multiple update will cause loss of backup. Use backup=false in copy to override.");
   CfLog(cfinform,OUTPUT,"");
   return true;
   }

PrependItem(&VREPOSLIST,file,NULL);

if ((strlen(localrepository) == 0) || HOMECOPY)
   {
   return false;
   }

Debug2("Repository(%s)\n",file);

strcpy (node,file);

buffer[0] = '\0';

for (sp = node; *sp != '\0'; sp++)
   {
   if (*sp == '/')
      {
      *sp = REPOSCHAR;
      }
   }

strncpy(buffer,localrepository,CF_BUFSIZE-2);
AddSlash(buffer);

if (BufferOverflow(buffer,node))
   {
   Debug("culprit: Repository()\n");
   return false;
   }

strcat(buffer,node);

if (!MakeDirectoriesFor(buffer,'y'))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Repository (%s),testfile (%s)",localrepository,buffer);
   }

if (stat(file,&sstat) == -1)
   {
   Debug2("Repository file %s not there\n",file);
   return true;
   }

stat(buffer,&dstat);

imagecopy = IMAGEBACKUP;   /* without this there would be loop between this */
IMAGEBACKUP = 'n';       /* and Repository */

dummy.server = "localhost";
dummy.inode_cache = NULL;
dummy.cache = NULL;
dummy.stealth = 'n';
dummy.encrypt = 'n'; 
dummy.preservetimes = 'n';
dummy.repository = NULL; 
 
CheckForHoles(&sstat,&dummy);

if (CopyReg(file,buffer,sstat,dstat,&dummy))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Moved %s to repository location %s\n",file,buffer);
   CfLog(cfinform,OUTPUT,"");
   IMAGEBACKUP = imagecopy;
   return true;
   }
else
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Failed to move %s to repository location %s\n",file,buffer);
   CfLog(cfinform,OUTPUT,"");
   IMAGEBACKUP = imagecopy;
   return false;
   }
}


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

/*********************************************************************/
/*                                                                   */
/* Tidy object                                                       */
/*                                                                   */
/*********************************************************************/

void TidyParticularFile(char *path,char *name,struct Tidy *tp,struct stat *statbuf,int is_dir,int level,int usepath)

{ struct TidyPattern *tlp;
  short savekilloldlinks = KILLOLDLINKS;

Debug2("TidyParticularFile(%s,%s)\n",path,name);

if (tp->tidylist == NULL)
   {
   return;
   }

for (tlp = tp->tidylist; tlp != NULL; tlp=tlp->next)
   {
   if (IsExcluded(tlp->classes))
      {
      continue;
      }
   
   ResetOutputRoute(tlp->log,tlp->inform);

   if (S_ISLNK(statbuf->st_mode) && is_dir && (tlp->dirlinks == 'k') && (tlp->rmdirs == 'n'))  /* Keep links to directories */
      {
      ResetOutputRoute('d','d');
      continue;
      }

   savekilloldlinks = KILLOLDLINKS;

   if (tlp->travlinks == 'K')
      {
      KILLOLDLINKS = true;
      }

   if (S_ISLNK(statbuf->st_mode))             /* No point in checking permission on a link */
      {
      Debug("Checking for dead links\n");
      if (tlp != NULL)
         {
         KillOldLink(path,tlp->defines);
         tlp->tidied = true;
         }
      else
         {
         KillOldLink(path,NULL);
         }
      }
   
   KILLOLDLINKS = savekilloldlinks;
   
   if (is_dir && tlp->rmdirs == 'n')               /* not allowed to rmdir */
      {
      ResetOutputRoute('d','d');
      continue;
      }

   if ((level == tp->maxrecurse) && tlp->rmdirs == 's') /* rmdir subdirs only */
      {
      ResetOutputRoute('d','d');
      continue;
      }
   
   if (level > tlp->recurse && tlp->recurse != CF_INF_RECURSE)
      {
      Debug2("[PATTERN %s RECURSE ENDED at %d(%d) BEFORE MAXVAL %d]\n",tlp->pattern,
             level,tlp->recurse,tp->maxrecurse);
      ResetOutputRoute('d','d');
      continue;
      }
   
   if (IsExcluded(tlp->classes))
      {
      ResetOutputRoute('d','d');
      continue;
      }

   if (!WildMatch(tlp->pattern,name))
      {
      Debug("Pattern did not match (first filter %s) %s\n",tlp->pattern,path);
      ResetOutputRoute('d','d');
      continue;
      }

   if (!FileObjectFilter(path,statbuf,tlp->filters,tidy))
      {
      Debug("Skipping filtered file %s\n",path);
      continue;
      }

   if (IgnoredOrExcluded(tidy,path,NULL,tp->exclusions))
      {
      Debug("Skipping ignored/excluded file %s\n",path);
      continue;
      }
      
   if (S_ISLNK(statbuf->st_mode) && is_dir && (tlp->dirlinks == 'y'))
      {
      Debug("Link to directory, dirlinks= says delete these\n");
      }
   else if (is_dir && !EmptyDir(path))
      {
      Debug("Non-empty directory %s, skipping..\n",path);
      ResetOutputRoute('d','d');
      continue;
      }
   
   Debug2("Matched %s to %s in %s\n",name,tlp->pattern,path);
   DoTidyFile(path,name,tlp,statbuf,CF_NOLOGFILE,is_dir,usepath);
   ResetOutputRoute('d','d');
   if (!tlp->tidied)
      {
      AddMultipleClasses(tlp->elsedef);
      }
   }
}

/*********************************************************************/
/* Level 2                                                           */
/*********************************************************************/

void DoTidyFile(char *path,char *name,struct TidyPattern *tlp,struct stat *statbuf,short logging_this,int isreallydir,int usepath)

{ time_t nowticks, fileticks = 0;
  int size_match = false, age_match = false;

Debug2("DoTidyFile(%s,%s)\n",path,name);

/* Here we can assume that we are in the right directory with chdir()! */
 
nowticks = time((time_t *)NULL);             /* cmp time in days */

switch (tlp->searchtype)
   {
   case 'a': fileticks = statbuf->st_atime;
             break;
   case 'm': fileticks = statbuf->st_mtime;
      break;
   case 'c': fileticks = statbuf->st_ctime;
      break;
   default:  printf("cfengine: Internal error in DoTidyFile()\n");
             break;
   }

if (isreallydir)
   {
   /* Directory age comparison by mtime, since examining will always alter atime */
   fileticks = statbuf->st_mtime;
   }
 
if ((nowticks-fileticks < 0) && (tlp->age > 0))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"ALERT: atime for %s is in the future. Check system clock!\n",path);
   CfLog(cfinform,OUTPUT,"");
   return;
   }

if (tlp->size == CF_EMPTYFILE)
   {
   if (statbuf->st_size == 0)
      {
      size_match = true;
      }
   else
      {
      size_match = false;
      }
   }
else
   {
   size_match = (tlp->size <= statbuf->st_size);
   }

age_match = tlp->age*CF_TICKS_PER_DAY <= (nowticks-fileticks) || (nowticks < fileticks);

if (age_match && size_match)
   {
   if (logging_this)
      {
      if (VLOGFP != NULL)
         {
         fprintf(VLOGFP,"cf: rm %s\n",path);
         }
      }
   
   if (! DONTDO)
      {
      if (S_ISDIR(statbuf->st_mode))
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Deleting directory %s\n",path);
         CfLog(cfinform,OUTPUT,"");
         AuditLog(tlp->logaudit,tlp->audit,tlp->lineno,OUTPUT,CF_CHG);
   
         if (usepath)
            {
            if (rmdir(path) == -1)
               {
               Debug("Special case remove top level %s\n",path);
               snprintf(OUTPUT,CF_BUFSIZE,"Delete top directory %s failed\n",path);
               CfLog(cfinform,OUTPUT,"unlink");
               }
            else
               {
               AddMultipleClasses(tlp->defines);
               tlp->tidied = true;
               }
            }
         else
            {
            if (rmdir(name) == -1)
               {
               snprintf(OUTPUT,CF_BUFSIZE,"Delete directory %s failed\n",path);
               CfLog(cfinform,OUTPUT,"unlink");
               AuditLog(tlp->logaudit,tlp->audit,tlp->lineno,OUTPUT,CF_FAIL);
               }            
            else
               {
               AddMultipleClasses(tlp->defines);
               tlp->tidied = true;
               }
            }
         }
      else
         {
         int ret=false;
         
         if (tlp->compress == 'y')
            {
            CompressFile(name);
            }
         else if ((ret = unlink(name)) == -1)
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't unlink %s tidying\n",path);
            CfLog(cfverbose,OUTPUT,"unlink");
            AuditLog(tlp->logaudit,tlp->audit,tlp->lineno,OUTPUT,CF_FAIL);
            }
         
         snprintf(OUTPUT,CF_BUFSIZE,"Deleting file %s\n",path);
         CfLog(cfinform,OUTPUT,"");
         AuditLog(tlp->logaudit,tlp->audit,tlp->lineno,OUTPUT,CF_CHG);
         snprintf(OUTPUT,CF_BUFSIZE,"Size=%d bytes, %c-age=%d days\n",
                  statbuf->st_size,tlp->searchtype,(nowticks-fileticks)/CF_TICKS_PER_DAY);
         CfLog(cfverbose,OUTPUT,"");
         
         if (ret != -1)
            {
            AddMultipleClasses(tlp->defines);
            tlp->tidied = true;
            }
         }
      }
   else
      {
      if (tlp->compress == 'y')
         {
         printf("%s: want to compress %s\n",VPREFIX,path);
         }
      else
         {
         printf("%s: want to delete %s\n",VPREFIX,path);
         }
      }
   }
 else
    {
    Debug2("(No age match)\n");
    }
}


/*********************************************************************/

void DeleteTidyList(struct TidyPattern *list)

{
if (list != NULL)
   {
   DeleteTidyList(list->next);
   list->next = NULL;

   if (list->classes != NULL)
      {
      free (list->classes);
      }

   free((char *)list);
   }
}








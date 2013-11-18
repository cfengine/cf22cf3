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
/*  TOOLKIT: the "item file extension" object for cfengine           */
/*                                                                   */
/*********************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*********************************************************************/

int LoadItemList(struct Item **liststart,char *file)

{ FILE *fp;
  struct stat statbuf;

if (stat(file,&statbuf) == -1)
   {
   if (DONTDO)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Currently no file %s to edit\n",file);
      CfLog(cfverbose,OUTPUT,"");
      return false;
      }
   else
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't stat %s - no file to edit\n",file);
      CfLog(cfverbose,OUTPUT,"stat");
      return false;
      }
   }

if ((EDITFILESIZE != 0) &&(statbuf.st_size > EDITFILESIZE))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"File %s is bigger than the limit <editfilesize>\n",file);
   CfLog(cfinform,OUTPUT,"");
   return(false);
   }

if (! S_ISREG(statbuf.st_mode))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"%s is not a plain file\n",file);
   CfLog(cfinform,OUTPUT,"");
   return false;
   }

if ((fp = fopen(file,"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't read file %s for editing\n",file);
   CfLog(cfinform,OUTPUT,"fopen");
   return false;
   }


memset(VBUFF,0,CF_BUFSIZE); 

while(!feof(fp))
   {
   ReadLine(VBUFF,CF_BUFSIZE,fp);

   if (!feof(fp) || (strlen(VBUFF) != 0))
      {
      AppendItem(liststart,VBUFF,NULL);
      }
   VBUFF[0] = '\0';
   }

fclose(fp);
return (true); 
}

/*********************************************************************/

int SaveItemList(struct Item *liststart,char *file,char *repository)

{ struct Item *ip;
  struct stat statbuf;
  char new[CF_BUFSIZE],backup[CF_BUFSIZE];
  FILE *fp;
  mode_t mask;
  char stamp[CF_BUFSIZE]; 
  time_t STAMPNOW;
  STAMPNOW = time((time_t *)NULL);
#ifdef WITH_SELINUX
  int selinux_enabled=0;
  security_context_t scontext=NULL;

  selinux_enabled = (is_selinux_enabled()>0);
  if(selinux_enabled)
      {
      /* get current security context */
      getfilecon(file, &scontext);
      }
#endif

if (stat(file,&statbuf) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't stat %s, which needed editing!\n",file);
   CfLog(cferror,OUTPUT,"");
   CfLog(cferror,"Check definition in program - is file NFS mounted?\n\n","");
   return false;
   }

strcpy(new,file);
strcat(new,CF_EDITED);

strcpy(backup,file);

sprintf(stamp, "_%d_%s", CFSTARTTIME, CanonifyName(ctime(&STAMPNOW)));

if (IMAGEBACKUP == 's')
   {
   strcat(backup,stamp);
   }
 
strcat(backup,CF_SAVED);
 
unlink(new); /* Just in case of races */ 
 
if ((fp = fopen(new,"w")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't write file %s after editing\n",new);
   CfLog(cferror,OUTPUT,"");
   return false;
   }

for (ip = liststart; ip != NULL; ip=ip->next)
   {
   fprintf(fp,"%s\n",ip->name);
   }

if (fclose(fp) == -1)
   {
   CfLog(cferror,"Unable to close file while writing","fclose");
   return false;
   }
 
if (ISCFENGINE && (ACTION == editfiles))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Edited file %s \n",file);
   CfLog(cfinform,OUTPUT,""); 
   }

if (IMAGEBACKUP != 'n')
   {
   if (! IsItemIn(VREPOSLIST,new))
      {
      if (rename(file,backup) == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Error while renaming backup %s\n",file);
         CfLog(cfverbose,OUTPUT,"rename ");
         unlink(new);
         return false;
         }
      else if (Repository(backup,repository))
         {
         unlink(backup);
         }
      }
   }

if (rename(new,file) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Error while renaming %s\n",file);
   CfLog(cfverbose,OUTPUT,"rename");
   return false;
   }       

mask = umask(0); 
chmod(file,statbuf.st_mode);                    /* Restore file permissions etc */
chown(file,statbuf.st_uid,statbuf.st_gid);
umask(mask); 
#ifdef WITH_SELINUX
if(selinux_enabled)
    {
    /* restore file context */
    setfilecon(file,scontext);
    }
#endif
return true;
}

/*********************************************************************/

int CompareToFile(struct Item *liststart,char *file)

/* returns true if file on disk is identical to file in memory */

{ FILE *fp;
  struct stat statbuf;
  struct Item *ip = liststart;
  unsigned char *finmem = NULL, fdata;
  unsigned long fip = 0, tmplen, idx;

Debug("CompareToFile(%s)\n",file);

if (stat(file,&statbuf) == -1)
   {
   return false;
   }

if (liststart == NULL)
   {
   return false;
   }

for (ip = liststart; ip != NULL; ip=ip->next)
   {
   tmplen = strlen(ip->name);
   if ((finmem = realloc(finmem, fip+tmplen+1)) == NULL)
	{
	Debug("CompareToFile(%s): can't realloc() memory\n",file);
	free(finmem);
	return false;
	}
   memcpy(finmem+fip, ip->name, tmplen);
   fip += tmplen;
   *(finmem+fip++) = '\n';
   }

if (statbuf.st_size != fip)
   {
   Debug("CompareToFile(%s): sizes are different: MEM:(%u) FILE:(%u)\n",file, fip, statbuf.st_size);
   free(finmem);
   return false;
   }
 
if ((fp = fopen(file,"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't read file %s for editing\n",file);
   CfLog(cferror,OUTPUT,"fopen");
   free(finmem);
   return false;
   }

for (idx = 0; idx < fip; idx++)
   {
   if (fread(&fdata, 1, 1, fp) != 1)
	{
	Debug("CompareToFile(%s): non-zero fread() before file-in-mem finished at %u-th byte MEM:(0x%x/%c)\n",file, idx, *(finmem+idx), *(finmem+idx));
	free(finmem);
	fclose(fp);
	return false;
	}
   if (fdata != *(finmem+idx))
	{
	Debug("CompareToFile(%s): difference found at %u-th byte MEM:(0x%x/%c) != FILE:(0x%x/%c)\n",file, idx, *(finmem+idx), *(finmem+idx), fdata, fdata);
	free(finmem);
	fclose(fp);
	return false;
	}
   }

free(finmem);
fclose(fp);
return (true);
}

/*********************************************************************/

void InsertFileAfter (struct Item **filestart,struct Item *ptr,char *string)

{ struct Item *ip;
  char *sp;
  FILE *fp;
  char linebuf[CF_BUFSIZE];

EditVerbose("Edit: Inserting file %s \n",string);

if ((fp=fopen(string,"r")) == NULL)
   {
   Verbose("Could not open file %s\n",string);
   return;
   }

while(!feof(fp) && ReadLine(linebuf,CF_BUFSIZE,fp))
   {
   if ((ip = (struct Item *)malloc(sizeof(struct Item))) == NULL)
      {
      CfLog(cferror,"","Can't allocate memory in InsertItemAfter()");
      FatalError("");
      }
   
   if ((sp = malloc(strlen(linebuf)+1)) == NULL)
      {
      CfLog(cferror,"","Can't allocate memory in InsertItemAfter()");
      FatalError("");
      }

   if (CURRENTLINEPTR == NULL)
      {
      if (*filestart == NULL)
         {
         *filestart = ip;
         ip->next = NULL;
         }
      else
         {
         ip->next = (*filestart)->next;
         (*filestart)->next = ip;     
         }
      
      strcpy(sp,linebuf);
      ip->name = sp;
      ip->classes = NULL;
      CURRENTLINEPTR = ip;
      CURRENTLINENUMBER = 1;
      } 
   else
      {
      ip->next = CURRENTLINEPTR->next;
      CURRENTLINEPTR->next = ip;
      CURRENTLINEPTR=ip;
      CURRENTLINENUMBER++;
      strcpy(sp,linebuf);
      ip->name = sp;
      ip->classes = NULL;
      }
   }

NUMBEROFEDITS++;

fclose(fp);
 
return;
}


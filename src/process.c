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


/*******************************************************************/
/*                                                                 */
/* Process handling                                                */
/*                                                                 */
/*******************************************************************/


#include "cf.defs.h"
#include "cf.extern.h"

int DEADLOCK = false;
FILE *PIPE;

/*******************************************************************/

int LoadProcessTable(struct Item **procdata,char *psopts)

{ FILE *pp;
  char pscomm[CF_MAXLINKSIZE], imgbackup;
  struct Item *rootprocs = NULL;
  struct Item *otherprocs = NULL;

snprintf(pscomm,CF_MAXLINKSIZE,"%s %s",VPSCOMM[VSYSTEMHARDCLASS],psopts);

Verbose("%s: Running process command %s\n",VPREFIX,pscomm); 
  
if ((pp = cfpopen(pscomm,"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open the process list with command %s\n",pscomm);
   CfLog(cferror,OUTPUT,"popen");
   return false;
   }

while (!feof(pp))
   {
   memset(VBUFF,0,CF_BUFSIZE);
   ReadLine(VBUFF,CF_BUFSIZE,pp);
   AppendItem(procdata,VBUFF,"");
   }

cfpclose(pp);

/* Now save the data */

imgbackup = IMAGEBACKUP;
IMAGEBACKUP = 'n'; 

snprintf(VBUFF,CF_MAXVARSIZE,"%s/state/cf_procs",CFWORKDIR);
SaveItemList(*procdata,VBUFF,"none");

CopyList(&rootprocs,*procdata);
CopyList(&otherprocs,*procdata);

while (DeleteItemNotContaining(&rootprocs,"root"))
   {
   }

while (DeleteItemContaining(&otherprocs,"root"))
   {
   }

PrependItem(&rootprocs,otherprocs->name,NULL);

snprintf(VBUFF,CF_MAXVARSIZE,"%s/state/cf_rootprocs",CFWORKDIR);
SaveItemList(rootprocs,VBUFF,"none");
DeleteItemList(rootprocs);
    
snprintf(VBUFF,CF_MAXVARSIZE,"%s/state/cf_otherprocs",CFWORKDIR);
SaveItemList(otherprocs,VBUFF,"none");
DeleteItemList(otherprocs);

IMAGEBACKUP = imgbackup;

return true;
}

/*******************************************************************/

void DoProcessCheck(struct Process *pp,struct Item *procdata)

{ char line[CF_BUFSIZE];
  int matches=0,dosignals=true;
  mode_t maskval;
  struct stat statbuf;
  struct Item *killlist = NULL;

}

/**********************************************************************************/

int FindMatches(struct Process *pp,struct Item *procdata,struct Item **killlist)

{ struct Item *ip, *ip2;
  char *sp,saveuid[16];
  int pid=-1, ret, matches=0, got, i, one_space;
  regex_t rx,rxcache;
  regmatch_t pmatch;
  pid_t cfengine_pid = getpid();
  char *names[CF_PROCCOLS];      /* ps headers */
  int start[CF_PROCCOLS];
  int end[CF_PROCCOLS];
  
Debug2("Looking for process %s\n",pp->expr);

if (CfRegcomp(&rxcache,pp->expr,REG_EXTENDED) != 0)
   {
   return 0;
   }

GetProcessColumns(procdata->name,(char **)names,start,end); 

for (ip = procdata; ip != NULL; ip=ip->next)
   {
   memcpy(&rx,&rxcache,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */

   if (regexec(&rx,ip->name,1,&pmatch,0) == 0)
      {
      pid = -1;
      got = true;
      Debug("Regex %s matched %s\n",ip->name,pp->expr);
      
      for (ip2 = pp->inclusions; ip2 != NULL; ip2 = ip2->next)
         {
         got = false;
         
         if (strstr(ip->name,ip2->name) || WildMatch(ip2->name,ip->name))
            {
            got = true;
            break;
            }
         }
      
      if (!got)
         {
         continue;
         }
      
      got = false;
      
      for (ip2 = pp->exclusions; ip2 != NULL; ip2 = ip2->next)
         {
         if (strstr(ip->name,ip2->name) || WildMatch(ip2->name,ip->name))
            {
            got = true;
            break;
            }
         }
      
      if (!ProcessFilter(ip->name,pp->filters,names,start,end))
         {
         Debug("%s Filtered away\n",ip->name);
         continue;
         }
      
      if (got)
         {
         continue;
         }
      
      Debug("Matched proc[%s]\n",ip->name);

      for (sp = ip->name; *sp != '\0'; sp++) /* if first field contains alpha, skip */
         {
         while (true)
            {
            /* HvB If start with alpha then skip it till the first space */
	    if (isalnum((int)*sp))
	       {
	       while ((*sp != ' ') && (*sp != '\0'))
                  {
                  sp++;
		  }
	       }

	       /* skip spaces and tabs */
               while ((*sp == ' ') && (*sp == '\t'))
	          {
		  sp++;
		  }
             break;
             } /* end while */
  
         sscanf(sp,"%d",&pid);
	
         if (pid != -1)
            {
            break;
            }
         } /* end for */
      
      if (pid == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Unable to extract pid while looking for %s\n",pp->expr);
         CfLog(cfverbose,OUTPUT,"");
         continue;
         }
      
      Debug2("Found matching pid %d\n",pid);
      
      matches++;
      
      if (pid == 1 && pp->signal == cfhup)
         {
         Verbose("(Okay to send HUP to init)\n");
         }
      else if (pid < 4)
         {
         Verbose("%s: will not signal or restart processes 0,1,2,3\n",VPREFIX);
         Verbose("%s: occurred while looking for %s\n",VPREFIX,pp->expr);
         continue;
         }
      
      if (pp->action == 'w')
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Process alert: %s\n",procdata->name);
         CfLog(cferror,OUTPUT,"");  
         snprintf(OUTPUT,CF_BUFSIZE*2,"Process alert: %s\n",ip->name);
         CfLog(cferror,OUTPUT,"");
         continue;
         }
      
      if (pp->signal != cfnosignal)
         {
         if (!DONTDO)
            {
            if (pid == cfengine_pid)
               {
               CfLog(cfverbose,"Cfengine will not kill itself!\n","");
               continue;
               }
            
            if (pp->action == 'm')
               {
               sprintf(saveuid,"%d",pid);
               PrependItem(killlist,saveuid,"");
               }
            else
               {
               if ((ret = kill((pid_t)pid,pp->signal)) < 0)
                  {
                  snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't send signal to pid %d\n",pid);
                  CfLog(cfverbose,OUTPUT,"kill");
                  
                  continue;
                  }
               
               snprintf(OUTPUT,CF_BUFSIZE*2,"Signalled process %d (%s) with %s\n",pid,pp->expr,SIGNALS[pp->signal]);
               CfLog(cfinform,OUTPUT,"");
               
               if ((pp->signal == cfkill || pp->signal == cfterm) && ret >= 0)
                  {
                  snprintf(OUTPUT,CF_BUFSIZE*2,"Killed: %s\n",ip->name);
                  CfLog(cfinform,OUTPUT,"");
                  }
               }
            }
         }
      }
   }
 
 for (i = 0; i < CF_PROCCOLS; i++)
    {
    if (names[i] != NULL)
       {
       free(names[i]);
       }
    }

 regfree(&rx);
 return matches;
}

/**********************************************************************************/

void DoSignals(struct Process *pp,struct Item *list)

{ struct Item *ip;
  pid_t pid;
  int ret;

Verbose("DoSignals(%s)\n",pp->expr);
  
if (list == NULL)
   {
   return;
   }

if (pp->signal == cfnosignal)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"No signal to send for %s\n",pp->expr);
   CfLog(cfinform,OUTPUT,"");
   return;
   }

for (ip = list; ip != NULL; ip=ip->next)
   {
   pid = (pid_t)-1;
   
   sscanf(ip->name,"%d",&pid);

   if (pid == (pid_t)-1)
      {
      CfLog(cferror,"Software error: Unable to decypher pid list","");
      return;
      }

   if (!DONTDO)
      {
      if ((ret = kill((pid_t)pid,pp->signal)) < 0)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't send signal to pid %d\n",pid);
         CfLog(cfverbose,OUTPUT,"kill");
         //pp->returnstatus = CF_FAIL;
         return;
         }
      
      if ((pp->signal == cfkill || pp->signal == cfterm) && ret >= 0)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Killed: %s\n",ip->name);
         CfLog(cfinform,OUTPUT,"");
         //pp->returnstatus = CF_CHG;
         }
      }
   }
}

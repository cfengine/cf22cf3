
/* cfengine for GNU
 
        Copyright (C) 1995/6,2000
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

/*****************************************************************************/
/*                                                                           */
/* File: filters.c                                                           */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*******************************************************************/
/* Parser                                                          */
/*******************************************************************/

void InstallFilter(char *filter)

{ struct Filter *ptr;
  int i;
 
Debug1("InstallFilter(%s)\n",filter);

if (FilterExists(filter))
   {
   yyerror("Redefinition of existing filter");
   return;
   }

if (! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing Edit no match\n");
   return;
   }

if ((ptr = (struct Filter *)malloc(sizeof(struct Filter))) == NULL)
   {
   FatalError("Memory Allocation failed for InstallFilterFilter() #1");
   }

if ((ptr->alias = strdup(filter)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallFilterFilter() #2");
   }
 
if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallFilterFilter() #3");
   }
 
if (VFILTERLISTTOP == NULL)                 /* First element in the list */
   {
   VFILTERLIST = ptr;
   }
else
   {
   VFILTERLISTTOP->next = ptr;
   }

for (i = 0; i < NoFilter; i++)
   {
   ptr->criteria[i] = NULL;
   }
 
ptr->next = NULL;
ptr->defines = NULL;
ptr->elsedef = NULL; 
VFILTERLISTTOP = ptr;
}

/********************************************************************/

void InstallFilterTest(char *alias,char *type,char *data)

{ int crit, i = -1;
  struct Filter *fp;
  char ebuff[CF_EXPANDSIZE],vbuff[CF_BUFSIZE],units='x',*sp;
  time_t now;

if (strlen(type) == 0)
   {
   return;
   }
  
Debug("InstallFilterTest(%s,%s,%s)\n",alias,type,data);

if (time(&now) == -1)
   {
   FatalError("Unable to access clock");
   }

crit = (int) FilterActionsToCode(type); 
 
ExpandVarstring(data,ebuff,NULL);
 
if (crit == NoFilter)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Unknown filter criterion (%s)\n",type);
   yyerror(OUTPUT);
   }
else
   {
   for (fp = VFILTERLIST; fp != NULL; fp = fp->next)
      {
      if (strcmp(alias,fp->alias) == 0)
         {
         switch (crit)
            {
            case filterdefclasses:
                fp->defines = strdup(ebuff);
                break;
            case filterelsedef:
                fp->elsedef = strdup(ebuff);
                break;
            case filterresult:
                IsInstallable(ebuff); /* syntax check */
                break;
                
            case filterfromctime:
                now = Date2Number(ebuff,now);
                break;
            case filtertoctime:
                now = Date2Number(ebuff,now);
                break;
            case filterfrommtime:
                now = Date2Number(ebuff,now);
                break;
            case filtertomtime:
                now = Date2Number(ebuff,now);
                break;
            case filterfromatime:
                now = Date2Number(ebuff,now);
                break;
            case filtertoatime:
                now = Date2Number(ebuff,now);
                break;
            case filterfromsize:
            case filtertosize:
                sscanf(ebuff,"%d%c",&i,&units);
                
                if (i < 0)
                   {
                   yyerror("filter size attribute with silly value (must be a non-negative number)");
                   }
                
                switch (units)
                   {
                   case 'k':
                   case 'K': i *= 1024;
                       break;
                   case 'm':
                   case 'M': i = i * 1024 * 1024;
                       break;
                   }
                
                sprintf(ebuff,"%d",i);
                break;
                
            case filterexecregex:
                for (sp = ebuff+strlen(ebuff)-1; (*sp != '(') && (sp > ebuff); sp--)
                   {
                   }
                
                sscanf(sp+1,"%256[^)]",vbuff);
                if (!RegexOK(vbuff))
                   {
                   yyerror("Regular expression error");
                   }
                break;
                
            case filternameregex:
                if (!RegexOK(ebuff))
                   {
                   yyerror("Regular expression error");
                   }
                break;
            case filterexec:
                /* test here */
                break;
            case filtersymlinkto:
                break;
            }
         
         if ((fp->criteria[crit] = strdup(ebuff)) == NULL)
            {
            CfLog(cferror,"Couldn't allocate filter memory","strdup");
            FatalError("Dying..");
            }
         else
            {
            return;
            }
         }
      }
   }
}

/********************************************************************/

void CheckFilters()

{ struct Filter *fp;
  time_t t;

if (time(&t) == -1)
   {
   FatalError("Clock unavailable");
   }

for (fp = VFILTERLIST; fp != NULL; fp = fp->next)
   {
   if (fp->criteria[filterresult] == NULL)
       {
       snprintf(OUTPUT,CF_BUFSIZE*2,"No result specified for filter %s",fp->alias);
       CfLog(cferror,OUTPUT,"");
       FatalError("Consistency error");
       }
       
   if ((fp->criteria[filterfromctime]!=NULL && fp->criteria[filtertoctime]==NULL) ||
       (fp->criteria[filterfromctime]==NULL && fp->criteria[filtertoctime]!=NULL))
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Incomplete ctime limit specification of filter %s",fp->alias);
      CfLog(cferror,OUTPUT,"");
      FatalError("Consistency errors");
      }
      
   if ((fp->criteria[filterfromatime]!=NULL && fp->criteria[filtertoatime]==NULL) ||
       (fp->criteria[filterfromatime]==NULL && fp->criteria[filtertoatime]!=NULL))
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Incomplete atime limit specification of filter %s",fp->alias);
      CfLog(cferror,OUTPUT,"");
      FatalError("Consistency errors");
      }
      
   if ((fp->criteria[filterfrommtime]!=NULL && fp->criteria[filtertomtime]==NULL) ||
       (fp->criteria[filterfrommtime]==NULL && fp->criteria[filtertomtime]!=NULL))
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Incomplete mtime limit specification of filter %s",fp->alias);
      CfLog(cferror,OUTPUT,"");
      FatalError("Consistency errors");
      }

   if ((fp->criteria[filterfromstime]!=NULL && fp->criteria[filtertostime]==NULL) ||
       (fp->criteria[filterfromstime]==NULL && fp->criteria[filtertostime]!=NULL))
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Incomplete stime limit specification of filter %s",fp->alias);
      CfLog(cferror,OUTPUT,"");
      FatalError("Consistency errors");
      }

   if ((fp->criteria[filterfromttime]!=NULL && fp->criteria[filtertottime]==NULL) ||
       (fp->criteria[filterfromttime]==NULL && fp->criteria[filtertottime]!=NULL))
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Incomplete ttime limit specification of filter %s",fp->alias);
      CfLog(cferror,OUTPUT,"");
      FatalError("Consistency errors");
      }

   
   if ((fp->criteria[filterfromsize]!=NULL && fp->criteria[filtertosize]==NULL) ||
       (fp->criteria[filterfromsize]==NULL && fp->criteria[filtertosize]!=NULL))
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Incomplete size limit specification of filter %s",fp->alias);
      CfLog(cferror,OUTPUT,"");
      FatalError("Consistency errors");
      }
   
   if (fp->criteria[filterfromatime] != NULL)
      {
      if (Date2Number(fp->criteria[filterfromatime],t) > Date2Number(fp->criteria[filtertoatime],t))
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"To/From atimes silly in filter %s (from > to)",fp->alias);
         CfLog(cferror,OUTPUT,"");
         FatalError("Consistency errors");
         }
      }
   
   if (fp->criteria[filterfromctime] != NULL)
      {
      if (Date2Number(fp->criteria[filterfromctime],t) > Date2Number(fp->criteria[filtertoctime],t))
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"To/From ctimes silly in filter %s (from > to)",fp->alias);
         CfLog(cferror,OUTPUT,"");
         FatalError("Consistency errors");
         }
      }
   
   if (fp->criteria[filterfrommtime] != NULL)
      {
      if (Date2Number(fp->criteria[filterfrommtime],t) > Date2Number(fp->criteria[filtertomtime],t))
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"To/From mtimes silly in filter %s (from > to)",fp->alias);
         CfLog(cferror,OUTPUT,"");
         FatalError("Consistency errors");
         }
      }
   
   if (fp->criteria[filterfromstime] != NULL)
      {
      if (Date2Number(fp->criteria[filterfromstime],t) > Date2Number(fp->criteria[filtertostime],t))
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"To/From stimes silly in filter %s (from > to)",fp->alias);
         CfLog(cferror,OUTPUT,"");
         FatalError("Consistency errors");
         }
      }
   
   if (fp->criteria[filterfromttime] != NULL)
      {
      if (Date2Number(fp->criteria[filterfromttime],t) > Date2Number(fp->criteria[filtertottime],t))
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"To/From ttimes silly in filter %s (from > to)",fp->alias);
         CfLog(cferror,OUTPUT,"");
         FatalError("Consistency errors");
         }

      if (strncmp(fp->criteria[filterfromttime],"accumulated",strlen("accumulated")) != 0)
         {
         yyerror("Must use accumulated time in FromTtime");
         }

      if (strncmp(fp->criteria[filtertottime],"accumulated",strlen("accumulated")) != 0)
         {
         if (strcmp(fp->criteria[filtertottime],"inf") != 0)
            {
            yyerror("Must use accumulated time in ToTtime");
            }
         }
      }
   
   if (fp->criteria[filterfromsize] != NULL)
      {
      if (strcmp(fp->criteria[filterfromsize],fp->criteria[filtertosize]) > 0)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"To/From size is silly in filter %s (from > to)",fp->alias);
         CfLog(cferror,OUTPUT,"");
         FatalError("Consistency errors");
         }
      }
   
   }
}

/********************************************************************/

enum filternames FilterActionsToCode(char *filtertype)

{ int i;

Debug("FilterActionsToCode(%s)\n",filtertype);

if (filtertype[strlen(filtertype)-1] != ':')
   {
   yyerror("Syntax error in filter type");
   return NoFilter;
   }
 
filtertype[strlen(filtertype)-1] = '\0';


for (i = 0; VFILTERNAMES[i] != '\0'; i++)
   {
   if (strcmp(VFILTERNAMES[i],filtertype) == 0)
      {
      return (enum filternames) i;
      }
   }

return (NoFilter);
}

/*******************************************************************/

int FilterExists(char *name)

{ struct Filter *ptr;

for (ptr=VFILTERLIST; ptr != NULL; ptr=ptr->next)
   {
   if (strcmp(ptr->alias,name) == 0)
      {
      return true;
      }
   }
return false;
}

/*********************************************************************/

time_t Date2Number(char *string,time_t now)

 /*

  FromCtime: "date(1999,2,22,0,0,0)"
  FromMtime: "tminus(0,0,2,0,0,0)"
    
  */

{ char type[CF_SMALLBUF],datestr[CF_SMALLBUF];
  int year=-1,month=-1,day=-1,hr=-1,min=-1,sec=-1; 
  time_t time;
  struct tm tmv;
  
if (string == NULL || strlen (string) == 0)
   {
   return now;
   }

if (strcmp(string,"now") == 0)
   {
   return now;
   }

if (strcmp(string,"inf")==0)
   {
   return (time_t) (now|0x0FFFFFFF);  /* time_t is signed long, assumes 64 bits */
   }
 
sscanf(string,"%15[^(](%d,%d,%d,%d,%d,%d)",type,&year,&month,&day,&hr,&min,&sec);

if (year*month*day*hr*min*sec < 0)
   {
   yyerror("Bad relative time specifier, should be type(year,month,day,hr,min,sec)");
   return now;
   }

if (strcmp(type,"date")==0)
    
   {
   if (year < 1970 || year > 3000)
      {
      yyerror("Year value is silly (1970-3000)");
      return now;
      }
   
   if (month < 1 || month > 12)
      {
      yyerror("Month value is silly (1-12)");
      return now;
      }
   
   if (day < 1 || day > 31)
      {
      yyerror("Day value is silly (1-31)");
      return now;
      }
   
   if (hr > 23)
      {
      yyerror("Hour value is silly (0-23)");
      return now;
      }
   
   if (min > 59)
      {
      yyerror("Minute value is silly (0-59)");
      return now;
      }
   
   if (sec > 59)
      {
      yyerror("Second value is silly (0-59)");
      return now;
      }
   
   Debug("Time is %s or (y=%d,m=%d,d=%d,h=%d,m=%d,s=%d)\n",datestr,year,month,day,hr,min,sec);
   
   tmv.tm_year = year - 1900;
   tmv.tm_mon  = month -1;
   tmv.tm_mday = day;
   tmv.tm_hour = hr;
   tmv.tm_min  = min;
   tmv.tm_sec  = sec;
   tmv.tm_isdst= -1;
   
   if ((time=mktime(&tmv))== -1)
      {
      yyerror("Illegal time value");
      return now;
      }
   
   Debug("Time computed from input was: %s\n",ctime(&time));
   return time; 
   }
 
else if (strcmp(type,"tminus")==0)
    
   {
   if (year > 30)
      {
      yyerror("Relative Year value is silly (0-30 year ago)");
      return now;
      }
   
   Debug("Time is %s or (y=%d,m=%d,d=%d,h=%d,m=%d,s=%d)\n",datestr,year,month,day,hr,min,sec);
   
   time = CFSTARTTIME;

   time -= sec;
   time -= min * 60;
   time -= hr * 3600;
   time -= day * 24 * 3600;
   time -= month * 30 * 24 * 3600;
   time -= year * 365 * 24 * 3600;

   Debug("Total negative offset = %.1f minutes\n",(double)(CFSTARTTIME-time)/60.0);
   Debug("Time computed from input was: %s\n",ctime(&time));
   return time; 
   }
 
else if (strcmp(type,"accumulated")==0)
    
   {
   sscanf(string,"%8[^(](%d,%d,%d,%d,%d,%d)",type,&year,&month,&day,&hr,&min,&sec);
   if (year*month*day*hr*min*sec < 0)
      {
      yyerror("Bad cumulative time specifier, should be type(year,month,day,hr,min,sec)");
      return now;
      }

   time = 0;
   time += sec;
   time += min * 60;
   time += hr * 3600;
   time += day * 24 * 3600;
   time += month * 30 * 24 * 3600;
   time += year * 365 * 24 * 3600;
   return time;
   }

yyerror("Illegal time specifier"); 
return now;
}


/*******************************************************************/
/* Filter code                                                     */
/* These functions return true if the input survives the filters   */
/* Several filters are ANDed together....                          */
/*******************************************************************/

int ProcessFilter(char *proc,struct Item *filterlist,char **names,int *start,int *end)

{ struct Item *tests = NULL;
  struct Filter *fp;
  int result = true, tmpres, i;
  char *line[CF_PROCCOLS];
  
Debug("ProcessFilter(%.6s...)\n",proc);

if (filterlist == NULL)
   {
   return true;
   }

if (strlen(proc) == 0)
   {
   return false;
   }
 
SplitLine(proc,names,start,end,line);

for (fp = VFILTERLIST; fp != NULL; fp=fp->next)
   {
   if (IsItemIn(filterlist,fp->alias))
      {
      Debug ("Applying filter %s\n",fp->alias);

      if (fp->criteria[filterresult] == NULL)
         {
         fp->criteria[filterresult] = strdup("Owner.PID.PPID.PGID.RSize.VSize.Status.Command.TTime.STime.TTY.Priority.Threads");
         }
      
      DoProc(&tests,fp->criteria,names,line);
      
      if (tmpres = EvaluateORString(fp->criteria[filterresult],tests,0))
         {
         AddMultipleClasses(fp->defines);
         }
      else
         {
         AddMultipleClasses(fp->elsedef);
         }
      
      result &= tmpres;
      }   
   }
 
 for (i = 0; i < CF_PROCCOLS; i++)
    {
    if (line[i] != NULL)
       {
       free(line[i]);
       }
    }
 
 DeleteItemList(tests);
 return result; 
}

/*******************************************************************/

int FileObjectFilter(char *file,struct stat *lstatptr,struct Item *filterlist,enum actions context)

{ struct Item *tests = NULL;
  struct Filter *fp;  int result = true, tmpres;
  
Debug("FileObjectFilter(%s)\n",file);
 
if (filterlist == NULL)
   {
   return true;
   }

for (fp = VFILTERLIST; fp != NULL; fp=fp->next)
   {
   if (IsItemIn(filterlist,fp->alias))
      {
      Debug ("Applying filter %s\n",fp->alias);
      
      if (fp->criteria[filterresult] == NULL)
         {
         fp->criteria[filterresult] = strdup("Type.Owner.Group.Mode.Ctime.Mtime.Atime.Size.ExecRegex.NameRegex.IsSymLinkTo.ExecProgram");
         }
      
      DoFilter(&tests,fp->criteria,lstatptr,file);
      
      if (tmpres = EvaluateORString(fp->criteria[filterresult],tests,0))
         {
         AddMultipleClasses(fp->defines);
         }
      else
         {
         AddMultipleClasses(fp->elsedef);
         }
      
      result &= tmpres;
      }
   }
 
DeleteItemList(tests);
Debug("Filter result on %s was %d\n",file,result);
return result; 
}

/*******************************************************************/

void DoFilter(struct Item **attr,char **crit,struct stat *lstatptr,char *filename)

{
if (crit[filtertype] != NULL)
   {
   if (FilterTypeMatch(lstatptr,crit[filtertype]))
      {
      PrependItem(attr,"Type","");
      }
   }

if (crit[filterowner] != NULL)
   {
   if (FilterOwnerMatch(lstatptr,crit[filterowner]))
      {
      PrependItem(attr,"Owner","");
      }
   }

if (crit[filtergroup] != NULL)
   {
   if (FilterGroupMatch(lstatptr,crit[filtergroup]))
      {
      PrependItem(attr,"Group","");
      }
   }

if (crit[filtermode] != NULL)
   {
   if (FilterModeMatch(lstatptr,crit[filtermode]))
      {
      PrependItem(attr,"Mode","");
      }
   }

 if (crit[filterfromatime] != NULL)
   {
   if (FilterTimeMatch(lstatptr->st_atime,crit[filterfromatime],crit[filtertoatime]))
      {
      PrependItem(attr,"Atime","");
      }
   }

if (crit[filterfromctime] != NULL)
   {
   if (FilterTimeMatch(lstatptr->st_ctime,crit[filterfromctime],crit[filtertoctime]))
      {
      PrependItem(attr,"Ctime","");
      }
   }

if (crit[filterfrommtime] != NULL)
   {
   if (FilterTimeMatch(lstatptr->st_mtime,crit[filterfrommtime],crit[filtertomtime]))
      {
      PrependItem(attr,"Mtime","");
      }
   }

if (crit[filternameregex] != NULL)
   {
   if (FilterNameRegexMatch(filename,crit[filternameregex]))
      {
      PrependItem(attr,"NameRegex","");
      }
   }

if ((crit[filtersymlinkto] != NULL) && (S_ISLNK(lstatptr->st_mode)))
   {
   if (FilterIsSymLinkTo(filename,crit[filtersymlinkto]))
      {
      PrependItem(attr,"IsSymLinkTo","");
      }
   }

if (crit[filterexecregex] != NULL)
   {
   if (FilterExecRegexMatch(filename,crit[filterexecregex]))
      {
      PrependItem(attr,"ExecRegex","");
      }
   }

if (crit[filterexec] != NULL)
   {
   if (FilterExecMatch(filename,crit[filterexec]))
      {
      PrependItem(attr,"ExecProgram","");
      }
   }
}

/*******************************************************************/

void DoProc(struct Item **attr,char **crit,char **names,char **line)

{
if (crit[filterowner] != NULL)
   {
   if (FilterProcMatch("UID","USER",crit[filterowner],names,line))
      {
      PrependItem(attr,"Owner","");
      }
   }
else
   {
   PrependItem(attr,"Owner","");
   }

 if (crit[filterpid] != NULL)
   {
   if (FilterProcMatch("PID","PID",crit[filterpid],names,line))
      {
      PrependItem(attr,"PID","");
      }
   }
else
   {
   PrependItem(attr,"PID","");
   }

if (crit[filterppid] != NULL)
   {
   if (FilterProcMatch("PPID","PPID",crit[filterppid],names,line))
      {
      PrependItem(attr,"PPID","");
      }
   }
else
   {
   PrependItem(attr,"PPID","");
   }

if (crit[filterpgid] != NULL)
   {
   if (FilterProcMatch("PGID","PGID",crit[filterpgid],names,line))
      {
      PrependItem(attr,"PGID","");
      }
   }
else
   {
   PrependItem(attr,"PGID","");
   }

if (crit[filtersize] != NULL)
   {
   if (FilterProcMatch("SZ","VSZ",crit[filtersize],names,line))
      {
      PrependItem(attr,"VSize","");
      }
   }
else
   {
   PrependItem(attr,"VSize","");
   }

if (crit[filterrsize] != NULL)
   {
   if (FilterProcMatch("RSS","RSS",crit[filterrsize],names,line))
      {
      PrependItem(attr,"RSize","");
      }
   }
else
   {
   PrependItem(attr,"RSize","");
   }
 
if (crit[filterstatus] != NULL)
   {
   if (FilterProcMatch("S","STAT",crit[filterstatus],names,line))
      {
      PrependItem(attr,"Status","");
      }
   }
else
   {
   PrependItem(attr,"Status","");
   }
 
if (crit[filtercmd] != NULL)
   {
   if (FilterProcMatch("CMD","COMMAND",crit[filtercmd],names,line))
      {
      PrependItem(attr,"Command","");
      }
   }
else
   {
   PrependItem(attr,"Command","");
   }
 
if (crit[filterfromttime] != NULL)
   {
   if (FilterProcTTimeMatch("TIME","TIME",crit[filterfromttime],crit[filtertottime],names,line))
      {
      PrependItem(attr,"TTime","");
      }
   }
else
   {
   PrependItem(attr,"TTime","");
   }
 
if (crit[filterfromstime] != NULL)
   {
   if (FilterProcSTimeMatch("STIME","START",crit[filterfromstime],crit[filtertostime],names,line))
      {
      PrependItem(attr,"STime","");
      }
   }
else
   {
   PrependItem(attr,"STime","");
   }
 
if (crit[filtertty] != NULL)
   {
   if (FilterProcMatch("TTY","TTY",crit[filtertty],names,line))
      {
      PrependItem(attr,"TTY","");
      }
   }
else
   {
   PrependItem(attr,"TTY","");
   }

if (crit[filterpriority] != NULL)
   {
   if (FilterProcMatch("NI","PRI",crit[filterpriority],names,line))
      {
      PrependItem(attr,"Priority","");
      }
   }
else
   {
   PrependItem(attr,"Priority","");
   }
 
if (crit[filterthreads] != NULL)
   {
   if (FilterProcMatch("NLWP","NLWP",crit[filterthreads],names,line))
      {
      PrependItem(attr,"Threads","");
      }
   }
else
   {
   PrependItem(attr,"Threads","");
   }
} 

/*******************************************************************/

int FilterTypeMatch(struct stat *lstatptr,char *crit)

{ struct Item *attrib = NULL;

if (S_ISLNK(lstatptr->st_mode))
   {
   PrependItem(&attrib,"link","");
   }
 
if (S_ISREG(lstatptr->st_mode))
   {
   PrependItem(&attrib,"reg","");
   PrependItem(&attrib,"file","");
   }

if (S_ISDIR(lstatptr->st_mode))
   {
   PrependItem(&attrib,"dir","");
   }
 
if (S_ISFIFO(lstatptr->st_mode))
    {
    PrependItem(&attrib,"fifo","");
    }
 
if (S_ISSOCK(lstatptr->st_mode))
   {
   PrependItem(&attrib,"socket","");
   }

if (S_ISCHR(lstatptr->st_mode))
   {
   PrependItem(&attrib,"char","");
   }

if (S_ISBLK(lstatptr->st_mode))
   {
   PrependItem(&attrib,"block","");
   }

#ifdef HAVE_DOOR_CREATE
if (S_ISDOOR(lstatptr->st_mode))
   {
   PrependItem(&attrib,"door","");
   }
#endif
 
if (EvaluateORString(crit,attrib,0))
   {
   DeleteItemList(attrib);
   return true;
   }
else
   {
   DeleteItemList(attrib);
   return false;
   }
}

/*******************************************************************/

int FilterProcMatch(char *name1,char *name2,char *expr,char **names,char **line)

{ int i;
  regex_t rx;
  regmatch_t pmatch;

Debug("FilterProcMatch(%s,%s,%s,<%x>,<%x>)\n",name1,name2,expr,names,line);


if (CfRegcomp(&rx,expr,REG_EXTENDED) != 0)
   {
   return false;
   }
 
for (i = 0; names[i] != NULL; i++)
   {
   if ((strcmp(names[i],name1) == 0) || (strcmp(names[i],name2) == 0))
      {
      Debug("Match (%s) to (%s)\n",expr,line[i]);
      if (regexec(&rx,line[i],1,&pmatch,0) == 0)
         {
         if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(line[i])))
            {
            regfree(&rx);
            return true;
            }
         }

      Debug("No match of (%s) to (%s)\n",expr,line[i]);
      regfree(&rx);
      return false;      
      }
   }
 
 regfree(&rx);
 return false; 
}

/*******************************************************************/

int FilterProcSTimeMatch(char *name1,char *name2,char *fromexpr,char *toexpr,char **names,char **line)

{ int i;
  time_t fromtime,totime, now = CFSTARTTIME,pstime;
  char year[5],month[4],hr[3],min[3],day[3],timestr[256];
  
memset(year,0,5);
strcpy(year,VYEAR);
memset(month,0,4); 
strcpy(month,VMONTH);
memset(day,0,3); 
strcpy(day,VDAY);
memset(hr,0,3); 
strcpy(hr,VHR);
memset(min,0,3); 
strcpy(min,VMINUTE); 

fromtime = Date2Number(fromexpr,now);
totime = Date2Number(toexpr,now);

for (i = 0; names[i] != NULL; i++)
   {
   if ((strcmp(names[i],name1) == 0) || (strcmp(names[i],name2) == 0))
      {
      if (strstr(line[i],":")) /* Hr:Min:Sec */
         {
         sscanf(line[i],"%2[^:]:%2[^:]:",hr,min);
         snprintf(timestr,256,"date(%s,%d,%s,%s,%s,0)",year,Month2Number(month),day,hr,min);
         }
      else                     /* date Month */
         {
         sscanf(line[i],"%3[a-zA-Z] %2[0-9]",month,day);
         snprintf(timestr,256,"date(%s,%d,%s,%s,%s,0)",year,Month2Number(month),day,hr,min);
         }
      
      if (Month2Number(month) < 0)
         {
         continue;
         }
      
      pstime = Date2Number(timestr,now);
      
      Debug("Stime %s converted to %s\n",timestr,ctime(&pstime));
      
      return ((fromtime < pstime) && (pstime < totime));
      }
   } 
 return false;
}

/*******************************************************************/

/*
 * HvB: Bas van der Vlies
 *  Parse different TTime values
*/

void ParseTTime(char *line, char *time_str)

{
int  day=0, hr=0, min=0, sec=0;
int r;

if (strstr(line,":")) /* day-Hr:Min:Sec */
   {
   /* 
    * first check the long fromat 
    *   day-hr:min:sec (posix)
    *       hr:min:sec (posix)
    *          min:sec (old)
   */ 
   if ((r = sscanf(line,"%d-%d:%d:%d",&day,&hr,&min,&sec)) == 4)
      {
      snprintf(time_str,256,"accumulated(0,0,%d,%d,%d,%d)",day,hr,min,sec);
      }
   else if ((r = sscanf(line,"%d:%d:%d",&hr,&min,&sec)) == 3 )
      {
      snprintf(time_str,256,"accumulated(0,0,0,%d,%d,%d)",hr,min,sec);
      }
   else if ((r = sscanf(line,"%d:%d",&min,&sec)) == 2)
      {
/* Removed test - apparent bug
   
      if (min > 59)
         {
         day = min / (24 * 60);
         hr  = (min - (day * 24 * 60)) / (60);
         min = min % 60;
         }
*/

      day = min / (24 * 60);
      hr  = (min - (day * 24 * 60)) / (60);
      min = min % 60;

      snprintf(time_str,256,"accumulated(0,0,%d,%d,%d,%d)",day,hr,min,sec);
      }
   }
}


/*******************************************************************/

int FilterProcTTimeMatch(char *name1,char *name2,char *fromexpr,char *toexpr,char **names,char **line)

{ int i;
  time_t fromtime,totime,now = CFSTARTTIME,pstime = CFSTARTTIME;
  char timestr[256];

fromtime = Date2Number(fromexpr,now);
totime = Date2Number(toexpr,now);

for (i = 0; names[i] != NULL; i++)
   {
   if ((strcmp(names[i],name1) == 0) || (strcmp(names[i],name2) == 0))
      {
      memset(timestr,0,sizeof(timestr));
      ParseTTime(line[i], timestr);
      Debug("ParseTTime = %s\n",timestr); 
      pstime = Date2Number(timestr,now);
      
      return ((fromtime < pstime) && (pstime < totime));
      }
   } 
return false;
}

/*******************************************************************/

int FilterOwnerMatch(struct stat *lstatptr,char *crit)

{ struct Item *attrib = NULL;
  char buffer[CF_SMALLBUF];
  struct passwd *pw;

sprintf(buffer,"%d",lstatptr->st_uid);
PrependItem(&attrib,buffer,""); 

if ((pw = getpwuid(lstatptr->st_uid)) != NULL)
   {
   PrependItem(&attrib,pw->pw_name,""); 
   }
else
   {
   PrependItem(&attrib,"none",""); 
   }
 
if (EvaluateORString(crit,attrib,0))
   {
   DeleteItemList(attrib);
   return true;
   }
else
   {
   DeleteItemList(attrib);
   return false;
   }
} 

/*******************************************************************/

int FilterGroupMatch(struct stat *lstatptr,char *crit)

{ struct Item *attrib = NULL;
  char buffer[CF_SMALLBUF];
  struct group *gr;

sprintf(buffer,"%d",lstatptr->st_gid);
PrependItem(&attrib,buffer,""); 

if ((gr = getgrgid(lstatptr->st_gid)) != NULL)
   {
   PrependItem(&attrib,gr->gr_name,""); 
   }
else
   {
   PrependItem(&attrib,"none",""); 
   }
 
if (EvaluateORString(crit,attrib,0))
   {
   DeleteItemList(attrib);
   return true;
   }
else
   {
   DeleteItemList(attrib);
   return false;
   }
} 

/*******************************************************************/

int FilterModeMatch(struct stat *lstatptr,char *crit)

{ mode_t plusmask,minusmask,newperm;


ParseModeString(crit,&plusmask,&minusmask);

newperm = (lstatptr->st_mode & 07777);
newperm |= plusmask;
newperm &= ~minusmask;
 
Debug("FilterModeMatch %s -> (+%o,-%o/%o) = %d \n",crit,plusmask,minusmask,lstatptr->st_mode &07777,((newperm & 07777) == (lstatptr->st_mode & 07777)));

return ((newperm & 07777) == (lstatptr->st_mode & 07777));
} 

/*******************************************************************/

int FilterTimeMatch(time_t stattime,char *from,char *to)

{ time_t fromtime,totime, now = CFSTARTTIME;

fromtime = Date2Number(from,now);
totime = Date2Number(to,now);

return ((fromtime < stattime) && (stattime < totime));
} 

/*******************************************************************/

int FilterNameRegexMatch(char *filename,char *crit)

{ regex_t rx;
  regmatch_t pmatch;

if (CfRegcomp(&rx,crit,REG_EXTENDED) != 0)
   {
   return false;
   }            

if (regexec(&rx,filename,1,&pmatch,0) == 0)
   {
   if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(filename)))
      {
      regfree(&rx);
      return true;
      }
  }

regfree(&rx);
return false;      
}

/*******************************************************************/

int FilterExecRegexMatch(char *filename,char *crit)

{ regex_t rx;
  regmatch_t pmatch;
  char buffer[CF_BUFSIZE],expr[CF_BUFSIZE],line[CF_BUFSIZE],ebuff[CF_EXPANDSIZE],*sp;
  FILE *pp;

AddMacroValue(CONTEXTID,"this",filename);
ExpandVarstring(crit,ebuff,NULL);
DeleteMacro(CONTEXTID,"this");
  
memset(buffer,0,CF_BUFSIZE);
memset(line,0,CF_BUFSIZE); 
memset(expr,0,CF_BUFSIZE);

for (sp = ebuff+strlen(ebuff)-1; (*sp != '(') && (sp > ebuff); sp--)
   {
   }
 
sscanf(sp+1,"%256[^)]",expr);
strncpy(buffer,ebuff,(int)(sp-ebuff));
 
if (CfRegcomp(&rx,expr,REG_EXTENDED) != 0)
   {
   return false;
   }            

if ((pp = cfpopen(buffer,"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open pipe to command %s\n",buffer);
   CfLog(cferror,OUTPUT,"cfpopen");
   return false;
   }
 
ReadLine(line,CF_BUFSIZE,pp);  /* One buffer only */

cfpclose(pp); 
 
if (regexec(&rx,line,1,&pmatch,0) == 0)
   {
   if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(line)))
      {
      regfree(&rx);
      return true;
      }
   }
 
regfree(&rx);
return false;      
}

/*******************************************************************/

int FilterIsSymLinkTo(char *filename,char *crit)

{ regex_t rx;
  regmatch_t pmatch;
  char buffer[CF_BUFSIZE];

if (CfRegcomp(&rx,crit,REG_EXTENDED) != 0)
   {
   return false;
   }

memset(buffer,0,CF_BUFSIZE);
 
if (readlink(filename,buffer,CF_BUFSIZE-1) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Unable to read link %s in filter",filename);
   CfLog(cferror,OUTPUT,"readlink");
   regfree(&rx);
   return false;      
   }

if (regexec(&rx,buffer,1,&pmatch,0) == 0)
   {
   if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(buffer)))
      {
      regfree(&rx);
      return true;
      }
  }

regfree(&rx);
return false;      
}

/*******************************************************************/

int FilterExecMatch(char *filename,char *crit)

  /* command can include $(this) for the name of the file */

{ char ebuff[CF_EXPANDSIZE];
 
AddMacroValue(CONTEXTID,"this",filename);
ExpandVarstring(crit,ebuff,NULL);
DeleteMacro(CONTEXTID,"this");

Debug("Executing filter command [%s]\n",ebuff);

if (ShellCommandReturnsZero(ebuff,false))
   {
   return true;
   }
else
   {
   return false;
   }
}

/*******************************************************************/

void GetProcessColumns(char *proc,char **names,int *start,int *end)

{ char *sp,title[16];
  int col,offset = 0;
  
for (col = 0; col < CF_PROCCOLS; col++)
   {
   start[col] = end[col] = -1;
   names[col] = NULL;
   }

col = 0; 
 
for (sp = proc; *sp != '\0'; sp++)
   {
   offset = sp - proc;

   if (isspace((int)*sp))
      {
      if (start[col] != -1)
         {
         Debug("End of %s is %d\n",title,offset-1);
         end[col++] = offset - 1;
         if (col > CF_PROCCOLS - 1)
            {
            CfLog(cferror,"Column overflow in process table","");
            break;
            }
         }
      continue;
      }
   
   else if (start[col] == -1)
      {
      start[col] = offset;
      sscanf(sp,"%15s",title);
      Debug("Start of %s is %d\n",title,offset);
      names[col] = strdup(title);
      Debug("Col[%d]=%s\n",col,names[col]);
      }
   }

if (end[col] == -1)
   {
   Debug("End of %s is %d\n",title,offset);
   end[col] = offset;
   }
}

/*******************************************************************/

void SplitLine(char *proc,char **names,int *start,int *end,char **line)

{ int i,s,e;

Debug("SplitLine(%s)\n",proc); 
 
for (i = 0; i < CF_PROCCOLS; i++)
   {
   line[i] = NULL;
   }
 
for (i = 0; names[i] != NULL; i++)
   {
   for (s = start[i]; (s >= 0) && !isspace((int)*(proc+s)); s--)
      {
      }

   if (s < 0)
      {
      s = 0;
      }
   
   while (isspace((int)proc[s]))
      {
      s++;
      }

   if (strcmp(names[i],"CMD") == 0 || strcmp(names[i],"COMMAND") == 0)
      {
      e = strlen(proc);
      }
   else
      {
      for (e = end[i]; (e <= end[i]+10) && !isspace((int)*(proc+e)); e++)
         {
         }
      
      while (isspace((int)proc[e]))
         {
         if (e > 0)
            {
            e--;
            }
         }
      }
   
   if (s <= e)
      {
      line[i] = (char *)malloc(e-s+2);
      memset(line[i],0,(e-s+2));
      strncpy(line[i],(char *)(proc+s),(e-s+1));
      }
   else
      {
      line[i] = (char *)malloc(1);
      line[i][0] = '\0';
      }
   
   Debug("  %s=(%s) of [%s]\n",names[i],line[i],proc);
   }

Debug("------------------------------------\n"); 
}

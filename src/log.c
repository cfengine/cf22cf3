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
/*****************************************************************************/
/*                                                                           */
/* File: log.c                                                               */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*****************************************************************************/

extern char CFLOCK[CF_BUFSIZE];

/*****************************************************************************/

void CfOpenLog()

{ char value[CF_BUFSIZE];
  int facility = LOG_USER; 
  static int lastsyslog = 0;
  
if (GetMacroValue(CONTEXTID,"SyslogFacility"))
   {
   strncpy(value,GetMacroValue(CONTEXTID,"SyslogFacility"),32);
   
   if (strcmp(value,"LOG_USER") == 0)
      {
      facility = LOG_USER;
      }
   if (strcmp(value,"LOG_DAEMON") == 0)
      {
      facility = LOG_DAEMON;
      }
   if (strcmp(value,"LOG_LOCAL0") == 0)
      {
      facility = LOG_LOCAL0;
      }
   if (strcmp(value,"LOG_LOCAL1") == 0)
      {
      facility = LOG_LOCAL1;
      }
   if (strcmp(value,"LOG_LOCAL2") == 0)
      {
      facility = LOG_LOCAL2;
      }
   if (strcmp(value,"LOG_LOCAL3") == 0)
      {
      facility = LOG_LOCAL3;
      }
   if (strcmp(value,"LOG_LOCAL4") == 0)
      {
      facility = LOG_LOCAL4;
      }
   if (strcmp(value,"LOG_LOCAL5") == 0)
      {
      facility = LOG_LOCAL5;
      }
   if (strcmp(value,"LOG_LOCAL6") == 0)
      {
      facility = LOG_LOCAL6;
      }   
   if (strcmp(value,"LOG_LOCAL7") == 0)
      {
      facility = LOG_LOCAL7;
      }
   
   if (lastsyslog != 1)
      {
      if (lastsyslog)
         {
         closelog();
         }
      }
   lastsyslog=1;
   openlog(VPREFIX,LOG_PID|LOG_NOWAIT|LOG_ODELAY,facility);
   }
else if (ISCFENGINE)
   {
   if (lastsyslog != 2)
      {
      if (lastsyslog)
         {
         closelog();
         }
      }
   lastsyslog=2;
   openlog(VPREFIX,LOG_PID|LOG_NOWAIT|LOG_ODELAY,LOG_USER);
   }
else
   {
   if (lastsyslog != 3)
      {
      if (lastsyslog)
         {
         closelog();
         }
      }
   lastsyslog=3;
   openlog(VPREFIX,LOG_PID|LOG_NOWAIT|LOG_ODELAY,LOG_DAEMON);
   }

}


/*****************************************************************************/

void CfCheckAudit()

{ DB_ENV *dbenv = NULL;
  char name[CF_BUFSIZE];
 
snprintf(name,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,CF_AUDITDB_FILE);

if (AUDIT)
   {
   if ((errno = db_create(&AUDITDBP,dbenv,0)) != 0)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open performance database %s\n",name);
      CfLog(cferror,OUTPUT,"db_open");
      return;
      }
   
#ifdef CF_OLD_DB
   if ((errno = (AUDITDBP->open)(AUDITDBP,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
   if ((errno = (AUDITDBP->open)(AUDITDBP,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open auditing database %s\n",name);
      CfLog(cferror,OUTPUT,"db_open");
      return;
      }

   AuditLog('y',NULL,0,"Cfagent starting",CF_NOP);
   }
}

/*****************************************************************************/

void CloseAuditLog()

{ double total;
  char *sp;
 
total = (double)(PR_KEPT+PR_NOTKEPT+PR_REPAIRED)/100.0;

if (sp = GetMacroValue(CONTEXTID,"cfinputs_version"))
   {
   }
else
   {
   sp = "(not specified)";
   }

if (total == 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Outcome of version %s: No checks were scheduled\n",sp);
   return;
   }
else
   {   
   snprintf(OUTPUT,CF_BUFSIZE,"Outcome of version %s: Promises observed to be kept %.0f%%, Promises repaired %.0f%%, Promises not repaired %.0f\%\n",
            sp,
            (double)PR_KEPT/total,
            (double)PR_REPAIRED/total,
            (double)PR_NOTKEPT/total);
   }

CfLog(cfverbose,OUTPUT,"");
AuditLog('y',NULL,0,OUTPUT,CF_REPORT);

if (AUDIT && AUDITDBP)
   {
   AuditLog('y',NULL,0,"Cfagent closing",CF_NOP);
   AUDITDBP->close(AUDITDBP,0);
   }
}

/*****************************************************************************/

void AuditLog(char yesno,struct Audit *ap,int lineno,char *str,char status)

{ time_t now = time(NULL);
  char date[CF_BUFSIZE],lock[CF_BUFSIZE],key[CF_BUFSIZE],operator[CF_BUFSIZE];
  struct AuditLog newaudit;
  struct timespec t;
  double keyval;

Debug("AuditLog(%s)\n",str);

switch(status)
   {
   case CF_CHG:
       PR_REPAIRED++;
       break;
       
   case CF_WARN:
       PR_NOTKEPT++;
       break;
       
   case CF_TIMEX:
       PR_NOTKEPT++;
       break;

   case CF_FAIL:
       PR_NOTKEPT++;
       break;
       
   case CF_DENIED:
       PR_NOTKEPT++;
       break;
       
   case CF_INTERPT:
       PR_NOTKEPT++;
       break;

   case CF_REGULAR:
       PR_REPAIRED++;
       break;
       
   case CF_NOP:
       PR_KEPT++;
       break;

   case CF_UNKNOWN:
       PR_KEPT++;
       break;
   }

if (AUDITDBP == NULL)
   {
   return;
   }

snprintf(date,CF_BUFSIZE,"%s",ctime(&now));
Chop(date);

if (ap == NULL)
   {
   snprintf(operator,CF_BUFSIZE,"Cycle complete %s, lock acquired",date);
   strncpy(newaudit.operator,operator,CF_AUDIT_COMMENT-1);
   }
else
   {
   ExtractOpLock(lock);
   snprintf(operator,CF_BUFSIZE-1,"[%s] op %s",date,lock);
   strncpy(newaudit.operator,operator,CF_AUDIT_COMMENT-1);
   }

if (clock_gettime(CLOCK_REALTIME,&t) == -1)
   {
   CfLog(cfverbose,"Clock gettime failure during audit transaction","clock_gettime");
   return;
   }

keyval = (double)(t.tv_sec)+(double)(t.tv_nsec)/(double)CF_BILLION;
      
snprintf(key,CF_BUFSIZE-1,"%lf",keyval);

if (DEBUG)
   {
   AuditStatusMessage(status);
   }

if (ap != NULL)
   {
   strncpy(newaudit.comment,str,CF_AUDIT_COMMENT-1);
   strncpy(newaudit.filename,ap->filename,CF_AUDIT_COMMENT-1);
   
   if (ap->version == NULL || strlen(ap->version) == 0)
      {
      Debug("Promised in %s (unamed version last edited at %s) at/before line %d\n",ap->filename,ap->date,lineno);
      newaudit.version[0] = '\0';
      }
   else
      {
      Debug("Promised in %s (version %s last edited at %s) at/before line %d\n",ap->filename,ap->version,ap->date,lineno);
      strncpy(newaudit.version,ap->version,CF_AUDIT_VERSION-1);
      }
   
   strncpy(newaudit.date,ap->date,CF_AUDIT_DATE);
   newaudit.lineno = lineno;
   }
else
   {
   strcpy(newaudit.date,date);
   strcpy(newaudit.comment,str);
   strcpy(newaudit.filename,"schedule");
   strcpy(newaudit.version,"");
   newaudit.lineno = 0;
   }

newaudit.status = status;

if (yesno == 'n')
   {
   return;
   }

WriteDB(AUDITDBP,key,&newaudit,sizeof(newaudit));
}

/*****************************************************************************/

void CfLog(enum cfoutputlevel level,char *string,char *errstr)

{ int endl = false;
  char *sp, buffer[1024];

if ((string == NULL) || (strlen(string) == 0))
   {
   return;
   }

strncpy(buffer,string,1022);
buffer[1023] = '\0'; 

/* Check for %s %m which someone might be able to insert into
   an error message in order to get a syslog buffer overflow...
   bug reported by Pekka Savola */
 
for (sp = buffer; *sp != '\0'; sp++)
   {
   if ((*sp == '%') && (*(sp+1) >= 'a'))
      {
      *sp = '?';
      }
   }


#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
if (!SILENT && (pthread_mutex_lock(&MUTEX_SYSCALL) != 0))
   {
   /* If we can't lock this could be dangerous to proceed with threaded file descriptors */
   return;
   /* CfLog(cferror,"pthread_mutex_lock failed","lock"); would lead to sick recursion */
   }
#endif
 
switch(level)
   {
   case cfsilent:
       if (! SILENT || VERBOSE || DEBUG || D2)
          {
          ShowAction();
          printf("%s: %s",VPREFIX,buffer);
          endl = true;
          }
       break;
       
   case cfinform:
       if (SILENT)
          {
          return;
          }
       
       if (INFORM || VERBOSE || DEBUG || D2)
          {
          ShowAction();
          printf("%s: %s",VPREFIX,buffer);
          endl = true;
          }
       
       if (LOGGING && IsPrivileged() && !DONTDO)
          {
          syslog(LOG_NOTICE, "%s", buffer); 
          
          if ((errstr != NULL) && (strlen(errstr) != 0))
             {
             syslog(LOG_ERR,"%s: %s",errstr,strerror(errno));  
             }
          }
       break;
   
   case cfverbose:
       if (VERBOSE || DEBUG || D2)
          {
          if ((errstr == NULL) || (strlen(errstr) > 0))
             {
             ShowAction();
             printf("%s: %s\n",VPREFIX,buffer);
             printf("%s: %s",VPREFIX,errstr);
             endl = true;
             }
          else
             {
             ShowAction();
             printf("%s: %s",VPREFIX,buffer);
             endl = true;
             }
          }
       break;

   case cfeditverbose:
       if (EDITVERBOSE || DEBUG)
          {
          ShowAction();
          printf("%s: %s",VPREFIX,buffer);
          endl = true;
          }
       break;
       
   case cflogonly: 

       if (LOGGING && IsPrivileged() && !DONTDO)
          {
          syslog(LOG_ERR," %s",buffer);    
          
          if ((errstr != NULL) && (strlen(errstr) > 0))
             {
             syslog(LOG_ERR," %s",errstr);  
             }
          }
       
       break;

   case cfloginform: 

       if (LOGGING && IsPrivileged() && !DONTDO)
          {
          syslog(LOG_INFO," %s",buffer);    
          
          if ((errstr != NULL) && (strlen(errstr) > 0))
             {
             syslog(LOG_INFO," %s",errstr);  
             }
          }
       
       break;

   case cferror:
       printf("# %s",buffer);
       
       if (LOGGING && IsPrivileged() && !DONTDO)
          {   
          syslog(LOG_ERR," %s",buffer);    
          }
       
       if (buffer[strlen(buffer)-1] != '\n')
          {
          printf("\n");
          }
       
       if ((errstr != NULL) && (strlen(errstr) > 0))
          {
          ShowAction();
          printf("# %s\n",errstr,strerror(errno));
          endl = true;
          }
   }

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
 if (pthread_mutex_unlock(&MUTEX_SYSCALL) != 0)
    {
    /* CfLog(cferror,"pthread_mutex_unlock failed","lock");*/
    }
#endif 
 
 
 if (endl && (buffer[strlen(buffer)-1] != '\n'))
    {
    printf("\n");
    }
}

/*****************************************************************************/

void ResetOutputRoute (char log,char inform)

{
if ((log == 'y') || (log == 'n') || (inform == 'y') || (inform == 'n'))
   {
   INFORM_save = INFORM;
   LOGGING_save = LOGGING;
   
   switch (log)
      {
      case 'y': LOGGING = true;
         break;
      case 'n': LOGGING = false;
         break;
      }

   switch (inform)
      {
      case 'y': INFORM = true;
         break;
      case 'n': INFORM = false;
         break;
      }
   }
else
   {
   INFORM = INFORM_save;
   LOGGING = LOGGING_save;
   }
}

/*****************************************************************************/

void ShowAction()

{
if (SHOWACTIONS)
   {
   printf("%s:",CFLOCK);
   }
}

/*****************************************************************************/

void AuditStatusMessage(char status)

{
switch (status) /* Reminder */
   {
   case CF_CHG:
       printf("made a system correction\n");
       break;
       
   case CF_WARN:
       printf("promise not kept, no action taken");
       break;
       
   case CF_TIMEX:
       printf("timed out\n");
       break;

   case CF_FAIL:
       printf("failed to make a correction\n");
       break;
       
   case CF_DENIED:
       printf("was denied access to an essential resource\n");
       break;
       
   case CF_INTERPT:
       printf("was interrupted\n");
       break;

   case CF_REGULAR:
       printf("was a regular (repeatable) maintenance task");
       break;
       
   case CF_NOP:
       printf("was applied but performed no required actions\n");
       break;

   case CF_UNKNOWN:
       printf("was applied but status unknown\n");
       break;

   case CF_REPORT:
       printf("report\n");
       break;
   }

}

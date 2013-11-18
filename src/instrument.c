
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
/* File: instrument.c                                                        */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"
#include <math.h>

# if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
pthread_mutex_t MUTEX_GETADDR = PTHREAD_MUTEX_INITIALIZER;
# endif

/* Alter this code at your peril. Berkeley DB is very sensitive to errors. */

/***************************************************************/

void RecordPerformance(char *eventname,time_t t,double value)

{ DB *dbp;
  DB_ENV *dbenv = NULL;
  char name[CF_BUFSIZE];
  struct Event e,newe;
  double lastseen,delta2;
  int lsea = CF_WEEK;
  time_t now = time(NULL);

Debug("PerformanceEvent(%s,%.1f s)\n",eventname,value);

snprintf(name,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,CF_PERFORMANCE);

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open performance database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = (dbp->open)(dbp,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = (dbp->open)(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open performance database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

if (ReadDB(dbp,eventname,&e,sizeof(e)))
   {
   lastseen = now - e.t;
   newe.t = t;
   newe.Q.q = value;
   newe.Q.expect = GAverage(value,e.Q.expect,0.3);
   delta2 = (value - e.Q.expect)*(value - e.Q.expect);
   newe.Q.var = GAverage(delta2,e.Q.var,0.3);

   /* Have to kickstart variance computation, assume 1% to start  */
   
   if (newe.Q.var <= 0.0009)
      {
      newe.Q.var =  newe.Q.expect / 100.0;
      }
   }
else
   {
   lastseen = 0.0;
   newe.t = t;
   newe.Q.q = value;
   newe.Q.expect = value;
   newe.Q.var = 0.001;
   }

if (lastseen > (double)lsea)
   {
   Verbose("Performance record %s expired\n",eventname);
   DeleteDB(dbp,eventname);   
   }
else
   {
   Verbose("Performance(%s): time=%.4f secs, av=%.4f +/- %.4f\n",eventname,value,newe.Q.expect,sqrt(newe.Q.var));
   WriteDB(dbp,eventname,&newe,sizeof(newe));
   }

dbp->close(dbp,0);
}

/***************************************************************/

void RecordClassUsage()

{ DB *dbp;
  DB_ENV *dbenv = NULL;
  DBC *dbcp;
  DBT key,stored;
  char name[CF_BUFSIZE];
  struct Event e,entry,newe;
  double lsea = CF_WEEK * 52; /* expire after a year */
  time_t now = time(NULL);
  struct Item *ip,*list = NULL;
  double lastseen,delta2;
  double vtrue = 1.0;      /* end with a rough probability */

Debug("RecordClassUsage\n");

for (ip = VHEAP; ip != NULL; ip=ip->next)
   {
   if (!IsItemIn(list,ip->name))
      {
      PrependItem(&list,ip->name,NULL);
      }
   }

for (ip = VALLADDCLASSES; ip != NULL; ip=ip->next)
   {
   if (!IsItemIn(list,ip->name))
      {
      PrependItem(&list,ip->name,NULL);
      }
   }
   
snprintf(name,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,CF_CLASSUSAGE);

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open performance database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = (dbp->open)(dbp,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = (dbp->open)(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open performance database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

/* First record the classes that are in use */

for (ip = list; ip != NULL; ip=ip->next)
   {
   if (ReadDB(dbp,ip->name,&e,sizeof(e)))
      {
      lastseen = now - e.t;
      newe.t = now;
      newe.Q.q = vtrue;
      newe.Q.expect = GAverage(vtrue,e.Q.expect,0.5);
      delta2 = (vtrue - e.Q.expect)*(vtrue - e.Q.expect);
      newe.Q.var = GAverage(delta2,e.Q.var,0.5);
      }
   else
      {
      lastseen = 0.0;
      newe.t = now;
      newe.Q.q = 0.5*vtrue;
      newe.Q.expect = 0.5*vtrue;  /* With no data it's 50/50 what we can say */
      newe.Q.var = 0.000;
      }
   
   if (lastseen > lsea)
      {
      Verbose("Class usage record %s expired\n",ip->name);
      DeleteDB(dbp,ip->name);   
      }
   else
      {
      Debug("Upgrading %s %f\n",ip->name,newe.Q.expect);
      WriteDB(dbp,ip->name,&newe,sizeof(newe));
      }
   }

/* Then update with zero the ones we know about that are not active */

/* Acquire a cursor for the database. */

if ((errno = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
   {
   Debug("Error reading from class database: ");
   dbp->err(dbp, errno, "DB->cursor");
   return;
   }

 /* Initialize the key/data return pair. */
 
memset(&key, 0, sizeof(key));
memset(&stored, 0, sizeof(stored));
memset(&entry, 0, sizeof(entry)); 

while (dbcp->c_get(dbcp, &key, &stored, DB_NEXT) == 0)
   {
   double measure,av,var;
   time_t then;
   char tbuf[CF_BUFSIZE],eventname[CF_BUFSIZE];

   strcpy(eventname,(char *)key.data);

   if (stored.data != NULL)
      {
      memcpy(&entry,stored.data,sizeof(entry));
      
      then    = entry.t;
      measure = entry.Q.q;
      av = entry.Q.expect;
      var = entry.Q.var;
      lastseen = now - then;
            
      snprintf(tbuf,CF_BUFSIZE-1,"%s",ctime(&then));
      tbuf[strlen(tbuf)-9] = '\0';                     /* Chop off second and year */

      if (lastseen > lsea)
         {
         Verbose("Class usage record %s expired\n",eventname);
         DeleteDB(dbp,eventname);   
         }
      else if (!IsItemIn(list,eventname))
         {
         newe.t = then;
         newe.Q.q = 0;
         newe.Q.expect = GAverage(0.0,av,0.5);
         delta2 = av*av;
         newe.Q.var = GAverage(delta2,var,0.5);
         Debug("Downgrading class %s from %lf to %lf\n",eventname,entry.Q.expect,newe.Q.expect);
         WriteDB(dbp,eventname,&newe,sizeof(newe));         
         }
      }
   }

dbp->close(dbp,0);
}

/***************************************************************/

void LastSeen(char *hostname,enum roles role)

{ DB *dbp,*dbpent;
  DB_ENV *dbenv = NULL, *dbenv2 = NULL;
  char name[CF_BUFSIZE],databuf[CF_BUFSIZE];
  time_t now = time(NULL);
  struct QPoint q,newq;
  double lastseen,delta2;
  int lsea = -1;

if (strlen(hostname) == 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"LastSeen registry for empty hostname with role %d",role);
   CfLog(cflogonly,OUTPUT,"");
   return;
   }

Debug("LastSeen(%s) reg\n",hostname);

/* Tidy old versions - temporary */
snprintf(name,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,CF_OLDLASTDB_FILE);
unlink(name);

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't init last-seen database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

snprintf(name,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,CF_LASTDB_FILE);

#ifdef CF_OLD_DB
if ((errno = (dbp->open)(dbp,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = (dbp->open)(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open last-seen database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

/* Now open special file for peer entropy record - INRIA intermittency */
snprintf(name,CF_BUFSIZE-1,"%s/%s.%s",CFWORKDIR,CF_LASTDB_FILE,hostname);

if ((errno = db_create(&dbpent,dbenv2,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't init last-seen database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = (dbpent->open)(dbpent,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = (dbpent->open)(dbpent,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open last-seen database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }


#ifdef HAVE_PTHREAD_H  
if (pthread_mutex_lock(&MUTEX_GETADDR) != 0)
   {
   CfLog(cferror,"pthread_mutex_lock failed","unlock");
   exit(1);
   }
#endif

switch (role)
   {
   case cf_accept:
       snprintf(databuf,CF_BUFSIZE-1,"-%s",Hostname2IPString(hostname));
       break;
   case cf_connect:
       snprintf(databuf,CF_BUFSIZE-1,"+%s",Hostname2IPString(hostname));
       break;
   }

#ifdef HAVE_PTHREAD_H  
if (pthread_mutex_unlock(&MUTEX_GETADDR) != 0)
   {
   CfLog(cferror,"pthread_mutex_unlock failed","unlock");
   exit(1);
   }
#endif


if (GetMacroValue(CONTEXTID,"LastSeenExpireAfter"))
   {
   lsea = atoi(GetMacroValue(CONTEXTID,"LastSeenExpireAfter"));
   lsea *= CF_TICKS_PER_DAY;
   }

if (lsea < 0)
   {
   lsea = CF_WEEK;
   }
   
if (ReadDB(dbp,databuf,&q,sizeof(q)))
   {
   lastseen = (double)now - q.q;
   newq.q = (double)now;                   /* Last seen is now-then */
   newq.expect = GAverage(lastseen,q.expect,0.3);
   delta2 = (lastseen - q.expect)*(lastseen - q.expect);
   newq.var = GAverage(delta2,q.var,0.3);
   }
else
   {
   lastseen = 0.0;
   newq.q = (double)now;
   newq.expect = 0.0;
   newq.var = 0.0;
   }

#ifdef HAVE_PTHREAD_H  
if (pthread_mutex_lock(&MUTEX_GETADDR) != 0)
   {
   CfLog(cferror,"pthread_mutex_lock failed","unlock");
   exit(1);
   }
#endif

if (lastseen > (double)lsea)
   {
   Verbose("Last seen %s expired\n",databuf);
   DeleteDB(dbp,databuf);   
   }
else
   {
   WriteDB(dbp,databuf,&newq,sizeof(newq));
   WriteDB(dbpent,GenTimeKey(now),&newq,sizeof(newq));
   }

#ifdef HAVE_PTHREAD_H  
if (pthread_mutex_unlock(&MUTEX_GETADDR) != 0)
   {
   CfLog(cferror,"pthread_mutex_unlock failed","unlock");
   exit(1);
   }
#endif

dbp->close(dbp,0);
dbpent->close(dbpent,0);
}

/***************************************************************/

void CheckFriendConnections(int hours)

/* Go through the database of recent connections and check for
   Long Time No See ...*/

{ DBT key,value;
  DB *dbp;
  DBC *dbcp;
  DB_ENV *dbenv = NULL;
  int ret, secs = CF_TICKS_PER_HOUR*hours, criterion, overdue, regex=false;
  time_t now = time(NULL),lsea = -1, tthen, then = 0;
  char name[CF_BUFSIZE],hostname[CF_BUFSIZE],datebuf[CF_MAXVARSIZE];
  char addr[CF_BUFSIZE],type[CF_BUFSIZE], *regexp;
  struct QPoint entry;
  double average = 0.0, var = 0.0, ticksperminute = 60.0;
  double ticksperhour = (double)CF_TICKS_PER_HOUR,ticksperday = (double)CF_TICKS_PER_DAY;
  regex_t rx,rxcache;
  regmatch_t pmatch;

if (regexp = GetMacroValue(CONTEXTID,"IgnoreFriendRegex"))
   {
   Verbose("IgnoreFriendRegex %s\n\n",regexp);
   if ((ret = regcomp(&rx,regexp,REG_EXTENDED)) != 0)
      {
      regerror(ret,&rx,name,1023);
      snprintf(OUTPUT,CF_BUFSIZE,"Regular expression error %d for %s: %s\n",ret,regexp,name);
      CfLog(cfinform,OUTPUT,"");
      regex = false;
      }
   else
      {
      regex = true;
      }
   }
 
Verbose("CheckFriendConnections(%d)\n",hours);
snprintf(name,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,CF_LASTDB_FILE);

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open last-seen database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = (dbp->open)(dbp,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = (dbp->open)(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open last-seen database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

/* Acquire a cursor for the database. */

if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
   {
   CfLog(cferror,"Error reading from last-seen database","");
   dbp->err(dbp, ret, "DB->cursor");
   return;
   }

 /* Walk through the database and print out the key/data pairs. */

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

while (dbcp->c_get(dbcp, &key, &value, DB_NEXT) == 0)
   {
   memset(&entry, 0, sizeof(entry)); 

   strcpy(hostname,(char *)key.data);

   if (value.data != NULL)
      {
      memcpy(&entry,value.data,sizeof(entry));
      then = (time_t)entry.q;
      average = (double)entry.expect;
      var = (double)entry.var;
      }
   else
      {
      continue;
      }

   /* Got data, now get expiry criterion */

   if (secs == 0)
      {
      /* Twice the average delta is significant */
      criterion = (now - then > (int)(average+2.0*sqrt(var)+0.5));
      overdue = now - then - (int)(average);
      }
   else
      {
      criterion = (now - then > secs);
      overdue =  (now - then - secs);
      }

   if (GetMacroValue(CONTEXTID,"LastSeenExpireAfter"))
      {
      lsea = atoi(GetMacroValue(CONTEXTID,"LastSeenExpireAfter"));
      lsea *= CF_TICKS_PER_DAY;
      }

   if (lsea < 0)
      {
      lsea = (time_t)CF_WEEK/7;
      }

   if (regex)
      {
      if (regexec(&rx,IPString2Hostname(hostname+1),1,&pmatch,0) == 0)
         {
         if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(hostname+1)))
            {
            Verbose("Not judging friend %s\n",hostname);
            criterion = false;
            lsea = CF_INFINITY;
            }
         }
      }
   
   tthen = (time_t)then;

   snprintf(datebuf,CF_BUFSIZE-1,"%s",ctime(&tthen));
   datebuf[strlen(datebuf)-9] = '\0';                     /* Chop off second and year */

   snprintf(addr,15,"%s",hostname+1);

   switch(*hostname)
      {
      case '+':
          snprintf(type,CF_BUFSIZE,"last responded to hails");
          break;
      case'-':
          snprintf(type,CF_BUFSIZE,"last hailed us");
          break;
      }

   snprintf(OUTPUT,CF_BUFSIZE,"Host %s i.e. %s %s @ [%s] (overdue by %d mins)",
            IPString2Hostname(hostname+1),
            addr,
            type,
            datebuf,
            overdue/(int)ticksperminute);

   if (criterion)
      {
      CfLog(cferror,OUTPUT,"");
      }
   else
      {
      CfLog(cfverbose,OUTPUT,"");
      }

   snprintf(OUTPUT,CF_BUFSIZE,"i.e. (%.2f) hrs ago, Av %.2f +/- %.2f hrs\n",
            ((double)(now-then))/ticksperhour,
            average/ticksperhour,
            sqrt(var)/ticksperhour);
   
   if (criterion)
      {
      CfLog(cferror,OUTPUT,"");
      }
   else
      {
      CfLog(cfverbose,OUTPUT,"");
      }
   
   if ((now-then) > lsea)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Giving up on host %s -- too long since last seen",IPString2Hostname(hostname+1));
      CfLog(cferror,OUTPUT,"");
      DeleteDB(dbp,hostname);
      }

   memset(&value,0,sizeof(value));
   memset(&key,0,sizeof(key)); 
   }
 
dbcp->c_close(dbcp);
dbp->close(dbp,0);
}


/***************************************************************/

void CheckFriendReliability()

{ DBT key,value;
  DB *dbp,*dbpent;
  DBC *dbcp;
  DB_ENV *dbenv = NULL, *dbenv2 = NULL;
  int i,ret;
  double n[CF_RELIABLE_CLASSES],n_av[CF_RELIABLE_CLASSES],total;
  double p[CF_RELIABLE_CLASSES],p_av[CF_RELIABLE_CLASSES];
  char name[CF_BUFSIZE],hostname[CF_BUFSIZE],timekey[CF_MAXVARSIZE];
  struct QPoint entry;
  struct Item *ip, *hostlist = NULL;
  double entropy,average,var,sum,sum_av,expect,actual;
  time_t now = time(NULL), then, lastseen = CF_WEEK;

Verbose("CheckFriendReliability()\n");
snprintf(name,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,CF_LASTDB_FILE);

average = (double) CF_HOUR;  /* It will take a week for a host to be deemed reliable */
var = 0;

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open last-seen database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = (dbp->open)(dbp,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = (dbp->open)(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open last-seen database %s\n",name);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
   {
   CfLog(cferror,"Error reading from last-seen database","");
   dbp->err(dbp, ret, "DB->cursor");
   return;
   }

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

while (dbcp->c_get(dbcp, &key, &value, DB_NEXT) == 0)
   {
   strcpy(hostname,IPString2Hostname((char *)key.data+1));

   if (!IsItemIn(hostlist,hostname))
      {
      /* Check hostname not recorded twice with +/- */
      AppendItem(&hostlist,hostname,NULL);
      Verbose(" Measuring reliability of %s\n",hostname);
      }
   }

dbcp->c_close(dbcp);
dbp->close(dbp,0);

/* Now go through each host and recompute entropy */

for (ip = hostlist; ip != NULL; ip=ip->next)
   {
   snprintf(name,CF_BUFSIZE-1,"%s/%s.%s",CFWORKDIR,CF_LASTDB_FILE,ip->name);

   if ((errno = db_create(&dbpent,dbenv2,0)) != 0)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't init reliability profile database %s\n",name);
      CfLog(cferror,OUTPUT,"db_open");
      return;
      }
   
#ifdef CF_OLD_DB
   if ((errno = (dbpent->open)(dbpent,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
   if ((errno = (dbpent->open)(dbpent,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open last-seen database %s\n",name);
      CfLog(cferror,OUTPUT,"db_open");
      continue;
      }

   for (i = 0; i < CF_RELIABLE_CLASSES; i++)
      {
      n[i] = n_av[i] = 0.0;
      }

   total = 0.0;

   for (now = CF_MONDAY_MORNING; now < CF_MONDAY_MORNING+CF_WEEK; now += CF_MEASURE_INTERVAL)
      {
      memset(&key,0,sizeof(key));       
      memset(&value,0,sizeof(value));
      
      strcpy(timekey,GenTimeKey(now));
      
      key.data = timekey;
      key.size = strlen(timekey)+1;

      if ((errno = dbp->get(dbp,NULL,&key,&value,0)) != 0)
         {
         if (errno != DB_NOTFOUND)
            {
            dbp->err(dbp,errno,NULL);
            exit(1);
            }
         }
      
      if (value.data != NULL)
         {
         memcpy(&entry,value.data,sizeof(entry));
         then = (time_t)entry.q;
         lastseen = now - then;
         if (lastseen < 0)
            {
            lastseen = 0; /* Never seen before, so pretend */
            }
         average = (double)entry.expect;
         var = (double)entry.var;
         Debug("%s => then = %ld, lastseen = %ld, average=%.2f\n",hostname,then,lastseen,average);
         }
      else
         {
         /* If we have no data, it means no contact for whatever reason.
            It could be unable to respond unwilling to respond, policy etc.
            Assume for argument that we expect regular responses ... */
         
         lastseen += CF_MEASURE_INTERVAL; /* infer based on no data */
         }

      for (i = 0; i < CF_RELIABLE_CLASSES; i++)
         {
         if (lastseen >= i*CF_HOUR && lastseen < (i+1)*CF_HOUR)
            {
            n[i]++;
            }
         
         if (average >= (double)(i*CF_HOUR) && average < (double)((i+1)*CF_HOUR))
            {
            n_av[i]++;
            }
         }
       
      total++;
      }

   sum = sum_av = 0.0;
   
   for (i = 0; i < CF_RELIABLE_CLASSES; i++)
      {
      p[i]    = n[i]/total;
      p_av[i] = n_av[i]/total;
      sum += p[i];
      sum_av += p_av[i];
      }

   Debug("Reliabilities sum to %.2f av %.2f\n\n",sum,sum_av);

   sum = sum_av = 0.0;
   
   for (i = 0; i < CF_RELIABLE_CLASSES; i++)
      {
      if (p[i] == 0.0)
         {
         continue;
         }
      sum -= p[i] * log(p[i]);
      }

   for (i = 0; i < CF_RELIABLE_CLASSES; i++)
      {
      if (p_av[i] == 0.0)
         {
         continue;
         }
      sum_av -= p_av[i] * log(p_av[i]);
      }

   actual = sum/log((double)CF_RELIABLE_CLASSES)*100.0;
   expect = sum_av/log((double)CF_RELIABLE_CLASSES)*100.0;
   
   Verbose("Scaled entropy for %s = %.1f %%\n",ip->name,actual);
   Verbose("Expected entropy for %s = %.1f %%\n\n",ip->name,expect);

   if (actual > expect)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"The reliability of %s has decreased!\n",ip->name);
      CfLog(cfinform,OUTPUT,"");
      }

   if (actual > 50.0)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"FriendStatus reports the intermittency of %s above 50%% (scaled entropy units)\n",ip->name);
      CfLog(cferror,OUTPUT,"");
      }

   if (expect > actual)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"The reliability of %s seems to be improving!\n",ip->name);
      CfLog(cfinform,OUTPUT,"");
      }
   
   dbpent->close(dbpent,0);
   }

DeleteItemList(hostlist);
}

/*****************************************************************************/
/* level 1                                                                   */
/*****************************************************************************/

int ReadDB(DB *dbp,char *name,void *ptr,int size)

{ DBT *key,value;
  
key = NewDBKey(name);
memset(&value,0,sizeof(DBT));

if ((errno = dbp->get(dbp,NULL,key,&value,0)) == 0)
   {
   memset(ptr,0,size);
   memcpy(ptr,value.data,size);
   
   Debug("READ %s\n",name);
   DeleteDBKey(key);
   return true;
   }
else
   {
   Debug("Database read failed: %s",db_strerror(errno));
   return false;
   }
}

/*****************************************************************************/

int WriteDB(DB *dbp,char *name,void *ptr,int size)

{ DBT *key,*value;
 
key = NewDBKey(name); 
value = NewDBValue(ptr,size);

if ((errno = dbp->put(dbp,NULL,key,value,0)) != 0)
   {
   Debug("Database write failed: %s",db_strerror(errno));
   DeleteDBKey(key);
   DeleteDBValue(value);
   return false;
   }
else
   {
   Debug("WriteDB => %s\n",name);

   DeleteDBKey(key);
   DeleteDBValue(value);
   return true;
   }
}

/*****************************************************************************/

void DeleteDB(DB *dbp,char *name)

{ DBT *key;

key = NewDBKey(name);

if ((errno = dbp->del(dbp,NULL,key,0)) != 0)
   {
   Debug("Database deletion failed: %s",db_strerror(errno));
   }

DeleteDBKey(key);
Debug("DELETED DB %s\n",name);
}


/*****************************************************************************/
/* Level 2                                                                   */
/*****************************************************************************/

DBT *NewDBKey(char *name)

{ char *dbkey;
  DBT *key;

if ((dbkey = malloc(strlen(name)+1)) == NULL)
   {
   FatalError("NewChecksumKey malloc error");
   }

if ((key = (DBT *)malloc(sizeof(DBT))) == NULL)
   {
   FatalError("DBT  malloc error");
   }

memset(key,0,sizeof(DBT));
memset(dbkey,0,strlen(name)+1);

strncpy(dbkey,name,strlen(name));

key->data = (void *)dbkey;
key->size = strlen(name)+1;

return key;
}

/*****************************************************************************/

void DeleteDBKey(DBT *key)

{
free((char *)key->data);
free((char *)key);
}

/*****************************************************************************/

DBT *NewDBValue(void *ptr,int size)

{ void *val;
  DBT *value;

if ((val = (void *)malloc(size)) == NULL)
   {
   FatalError("NewDBKey malloc error");
   }

if ((value = (DBT *) malloc(sizeof(DBT))) == NULL)
   {
   FatalError("DBT Value malloc error");
   }

memset(value,0,sizeof(DBT)); 
memset(val,0,size);
memcpy(val,ptr,size);

value->data = val;
value->size = size;

return value;
}

/*****************************************************************************/

void DeleteDBValue(DBT *value)

{
free((char *)value->data);
free((char *)value);
}

/*****************************************************************************/
/* Toolkit                                                                   */
/*****************************************************************************/

double GAverage(double anew,double aold,double p)

/* return convex mixture - p is the trust in the new value */
    
{
return (p*anew + (1-p)*aold);
}

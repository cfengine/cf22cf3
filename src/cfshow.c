/*****************************************************************************/
/*                                                                           */
/* File: cfshow.c                                                            */
/*                                                                           */
/* Created: Wed Sep 22 16:47:23 2004                                         */
/*                                                                           */
/* Author: Mark                                                              */
/*                                                                           */
/* Description: Print data from the Berkeeley databases in reable form       */
/*                                                                           */
/*****************************************************************************/

#include "../pub/getopt.h"
#include "cf.defs.h"
#include "cf.extern.h"

/*******************************************************************/
/* GLOBAL VARIABLES                                                */
/*******************************************************************/

struct option CFSHOPTIONS[] =
   {
   { "help",no_argument,0,'h' },
   { "debug",optional_argument,0,'d' }, 
   { "verbose",no_argument,0,'v' },
   { "locks",no_argument,0,'l'},
   { "last-seen",no_argument,0,'s'},
   { "performance",no_argument,0,'p'},
   { "checksum",no_argument,0,'c'},
   { "active",no_argument,0,'a'},
   { "classes",no_argument,0,'C'},
   { "version",no_argument,0,'V'},
   { "html",no_argument,0,'H'},
   { "xml",no_argument,0,'X'},
   { "purge",no_argument,0,'P'},
   { "audit",no_argument,0,'A'},
   { "regex",required_argument,0,'r'},
   { "filename",required_argument,0,'f'},
   { NULL,0,0,0 }
   };

enum databases
   {
   cf_db_lastseen,
   cf_db_locks,
   cf_db_active,
   cf_db_checksum,
   cf_db_performance,
   cf_db_audit,
   cf_db_classes,
   cf_db_regex
   };

enum databases TODO = -1;

#define CF_ACTIVE 1
#define CF_INACTIVE 0

/*******************************************************************/

struct CEnt /* For sorting */
   {
   char name[256];
   char date[32];
   double q;
   double d;
   };

/*******************************************************************/

enum cf_formatindex
   {
   cfb,
   cfe,
   };

enum cf_format
   {
   cfx_entry,
   cfx_event,
   cfx_host,
   cfx_pm,
   cfx_ip,
   cfx_date,
   cfx_q,
   cfx_av,
   cfx_dev,
   cfx_version,
   cfx_ref,
   cfx_filename,
   cfx_index
   };

short XML = false;

char *CFX[][2] =
   {
    "<entry>\n","\n</entry>\n",
    "<event>\n","\n</event>\n",
    "<hostname>\n","\n</hostname>\n",
    "<pm>\n","\n</pm>\n",
    "<ip>\n","\n</ip>\n",
    "<date>\n","\n</date>\n",
    "<q>\n","\n</q>\n",
    "<expect>\n","\n</expect>\n",
    "<sigma>\n","\n</sigma>\n",
    "<version>\n","\n</version>\n",
    "<ref>\n","\n</ref>\n",
    "<filename>\n","\n</filename>\n",
    "<index>\n","\n</index>\n",
    NULL,NULL
   };

short HTML = false;

char *CFH[][2] =
   {
    "<tr>","</tr>\n\n",
    "<td>","</td>\n",
    "<td>","</td>\n",
    "<td bgcolor=#add8e6>","</td>\n",
    "<td bgcolor=#e0ffff>","</td>\n",
    "<td bgcolor=#f0f8ff>","</td>\n",
    "<td bgcolor=#fafafa>","</td>\n",
    "<td bgcolor=#ededed>","</td>\n",
    "<td bgcolor=#e0e0e0>","</td>\n",
    "<td bgcolor=#add8e6>","</td>\n",
    "<td bgcolor=#e0ffff>","</td>\n",
    "<td bgcolor=#fafafa><small>","</small></td>\n",
    "<td bgcolor=#fafafa><small>","</small></td>\n",
    NULL,NULL
   };

/*******************************************************************/
/* Functions internal to cfshow.c                                  */
/*******************************************************************/

void CheckOptsAndInit (int argc,char **argv);
void Syntax (void);
void PrintDB(void);
void ShowLastSeen(void);
void ShowChecksums(void);
void ShowClasses(void);
void ShowRegex(char *regex);
void ShowLocks(int active);
void ShowPerformance(void);
void ShowCurrentAudit(void);
char *ChecksumDump(unsigned char digest[EVP_MAX_MD_SIZE+1]);
char *Format(char *s,int width);
int CompareClasses(const void *a, const void *b);

/*******************************************************************/
/* Level 0 : Main                                                  */
/*******************************************************************/

int main (int argc,char **argv)

{
CheckOptsAndInit(argc,argv);

PrintDB();
return 0;
}

/********************************************************************/
/* Level 1                                                          */
/********************************************************************/

void CheckOptsAndInit(int argc,char **argv)

{ extern char *optarg;
  int optindex = 0;
  int c;

PURGE = 'n';
AUDIT = false;

while ((c=getopt_long(argc,argv,"AChdvaVlr:f:scpPXH",CFSHOPTIONS,&optindex)) != EOF)
  {
  switch ((char) c)
      {
      case 'd': 

          switch ((optarg==NULL)?3:*optarg)
             {
             case '1': D1 = true;
                 break;
             case '2': D2 = true;
                 break;
             default:  DEBUG = true;
                 break;
             }
          
          VERBOSE = true;
          printf("cfshow Debug mode: running in foreground\n");
          break;
          
      case 'v': VERBOSE = true;
         break;

      case 'V': printf("GNU %s-%s db tool\n%s\n",PACKAGE,VERSION,COPYRIGHT);
          printf("This program is covered by the GNU Public License and may be\n");
          printf("copied free of charge. No warrenty is implied.\n\n");
          exit(0);
          break;
          
      case 'a':
          TODO = cf_db_active;
          break;

      case 'A':
          AUDIT = true;
          TODO = cf_db_audit;
          break;

      case 'l':
          TODO = cf_db_locks;
          break;

      case 's':
          TODO = cf_db_lastseen;
          break;

      case 'c':
          TODO = cf_db_checksum;
          break;

      case 'C':
          TODO = cf_db_classes;
          break;

      case 'p':
          TODO = cf_db_performance;
          break;

      case 'X':
          XML = true;
          break;

      case 'H':
          HTML = true;
          break;

      case 'P':
          PURGE = 'y';
          break;

      case'f':
          strncpy(VINPUTFILE,optarg, CF_BUFSIZE-1);
          VINPUTFILE[CF_BUFSIZE-1] = '\0';
          MINUSF = true;
          printf("Looking for %s\n",VINPUTFILE);
          break;
          
      case 'r':
          ShowRegex(optarg);
          TODO = cf_db_regex;
          break;

      default:  Syntax();
          exit(1);
          
      }
  }

strcpy(CFWORKDIR,WORKDIR);

#ifndef NT
if (getuid() > 0)
   {
   char *homedir;
   
   if ((homedir = getenv("HOME")) != NULL)
      {
      strcpy(CFWORKDIR,homedir);
      strcat(CFWORKDIR,"/.cfagent");
      }
   }
#endif

GetNameInfo();
strcpy(VFQNAME,VSYSNAME.nodename);
}

/********************************************************************/

void PrintDB()

{
switch (TODO)
   {
   case cf_db_lastseen:
       ShowLastSeen();
       break;
   case cf_db_locks:
       ShowLocks(CF_INACTIVE);
       break;
   case cf_db_active:
       ShowLocks(CF_ACTIVE);
       break;
   case cf_db_checksum:
       ShowChecksums();
       break;
   case cf_db_performance:
       ShowPerformance();
       break;
   case cf_db_audit:
       ShowCurrentAudit();
       break;
   case cf_db_classes:
       ShowClasses();
       break;
   default:
       break;
   }
}

/*********************************************************************/
/* Level 2                                                           */
/*********************************************************************/

void Syntax()

{ int i;

printf("GNU cfengine db tool\n%s-%s\n%s\n",PACKAGE,VERSION,COPYRIGHT);
printf("\n");
printf("Options:\n\n");

for (i=0; CFSHOPTIONS[i].name != NULL; i++)
   {
   printf("--%-20s    (-%c)\n",CFSHOPTIONS[i].name,(char)CFSHOPTIONS[i].val);
   }

printf("\nBug reports to bug-cfengine@cfengine.org\n");
printf("General help to help-cfengine@cfengine.org\n");
printf("Info & fixes at http://www.cfengine.org\n");
}

/*******************************************************************/

void ShowLastSeen()

{ DBT key,value;
  DB *dbp;
  DBC *dbcp;
  DB_ENV *dbenv = NULL;
  double now = (double)time(NULL),average = 0, var = 0;
  double ticksperhr = (double)CF_TICKS_PER_HOUR;
  char name[CF_BUFSIZE],hostname[CF_BUFSIZE];
  struct QPoint entry;
  int ret;
  
snprintf(name,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,CF_LASTDB_FILE);

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   printf("Couldn't open last-seen database %s\n",name);
   perror("db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = (dbp->open)(dbp,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = (dbp->open)(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   printf("Couldn't open last-seen database %s\n",name);
   perror("db_open");
   dbp->close(dbp,0);
   return;
   }

if (HTML)
   {
   printf("<html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"http://www.cfengine.org/menus.css\" /><link rel=\"stylesheet\" type=\"text/css\" href=\"http://www.cfengine.org/cf_blue.css\"/></head><body><h1>Peers recently seen by %s</h1><p><table class=border cellpadding=5>",VFQNAME);
   }

if (XML)
   {
   printf("<?xml version=\"1.0\"?>\n<output>\n");
   }

/* Acquire a cursor for the database. */

if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
   {
   printf("Error reading from last-seen database: ");
   dbp->err(dbp, ret, "DB->cursor");
   return;
   }

 /* Initialize the key/data return pair. */

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));
memset(&entry, 0, sizeof(entry)); 
 
 /* Walk through the database and print out the key/data pairs. */

while (dbcp->c_get(dbcp, &key, &value, DB_NEXT) == 0)
   {
   double then;
   time_t fthen;
   char tbuf[CF_BUFSIZE],addr[CF_BUFSIZE];

   memcpy(&then,value.data,sizeof(then));
   strcpy(hostname,(char *)key.data);

   if (value.data != NULL)
      {
      memcpy(&entry,value.data,sizeof(entry));
      then = entry.q;
      average = (double)entry.expect;
      var = (double)entry.var;
      }
   else
      {
      continue;
      }

   if (PURGE == 'y')
      {
      if (now - then > CF_WEEK)
         {
         if ((errno = dbp->del(dbp,NULL,&key,0)) != 0)
            {
            CfLog(cferror,"","db_store");
            }
         }

      fprintf(stderr,"Deleting expired entry for %s\n",hostname);
      continue;
      }
   
   fthen = (time_t)then;                            /* format date */
   snprintf(tbuf,CF_BUFSIZE-1,"%s",ctime(&fthen));
   tbuf[strlen(tbuf)-9] = '\0';                     /* Chop off second and year */

   if (strlen(hostname+1) > 15)
      {
      snprintf(addr,15,"...%s",hostname+strlen(hostname)-10); /* ipv6 */
      }
   else
      {
      snprintf(addr,15,"%s",hostname+1);
      }

   if (XML)
      {
      printf("%s",CFX[cfx_entry][cfb]);
      printf("%s%c%s",CFX[cfx_pm][cfb],*hostname,CFX[cfx_pm][cfe]);
      printf("%s%s%s",CFX[cfx_host][cfb],IPString2Hostname(hostname+1),CFX[cfx_host][cfe]);
      printf("%s%s%s",CFX[cfx_ip][cfb],hostname+1,CFX[cfx_ip][cfe]);
      printf("%s%s%s",CFX[cfx_date][cfb],tbuf,CFX[cfx_date][cfe]);
      printf("%s%.2f%s",CFX[cfx_q][cfb],((double)(now-then))/ticksperhr,CFX[cfx_q][cfe]);
      printf("%s%.2f%s",CFX[cfx_av][cfb],average/ticksperhr,CFX[cfx_av][cfe]);
      printf("%s%.2f%s",CFX[cfx_dev][cfb],sqrt(var)/ticksperhr,CFX[cfx_dev][cfe]);
      printf("%s",CFX[cfx_entry][cfe]);
      }
   else if (HTML)
      {
      printf("%s",CFH[cfx_entry][cfb]);
      printf("%s%c%s",CFH[cfx_pm][cfb],*hostname,CFH[cfx_pm][cfe]);
      printf("%s%s%s",CFH[cfx_host][cfb],IPString2Hostname(hostname+1),CFH[cfx_host][cfe]);
      printf("%s%s%s",CFH[cfx_ip][cfb],hostname+1,CFH[cfx_ip][cfe]);
      printf("%s Last seen at %s%s",CFH[cfx_date][cfb],tbuf,CFH[cfx_date][cfe]);
      printf("%s %.2f hrs ago %s",CFH[cfx_q][cfb],((double)(now-then))/ticksperhr,CFH[cfx_q][cfe]);
      printf("%s Av %.2f hrs %s",CFH[cfx_av][cfb],average/ticksperhr,CFH[cfx_av][cfe]);
      printf("%s &plusmn; %.2f hrs %s",CFH[cfx_dev][cfb],sqrt(var)/ticksperhr,CFH[cfx_dev][cfe]);
      printf("%s",CFH[cfx_entry][cfe]);
      }
   else
      {
      printf("IP %c %25.25s %15.15s  @ [%s] not seen for (%.2f) hrs, Av %.2f +/- %.2f hrs\n",
             *hostname,
             IPString2Hostname(hostname+1),
             addr,
             tbuf,
             ((double)(now-then))/ticksperhr,
             average/ticksperhr,
             sqrt(var)/ticksperhr);
      }
   }

if (HTML)
   {
   printf("</table>");
   }

if (XML)
   {
   printf("</output>\n");
   }

dbcp->c_close(dbcp);
dbp->close(dbp,0);
}

/*******************************************************************/

void ShowPerformance()

{ DBT key,value;
  DB *dbp;
  DBC *dbcp;
  DB_ENV *dbenv = NULL;
  double now = (double)time(NULL),average = 0, var = 0;
  double ticksperminute = 60.0;
  char name[CF_BUFSIZE],eventname[CF_BUFSIZE];
  struct Event entry;
  int ret;
  
snprintf(name,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,CF_PERFORMANCE);

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   printf("Couldn't open performance database %s\n",name);
   perror("db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = (dbp->open)(dbp,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = (dbp->open)(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   printf("Couldn't open performance database %s\n",name);
   perror("db_open");
   dbp->close(dbp,0);
   return;
   }

/* Acquire a cursor for the database. */

if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
   {
   printf("Error reading from performance database: ");
   dbp->err(dbp, ret, "DB->cursor");
   return;
   }

 /* Initialize the key/data return pair. */
 
memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));
memset(&entry, 0, sizeof(entry)); 

if (HTML)
   {
   printf("<html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"http://www.cfengine.org/menus.css\" /><link rel=\"stylesheet\" type=\"text/css\" href=\"http://www.cfengine.org/cf_blue.css\"/></head><body><h1>Peformance recently measured on %s</h1><p><table class=border cellpadding=5>",VFQNAME);
   printf("<div id=\"performance\">");
   }

if (XML)
   {
   printf("<?xml version=\"1.0\"?>\n<output>\n");
   }

 /* Walk through the database and print out the key/data pairs. */

while (dbcp->c_get(dbcp, &key, &value, DB_NEXT) == 0)
   {
   double measure;
   time_t then;
   char tbuf[CF_BUFSIZE],addr[CF_BUFSIZE];

   memcpy(&then,value.data,sizeof(then));
   strcpy(eventname,(char *)key.data);

   if (value.data != NULL)
      {
      memcpy(&entry,value.data,sizeof(entry));

      then    = entry.t;
      measure = entry.Q.q/ticksperminute;;
      average = entry.Q.expect/ticksperminute;;
      var     = entry.Q.var;

      snprintf(tbuf,CF_BUFSIZE-1,"%s",ctime(&then));
      tbuf[strlen(tbuf)-9] = '\0';                     /* Chop off second and year */

      if (PURGE == 'y')
         {
         if (now - then > CF_WEEK)
            {
            if ((errno = dbp->del(dbp,NULL,&key,0)) != 0)
               {
               CfLog(cferror,"","db_store");
               }
            }
         
         fprintf(stderr,"Deleting expired entry for %s\n",eventname);

         if (measure < 0 || average < 0 || measure > 4*CF_WEEK)
            {
            if ((errno = dbp->del(dbp,NULL,&key,0)) != 0)
               {
               CfLog(cferror,"","db_store");
               }
            }
         
         fprintf(stderr,"Deleting entry for %s because it seems to take longer than 4 weeks to complete\n",eventname);

         continue;
         }
      
      if (XML)
         {
         printf("%s",CFX[cfx_entry][cfb]);
         printf("%s%s%s",CFX[cfx_event][cfb],eventname,CFX[cfx_event][cfe]);
         printf("%s%s%s",CFX[cfx_date][cfb],tbuf,CFX[cfx_date][cfe]);
         printf("%s%.4lf%s",CFX[cfx_q][cfb],measure,CFX[cfx_q][cfe]);
         printf("%s%.4lf%s",CFX[cfx_av][cfb],average,CFX[cfx_av][cfe]);
         printf("%s%.4lf%s",CFX[cfx_dev][cfb],sqrt(var)/ticksperminute,CFX[cfx_dev][cfe]);
         printf("%s",CFX[cfx_entry][cfe]);         
         }
      else if (HTML)
         {
         printf("%s",CFH[cfx_entry][cfb]);
         printf("%s%s%s",CFH[cfx_event][cfb],eventname,CFH[cfx_event][cfe]);
         printf("%s last performed at %s%s",CFH[cfx_date][cfb],tbuf,CFH[cfx_date][cfe]);
         printf("%s completed in %.4lf mins %s",CFH[cfx_q][cfb],measure,CFH[cfx_q][cfe]);
         printf("%s Av %.4lf mins %s",CFH[cfx_av][cfb],average,CFH[cfx_av][cfe]);
         printf("%s &plusmn; %.4lf mins %s",CFH[cfx_dev][cfb],sqrt(var)/ticksperminute,CFH[cfx_dev][cfe]);
         printf("%s",CFH[cfx_entry][cfe]);
         }
      else
         {
         printf("(%7.4lf mins @ %s) Av %7.4lf +/- %7.4lf for %s \n",measure,tbuf,average,sqrt(var)/ticksperminute,eventname);
         }
      }
   else
      {
      continue;
      }
   }

if (HTML)
   {
   printf("</table>");
   printf("</div>\n</body></html>\n");
   }

if (XML)
   {
   printf("</output>\n");
   }

dbcp->c_close(dbcp);
dbp->close(dbp,0);
}


/*******************************************************************/

void ShowClasses()

{ DBT key,value;
  DB *dbp;
  DBC *dbcp;
  DB_ENV *dbenv = NULL;
  double now = (double)time(NULL),average = 0, var = 0;
  double ticksperminute = 60.0;
  char name[CF_BUFSIZE],eventname[CF_BUFSIZE];
  struct Event entry;
  struct CEnt array[1024];
  int ret, i;
  
snprintf(name,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,CF_CLASSUSAGE);

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   printf("Couldn't open class database %s\n",name);
   perror("db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = (dbp->open)(dbp,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = (dbp->open)(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   printf("Couldn't open class database %s\n",name);
   perror("db_open");
   dbp->close(dbp,0);
   return;
   }

/* Acquire a cursor for the database. */

if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
   {
   printf("Error reading from class database: ");
   dbp->err(dbp, ret, "DB->cursor");
   return;
   }

 /* Initialize the key/data return pair. */
 
memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));
memset(&entry, 0, sizeof(entry)); 

if (HTML)
   {
   time_t now = time(NULL);
   
   printf("<html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"http://www.cfengine.org/menus.css\" /><link rel=\"stylesheet\" type=\"text/css\" href=\"http://www.cfengine.org/cf_blue.css\"/></head><body><h1>Class probabilities observed on %s at %s</h1><p><table class=border cellpadding=5>",VFQNAME,ctime(&now));
   }

if (XML)
   {
   printf("<?xml version=\"1.0\"?>\n<output>\n");
   }

 /* Walk through the database and print out the key/data pairs. */

for (i = 0; i < 1024; i++)
   {
   array[i].q = -1;
   }

i = 0;

while (dbcp->c_get(dbcp, &key, &value, DB_NEXT) == 0)
   {
   double measure;
   time_t then;
   char tbuf[CF_BUFSIZE],addr[CF_BUFSIZE];

   memcpy(&then,value.data,sizeof(then));
   strcpy(eventname,(char *)key.data);

   if (value.data != NULL)
      {
      memcpy(&entry,value.data,sizeof(entry));

      then    = entry.t;
      measure = entry.Q.q;
      average = entry.Q.expect;
      var     = entry.Q.var;

      snprintf(tbuf,CF_BUFSIZE-1,"%s",ctime(&then));
      tbuf[strlen(tbuf)-9] = '\0';                     /* Chop off second and year */

      if (PURGE == 'y')
         {
         if (now - then > CF_WEEK*52)
            {
            if ((errno = dbp->del(dbp,NULL,&key,0)) != 0)
               {
               CfLog(cferror,"","db_store");
               }
            }
         
         fprintf(stderr,"Deleting expired entry for %s\n",eventname);
         continue;
         }
      
      if (i++ < 1024)
         {
         strncpy(array[i].date,tbuf,31);
         strncpy(array[i].name,eventname,255);
         array[i].q = average;
         array[i].d = var;
         }
      else
         {
         break;
         }
      }
   }

#ifdef HAVE_QSORT
qsort(array,1024,sizeof(struct CEnt),CompareClasses);
#endif

for (i = 0; array[i].q > 0; i++)
   {
   if (XML)
      {
      printf("%s",CFX[cfx_entry][cfb]);
      printf("%s%s%s",CFX[cfx_event][cfb],array[i].name,CFX[cfx_event][cfe]);
      printf("%s%s%s",CFX[cfx_date][cfb],array[i].date,CFX[cfx_date][cfe]);
      printf("%s%.4f%s",CFX[cfx_av][cfb],array[i].q,CFX[cfx_av][cfe]);
      printf("%s%.4f%s",CFX[cfx_dev][cfb],sqrt(array[i].d),CFX[cfx_dev][cfe]);
      printf("%s",CFX[cfx_entry][cfe]);         
      }
   else if (HTML)
      {
      printf("%s",CFH[cfx_entry][cfb]);
      printf("%s%s%s",CFH[cfx_event][cfb],array[i].name,CFH[cfx_event][cfe]);
      printf("%s last occured at %s%s",CFH[cfx_date][cfb],array[i].date,CFH[cfx_date][cfe]);
      printf("%s Probability %.4f %s",CFH[cfx_av][cfb],array[i].q,CFH[cfx_av][cfe]);
      printf("%s &plusmn; %.4f %s",CFH[cfx_dev][cfb],sqrt(array[i].d),CFH[cfx_dev][cfe]);
      printf("%s",CFH[cfx_entry][cfe]);
      }
   else
      {
      printf("Probability %7.4f +/- %7.4f for %s (last oberved @ %s)\n",array[i].q,sqrt(array[i].d),array[i].name,array[i].date);
      }
   }

if (HTML)
   {
   printf("</table>");
   }

if (XML)
   {
   printf("</output>\n");
   }

dbcp->c_close(dbcp);
dbp->close(dbp,0);
}

/*******************************************************************/

void ShowChecksums()

{ DBT key,value;
  DB *dbp;
  DBC *dbcp;
  DB_ENV *dbenv = NULL;
  int ret;
  FILE *pp;
  char checksumdb[CF_BUFSIZE];
  struct stat statbuf;
 
snprintf(checksumdb,CF_BUFSIZE,"%s/%s",CFWORKDIR,CF_CHKDB);
  
if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   printf("Couldn't open checksum database %s\n",checksumdb);
   perror("db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = (dbp->open)(dbp,checksumdb,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = (dbp->open)(dbp,NULL,checksumdb,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   printf("Couldn't open checksum database %s\n",checksumdb);
   perror("db_open");
   dbp->close(dbp,0);
   return;
   }

if (HTML)
   {
   printf("<html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"http://www.cfengine.org/menus.css\" /><link rel=\"stylesheet\" type=\"text/css\" href=\"http://www.cfengine.org/cf_blue.css\"/></head><body><h1>Message Digests sampled on %s</h1><p><table class=border cellpadding=5 width=800>",VFQNAME);
   }

if (XML)
   {
   printf("<?xml version=\"1.0\"?>\n<output>\n");
   }

/* Acquire a cursor for the database. */

 if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
    {
    printf("Error reading from checksum database");
    dbp->err(dbp, ret, "DB->cursor");
    return;
    }

 /* Initialize the key/data return pair. */

 memset(&key,0,sizeof(key));
 memset(&value,0,sizeof(value));
 
 /* Walk through the database and print out the key/data pairs. */

 while (dbcp->c_get(dbcp, &key, &value, DB_NEXT) == 0)
    {
    char type;
    char strtype[CF_MAXVARSIZE];
    char name[CF_BUFSIZE];
    struct Checksum_Value chk_val;
    unsigned char digest[EVP_MAX_MD_SIZE+1];
    
    memset(digest,0,EVP_MAX_MD_SIZE+1);
    memset(&chk_val,0,sizeof(chk_val));
    
    memcpy(&chk_val,value.data,sizeof(chk_val));
    memcpy(digest,chk_val.mess_digest,EVP_MAX_MD_SIZE+1);

    strncpy(strtype,key.data,CF_MAXDIGESTNAMELEN);
    strncpy(name,(char *)key.data+CF_CHKSUMKEYOFFSET,CF_BUFSIZE-1);

    type = ChecksumType(strtype);

    if (XML)
       {
       printf("%s",CFX[cfx_entry][cfb]);
       printf("%s%s%s",CFX[cfx_event][cfb],name,CFX[cfx_event][cfe]);
       printf("%s%s%s",CFX[cfx_q][cfb],ChecksumPrint(type,digest),CFX[cfx_q][cfe]);
       printf("%s",CFX[cfx_entry][cfe]);
       }
    else if (HTML)
       {
       printf("%s",CFH[cfx_entry][cfb]);
       printf("%s%s%s",CFH[cfx_filename][cfb],name,CFH[cfx_filename][cfe]);
       printf("%s%s%s",CFH[cfx_q][cfb],ChecksumPrint(type,digest),CFH[cfx_q][cfe]);
       printf("%s",CFH[cfx_entry][cfe]);         
       }
    else
       {
       printf("%s = ",name);
       printf("%s\n",ChecksumPrint(type,digest));
       /* attr_digest too here*/
       
       memset(&key,0,sizeof(key));
       memset(&value,0,sizeof(value));       
       }
    }

if (HTML)
   {
   printf("</table>");
   }

if (XML)
   {
   printf("</output>\n");
   }

dbcp->c_close(dbcp);
dbp->close(dbp,0);
}

/*********************************************************************/

void ShowLocks (int active)

{ DBT key,value;
  DB *dbp;
  DBC *dbcp;
  DB_ENV *dbenv = NULL;
  int ret;
  char lockdb[CF_BUFSIZE];
  struct LockData entry;

  
snprintf(lockdb,CF_BUFSIZE,"%s/cfengine_lock_db",CFWORKDIR);
  
if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   printf("Couldn't open checksum database %s\n",lockdb);
   perror("db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = (dbp->open)(dbp,lockdb,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = (dbp->open)(dbp,NULL,lockdb,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   printf("Couldn't open checksum database %s\n",lockdb);
   perror("db_open");
   dbp->close(dbp,0);
   return;
   }

if (HTML)
   {
   printf("<html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"http://www.cfengine.org/menus.css\" /><link rel=\"stylesheet\" type=\"text/css\" href=\"http://www.cfengine.org/cf_blue.css\"/></head><body><h1>Current lock database on %s</h1><p><table class=border cellpadding=5 width=800>",VFQNAME);
   }

if (XML)
   {
   printf("<?xml version=\"1.0\"?>\n<output>\n");
   }

/* Acquire a cursor for the database. */

 if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
    {
    printf("Error reading from checksum database");
    dbp->err(dbp, ret, "DB->cursor");
    return;
    }

 /* Initialize the key/data return pair. */

 memset(&key,0,sizeof(key));
 memset(&value,0,sizeof(value));
 
 /* Walk through the database and print out the key/data pairs. */

 while (dbcp->c_get(dbcp, &key, &value, DB_NEXT) == 0)
    {
    if (active)
       {
       if (strncmp("lock",(char *)key.data,4) == 0)
          {
          if (XML)
             {
             printf("%s",CFX[cfx_entry][cfb]);
             printf("%s%s%s",CFX[cfx_filename][cfb],(char *)key.data,CFX[cfx_filename][cfe]);
             printf("%s%s%s",CFX[cfx_date][cfb],ctime(&entry.time),CFX[cfx_date][cfe]);
             printf("%s",CFX[cfx_entry][cfe]);         
             }
          else if (HTML)
             {
             printf("%s",CFH[cfx_entry][cfb]);
             printf("%s%s%s",CFH[cfx_filename][cfb],(char *)key.data,CFH[cfx_filename][cfe]);
             printf("%s%s%s",CFH[cfx_date][cfb],ctime(&entry.time),CFH[cfx_date][cfe]);
             printf("%s",CFH[cfx_entry][cfe]);         
             }
          else
             {
             printf("%s = ",(char *)key.data);
             
             if (value.data != NULL)
                {
                memcpy(&entry,value.data,sizeof(entry));
                printf("%s\n",ctime(&entry.time));
                }
             }
          }
       }
    else
       {
       if (strncmp("last",(char *)key.data,4) == 0)
          {
          if (XML)
             {
             printf("%s",CFX[cfx_entry][cfb]);
             printf("%s%s%s",CFX[cfx_filename][cfb],(char *)key.data,CFX[cfx_filename][cfe]);
             printf("%s%s%s",CFX[cfx_date][cfb],ctime(&entry.time),CFX[cfx_date][cfe]);
             printf("%s",CFX[cfx_entry][cfe]);         
             }
          else if (HTML)
             {
             printf("%s",CFH[cfx_entry][cfb]);
             printf("%s%s%s",CFH[cfx_filename][cfb],(char *)key.data,CFH[cfx_filename][cfe]);
             printf("%s%s%s",CFH[cfx_date][cfb],ctime(&entry.time),CFH[cfx_date][cfe]);
             printf("%s",CFH[cfx_entry][cfe]);         
             }
          else
             {
             printf("%s = ",(char *)key.data);
             
             if (value.data != NULL)
                {
                memcpy(&entry,value.data,sizeof(entry));
                printf("%s\n",ctime(&entry.time));
                }
             }
          }
       }
    }

if (HTML)
   {
   printf("</table>");
   }

if (XML)
   {
   printf("</output>\n");
   } 

dbcp->c_close(dbcp);
dbp->close(dbp,0);
}

/*******************************************************************/

void ShowCurrentAudit()

{ char operation[CF_BUFSIZE],name[CF_BUFSIZE];
  struct AuditLog entry;
  DB_ENV *dbenv = NULL;
  DBT key,value;
  DB *dbp;
  DBC *dbcp;
  int ret;

  
snprintf(name,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,CF_AUDITDB_FILE);

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   printf("Couldn't open last-seen database %s\n",name);
   perror("db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = (dbp->open)(dbp,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = (dbp->open)(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   printf("Couldn't open audit database %s\n",name);
   perror("db_open");
   dbp->close(dbp,0);
   return;
   }

/* Acquire a cursor for the database. */

if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
   {
   printf("Error reading from last-seen database: ");
   dbp->err(dbp, ret, "DB->cursor");
   return;
   }

 /* Initialize the key/data return pair. */
 
memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));
memset(&entry, 0, sizeof(entry)); 

if (HTML)
   {
   printf("<html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"http://www.cfengine.org/menus.css\" /><link rel=\"stylesheet\" type=\"text/css\" href=\"http://www.cfengine.org/cf_blue.css\"/></head><body><h1>Audit log %s</h1><p><table class=border cellpadding=2 cellspacing=2>",VFQNAME);
   /* printf("<th> t-index </th>");*/
   printf("<th> Scan convergence </th>");
   printf("<th> Observed </th>");
   printf("<th> Promise made </th>");
   printf("<th> Promise originates in </th>");
   printf("<th> Promise version </th>");
   printf("<th> line </th>");
   }

if (XML)
   {
   printf("<?xml version=\"1.0\"?>\n<output>\n");
   }

 /* Walk through the database and print out the key/data pairs. */

while (dbcp->c_get(dbcp, &key, &value, DB_NEXT) == 0)
   {
   strncpy(operation,(char *)key.data,CF_BUFSIZE-1);

   if (value.data != NULL)
      {
      memcpy(&entry,value.data,sizeof(entry));
      
      if (XML)
         {
         printf("%s",CFX[cfx_entry][cfb]);
         printf("%s %s %s",CFX[cfx_index][cfb],operation,CFX[cfx_index][cfe]);
         printf("%s %s, ",CFX[cfx_event][cfb],entry.operator);
         AuditStatusMessage(entry.status);
         printf("%s",CFX[cfx_event][cfe]);
         printf("%s %s %s",CFX[cfx_q][cfb],entry.comment,CFX[cfx_q][cfe]);
         printf("%s %s %s",CFX[cfx_date][cfb],entry.date,CFX[cfx_date][cfe]);
         printf("%s %s %s",CFX[cfx_av][cfb],entry.filename,CFX[cfx_av][cfe]);
         printf("%s %s %s",CFX[cfx_version][cfb],entry.version,CFX[cfx_version][cfe]);
         printf("%s %d %s",CFX[cfx_ref][cfb],entry.lineno,CFX[cfx_ref][cfe]);
         printf("%s",CFX[cfx_entry][cfe]);
         }
      else if (HTML)
         {
         printf("%s",CFH[cfx_entry][cfb]);
         /* printf("%s %s %s",CFH[cfx_index][cfb],operation,CFH[cfx_index][cfe]);*/
         printf("%s %s, ",CFH[cfx_event][cfb],Format(entry.operator,40));
         AuditStatusMessage(entry.status);
         printf("%s",CFH[cfx_event][cfe]);
         printf("%s %s %s",CFH[cfx_q][cfb],Format(entry.comment,40),CFH[cfx_q][cfe]);
         printf("%s %s %s",CFH[cfx_date][cfb],entry.date,CFH[cfx_date][cfe]);
         printf("%s %s %s",CFH[cfx_av][cfb],entry.filename,CFH[cfx_av][cfe]);
         printf("%s %s %s",CFH[cfx_version][cfb],entry.version,CFH[cfx_version][cfe]);
         printf("%s %d %s",CFH[cfx_ref][cfb],entry.lineno,CFH[cfx_ref][cfe]);
         printf("%s",CFH[cfx_entry][cfe]);

         if (strstr(entry.comment,"closing"))
            {
            printf("<th></th>");
            printf("<th></th>");
            printf("<th></th>");
            printf("<th></th>");
            printf("<th></th>");
            printf("<th></th>");
            printf("<th></th>");
            }
         }
      else
         {
         printf(". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .\n");
         printf("Converge \'%s\' ",entry.operator);
         
         AuditStatusMessage(entry.status); /* Reminder */

         if (strlen(entry.comment) > 0)
            {
            printf("Comment: %s\n",entry.comment);
            }

         if (strcmp(entry.filename,"Terminal") == 0)
            {
            if (strstr(entry.comment,"closing"))
               {
               printf("\n===============================================================================================\n\n");
               }
            }
         else
            {
            if (strlen(entry.version) == 0)
               {
               printf("Promised in %s (unamed version last edited at %s) at/before line %d\n",entry.filename,entry.date,entry.lineno);
               }
            else
               {
               printf("Promised in %s (version %s last edited at %s) at/before line %d\n",entry.filename,entry.version,entry.date,entry.lineno);
               }
            }
         }
      }
   else
      {
      continue;
      }
   }

if (HTML)
   {
   printf("</table>");
   }

if (XML)
   {
   printf("</output>\n");
   }

dbcp->c_close(dbcp);
dbp->close(dbp,0);
}

/*********************************************************************/
/* Level 3                                                           */
/*********************************************************************/

char *ChecksumDump(unsigned char digest[EVP_MAX_MD_SIZE+1])

{ unsigned int i;
  static char buffer[EVP_MAX_MD_SIZE*4];
  int len = 1;

for (i = 0; buffer[i] != 0; i++)
   {
   len++;
   }

if (len == 16 || len == 20)
   {
   }
else
   {
   len = 16;
   }

switch(len)
   {
   case 20: sprintf(buffer,"SHA=  ");
       break;
   case 16: sprintf(buffer,"MD5=  ");
       break;
   }
  
for (i = 0; i < len; i++)
   {
   sprintf((char *)(buffer+4+2*i),"%02x", digest[i]);
   }

return buffer; 
}    

/*********************************************************************/

void ShowRegex(char *regex)

{
SetSignals();
SetReferenceTime(false);
SetStartTime(true);
OpenSSL_add_all_algorithms();
ERR_load_crypto_strings();
VREPOSITORY = strdup("\0");
VCANONICALFILE = strdup(VINPUTFILE);
VDEFAULTBINSERVER.name = "";
AddClassToHeap("any");
PARSEONLY = INSTALLALL = true;

strcpy(CFWORKDIR,WORKDIR);
SetContext("cfshow");
ISCFENGINE = true;
ParseInputFile(VINPUTFILE,true);
ISCFENGINE = false;

ListDefinedInterfaces(regex);
printf("------------------------------------------------------------\n");
ListDefinedBinservers(regex);
printf("------------------------------------------------------------\n");
ListDefinedHomeservers(regex);
printf("------------------------------------------------------------\n");
ListDefinedHomePatterns(regex);
printf("------------------------------------------------------------\n");
ListDefinedStrategies(regex);
printf("------------------------------------------------------------\n");
ListACLs();
printf("------------------------------------------------------------\n");
ListFilters(regex);
printf("------------------------------------------------------------\n");   
ListDefinedIgnore(regex);
printf("------------------------------------------------------------\n");
ListDefinedAlerts(regex);
printf("------------------------------------------------------------\n");
ListDefinedDisable(regex);
printf("------------------------------------------------------------\n");
ListFiles(regex);
ListDefinedMakePaths(regex);
printf("------------------------------------------------------------\n");
ListFileEdits(regex);
printf("------------------------------------------------------------\n");
ListDefinedImages(regex);
printf("------------------------------------------------------------\n");
ListDefinedLinks(regex);
printf("------------------------------------------------------------\n");
ListDefinedLinkchs(regex);
printf("------------------------------------------------------------\n");
ListDefinedMethods(regex);
printf("------------------------------------------------------------\n");
ListDefinedMountables(regex);
printf("------------------------------------------------------------\n");
ListMiscMounts(regex);
printf("------------------------------------------------------------\n");
ListUnmounts(regex);
printf("------------------------------------------------------------\n");
ListDefinedPackages(regex);
printf("------------------------------------------------------------\n");
ListProcesses(regex);
printf("------------------------------------------------------------\n");
ListDefinedRequired(regex);
printf("------------------------------------------------------------\n");
ListDefinedResolvers(regex);
printf("------------------------------------------------------------\n");
ListDefinedSCLI(regex);
printf("------------------------------------------------------------\n");
ListDefinedScripts(regex);
printf("------------------------------------------------------------\n");
ListDefinedTidy(regex);
printf("------------------------------------------------------------\n");
}

/*********************************************************************/

char *Format(char *s,int width)

{ static char buffer[CF_BUFSIZE];
  char *sp;
  int i = 0, count = 0;
  
for (sp = s; *sp != '\0'; sp++)
   {
   buffer[i++] = *sp;
   buffer[i] = '\0';
   count++;

   if ((count > width - 5) && ispunct(*sp))
      {
      strcat(buffer,"<br>");
      i += strlen("<br>");
      count = 0;
      }
   }

return buffer;
}

/*************************************************************/

int CompareClasses(const void *a, const void *b)

{
struct CEnt *da = (struct CEnt *) a;
struct CEnt *db = (struct CEnt *) b;

return (da->q < db->q) - (da->q > db->q);
}

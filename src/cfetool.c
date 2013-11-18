/* cfetool for GNU

        Copyright (C) 2001-
        Free Software Foundation, Inc.

   This file is part of GNU cfetool - an addition to GNU cfengine.
   It is written by Elizabeth Cassell and Alf Wachsmann,
   Stanford Linear Accelerator Center (SLAC),
   SLAC Computing Services, 2575 Sand Hill Road M/S 97,
   Menlo Park, CA 94025, USA.

   GNU cfengine is written and maintained 
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
/* File: cfetool.c                                                           */
/*                                                                           */
/* Description: Standalone UI for Long term state registry                   */
/*                                                                           */
/*****************************************************************************/

#include "../pub/getopt.h"
#include "cf.defs.h"
#include "cf.extern.h"
#include "cfetooldefs.h"


struct option CREATEOPTIONS[] = {
  {"step", required_argument, 0, 's'},
  {"cfenvd", no_argument, 0, 'c'},
  {"weekly", no_argument, 0, 'w'},
  {"daily", no_argument, 0, 'd'},
  {"yearly", no_argument, 0, 'y'},
  {"path", required_argument, 0, 'p'},
  {"debug", optional_argument, 0, 'D'},
  {"histograms", no_argument, 0, 'H'},
  {"verbose", no_argument, 0, 'v'},
  {"help", no_argument, 0, 'h'},
  {"file", required_argument, 0, 'f'},
  {NULL, 0, 0, 0}
};

struct option UPDATEOPTIONS[] = {
  {"cfenvd", no_argument, 0, 'c'},
  {"weekly", no_argument, 0, 'w'},
  {"daily", no_argument, 0, 'd'},
  {"yearly", no_argument, 0, 'y'},
  {"path", required_argument, 0, 'p'},
  {"help", no_argument, 0, 'h'},
  {"debug", optional_argument, 0, 'D'},
  {"verbose", no_argument, 0, 'v'},
  {"time", required_argument, 0, 't'},
  {"histograms", no_argument, 0, 'H'},
  {"value", required_argument, 0, 'V'},
  {"debugging", no_argument, 0, 'E'},
  {"debugging2", no_argument, 0, 'Y'},
  {NULL, 0, 0, 0}
};

struct option CHECKOPTIONS[] = {
  {"weekly", no_argument, 0, 'w'},
  {"daily", no_argument, 0, 'd'},
  {"yearly", no_argument, 0, 'y'},
  {"path", required_argument, 0, 'p'},
  {"help", no_argument, 0, 'h'},
  {"debug", optional_argument, 0, 'D'},
  {"verbose", no_argument, 0, 'v'},
  {"time", required_argument, 0, 't'},
  {"histograms", no_argument, 0, 'H'},
  {"value", required_argument, 0, 'V'},
  {"debugging", no_argument, 0, 'E'},
  {"debugging2", no_argument, 0, 'Y'},
  {NULL, 0, 0, 0}
};

struct option INFOOPTIONS[] = {
  {"weekly", no_argument, 0, 'w'},
  {"daily", no_argument, 0, 'd'},
  {"yearly", no_argument, 0, 'y'},
  {"path", required_argument, 0, 'p'},
  {"help", no_argument, 0, 'h'},
  {"debug", optional_argument, 0, 'D'},
  {"verbose", no_argument, 0, 'v'},
  {NULL, 0, 0, 0}
};

struct option DUMPOPTIONS[] = {
  {"weekly", no_argument, 0, 'w'},
  {"daily", no_argument, 0, 'd'},
  {"yearly", no_argument, 0, 'y'},
  {"path", required_argument, 0, 'p'},
  {"help", no_argument, 0, 'h'},
  {"debug", optional_argument, 0, 'D'},
  {"verbose", no_argument, 0, 'v'},
  {"file", required_argument, 0, 'f'},
  {NULL, 0, 0, 0}
};

struct option IMPORTOPTIONS[] = {
  {"weekly", no_argument, 0, 'w'},
  {"daily", no_argument, 0, 'd'},
  {"yearly", no_argument, 0, 'y'},
  {"path", required_argument, 0, 'p'},
  {"help", no_argument, 0, 'h'},
  {"debug", optional_argument, 0, 'D'},
  {"verbose", no_argument, 0, 'v'},
  {"file", required_argument, 0, 'f'},
  {NULL, 0, 0, 0}
};

/*************************************************************************/
/* GLOBALS                                                               */
/*************************************************************************/

int weekly = false;
int daily = false;
int yearly = false;
int cfenvd_compatible = false;

unsigned int HISTOGRAM[24][CF_GRAINS];
int HISTO = false;
double VALUE=0;
char *NAME;
char LOCATION[CF_BUFSIZE];
struct Average LOCALAV;
char OUTPUT[CF_BUFSIZE * 2];
char BATCHFILE[CF_BUFSIZE];
char AVDB[1024];
char PATHNAME[CF_BUFSIZE];
double ITER=0;				/* Iteration since start */
double AGE=0, WAGE=0;			/* Age and weekly age of database */

time_t update_time=0;
time_t last_time=0;

int STEP = 5;
int INTERVAL=0;
char *PROG_NAME;
int BATCH_MODE = false;
DBT key, value;
DB *dbp;
int time_to_update = false;
int DEBUGGING = false;
int DEBUGGING2 = false;
char ENVCF_NEW[CF_BUFSIZE];
char ENVCF[CF_BUFSIZE];

/*************************************************************************/

#define Debugging if(DEBUGGING) printf
#define Debugging2 if(DEBUGGING2) printf

/*************************************************************************/

void parse_create_opts (int argc, char **argv);
void parse_update_opts (int argc, char **argv);
void parse_check_opts (int argc, char **argv);
void parse_info_opts (int argc, char **argv);
void parse_dump_opts (int argc, char **argv);
void parse_import_opts (int argc, char **argv);
void Create (int step, int dbtype);
int Update (double value, time_t u_time, int dbtype);
int Check (double value, time_t u_time, int dbtype, int *bucket);
void Info (int dbtype);
void Dump (FILE * fp, int dbtype);
void Import (FILE * fp, int dbtype);
void skip (char **buf);
void parse_entry (char **buffer);
void GetDatabaseAge (int dbtype);
struct Average EvalAvQ (char *t, int dbtype, int update);
struct Average *GetCurrentAverage (char *timekey, int dbtype);
void UpdateAverage (char *timekey, struct Average newvals);
void UpdateDistribution (char *timekey, struct Average * av, int dbtype);
double RejectAnomaly (double new, double av, double var,
                              double av2, double var2);
double WAverage (double newvals, double oldvals, double age);
void LoadHistogram (int dbtype);
int ArmClasses (struct Average av, char *timekey);
int SetClasses (char *name, double variable, double av_expect,
                         double av_var, double localav_expect,
                         double localav_var, struct Item ** classlist,
                         char *timekey, int *code);
void SetVariable (char *name,double now, double average,
                          double stddev, struct Item **list);
void Syntax (void);
void CreateSyntax (void);
void UpdateSyntax (void);
void CheckSyntax (void);
void InfoSyntax (void);
void DumpSyntax (void);
void ImportSyntax (void);
void FatalError (char *s);
void yyerror (char *s);
void DoBatch (int dbtype);
void CloseDatabase (void);
int OpenDatabase (int create);

/*************************************************************************/
/* Level 0                                                               */
/*************************************************************************/

int main(int argc, char **argv)

{ char *command;
  int i;

IGNORELOCK = false;
OUTPUT[0] = '\0';
PROG_NAME = argv[0];
memset(PATHNAME, 0, sizeof(PATHNAME));

if (argc < 2)
   {
   Syntax();
   exit(1);
   }
command = argv[1];

if(argc < 3 || argv[2][0] == '-')
   {
   if (strcmp(command, "import") == 0)
       ImportSyntax();
   else if (strcmp(command, "update") == 0)
       UpdateSyntax();
   else if (strcmp(command, "check") == 0)
       CheckSyntax();
   else if (strcmp(command, "info") == 0)
       InfoSyntax();
   else if (strcmp(command, "dump") == 0)
       DumpSyntax();
   else if (strcmp(command, "create") == 0)
       CreateSyntax();
   else
       Syntax();
   exit(1);
   }

NAME = argv[2];

for( i = 0; NAME[i] != '\0'; i++)
   {
   if(!isalnum(NAME[i]))
      {
      fprintf(stderr, "Name must contain only letters (A to Z or a to z) or digits (0 to 9)\n");
      exit(1);
      }
   }

if (strcmp(NAME, "value") == 0)
   {
   fprintf(stderr, "Name cannot be \"value\"\n");
   exit(1);
   }

if(strcmp(NAME, "average") == 0)
   {
   fprintf(stderr, "Name cannot be \"average\"\n");
   exit(1);
   }

if(strcmp(NAME, "stddev") == 0)
   {
   fprintf(stderr, "Name cannot be \"stddev\"\n");
   exit(1);
   }

strncpy(VLOCKDIR,WORKDIR,CF_BUFSIZE-1);
strncpy(VLOGDIR,WORKDIR,CF_BUFSIZE-1);
snprintf(ENVCF_NEW,CF_BUFSIZE,"%s/state/%s",WORKDIR,CF_ENVNEW_FILE);
snprintf(ENVCF,CF_BUFSIZE,"%s/state/%s",WORKDIR,CF_ENV_FILE);
argv += 2;
argc -= 2;

if (strcmp(command, "create") == 0)
   {
   parse_create_opts(argc, argv);
   }
else if (strcmp(command, "update") == 0)
   {
   parse_update_opts(argc, argv);
   }
else if (strcmp(command, "check") == 0)
   {
   parse_check_opts(argc, argv);
   }
else if (strcmp(command, "info") == 0)
   {
   parse_info_opts(argc, argv);
   }
else if (strcmp(command, "dump") == 0)
   {
   parse_dump_opts(argc, argv);
   }
else if (strcmp(command, "import") == 0)
   {
   parse_import_opts(argc, argv);
   }
else
   {
   Syntax();
   exit(1);
   }
return 0;
}

/*********************************************************************/

void parse_create_opts(int argc, char **argv)

{ extern char *optarg;
  int optindex = 0;
  int c;

while ((c = getopt_long(argc, argv, "p:s:dwyf:DHhvc",
                          CREATEOPTIONS, &optindex)) != EOF)
   {
   switch ((char)c)
      {
      case 'D':
          DEBUG = true;
          break;
          
      case 'c':
          cfenvd_compatible = true;
          break;
          
      case 'f':
          BATCH_MODE = true;
          strcpy(BATCHFILE, optarg);
          break;
          
      case 'v':
          VERBOSE = true;
          break;
          
      case 'H':
          HISTO = true;
          break;
          
      case 'p':
          strcpy(PATHNAME,optarg);
          break;
          
      case 's':
          STEP = atoi(optarg);
          if (STEP > 1440 || STEP < 1 || 1440 % STEP)
             {
             fprintf(stderr,
                     "step must be an integer number of minutes that divides evenly into 24 hours.\n");
             exit(1);
             }
          break;
          
      case 'd':
          daily = true;
          break;
          
      case 'w':
          weekly = true;
          break;
          
      case 'y':
          yearly = true;
          break;
          
      default:
          printf("Syntax:\n\n");
          CreateSyntax();
          exit(1);
          
      }
   }

if (daily)
   {
   Create(STEP, DAILY);
   }

if (weekly || (!daily && !yearly))
   {
   Create(STEP, WEEKLY);
   }

if (yearly)
   {
   Create(STEP, YEARLY);
   }
}

/*********************************************************************/

void parse_update_opts(int argc, char **argv)

{ int dcode=0, wcode=0, ycode=0;
  double value = 0;
  time_t u_time = 0;
  extern char *optarg;
  int c, got_time = 0, got_val = 0;
  int optindex = 0;

while ((c = getopt_long(argc, argv, "ct:V:DvhYHEdwyp:",
                          UPDATEOPTIONS, &optindex)) != EOF)
   {
   switch ((char)c)
      {
      case 'c':
          cfenvd_compatible = true;
          break;
          
      case 'D':
          DEBUG = true;
          break;
          
      case 'E':
          DEBUGGING = true;
          break;
          
      case 'Y':
          DEBUGGING2 = true;
          break;
          
      case 'v':
          VERBOSE = true;
          break;
          
      case 'H':
          HISTO = true;
          break;
          
      case 'p':
          strcpy(PATHNAME,optarg);
          break;
          
      case 't':
          u_time = (time_t) atoi(optarg);
          got_time = 1;
          break;
          
      case 'V':
          value = atof(optarg);
          got_val = 1;
          break;
          
      case 'd':
          daily = true;
          break;
          
      case 'w':
          weekly = true;
          break;
          
      case 'y':
          yearly = true;
          break;
          
      default:
          printf("Syntax:\n\n");
          UpdateSyntax();
          exit(1);
          
      }
   }

if (!got_val)
   {
   printf("Syntax:\n\n");
   UpdateSyntax();
   exit(1);
   }

if (!got_time)
   {
   u_time = time(NULL);
   Verbose("using current time\n");
   }

if(daily)
   {
   dcode = Update(value, u_time, DAILY);
   }

if (weekly || (!daily && !yearly))
   {
   wcode = Update(value, u_time, WEEKLY);
   }

if (yearly)
   {
   ycode = Update(value, u_time, YEARLY);
   }

printf("yrly=%d,wkly=%d,dly=%d\n", ycode, wcode, dcode);
Verbose("\n");

if (dcode <= wcode && dcode <= ycode)
   {
   exit(dcode);
   }
else if(wcode <= dcode && wcode <= ycode)
   {
   exit(wcode);
   }
else
   {
   exit(ycode);
   }
}

/*********************************************************************/

void parse_check_opts(int argc, char **argv)

{ int dcode=0, wcode=0, ycode=0;
  int dbucket=0, wbucket=0, ybucket=0;
  double value = 0;
  time_t u_time = 0;
  extern char *optarg;
  int c, got_time = 0, got_val = 0;
  int optindex = 0;

while ((c = getopt_long(argc, argv, "t:V:vhHDEYdwyp:",
                        CHECKOPTIONS, &optindex)) != EOF)
   {
   switch ((char)c)
      {
      case 'D':
          DEBUG = true;
          break;
          
      case 'E':
          DEBUGGING = true;
          break;
          
      case 'Y':
          DEBUGGING2 = true;
          break;
          
      case 'v':
          VERBOSE = true;
          break;
          
      case 'H':
          HISTO = true;
          break;
          
      case 'p':
          strcpy(PATHNAME,optarg);
          break;
          
      case 't':
          u_time = (time_t) atoi(optarg);
          got_time = 1;
          break;
          
      case 'V':
          value = atof(optarg);
          got_val = 1;
          break;
          
      case 'd':
          daily = true;
          break;
          
      case 'w':
          weekly = true;
          break;
          
      case 'y':
          yearly = true;
          break;
          
      default:
          printf("Syntax:\n\n");
          CheckSyntax();
          exit(1);
          
      }
   }

if (!got_val)
   {
   printf("Syntax:\n\n");
   CheckSyntax();
   exit(1);
   }

if (!got_time)
   {
   u_time = time(NULL);
   Verbose("using current time\n");
   }

if (daily)
   {
   dcode = Check(value, u_time, DAILY, &dbucket);
   }

if (weekly || (!daily && !yearly))
   {
   wcode = Check(value, u_time, WEEKLY, &wbucket);
   }

if (yearly)
   {
   ycode = Check(value, u_time, YEARLY, &ybucket);
   }

printf("yrly=%d,bkt=%d;wkly=%d,bkt=%d;dly=%d,bkt=%d\n", 
       ycode, ybucket, wcode, wbucket, dcode, dbucket);

Verbose("\n");

if (dcode <= wcode && dcode <= ycode)
   {
   exit(dcode);
   }

if (wcode <= dcode && wcode <= ycode)
   {
   exit(wcode);
   }

if (ycode <= dcode && ycode <= wcode)
   {
   exit(ycode);
   }
}

/*********************************************************************/

void parse_info_opts(int argc, char **argv)

{ int optindex = 0, c;

while ((c = getopt_long(argc, argv, "DvYEhdwyp:",
                        INFOOPTIONS, &optindex)) != EOF)
   {
   switch ((char)c)
      {
      case 'D':
          DEBUG = true;
          break;
          
      case 'p':
          strcpy(PATHNAME,optarg);
          break;
          
      case 'E':
          DEBUGGING = true;
          break;
          
      case 'Y':
          DEBUGGING2 = true;
          break;
          
      case 'v':
          VERBOSE = true;
          break;
          
      case 'd':
          daily = true;
          break;
          
      case 'w':
          weekly = true;
          break;
          
      case 'y':
          yearly = true;
          break;
          
      default:
          printf("Syntax:\n\n");
          InfoSyntax();
          exit(1);
          
      }
   }

if (daily)
   {
   printf("\nDaily average database:\n");
   Info(DAILY);
   }

if (weekly || (!daily && !yearly))
   {
   printf("\nWeekly average database:\n");
   Info(WEEKLY);
   }

if (yearly)
   {
   printf("\nYearly average database:\n");
   Info(YEARLY);
   }
}

/*********************************************************************/

void parse_dump_opts(int argc, char **argv)

{ int optindex = 0, c;
  char DUMPFILE[CF_BUFSIZE];
  FILE *fp;

DUMPFILE[0] = '\0';

while ((c = getopt_long(argc, argv, "DvYEhf:dwyp:",
                        DUMPOPTIONS, &optindex)) != EOF)
   {
   switch ((char)c)
      {
      case 'D':
          DEBUG = true;
          break;
          
      case 'E':
          DEBUGGING = true;
          break;
          
      case 'p':
          strcpy(PATHNAME,optarg);
          break;
          
      case 'Y':
          DEBUGGING2 = true;
          break;
          
      case 'v':
          VERBOSE = true;
          break;
          
      case 'f':
          strcpy(DUMPFILE, optarg);
          break;
          
      case 'd':
          daily = true;
          break;
          
      case 'w':
          weekly = true;
          break;
          
      case 'y':
          yearly = true;
          break;
          
      default:
          printf("Syntax:\n\n");
          DumpSyntax();
          exit(1);
          
      }
   }


if (DUMPFILE[0] == '\0')
   {
   fprintf(stdout, "<!-- %s Database Dump -->\n", PROG_NAME);

   if(daily)
      {
      Dump(stdout, DAILY);
      }
   if(weekly || (!daily && !yearly))
      {
      Dump(stdout, WEEKLY);
      }
   if(yearly)
      {
      Dump(stdout, YEARLY);
      }
   }
else if ((fp = fopen(DUMPFILE, "w")) != NULL)
   {
   fprintf(fp, "<!-- %s Database Dump -->\n", PROG_NAME);

   if (daily)
      {
      Dump(fp, DAILY);
      }
   
   if (weekly || (!daily && !yearly))
      {
      Dump(fp, WEEKLY);
      }
   
   if (yearly)
      {
      Dump(fp, YEARLY);
      }
   fclose(fp);
   }
else
   {
   fprintf(stderr, "Unable to open file \"%s\"\n", DUMPFILE);
   fprintf(stderr, "fopen: %s\n", strerror(errno));
   }
}

/*********************************************************************/

void parse_import_opts(int argc, char **argv)

{ int optindex = 0, c;
  char IMPORTFILE[CF_BUFSIZE];
  FILE *fp;

IMPORTFILE[0] = '\0';

while ((c = getopt_long(argc, argv, "DvYEhf:dwy",
                        IMPORTOPTIONS, &optindex)) != EOF)
   {
   switch ((char)c)
      {
      case 'D':
          DEBUG = true;
          break;
          
      case 'E':
          DEBUGGING = true;
          break;
          
      case 'Y':
          DEBUGGING2 = true;
          break;
          
      case 'p':
          strcpy(PATHNAME,optarg);
          break;
          
      case 'v':
          VERBOSE = true;
          break;
          
      case 'f':
          strcpy(IMPORTFILE, optarg);
          break;
          
      case 'd':
          daily = true;
          break;
          
      case 'w':
          weekly = true;
          break;
          
      case 'y':
          yearly = true;
          break;
          
      default:
          printf("Syntax:\n\n");
          ImportSyntax();
          exit(1);
          
      }
   }

if (IMPORTFILE[0] == '\0')
   {
   printf("Syntax:\n\n");
   ImportSyntax();
   exit(1);
   }
else if ((fp = fopen(IMPORTFILE, "r")) != NULL)
   {
   if (daily)
      {
      Import(fp, DAILY);
      rewind(fp);
      }
   
   if (weekly || (!daily && !yearly))
      {
      Import(fp, WEEKLY);
      rewind(fp);
      }
   
   if (yearly)
      {
      Import(fp, YEARLY);
      rewind(fp);
      }
   fclose(fp);
   }
else
   {
   fprintf(stderr, "Unable to open file \"%s\"\n", IMPORTFILE);
   fprintf(stderr, "fopen: %s\n", strerror(errno));
   }
}

/*********************************************************************/

void Create(int step, int dbtype)

{ time_t timestamp;
  char filename[CF_BUFSIZE];
  int cwdbufsize = CF_BUFSIZE - strlen(PATHNAME) - strlen(NAME);
  char current_dir[CF_BUFSIZE];

AGE = WAGE = 0;
ITER = 0;
LOCALAV.expect = 0.0;
LOCALAV.var = 0.0;

/* make file */

if (getcwd(current_dir, cwdbufsize) == NULL)
   {
   perror("getcwd");
   }

if (PATHNAME[0] == '\0')
   {
   sprintf(LOCATION, "%s/%s", current_dir, NAME);
   }
else if(PATHNAME[0] == '/')
   {
   sprintf(LOCATION, "%s/%s", PATHNAME, NAME);
   }
else
   {
   sprintf(LOCATION, "%s/%s/%s", current_dir, PATHNAME, NAME);
   }

switch(dbtype)
   {
   case DAILY:
       sprintf(AVDB, "%s/daily.db", LOCATION);
       break;
   case YEARLY:
       sprintf(AVDB, "%s/yearly.db", LOCATION);
       break;
   default: /* weekly */
       sprintf(AVDB, "%s/weekly.db", LOCATION);
       break;
   }

Verbose("Creating new database: %s\n", AVDB);
MakeDirectoriesFor(AVDB, 'n');

if (HISTO)
   {
   switch(dbtype)
      {
      case DAILY:
          snprintf(filename, CF_BUFSIZE, "%s/daily.hist", LOCATION);
          break;
      case YEARLY:
          snprintf(filename, CF_BUFSIZE, "%s/yearly.hist", LOCATION);
          break;
      default: /* weekly */
          snprintf(filename, CF_BUFSIZE, "%s/weekly.hist", LOCATION);
          break;
      }
   Verbose("Creating histogram file: %s\n", filename);
   CreateEmptyFile(filename);
   }

if (OpenDatabase(true) != 0)
   {
   exit(1);
   }

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

value.data = &AGE;
value.size = sizeof(double);
key.data = "DATABASE_AGE";
key.size = strlen("DATABASE_AGE") + 1;

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   return;
   }

timestamp = time(NULL);
memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

value.data = &timestamp;
value.size = sizeof(timestamp);
key.data = "TIMESTAMP";
key.size = strlen("TIMESTAMP") + 1;

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   return;
   }

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

value.data = &LOCALAV;
value.size = sizeof(LOCALAV);
key.data = "LOCALAV";
key.size = strlen("LOCALAV") + 1;

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   return;
   }

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

value.data = &step;
value.size = sizeof(step);
key.data = "STEP";
key.size = strlen("STEP") + 1;

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   return;
   }

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

value.data = &ITER;
value.size = sizeof(ITER);
key.data = "ITERATIONS";
key.size = strlen("ITERATIONS") + 1;

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   return;
   }

Verbose("Successfully created database at %s\n", AVDB);

if (BATCH_MODE)
   {
   DoBatch(dbtype);
   }

CloseDatabase();
}

/*********************************************************************/

int Update(double value, time_t u_time, int dbtype)

{ struct Average average;
  char *timekey;
  int i, j;

update_time = u_time;
VALUE = value;

if(PATHNAME[0] != '\0')
   {
   sprintf(LOCATION, "%s/%s", PATHNAME, NAME);
   }
else
   {
   sprintf(LOCATION, "./%s", NAME);
   }

switch(dbtype)
   {
   case DAILY:
       sprintf(AVDB, "%s/daily.db", LOCATION);
       break;
   case YEARLY:
       sprintf(AVDB, "%s/yearly.db", LOCATION);
       break;
   default: /* weekly */
       sprintf(AVDB, "%s/weekly.db", LOCATION);
       break;
   }

Verbose("Updating database: %s\n", AVDB);

LOCALAV.expect = 0.0;
LOCALAV.var = 0.0;
ITER = 0.0;

for (i = 0; i < 7; i++)
   {
   for (j = 0; j < CF_GRAINS; j++)
      {
      HISTOGRAM[i][j] = 0;
      }
   }

if (OpenDatabase(false) != 0)
   {
   exit(1);
   }

srand((unsigned int)time(NULL));

LoadHistogram(dbtype);
GetDatabaseAge(dbtype);

timekey = GenTimeKey2(update_time, dbtype);
Verbose("Time key is: %s\n", timekey);
average = EvalAvQ(timekey, dbtype, true);

CloseDatabase();

return ArmClasses(average, timekey);
}

/*********************************************************************/

int Check(double value, time_t u_time, int dbtype, int *bucket)

{ struct Average average;
  char *timekey;
  int i, j, sig;

Verbose("Checking %.3lf, time %d\n", value, u_time);
  
update_time = u_time;
VALUE = value;

if(PATHNAME[0] != '\0')
   {
   sprintf(LOCATION, "%s/%s", PATHNAME, NAME);
   }
else
   {
   sprintf(LOCATION, "./%s", NAME);
   }

switch(dbtype)
   {
   case DAILY:
       sprintf(AVDB, "%s/daily.db", LOCATION);
      break;
   case YEARLY:
       sprintf(AVDB, "%s/yearly.db", LOCATION);
       break;
   default: /* weekly */
       sprintf(AVDB, "%s/weekly.db", LOCATION);
       break;
   }

Verbose("Updating database: %s\n", AVDB);

LOCALAV.expect = 0.0;
LOCALAV.var = 0.0;
ITER = 0.0;

for (i = 0; i < 7; i++)
   {
   for (j = 0; j < CF_GRAINS; j++)
      {
      HISTOGRAM[i][j] = 0;
      }
   }

if (OpenDatabase(false) != 0)
   {
   exit(1);
   }

srand((unsigned int)time(NULL));
LoadHistogram(dbtype);
GetDatabaseAge(dbtype);
timekey = GenTimeKey2(update_time, dbtype);
Verbose("Time key is: %s\n", timekey);
average = EvalAvQ(timekey, dbtype, false);

*bucket = CF_GRAINS / 2 + (int)(0.5 + (VALUE - average.expect) * CF_GRAINS / (4.0 * sqrt(average.var)));

CloseDatabase();

return ArmClasses(average, timekey); 
}

/*********************************************************************/

void Info(int dbtype)
    
{ int i;
  char lastupdate[CF_BUFSIZE];
  char histfile[CF_BUFSIZE];
  FILE *tempfile;

memset(lastupdate, 0, CF_BUFSIZE);

Verbose("Gathering database info...\n");

if (PATHNAME[0] != '\0')
   {
   sprintf(LOCATION, "%s/%s", PATHNAME, NAME);
   }
else
   {
   sprintf(LOCATION, "./%s", NAME);
   }

switch(dbtype)
  {
  case DAILY:
      sprintf(AVDB, "%s/daily.db", LOCATION);
      sprintf(histfile, "%s/daily.hist", LOCATION);
      break;
  case YEARLY:
      sprintf(AVDB, "%s/yearly.db", LOCATION);
      sprintf(histfile, "%s/yearly.hist", LOCATION);
      break;
  default: /* weekly */
      sprintf(AVDB, "%s/weekly.db", LOCATION);
      sprintf(histfile, "%s/weekly.hist", LOCATION);
      break;
  }

printf("Database Location: %s\n", AVDB);

if((tempfile = fopen(histfile, "r")) != NULL)
   {
   printf("Histogram file: %s\n", histfile);
   fclose(tempfile);
   }
else
   {
   printf("No histogram file\n");
   }

LOCALAV.expect = 0.0;
LOCALAV.var = 0.0;
ITER = 0.0;

if(OpenDatabase(false) != 0)
   {
   exit(1);
   }

GetDatabaseAge(dbtype);

printf("Step: %d minutes\n", (int)STEP);

switch(dbtype)
   {
   case DAILY:
       printf("Database Age: %lf days (%d steps)\n", (double)WAGE, (int)AGE);
       break;
   case YEARLY:
       printf("Database Age: %lf years (%d steps)\n", (double)WAGE, (int)AGE);
       break;
   default: /* weekly */
       printf("Database Age: %lf weeks (%d steps)\n", (double)WAGE, (int)AGE);
       break;
   }

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

key.data = "TIMESTAMP";
key.size = strlen("TIMESTAMP") + 1;

if ((errno = dbp->get(dbp, NULL, &key, &value, 0)) != 0)
   {
   if (errno != DB_NOTFOUND)
      {
      dbp->err(dbp, errno, NULL);
      CloseDatabase();
      exit(1);
      }
   }

if (value.data != NULL)
   {
   last_time = *(time_t *) value.data;
   strncpy(lastupdate, ctime(&last_time), strlen(ctime(&last_time)) - 1);
   printf("Last update: %s (%d)\n", lastupdate, (int)last_time);
   }
else
   {
   Debug("No timestamp\n");
   }

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

key.data = "LOCALAV";
key.size = strlen("LOCALAV") + 1;

if ((errno = dbp->get(dbp, NULL, &key, &value, 0)) != 0)
   {
   if (errno != DB_NOTFOUND)
      {
      dbp->err(dbp, errno, NULL);
      CloseDatabase();
      exit(1);
      }
   }

if (value.data != NULL)
   {
   memcpy(&LOCALAV, value.data, sizeof(LOCALAV));
   printf("Current average: %lf, Var: %lf\n", (double)LOCALAV.expect,(double)LOCALAV.var);
   }
else
    Debug("No LOCALAV\n");

CloseDatabase();
}

/*********************************************************************/

void Dump(FILE * fp, int dbtype)

{ time_t NOW;
 char str[256];
 char *timekey;
 struct Average entry;
 int begin_time;
 int total_time;

if(PATHNAME[0] != '\0')
   {
   sprintf(LOCATION, "%s/%s", PATHNAME, NAME);
   }
else
   {
   sprintf(LOCATION, "./%s", NAME);
   }

memset(str, 0, 256);

switch(dbtype)
   {
   case DAILY:
       sprintf(AVDB, "%s/daily.db", LOCATION);
       begin_time = MONDAY_MORNING;
       total_time = ONE_DAY;
       break;
   case YEARLY:
       sprintf(AVDB, "%s/yearly.db", LOCATION);
       begin_time = JANUARY_FIRST;
       total_time = ONE_YEAR;
       break;
   default: /* weekly */
       sprintf(AVDB, "%s/weekly.db", LOCATION);
       begin_time = MONDAY_MORNING;
       total_time = ONE_WEEK;
       break;
   }

Verbose("Dumping database: %s\n",AVDB);

if (OpenDatabase(false) != 0)
   {
   exit(1);
   }

GetDatabaseAge(dbtype);

memset(&value, 0, sizeof(value));
key.data = "TIMESTAMP";
key.size = strlen("TIMESTAMP") + 1;

if ((errno = dbp->get(dbp, NULL, &key, &value, 0)) != 0)
   {
   if (errno != DB_NOTFOUND)
      {
      dbp->err(dbp, errno, NULL);
      CloseDatabase();
      exit(1);
      }
   }

if (value.data != NULL)
   {
   last_time = *(time_t *) value.data;
   }
else
   {
   Debug("No timestamp\n");
   }

Verbose("Timestamp is %d\n", last_time);

memset(&value, 0, sizeof(value));
key.data = "LOCALAV";
key.size = strlen("LOCALAV") + 1;

if ((errno = dbp->get(dbp, NULL, &key, &value, 0)) != 0)
   {
   if (errno != DB_NOTFOUND)
      {
      dbp->err(dbp, errno, NULL);
      CloseDatabase();
      exit(1);
      }
   }

if (value.data != NULL)
   {
   memcpy(&LOCALAV, value.data, sizeof(LOCALAV));
   }
else
   {
   Debug("No previous LOCALAV\n");
   }

Verbose("LOCALAV.expect=%lf, LOCALAV.var=%lf\n", LOCALAV.expect, LOCALAV.var);

memset(&value, 0, sizeof(value));
key.data = "ITERATIONS";
key.size = strlen("ITERATIONS") + 1;

if ((errno = dbp->get(dbp, NULL, &key, &value, 0)) != 0)
   {
   if (errno != DB_NOTFOUND)
      {
      dbp->err(dbp, errno, NULL);
      CloseDatabase();
      exit(1);
      }
   }

if (value.data != NULL)
   {
   ITER = *(double *)value.data;
   }
else
   {
   Debug("No previous iterations\n");
   }

Verbose("ITER = %d\n", (int)ITER);

switch(dbtype)
   {
   case DAILY:
       fprintf(fp, "\n<dailydump>\n\n");
       break;
   case YEARLY:
       fprintf(fp, "\n<yearlydump>\n\n");
       break;
   default: /* weekly */
       fprintf(fp, "\n<weeklydump>\n\n");
       break;
   }

fprintf(fp, "\t<name> %s </name>\n\n", NAME);

fprintf(fp, "\t<age> %d </age> <!-- steps (%.3f", (int)AGE,(float)WAGE);

switch(dbtype)
   {
   case DAILY:
       fprintf(fp, " days) -->\n\n");
       break;
   case YEARLY:
       fprintf(fp, " years) -->\n\n");
       break;
   default: /* weekly */
       fprintf(fp, " weeks) -->\n\n");
       break;
   }

fprintf(fp, "\t<step> %d </step> <!-- minutes -->\n\n", (int)STEP);

fprintf(fp, "\t<!-- consecutive updates -->\n");
fprintf(fp, "\t<iterations> %d </iterations>\n\n", (int)ITER);

strncpy(str, ctime(&last_time), strlen(ctime(&last_time)) - 1);
fprintf(fp, "\t<!-- last update at %s -->\n", str);
fprintf(fp, "\t<timestamp> %d </timestamp>\n\n", (int)last_time);

fprintf(fp, "\t<!-- current weighted average -->\n");
fprintf(fp, "\t<expect> %lf </expect>\n\n", (double)LOCALAV.expect);

fprintf(fp, "\t<!-- current variance -->\n");
fprintf(fp, "\t<var> %lf </var>\n\n", (double)LOCALAV.var);

fprintf(fp, "\n<!-- data entries -->\n");

for (NOW = begin_time; NOW < begin_time + total_time; NOW += INTERVAL)
   {
   sprintf(str, "%s", ctime(&NOW));
   timekey = ConvTimeKey2(str, dbtype);
   
   memset(&value, 0, sizeof(value));
   memset(&entry, 0, sizeof(entry));
   memset(&key, 0, sizeof(key));
   key.data = timekey;
   key.size = strlen(timekey) + 1;
   
   if ((errno = dbp->get(dbp, NULL, &key, &value, 0)) != 0)
      {
      if (errno != DB_NOTFOUND)
         {
         dbp->err(dbp, errno, NULL);
         CloseDatabase();
         exit(1);
         }
      }

   if (value.data != NULL)
      {
      memcpy(&entry, value.data, sizeof(entry));
      fprintf(fp, "\t<entry>");
      fprintf(fp, "\t<timekey> %s </timekey>\n", timekey);
      fprintf(fp, "\t\t<expect> %lf </expect>\n", (double)entry.expect);
      fprintf(fp, "\t\t<var> %lf </var>\n", (double)entry.var);
      fprintf(fp, "\t</entry>\n");
      }
   else
       Debug("No previous value for time index %s\n", timekey);
   }

switch(dbtype)
   {
   case DAILY:
       fprintf(fp, "\n</dailydump>\n");
       break;
   case YEARLY:
       fprintf(fp, "\n</yearlydump>\n");
       break;
   default: /* weekly */
       fprintf(fp, "\n</weeklydump>\n");
       break;
   }

CloseDatabase();
}

/*********************************************************************/

void Import(FILE * fp, int dbtype)

{ char *buffer;
  char command[CF_BUFSIZE];
  int i=0, j=0;
  char info[CF_BUFSIZE], temp[CF_BUFSIZE];
  char *dumpcommand;
  char begincommand[128], endcommand[128];
 
if (PATHNAME[0] != '\0')
   {
   sprintf(LOCATION, "%s/%s", PATHNAME, NAME);
   }
else
   {
   sprintf(LOCATION, "./%s", NAME);
   }

LOCALAV.expect = 0.0;
LOCALAV.var = 0.0;
STEP = 5;
AGE = 0;
last_time = 0;
ITER = 0;

mkdir(LOCATION, 0755);
switch(dbtype)
   {
   case DAILY:
       dumpcommand = "dailydump";
       sprintf(AVDB, "%s/daily.db", LOCATION);
       break;
   case YEARLY:
       dumpcommand = "yearlydump";
       sprintf(AVDB, "%s/yearly.db", LOCATION);
       break;
   default: /* weekly */
       dumpcommand = "weeklydump";
       sprintf(AVDB, "%s/weekly.db", LOCATION);
       break;
   }

sprintf(begincommand, "<%s>", dumpcommand);
sprintf(endcommand, "/%s", dumpcommand);

Verbose("Importing to database: %s\n", AVDB);

if (OpenDatabase(true) != 0)
   {
   exit(1);
   }

while (!feof(fp))
   {
   getc(fp);
   i++;
   }

rewind(fp);

if ((buffer = (char *)malloc(i * sizeof(char))) == NULL)
   {
   fprintf(stderr, "Couldn't allocate buffer of %d chars\n", i);
   CloseDatabase();
   exit(1);
   }

j = (int)fread((void *)buffer, sizeof(char), (size_t) i, fp);

if (j < i - 1)
   {
   fprintf(stderr, "Couldn't read file into buffer, got %d\n", j);
   CloseDatabase();
   exit(1);
   }

while (*buffer && strncmp(buffer, begincommand, strlen(dumpcommand)) != 0)
   {
   buffer++;
   skip(&buffer);
   }

while (*buffer)
   {
   sscanf(buffer, "<%[^<> \t\r\n]>", command);
   buffer += strlen(command) + 2;
   skip(&buffer);
   if (strlen(command) == 0)
      {
      fprintf(stderr, "Couldn't get next command!\n");
      CloseDatabase();
      exit(1);
      }
   else if (strcmp(command, dumpcommand) == 0)
      {
      continue;
      }
   else if (strcmp(command, endcommand) == 0)
      {
      break;
      }
   else if (strcmp(command, "name") == 0)
      {
      sscanf(buffer, "%[^<> \t\r\n]", info);
      buffer += strlen(info);
      if (strcmp(info, NAME) != 0)
          Verbose("Previous database name was %s (now %s)\n", info,
                  NAME);
      }
   else if (strcmp(command, "age") == 0)
      {
      sscanf(buffer, "%lf", &AGE);
      sscanf(buffer, "%[^<> \t\r\n]", info);
      buffer += strlen(info);
      }
   else if (strcmp(command, "step") == 0)
      {
      sscanf(buffer, "%d", &STEP);
      sscanf(buffer, "%[^<> \t\r\n]", info);
      buffer += strlen(info);
      }
   else if (strcmp(command, "iterations") == 0)
      {
      sscanf(buffer, "%lf", &ITER);
      sscanf(buffer, "%[^<> \t\r\n]", info);
      buffer += strlen(info);
      }
   else if (strcmp(command, "timestamp") == 0)
      {
      sscanf(buffer, "%d", &last_time);
      sscanf(buffer, "%[^<> \t\r\n]", info);
      buffer += strlen(info);
      }
   else if (strcmp(command, "expect") == 0)
      {
      sscanf(buffer, "%lf", &(LOCALAV.expect));
      sscanf(buffer, "%[^<> \t\r\n]", info);
      buffer += strlen(info);
      }
   else if (strcmp(command, "var") == 0)
      {
      sscanf(buffer, "%lf", &(LOCALAV.var));
      sscanf(buffer, "%[^<> \t\r\n]", info);
      buffer += strlen(info);
      }
   else if (strcmp(command, "entry") == 0)
      {
      parse_entry(&buffer);
      }
   else
      {
      fprintf(stderr, "Unrecognized tag: <%s>\n", command);
      CloseDatabase();
      exit(1);
      }
   skip(&buffer);
   sprintf(temp, "</%s>", command);
   if (strncmp(buffer, temp, strlen(temp)) != 0)
      {
      fprintf(stderr, "Couldn't find </%s>!\n", command);
      memset(temp, 0, sizeof(temp));
      strncpy(temp, buffer, 20);
      Debug("Next 20 chars in buffer: \"%s\"\n", temp);
      CloseDatabase();
      exit(1);
      }
   buffer += strlen(temp);
   skip(&buffer);
   }

Verbose("Storing database age (%d)\n", (int)AGE);
memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));
value.data = &AGE;
value.size = sizeof(double);
key.data = "DATABASE_AGE";
key.size = strlen("DATABASE_AGE") + 1;

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   exit(1);
   }

Verbose("Storing timestamp (%d)\n", (int)last_time);
memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));
value.data = &last_time;
value.size = sizeof(last_time);
key.data = "TIMESTAMP";
key.size = strlen("TIMESTAMP") + 1;

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   exit(1);
   }

Verbose("Storing LOCALAV (expect: %.3lf, var: %.3lf)\n", LOCALAV.expect, LOCALAV.var);
memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));
value.data = &LOCALAV;
value.size = sizeof(LOCALAV);
key.data = "LOCALAV";
key.size = strlen("LOCALAV") + 1;

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   exit(1);
   }

Verbose("Storing step (%d)\n", (int)STEP);
memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));
value.data = &STEP;
value.size = sizeof(STEP);
key.data = "STEP";
key.size = strlen("STEP") + 1;

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   exit(1);
   }

Verbose("Storing iterations (%d)\n", (int)ITER);
memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));
value.data = &ITER;
value.size = sizeof(ITER);
key.data = "ITERATIONS";
key.size = strlen("ITERATIONS") + 1;

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   exit(1);
   }

CloseDatabase();
}

/*********************************************************************/

/* copied from RRD tool */

void skip(char **buf)

{ char *ptr;

ptr = (*buf);
do
   {
   (*buf) = ptr;
   
   while ((*(ptr + 1)) && 
          ((*ptr) == ' ' || (*ptr) == '\r' || 
           (*ptr) == '\n' || (*ptr) == '\t'))
      {
      ptr++;
      }
   
   if (strncmp(ptr, "<!--", 4) == 0)
      {
      ptr = strstr(ptr, "-->");
      if (ptr)
          ptr += 3;
      else
         {
         fprintf(stderr, "Syntax error: Dangling Comment");
         CloseDatabase();
         exit(1);
         }
      }
   }
while ((*buf) != ptr);
}

/*********************************************************************/

void parse_entry(char **buffer)

{ int t=0, e=0, v=0;
  struct Average current;
  char timekey[CF_BUFSIZE], temp[CF_BUFSIZE];
  char command[CF_BUFSIZE];

while(strncmp(*buffer, "</entry>", 8) != 0)
   {
   sscanf(*buffer, "<%[^<> \t\r\n]>", command);
   (*buffer) += strlen(command) + 2;
   skip(buffer);

   if (strlen(command) == 0)
      {
      fprintf(stderr, "Couldn't get next command!\n");
      CloseDatabase();
      exit(1);
      }
   else if(strcmp(command, "timekey") == 0)
      {
      sscanf(*buffer, "%[^<> \t\r\n]", timekey);
      (*buffer) += strlen(timekey);

      if(strlen(timekey) != 0)
         {
         t++;
         }
      }
   else if(strcmp(command, "expect") == 0)
      {
      sscanf(*buffer, "%[^<> \t\r\n]", temp);
      sscanf(temp, "%lf", &(current.expect));
      (*buffer) += strlen(temp);

      if (strlen(temp) != 0)
         {
         e++;
         }
      }
   else if(strcmp(command, "var") == 0)
      {
      sscanf(*buffer, "%[^<> \t\r\n]", temp);
      sscanf(temp, "%lf", &(current.var));
      (*buffer) += strlen(temp);

      if(strlen(temp) != 0)
         {
         v++;
         }
      }
   else
      {
      fprintf(stderr, "Unrecognized tag: <%s>\n", command);
      CloseDatabase();
      exit(1);
      }
   skip(buffer);
   sprintf(temp, "</%s>", command);

   if (strncmp(*buffer, temp, strlen(temp)) != 0)
      {
      fprintf(stderr, "Couldn't find </%s>!\n", command);
      memset(temp, 0, sizeof(temp));
      strncpy(temp, *buffer, 20);
      Debug("Next 20 chars in buffer: \"%s\"\n", temp);
      CloseDatabase();
      exit(1);
      }
   (*buffer) += strlen(temp);
   skip(buffer);
   }

if (t != 1 || e != 1 || v != 1)
   {
   fprintf(stderr, 
           "Couldn't find exactly one <timekey>, <expect> and <var> inside <entry> tag\n"
           );
   CloseDatabase();
   exit(1);
   }

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));
key.data = timekey;
key.size = strlen(timekey) + 1;
value.data = &current;
value.size = sizeof(current);

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   exit(1);
   }
}

/*********************************************************************/

void GetDatabaseAge(int dbtype)

{ int errno;

STEP = 5;
AGE = 0;
WAGE = 0;

Verbose("Getting database age and step\n");

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

key.data = "STEP";
key.size = strlen("STEP") + 1;

if ((errno = dbp->get(dbp, NULL, &key, &value, 0)) != 0)
   {
   if (errno != DB_NOTFOUND)
      {
      dbp->err(dbp, errno, NULL);
      CloseDatabase();
      return;
      }
   }

if (value.data != NULL)
   {
   STEP = *(int *)(value.data);
   }
else
   {
   Debug("No STEP!\n");
   }

INTERVAL = STEP * 60;
Verbose("Step: %d (Interval: %d)\n", (int)STEP, (int)INTERVAL);

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));
key.data = "DATABASE_AGE";
key.size = strlen("DATABASE_AGE") + 1;

if ((errno = dbp->get(dbp, NULL, &key, &value, 0)) != 0)
   {
   if (errno != DB_NOTFOUND)
      {
      dbp->err(dbp, errno, NULL);
      CloseDatabase();
      return;
      }
   }

if (value.data != NULL)
   {
   AGE = *(double *)(value.data);
   switch(dbtype)
      {
      case DAILY:
          WAGE = (double)(AGE*STEP)/1440.0;
          break;
      case YEARLY:
          WAGE = (double)(AGE*STEP)/(1440.0*365.242);
          break;
      default: /* weekly */
          WAGE = AGE / ONE_WEEK * INTERVAL;
          break;
      }
   }
else
   {
   Debug("No previous AGE\n");
   }

Verbose("Age: %d (%.3lf", (int)AGE, (double)WAGE);
switch(dbtype)
   {
   case DAILY:
       Verbose(" days)\n");
       break;
   case YEARLY:
       Verbose(" years)\n");
       break;
   default: /* weekly */
       Verbose(" weeks)\n");
       break;
   }
}

/*********************************************************************/

struct Average EvalAvQ(char *t, int dbtype, int update)

{ struct Average *currentvals, newvals;
  int i;
  double new_value;

if ((currentvals = GetCurrentAverage(t, dbtype)) == NULL)
   {
   fprintf(stderr, "Error reading average database\n");
   exit(1);
   }

/* Discard any apparently anomalous behaviour before renormalizing database 
 */

if (update)
   {
   new_value =
       RejectAnomaly(VALUE, currentvals->expect, currentvals->var,
                     LOCALAV.expect, LOCALAV.var);
   }
else
   {
   new_value = VALUE;
   }

Verbose("new_value = %lf\n", (double)new_value);

Debugging("new_value = RejectAnomaly(%lf, %lf, %lf, %lf, %lf) = %lf\n",
          (double)VALUE,
          (double)currentvals->expect,
          (double)currentvals->var,
          (double)LOCALAV.expect, (double)LOCALAV.var, (double)new_value);

newvals.expect = WAverage(new_value, currentvals->expect, WAGE);

Verbose("newvals.expect = %lf\n", (double)newvals.expect);

Debugging("newvals.expect = WAverage(%lf, %lf, %lf) = %lf\n",
          (double)new_value,
          (double)currentvals->expect,
          (double)WAGE, (double)newvals.expect);

LOCALAV.expect = WAverage(newvals.expect, LOCALAV.expect, ITER);

Verbose("LOCALAV.expect = %lf\n", (double)LOCALAV.expect);

Debugging("LOCALAV.expect = WAverage(%lf, %lf, %lf) = %lf\n",
          (double)newvals.expect,
          (double)LOCALAV.expect, (double)ITER, (double)LOCALAV.expect);

newvals.var = WAverage((new_value - newvals.expect) * (new_value - newvals.expect), currentvals->var, WAGE);

Verbose("newvals.var = %lf\n", (double)newvals.var);

Debugging("newvals.var = WAverage(%lf, %lf, %lf) = %lf\n",
          (double)(new_value - newvals.expect) * (new_value -
                                                  newvals.expect),
          (double)currentvals->var, (double)WAGE, (double)newvals.var);

LOCALAV.var =
    WAverage((new_value - LOCALAV.expect) * (new_value - LOCALAV.expect),
             LOCALAV.var, ITER);

Verbose("LOCALAV.var = %lf\n", (double)LOCALAV.var);

Debugging("LOCALAV.var = WAverage(%lf, %lf, %lf) = %lf\n",
          (double)(new_value - LOCALAV.expect) * (new_value -
                                                  LOCALAV.expect),
          (double)LOCALAV.var, (double)ITER, (double)LOCALAV.var);

Debugging("VALUE: %d, new_value: %lf\n", (int)VALUE, new_value);
Debugging("newvals.expect: %lf, sqrt(newvals.var): %lf\n",
          newvals.expect, sqrt(newvals.var));
Debugging("               LOCALAV.expect: %lf, sqrt(LOCALAV.var): %lf\n",
          LOCALAV.expect, sqrt(LOCALAV.var));

if (update)
   {
   UpdateAverage(t, newvals);
   
   if (WAGE > CFGRACEPERIOD)
      {
      UpdateDistribution(t, currentvals, dbtype); /* Distribution about mean */
      }
   }

return newvals;
}

/*********************************************************************/

struct Average *GetCurrentAverage(char *timekey, int dbtype)

{ int errno;
  static struct Average entry;
  char str[64];

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

key.data = "TIMESTAMP";
key.size = strlen("TIMESTAMP") + 1;

if ((errno = dbp->get(dbp, NULL, &key, &value, 0)) != 0)
   {
   if (errno != DB_NOTFOUND)
      {
      dbp->err(dbp, errno, NULL);
      CloseDatabase();
      return NULL;
      }
   }

if (value.data != NULL
    && (last_time = *(time_t *) value.data) + 2 * INTERVAL > update_time)
   {
   key.data = "ITERATIONS";
   key.size = strlen("ITERATIONS") + 1;
   
   if ((errno = dbp->get(dbp, NULL, &key, &value, 0)) != 0)
      {
      if (errno != DB_NOTFOUND)
         {
         dbp->err(dbp, errno, NULL);
         CloseDatabase();
         return NULL;
         }
      }
   
   if (value.data != NULL)
      {
      ITER = *(double *)value.data;
      }
   else
      {
      Debug("No previous iterations\n");
      }
   }
else
   {
   Debug("Resetting ITER\n");
   }

Verbose("timestamp is %d, ITER = %d\n", (int)last_time, (int)ITER);

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));
key.data = "LOCALAV";
key.size = strlen("LOCALAV") + 1;

if ((errno = dbp->get(dbp, NULL, &key, &value, 0)) != 0)
   {
   if (errno != DB_NOTFOUND)
      {
      dbp->err(dbp, errno, NULL);
      CloseDatabase();
      return NULL;
      }
   }

if (value.data != NULL)
   {
   memcpy(&LOCALAV, value.data, sizeof(LOCALAV));
   }
else
   {
   Debug("No previous LOCALAV\n");
   }
Verbose("LOCALAV.expect=%.3lf, LOCALAV.var=%.3lf\n", LOCALAV.expect, LOCALAV.var);

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));
memset(&entry, 0, sizeof(entry));
key.data = timekey;
key.size = strlen(timekey) + 1;

if ((errno = dbp->get(dbp, NULL, &key, &value, 0)) != 0)
   {
   Debugging("Error getting previous value\n");
   if (errno != DB_NOTFOUND)
      {
      dbp->err(dbp, errno, NULL);
      CloseDatabase();
      return NULL;
      }
   }

if (value.data != NULL)
   {
   memcpy(&entry, value.data, sizeof(entry));
   Verbose("Got value (%.3lf,%.3lf) at time key %s\n", entry.expect, entry.var, timekey);
   Debugging("Previous value (%lf,%lf) for time index %s\n\n", entry.expect,
             entry.var, timekey);
   }
else
   {
   Debugging("No previous value for time index %s\n", timekey);
   Debug("No previous value for time index %s\n", timekey);
   }

sprintf(str, "%s", ctime(&last_time));
Verbose("time key for last update was %s\n", ConvTimeKey2(str, dbtype));
Verbose("comparing to current timekey %s\n", timekey);

if (strcmp(ConvTimeKey2(str, dbtype), timekey) != 0)
   {
   Verbose("incrementing age\n");
   AGE++;
   }
else
   {
   Verbose("NOT incrementing age\n");
   }
switch(dbtype)
   {
   case DAILY:
       WAGE = (double)(AGE*STEP)/1440.0;
       break;
    case YEARLY:
        WAGE = (double)(AGE*STEP)/(1440.0*365.242);
        break;
   default: /* weekly */
       WAGE = AGE / ONE_WEEK * INTERVAL;
       break;
   }

return &entry;
}

/*****************************************************************************/

void UpdateAverage(char *timekey, struct Average newvals)

{ int errno;
  time_t timestamp;

Verbose("Storing (%.3lf,%.3lf) at key %s\n", newvals.expect, newvals.var, timekey);

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

Debugging("Storing %s (expect: %lf, var: %lf)\n",
          timekey, (double)newvals.expect, (double)newvals.var);

key.data = timekey;
key.size = strlen(timekey) + 1;

value.data = &newvals;
value.size = sizeof(newvals);

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   return;
   }

Verbose("Storing age (%d)\n", (int)AGE);

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

value.data = &AGE;
value.size = sizeof(double);
key.data = "DATABASE_AGE";
key.size = strlen("DATABASE_AGE") + 1;

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   return;
   }

Verbose("Storing localav (%.3lf,%.3lf)\n", LOCALAV.expect, LOCALAV.var);

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

value.data = &LOCALAV;
value.size = sizeof(LOCALAV);
key.data = "LOCALAV";
key.size = strlen("LOCALAV") + 1;

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   return;
   }

ITER++;

Verbose("Storing iterations (%d)\n", (int)ITER);

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

value.data = &ITER;
value.size = sizeof(ITER);
key.data = "ITERATIONS";
key.size = strlen("ITERATIONS") + 1;

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   return;
   }

Verbose("Storing timestamp (%d)\n", update_time);

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

value.data = &update_time;
value.size = sizeof(update_time);
key.data = "TIMESTAMP";
key.size = strlen("TIMESTAMP") + 1;

if ((errno = dbp->put(dbp, NULL, &key, &value, 0)) != 0)
   {
   dbp->err(dbp, errno, NULL);
   CloseDatabase();
   return;
   }
}

/*****************************************************************************/

void UpdateDistribution(char *timekey, struct Average *av, int dbtype)

{ int position;
  int day, i, tmp, sig;
  char tempstr[4];
  tempstr[3] = '\0';

  /* Take an interval of 4 standard deviations from -2 to +2, divided into
   * CF_GRAINS parts. Centre each measurement on CF_GRAINS/2 and scale each
   * measurement by the std-deviation for the current time. */

if (HISTO)
   {
   Verbose("Updating histogram\n");
   switch(dbtype)
      {
      case DAILY:
          tmp = atoi(timekey);
          day = (double)(tmp*STEP)/60.0; /* day is actually hour */
        break;
      case YEARLY:
          strncpy(tempstr, timekey, 3);
          day = Month2Number(tempstr); /* day is actually month */
          if(day == -1)
             {
             fprintf(stderr, "Couldn't convert month to number\n");
             CloseDatabase();
             exit(1);
             }
          break;
      default: /* weekly */
          day = Day2Number(timekey); /* day really means day */
          break;
      }
   
   position = CF_GRAINS / 2 +
       (int)(0.5 + (VALUE - av->expect) * CF_GRAINS / (4.0 * sqrt(av->var)));
   
   if (0 <= position && position < CF_GRAINS)
      {
      HISTOGRAM[day][position]++;
      }
   
   if (!BATCH_MODE || time_to_update)
      {
      FILE *fp;
      char filename[CF_BUFSIZE];
      switch(dbtype)
         {
         case DAILY:
             snprintf(filename, CF_BUFSIZE, "%s/daily.hist", LOCATION);
             break;
         case YEARLY:
             snprintf(filename, CF_BUFSIZE, "%s/yearly.hist", LOCATION);
             break;
         default: /* weekly */
             snprintf(filename, CF_BUFSIZE, "%s/weekly.hist", LOCATION);
             break;
         }
      
      if ((fp = fopen(filename, "w")) == NULL)
         {
         fprintf(stderr, "Unable to save histograms\n");
         fprintf(stderr, "fopen: %s\n", strerror(errno));
         return;
         }
      
      for (position = 0; position < CF_GRAINS; position++)
         {
         fprintf(fp, "%u ", position);
         for (day = 0; day < 24; day++)
            {
            fprintf(fp, "%u ", HISTOGRAM[day][position]);
            }
         fprintf(fp, "\n");
         }
      
      fclose(fp);
      }
   }
}

/*********************************************************************/

double RejectAnomaly(double new, double average, double variance,
                     double localav, double localvar)
{ double dev = sqrt(variance + localvar);   /* Geometrical average dev */
  double delta;
  int bigger;

Verbose("Checking %.3lf for an anomaly..\n", new);

if (average == 0)
   {
   Verbose("Keeping value; no previous average\n");
   return new;
   }

if (new > big_number)
   {
   Verbose("Using previous average (%.3lf); new value is too big\n", average);
   return average;
   }

if ((new - average) * (new - average) < cf_noise_threshold * cf_noise_threshold)
   {
   Verbose("Keeping value; value is within noise threshold\n");
   return new;
   }

if (new - average > 0)
   {
   bigger = true;
   }
else
   {
   bigger = false;
   }

/* This routine puts some inertia into the changes, so that the system
 * doesn't respond to every little change ...  IR and UV cutoff */

delta = sqrt((new - average) * (new - average) + (new - localav) * (new - localav));

if (delta > 4.0 * dev)    /* IR */
   {
   srand48((unsigned int)time(NULL));
   
   if (drand48() < 0.7)    /* 70% chance of using full value - as in *
                            * learning policy */
      {
      Verbose("Keeping value; using full value of anomaly\n");
      return new;
      }
   else
      {
      if (bigger)
         {
         Verbose("Using previous ave (%.3lf) + 2 std dev (%.3lf); rejecting anomaly\n", average, dev);
         return average+2.0*dev;
         }
      else
         {
         Verbose("Using previous ave (%.3lf) - 2 std dev (%.3lf); rejecting anomaly\n", average, dev);
         return average-2.0*dev;
         }
      }
   }
else
   {
   Verbose("Keeping value; value is reasonable\n");
   return new;
   }
}

/*****************************************************************************/

double WAverage(double anew, double aold, double age)
 /* For a couple of weeks, learn eagerly. Otherwise variances will be way too 
  * large. Then downplay newer data somewhat, and rely on experience of a
  * couple of months of data ... */

{ double av;
  double wnew, wold;

if (age < 2.0)
   {
   wnew = 0.7;
   wold = 0.3;
   }
else
   {
   wnew = 0.3;
   wold = 0.7;
   }

av = (wnew * anew + wold * aold) / (wnew + wold);

if (av > big_number)  /* some kind of bug? */
   {
   Verbose("New average is too big; cutting it to 10.0\n");
   return 10.0;
   }

return av;
}

/*****************************************************************************/

void LoadHistogram(int dbtype)

{ FILE *fp;
  int position, i, day;
  
Verbose("LoadHistogram()\n");
  
if (HISTO)
   {
   char filename[CF_BUFSIZE];
   switch(dbtype)
      {
      case DAILY:
          snprintf(filename, CF_BUFSIZE, "%s/daily.hist", LOCATION);
          break;
      case YEARLY:
          snprintf(filename, CF_BUFSIZE, "%s/yearly.hist", LOCATION);
          break;
      default: /* weekly */
          snprintf(filename, CF_BUFSIZE, "%s/weekly.hist", LOCATION);
          break;
      }
   
   Verbose("Loading histogram: %s\n", filename);
   
   if ((fp = fopen(filename, "r")) == NULL)
      {
      fprintf(stderr, "fopen: %s\n", strerror(errno));
      fprintf(stderr, "Unable to load histogram data\n");
      return;
      }
   
   for (position = 0; position < CF_GRAINS; position++)
      {
      fscanf(fp, "%d ", &position);
      
      for (day = 0; day < 7; day++)
         {
         fscanf(fp, "%d ", &(HISTOGRAM[day][position]));
         }
      }
   
   Verbose("Done loading histogram.\n");
   
   fclose(fp);
   }
}

/*********************************************************************/

int ArmClasses(struct Average av, char *timekey)

{ double sig;
  int code;
  struct Item *classlist = NULL, *ip;
  int i;
  char buffer[CF_BUFSIZE];
  char temp1[256], temp2[256];
  FILE *newfp;
  FILE *oldfp;
  struct flock lock;

/* printf("Arm classes for %s\n", timekey); */
  
sig = SetClasses(NAME, VALUE, av.expect, av.var, LOCALAV.expect, LOCALAV.var,
                   &classlist, timekey, &code);
SetVariable(NAME,VALUE,av.expect,sig,&classlist);

if(cfenvd_compatible)
   {
   unlink(ENVCF_NEW);
   strcpy(temp1, NAME);
   strcat(temp1, "_");
   temp2[0] = '_';
   temp2[1] = '\0';
   strcat(temp2, NAME);
   strcat(temp2, "=");
   
   for (i=0; i<10; i++)
      {
      lock.l_type = F_RDLCK;
      lock.l_start = 0;
      lock.l_whence = SEEK_SET;
      lock.l_len = 0;
      
      if ((newfp = fopen(ENVCF_NEW,"a")) == NULL)
         {
         DeleteItemList(classlist);
         return code; 
         }
      if ((oldfp = fopen(ENVCF, "r")) != NULL)
         {
         if (fcntl(fileno(oldfp), F_SETLK, &lock) == -1)
            {
            Debug("Couldn't get lock, attempt %d of 10\n", i+1);
            fclose(oldfp);
            sleep(1);
            continue;
            }
         while(fgets(buffer, CF_BUFSIZE, oldfp) != NULL)
            {
            if(strstr(buffer, temp2) == NULL && strncmp(buffer, temp1, strlen(temp1)) != 0)
               {
               fprintf(newfp, "%s", buffer);
               }
            }
         fclose(oldfp);
         }
      
      for (ip = classlist; ip != NULL; ip=ip->next)
         {
         fprintf(newfp,"%s\n",ip->name);
         }
      
      fclose(newfp);
      
      rename(ENVCF_NEW,ENVCF);
      break;
      }
   }

DeleteItemList(classlist);

return code;
}

/*********************************************************************/

int SetClasses(char *name, double variable, double av_expect, double av_var,
                double localav_expect, double localav_var,
                struct Item **classlist, char *timekey, int *code)
{
  char buffer[CF_BUFSIZE], buffer2[CF_BUFSIZE];
  double dev, delta, sigma, ldelta, lsigma, sig;

delta = variable - av_expect;
sigma = sqrt(av_var);
ldelta = variable - localav_expect;
lsigma = sqrt(localav_var);
sig = sqrt(sigma * sigma + lsigma * lsigma);

if (sigma == 0.0 || lsigma == 0.0)
   {
   Debugging2(" No sigma variation .. can't measure class\n");
   Verbose("Returning -2 (No sigma variation)\n");
   *code = -2;
   return sig;
   }

if (fabs(delta) < cf_noise_threshold) /* Arbitrary limits on sensitivity */
   {
   Debug(" Sensitivity too high ..\n");
   
   buffer[0] = '\0';
   strcpy(buffer, name);
   
   if ((delta > 0) && (ldelta > 0))
      {
      strcat(buffer, "_high");
      *code = -6;
      }
   else if ((delta < 0) && (ldelta < 0))
      {
      strcat(buffer, "_low");
      *code = -4;
      }
   else
      {
      strcat(buffer, "_normal");
      *code = -5;
      }
   
   dev =
       sqrt(delta * delta / (1.0 + sigma * sigma) +
            ldelta * ldelta / (1.0 + lsigma * lsigma));
   
   if (dev > 2.0 * sqrt(2.0))
      {
      strcpy(buffer2, buffer);
      strcat(buffer2, "_microanomaly");
      Debugging2("!! %s !!\n", buffer2);
      *code += -10;
      AppendItem(classlist,buffer2,"2");
      if(cfenvd_compatible) AddPersistentClass(buffer2,40,cfpreserve); 
      }
   }
else
   {
   buffer[0] = '\0';
   strcpy(buffer, name);
   
   if ((delta > 0) && (ldelta > 0))
      {
      strcat(buffer, "_high");
      *code = -6;
      }
   else if ((delta < 0) && (ldelta < 0))
      {
      strcat(buffer, "_low");
      *code = -4;
      }
   else
      {
      strcat(buffer, "_normal");
      *code = -5;
      }
   
   dev =
       sqrt(delta * delta / (1.0 + sigma * sigma) +
            ldelta * ldelta / (1.0 + lsigma * lsigma));
   
   if (dev <= sqrt(2.0))
      {
      strcpy(buffer2, buffer);
      strcat(buffer2, "_normal");
      Debugging2("!! %s !!\n", buffer2);
      *code += -20;
      AppendItem(classlist,buffer2,"0");
      }
   else
      {
      strcpy(buffer2, buffer);
      strcat(buffer2, "_dev1");
      Debugging2("!! %s !!\n", buffer2);
      *code += -30;
      AppendItem(classlist,buffer2,"0");
      }

   if (dev > 2.0*sqrt(2.0))
      {
      strcpy(buffer2, buffer);
      strcat(buffer2, "_dev2");
      Debugging2("!! %s !!\n", buffer2);
      *code += -10;
      AppendItem(classlist,buffer2,"2");
      if(cfenvd_compatible) AddPersistentClass(buffer2,40,cfpreserve); 
      }
   
   if (dev > 3.0*sqrt(2.0))
      {
      strcpy(buffer2, buffer);
      strcat(buffer2, "_anomaly");
      Debugging2("!! %s !!\n", buffer2);
      *code += -10;
      AppendItem(classlist,buffer2,"3");
      if(cfenvd_compatible) AddPersistentClass(buffer2,40,cfpreserve); 
      }
   }

Verbose("Returning %d", *code);

if (*code < -40)
   {
   Verbose(" (Anomaly!)");
   }

Verbose("\n");
return sig;
}

/*****************************************************************************/

void SetVariable(char *name,double value,double average,double stddev,struct Item **classlist)

{ char var[CF_BUFSIZE];

sprintf(var,"value_%s=%d",name,(int)value);
AppendItem(classlist,var,"");

sprintf(var,"average_%s=%1.1f",name,average);
AppendItem(classlist,var,"");

sprintf(var,"stddev_%s=%1.1f",name,stddev);
AppendItem(classlist,var,""); 
}

/*********************************************************************/

void DoBatch(int dbtype)

{ FILE *fp;
  char buffer[1024], timekey[256];
  char name1[256], name2[256], timebuf[256];
  struct tm timevals;
  double val=0;
  float val1=0, val2=0, val3=0, val4=0, val5=0, val6=0, val7=0, val8=0, val9=0, val10=0;
  int i = 0, j = 0, n = 0, y = 0, k = 0, w = 0;
  int timeint = -1;
  struct Average av;

time_to_update = false;

Verbose("Batch mode\n");

LOCALAV.expect = 0.0;
LOCALAV.var = 0.0;
ITER = 0.0;

for (i = 0; i < 7; i++)
   {
   for (j = 0; j < CF_GRAINS; j++)
      {
      HISTOGRAM[i][j] = 0;
      }
   }
i = 0;

srand((unsigned int)time(NULL));
LoadHistogram(dbtype);
GetDatabaseAge(dbtype);

if ((fp = fopen(BATCHFILE, "r")) == NULL)
   {
   fprintf(stderr, "Cannot open %s\n", BATCHFILE);
   return;
   }

printf("\n                   [      ]");
printf("\ryear 01, month 01: ["); 
k++;

bzero(buffer, 1024);
fgets(buffer, 1024, fp);

while (!feof(fp))
   {
   if (n >= 11)
      {
      y++;
      n = -1;
      }
   if (k >= 5 * 24 * 60 * w / STEP)
      {
      w++;
      printf("."); 
      fflush(stdout);
      }
   if (k >= 30 * 24 * 60 / STEP)
      {
      n++;
      printf("\r                   [      ]\ryear %02d, month %02d: [", y+1, n+1); 
      fflush(stdout);
      k = 0;
      w = 0;
      }
   
   if (strlen(buffer) == 0)
      {
      bzero(buffer, 1024);
      fgets(buffer, 1024, fp);
      continue;
      }

   if (sscanf(buffer, "%d/%d/%d %d:%d:%d %s %s %lf",
              &(timevals.tm_mon), &(timevals.tm_mday), &(timevals.tm_year),
              &(timevals.tm_hour), &(timevals.tm_min), &(timevals.tm_sec),
              name1, name2, &val) != 9)
      {
      if (sscanf(buffer, 
                 "%*d %d %*d %lf %f %f %f %f %f %f %f %f %f %f",
                 &timeint, &val, &val1, &val2, &val3, &val4, &val5, &val6, &val7, 
                 &val8, &val9, &val10) == 12)
         {
         timevals.tm_year = timeint / 10000;
         timevals.tm_mon = (timeint / 100) % 100;
         timevals.tm_mday = timeint % 100;
         timevals.tm_hour = 12;
         timevals.tm_min = 0;
         timevals.tm_sec = 0;
         timeint = -1;
         }
      else if(sscanf(buffer, "%d/%d/%d %d:%d:%d %lf",
                     &(timevals.tm_mon), &(timevals.tm_mday), &(timevals.tm_year),
                     &(timevals.tm_hour), &(timevals.tm_min), &(timevals.tm_sec),
                     &val) == 7)
         {
         }
      else if (sscanf(buffer, "%d %lf", &timeint,&val) == 2)
         {
         }
      else
         {
         fprintf(stderr, "\nSkipping incomplete line: \"%s\"\n", buffer);
         bzero(buffer, 1024);
         fgets(buffer, 1024, fp);
         continue;
         }
      }
   
   Debug("%d/%d/%d %d:%d:%d %s %s %f\n", timevals.tm_mon, timevals.tm_mday,
         timevals.tm_year, timevals.tm_hour, timevals.tm_min, timevals.tm_sec,
         name1, name2, val);
   
   bzero(buffer, 1024);
   fgets(buffer, 1024, fp);
   
   timevals.tm_year = timevals.tm_year - 1900;
   timevals.tm_mon = timevals.tm_mon - 1;
   VALUE = (double)val;
   
   if (timeint < 0) {
   update_time = mktime(&timevals);
   } else {
   update_time = (time_t) timeint;
   }
   
   strcpy(timebuf, ctime(&update_time));
   Debug("- Time converted to %s, ", timebuf);
   if(strcmp(timekey, ConvTimeKey2(timebuf, dbtype)) != 0)
       k++;
   strcpy(timekey, ConvTimeKey2(timebuf, dbtype));
   Debug("then to %s\n", timekey);
   
   if (feof(fp)) 
       time_to_update = true;
   
   av = EvalAvQ(timekey, dbtype, true);
   ArmClasses(av, timekey);
   
   }



fclose(fp);
printf("\rDone                       \ndatabase saved to: %s\n", AVDB);
printf("Run cfetoolgraph %s [-tTrndwy] [--path %s] to generate graphs\n\n", NAME, PATHNAME); 
}

/*****************************************************************************/

void Syntax()

{ int i;

printf("Syntax:\n\n");

CreateSyntax();
printf("\n");
UpdateSyntax();
printf("\n");
CheckSyntax();
printf("\n");
InfoSyntax();
printf("\n");
DumpSyntax();
printf("\n");
ImportSyntax();
}

/*********************************************************************/

void CreateSyntax()

{ int i;

printf("%s create name ", PROG_NAME);

for (i = 0; CREATEOPTIONS[i].name != NULL; i++)
   {
   printf("(--%s|-%c", CREATEOPTIONS[i].name, (char)CREATEOPTIONS[i].val);
   if (CREATEOPTIONS[i].has_arg == required_argument)
      {
      printf(" ...");
      }
   else if (CREATEOPTIONS[i].has_arg == optional_argument)
      {
      printf(" [...]");
      }
   printf(") ");
   }
printf("\n");
}

/*********************************************************************/

void UpdateSyntax()

{ int i;

printf("%s update name ", PROG_NAME);
for (i = 0; UPDATEOPTIONS[i].name != NULL; i++)
   {
   printf("(--%s|-%c", UPDATEOPTIONS[i].name, (char)UPDATEOPTIONS[i].val);
   if (UPDATEOPTIONS[i].has_arg == required_argument)
      {
      printf(" ...");
      }
   else if (UPDATEOPTIONS[i].has_arg == optional_argument)
      {
      printf(" [...]");
      }
   printf(") ");
   }
printf("\n");
}

/*********************************************************************/

void CheckSyntax()

{ int i;

printf("%s check name ", PROG_NAME);
for (i = 0; CHECKOPTIONS[i].name != NULL; i++)
   {
   printf("(--%s|-%c", CHECKOPTIONS[i].name, (char)CHECKOPTIONS[i].val);
   if (CHECKOPTIONS[i].has_arg == required_argument)
      {
      printf(" ...");
      }
   else if (CHECKOPTIONS[i].has_arg == optional_argument)
      {
      printf(" [...]");
      }
   printf(") ");
   }
printf("\n");
}

/*********************************************************************/

void InfoSyntax()

{ int i;
 printf("%s info name ", PROG_NAME);

 for (i = 0; INFOOPTIONS[i].name != NULL; i++)
    {
    printf("(--%s|-%c", INFOOPTIONS[i].name, (char)INFOOPTIONS[i].val);

    if (INFOOPTIONS[i].has_arg == required_argument)
       {
       printf(" ...");
       }
    else if (INFOOPTIONS[i].has_arg == optional_argument)
       {
       printf(" [...]");
       }
    printf(") ");
    }

printf("\n");
}

/*********************************************************************/

void DumpSyntax()

{ int i;

printf("%s dump name ", PROG_NAME);

for (i = 0; DUMPOPTIONS[i].name != NULL; i++)
   {
   printf("(--%s|-%c", DUMPOPTIONS[i].name, (char)DUMPOPTIONS[i].val);
   if (DUMPOPTIONS[i].has_arg == required_argument)
      {
      printf(" ...");
      }
   else if (DUMPOPTIONS[i].has_arg == optional_argument)
      {
      printf(" [...]");
      }
   printf(") ");
   }
printf("\n");
}

/*********************************************************************/

void ImportSyntax()

{ int i;

printf("%s import name ", PROG_NAME);
for (i = 0; IMPORTOPTIONS[i].name != NULL; i++)
   {
   printf("(--%s|-%c", IMPORTOPTIONS[i].name, (char)IMPORTOPTIONS[i].val);
   if (IMPORTOPTIONS[i].has_arg == required_argument)
      {
      printf(" ...");
      }
   else if (IMPORTOPTIONS[i].has_arg == optional_argument)
      {
      printf(" [...]");
      }
   printf(") ");
   }
printf("\n");
}

/*********************************************************************/

void CloseDatabase()

{
Verbose("Closing database\n");
dbp->close(dbp, 0);
}

/*********************************************************************/

int OpenDatabase(int create)

{ int flags;

if(create)
   {
   flags = DB_CREATE|DB_EXCL;
   }
else
   {
   flags = 0;
   }

if ((errno = db_create(&dbp, NULL, 0)) != 0)
   {
   snprintf(OUTPUT, CF_BUFSIZE, "Couldn't create database handle\n");
   fprintf(stderr, "db_create: %s\n", strerror(errno));
   fprintf(stderr, "%s", OUTPUT);
   return -1;
  }

#ifdef CF_OLD_DB
if ((errno = (dbp->open)(dbp, AVDB, NULL, DB_BTREE, flags, 0644)) != 0)
#else
if ((errno = (dbp->open)(dbp, NULL, AVDB, NULL, DB_BTREE, flags, 0644)) != 0)
#endif
   {
   snprintf(OUTPUT, CF_BUFSIZE, "Couldn't open average database %s\n", AVDB);
   fprintf(stderr, "db_open: %s\n", strerror(errno));
   fprintf(stderr, "%s", OUTPUT);
   return -1;
   }

Verbose("Opened database\n");
return 0;
}

/*********************************************************************/

char *GenTimeKey2(time_t now, int dbtype)

{ char str[64];

sprintf(str, "%s", ctime(&now));
return ConvTimeKey2(str, dbtype);
}

/*********************************************************************/

char *ConvTimeKey2(char *str, int dbtype)

{ int i, hr, min, timeinmins;
  char buf1[10], buf2[10], buf3[10], buf4[10], buf5[10], buf[10], out[10];
  char minbuf[10];
  char *timekey;

timekey = calloc(64, sizeof(char));

sscanf(str, "%s %s %s %s %s", buf1, buf2, buf3, buf4, buf5);

timekey[0] = '\0';

/* Day */

switch(dbtype)
   {
   case DAILY:
       break;
   case YEARLY:
       sprintf(timekey, "%s%s:", buf2, buf3);
       break;
   default: /* weekly */
       sprintf(timekey, "%s:", buf1);
       break;
   }

/* Hours */

sscanf(buf4, "%d:%d", &hr, &min);
timeinmins = 60*hr + min;

if(STEP == 1)
   {
   sprintf(minbuf, "%04d", timeinmins / STEP );
   }
else if (STEP < 15)
   {
   sprintf(minbuf, "%03d", timeinmins / STEP );
   }
else if (STEP < 145)
   {
   sprintf(minbuf, "%02d", timeinmins / STEP );
   }
else
   {
   sprintf(minbuf, "%d", timeinmins / STEP );
   }

strcat(timekey, minbuf);

return timekey;
}

/*********************************************************************/

void FatalError(char *s)

{
 fprintf(stderr, "%s\n", s);
 exit(1);
}

void yyerror(char *s)
{
 /* Dummy */
}

void RotateFiles(char *name, int number)
{
 /* Dummy */
}

/*********************************************************************/

int RecursiveTidySpecialArea(char *name, struct Tidy *tp, int maxrecurse,
                             struct stat *sb)
{
 return true;
}

int Repository(char *file, char *repository)
{
 return false;
}

char *GetMacroValue(char *s, char *sp)
{
 return NULL;
}

void Banner(char *s)
{
}

void AddMacroValue(char *scope, char *name, char *value)
{
}

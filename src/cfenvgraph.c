/* 

        Copyright (C) 2001-
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
/* File: cfenvgraph.c                                                        */
/*                                                                           */
/* Created: Wed Apr 18 13:19:22 2001                                         */
/*                                                                           */
/* Author: Mark                                                              */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"
#include <math.h>
#include <db.h>

/*****************************************************************************/
/* Prototypes                                                                */
/*****************************************************************************/

void CheckOpts(int argc, char **argv);
void Syntax(void);
void ReadAverages(void);
void SummarizeAverages(void);
void WriteGraphFiles(void);
void WriteHistograms(void);
void DiskArrivals(void);
void PeerIntermittency(void);
void GetFQHN(void);
void OpenFiles(void);
void CloseFiles(void);
void MagnifyNow(void);
void OpenMagnifyFiles(void);
void CloseMagnifyFiles(void);
void EraseAverages(void);

/*****************************************************************************/

struct option GRAPHOPTIONS[] =
   {
   { "help",no_argument,0,'h' },
   { "file",required_argument,0,'f' },
   { "erasehistory",required_argument,0,'E' },
   { "outputdir",required_argument,0,'o' },
   { "titles",no_argument,0,'t'},
   { "timestamps",no_argument,0,'T'},
   { "resolution",no_argument,0,'r'},
   { "separate",no_argument,0,'s'},
   { "no-error-bars",no_argument,0,'e'},
   { "no-scaling",no_argument,0,'n'},
   { "now",no_argument,0,'N'},
   { "verbose",no_argument,0,'v'},
   { NULL,0,0,0 }
   };

int TITLES = false;
int TIMESTAMPS = false;
int HIRES = false;
int SEPARATE = false;
int ERRORBARS = true;
int NOSCALING = true;
int NOWOPT = false;

char FILENAME[CF_BUFSIZE];
unsigned int HISTOGRAM[CF_OBSERVABLES][7][CF_GRAINS];
int SMOOTHHISTOGRAM[CF_OBSERVABLES][7][CF_GRAINS];
char VFQNAME[CF_MAXVARSIZE];
char ERASE[CF_BUFSIZE];
int ERRNO;
time_t NOW;

DB *DBP;
static struct Averages ENTRY,MAX,MIN,DET;
char TIMEKEY[CF_SMALLBUF],FLNAME[CF_BUFSIZE],*sp;
double AGE;
FILE *FPAV=NULL,*FPVAR=NULL, *FPNOW=NULL;
FILE *FPE[CF_OBSERVABLES],*FPQ[CF_OBSERVABLES];
FILE *FPM[CF_OBSERVABLES];

/*****************************************************************************/

int main (int argc,char **argv)

{
CheckOpts(argc,argv);
GetFQHN();

if (strlen(ERASE) > 0)
   {
   EraseAverages();
   exit(0);
   }


ReadAverages(); 
SummarizeAverages();

if (strlen(FLNAME) == 0)
   {
   if (TIMESTAMPS)
      {
      if ((NOW = time((time_t *)NULL)) == -1)
         {
         Verbose("Couldn't read system clock\n");
         }
      sprintf(FLNAME,"cfenvgraphs-%s-%s",CanonifyName(VFQNAME),ctime(&NOW));
      }
   else
      {
      sprintf(FLNAME,"cfenvgraphs-snapshot-%s",CanonifyName(VFQNAME));
      }
   }

Verbose("Creating sub-directory %s\n",FLNAME);

if (mkdir(FLNAME,0755) == -1)
   {
   Verbose("Writing to existing directory\n");
   }
 
if (chdir(FLNAME))
   {
   perror("chdir");
   exit(0);
   }

Verbose("Writing data to sub-directory %s: \n   x,y1,y2,y3...\n ",FLNAME);

if (NOWOPT)
   {
   MagnifyNow();
   }
else
   {
   WriteGraphFiles();
   WriteHistograms();
   DiskArrivals();
   PeerIntermittency();
   }

return 0;
}

/*****************************************************************************/
/* Level 1                                                                   */
/*****************************************************************************/

void GetFQHN()

{ FILE *pp;
  char cfcom[CF_BUFSIZE];
  static char line[CF_BUFSIZE],*sp;

snprintf(cfcom,CF_BUFSIZE-1,"%s/bin/cfagent -Q fqhost",CFWORKDIR);
 
if ((pp=popen(cfcom,"r")) ==  NULL)
   {
   Verbose("Couldn't open cfengine data ");
   perror("popen");
   exit(0);
   }

line[0] = '\0'; 
fgets(line,CF_BUFSIZE,pp);
for (sp = line; *sp != '\0'; sp++)
   {
   if (*sp == '=')
      {
      sp++;
      break;
      }
   }

strcpy(VFQNAME,line);

if (strlen(VFQNAME) == 0)
   {
   struct utsname sys;
   if (uname(&sys) == -1)
      {
      perror("uname ");
      exit(0);
      }
   strcpy(VFQNAME,sys.sysname);
   } 
else
   {
   VFQNAME[strlen(VFQNAME)-1] = '\0';
   Verbose("Got fully qualified name (%s)\n",VFQNAME);
   }
 
pclose(pp);
}

/****************************************************************************/

void ReadAverages()

{ int i;
  DBT key,value;

Verbose("\nLooking for database %s\n",FILENAME);
Verbose("\nFinding MAXimum values...\n\n");
Verbose("N.B. socket values are numbers in CLOSE_WAIT. See documentation.\n"); 
  
if ((ERRNO = db_create(&DBP,NULL,0)) != 0)
   {
   Verbose("Couldn't create average database %s\n",FILENAME);
   exit(1);
   }

#ifdef CF_OLD_DB 
if ((ERRNO = (DBP->open)(DBP,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)
#else
if ((ERRNO = (DBP->open)(DBP,NULL,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)    
#endif
   {
   Verbose("Couldn't open average database %s\n",FILENAME);
   DBP->err(DBP,ERRNO,NULL);
   exit(1);
   }

for (i = 0; i < CF_OBSERVABLES; i++)
   {
   MAX.Q[i].var = MAX.Q[i].expect = MAX.Q[i].q = 0.01;
   MIN.Q[i].var = MIN.Q[i].expect = MIN.Q[i].q = 9999.0;
   FPE[i] = FPQ[i] = NULL;
   }
 
for (NOW = CF_MONDAY_MORNING; NOW < CF_MONDAY_MORNING+CF_WEEK; NOW += CF_MEASURE_INTERVAL)
   {
   memset(&key,0,sizeof(key));       
   memset(&value,0,sizeof(value));
   memset(&ENTRY,0,sizeof(ENTRY));

   strcpy(TIMEKEY,GenTimeKey(NOW));

   key.data = TIMEKEY;
   key.size = strlen(TIMEKEY)+1;
   
   if ((ERRNO = DBP->get(DBP,NULL,&key,&value,0)) != 0)
      {
      if (ERRNO != DB_NOTFOUND)
         {
         DBP->err(DBP,ERRNO,NULL);
         exit(1);
         }
      }
   
   if (value.data != NULL)
      {
      memcpy(&ENTRY,value.data,sizeof(ENTRY));
      
      for (i = 0; i < CF_OBSERVABLES; i++)
         {
         if (fabs(ENTRY.Q[i].expect) > MAX.Q[i].expect)
            {
            MAX.Q[i].expect = fabs(ENTRY.Q[i].expect);
            }

         if (fabs(ENTRY.Q[i].q) > MAX.Q[i].q)
            {
            MAX.Q[i].q = fabs(ENTRY.Q[i].q);
            }

         if (fabs(ENTRY.Q[i].expect) < MIN.Q[i].expect)
            {
            MIN.Q[i].expect = fabs(ENTRY.Q[i].expect);
            }
         
         if (fabs(ENTRY.Q[i].q) < MIN.Q[i].q)
            {
            MIN.Q[i].q = fabs(ENTRY.Q[i].q);
            }
         }
      }
   }
 
 DBP->close(DBP,0);
}

/****************************************************************************/

void EraseAverages()

{ int i;
  DBT key,value;
  struct Item *list = NULL;
      
Verbose("\nLooking through current database %s\n",FILENAME);

list = SplitStringAsItemList(ERASE,',');

if ((ERRNO = db_create(&DBP,NULL,0)) != 0)
   {
   Verbose("Couldn't create average database %s\n",FILENAME);
   exit(1);
   }

#ifdef CF_OLD_DB 
if ((ERRNO = (DBP->open)(DBP,FILENAME,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((ERRNO = (DBP->open)(DBP,NULL,FILENAME,NULL,DB_BTREE,DB_CREATE,0644)) != 0)    
#endif
   {
   Verbose("Couldn't open average database %s\n",FILENAME);
   DBP->err(DBP,ERRNO,NULL);
   exit(1);
   }

memset(&key,0,sizeof(key));       
memset(&value,0,sizeof(value));

for (i = 0; i < CF_OBSERVABLES; i++)
   {
   FPE[i] = FPQ[i] = NULL;
   }
 
for (NOW = CF_MONDAY_MORNING; NOW < CF_MONDAY_MORNING+CF_WEEK; NOW += CF_MEASURE_INTERVAL)
   {
   memset(&key,0,sizeof(key));       
   memset(&value,0,sizeof(value));
   memset(&ENTRY,0,sizeof(ENTRY));

   strcpy(TIMEKEY,GenTimeKey(NOW));

   key.data = TIMEKEY;
   key.size = strlen(TIMEKEY)+1;
   
   if ((ERRNO = DBP->get(DBP,NULL,&key,&value,0)) != 0)
      {
      if (ERRNO != DB_NOTFOUND)
         {
         DBP->err(DBP,ERRNO,NULL);
         exit(1);
         }
      }
   
   if (value.data != NULL)
      {
      memcpy(&ENTRY,value.data,sizeof(ENTRY));
      
      for (i = 0; i < CF_OBSERVABLES; i++)
         {
         if (IsItemIn(list,OBS[i][0]))
            {
            /* Set history but not most recent to zero */
            ENTRY.Q[i].expect = 0;
            ENTRY.Q[i].var = 0;
            }
         }

      value.data = &ENTRY;
      
      if ((ERRNO = DBP->put(DBP,NULL,&key,&value,0)) != 0)
         {
         DBP->err(DBP,ERRNO,NULL);
         exit(1);
         }
      }
   }
 
DBP->close(DBP,0);
}

/*****************************************************************************/

void SummarizeAverages()

{ int i;
  DBT key,value;

Verbose(" x  yN (Variable content)\n---------------------------------------------------------\n");

 for (i = 0; i < CF_OBSERVABLES; i++)
   {
   Verbose("%2d. MAX <%-10s-in>   = %10f - %10f u %10f\n",i,OBS[i][0],MIN.Q[i].expect,MAX.Q[i].expect,sqrt(MAX.Q[i].var));
   }
 
if ((ERRNO = db_create(&DBP,NULL,0)) != 0)
   {
   Verbose("Couldn't open average database %s\n",FILENAME);
   exit(1);
   }

#ifdef CF_OLD_DB 
if ((ERRNO = (DBP->open)(DBP,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)
#else
if ((ERRNO = (DBP->open)(DBP,NULL,FILENAME,NULL,DB_BTREE,DB_RDONLY,0644)) != 0)
#endif
   {
   Verbose("Couldn't open average database %s\n",FILENAME);
   exit(1);
   }

memset(&key,0,sizeof(key));       
memset(&value,0,sizeof(value));
      
key.data = "DATABASE_AGE";
key.size = strlen("DATABASE_AGE")+1;

if ((ERRNO = DBP->get(DBP,NULL,&key,&value,0)) != 0)
   {
   if (ERRNO != DB_NOTFOUND)
      {
      DBP->err(DBP,ERRNO,NULL);
      exit(1);
      }
   }
 
if (value.data != NULL)
   {
   AGE = *(double *)(value.data);
   Verbose("\n\nDATABASE_AGE %.1f (weeks)\n\n",AGE/CF_WEEK*CF_MEASURE_INTERVAL);
   }
}

/*****************************************************************************/

void WriteGraphFiles()

{ int its,i,j,k, count = 0;
  DBT key,value;
  struct stat statbuf;

OpenFiles();

if (TITLES)
   {
   for (i = 0; i < CF_OBSERVABLES; i+=2)
      {
      fprintf(FPAV,"# Column %d: %s\n",i,OBS[i][0]);
      fprintf(FPVAR,"# Column %d: %s\n",i,OBS[i][0]);
      fprintf(FPNOW,"# Column %d: %s\n",i,OBS[i][0]);
      }

   fprintf(FPAV,"##############################################\n");
   fprintf(FPVAR,"##############################################\n");
   fprintf(FPNOW,"##############################################\n");
   }

if (HIRES)
   {
   its = 1;
   }
else
   {
   its = 12;
   }

NOW = CF_MONDAY_MORNING;
memset(&ENTRY,0,sizeof(ENTRY)); 
 
while (NOW < CF_MONDAY_MORNING+CF_WEEK)
   {
   for (j = 0; j < its; j++)
      {
      memset(&key,0,sizeof(key));       
      memset(&value,0,sizeof(value));
      
      strcpy(TIMEKEY,GenTimeKey(NOW));
      
      key.data = TIMEKEY;
      key.size = strlen(TIMEKEY)+1;

      if ((ERRNO = DBP->get(DBP,NULL,&key,&value,0)) != 0)
         {
         if (ERRNO != DB_NOTFOUND)
            {
            DBP->err(DBP,ERRNO,NULL);
            exit(1);
            }
         }

      /* Work out local average over grain size "its" */
      
      if (value.data != NULL)
         {
         memcpy(&DET,value.data,sizeof(DET));
         
         for (i = 0; i < CF_OBSERVABLES; i++)
            {
            ENTRY.Q[i].expect += DET.Q[i].expect/(double)its;
            ENTRY.Q[i].var += DET.Q[i].var/(double)its;
            ENTRY.Q[i].q += DET.Q[i].q/(double)its;
            }         
         
         if (NOSCALING)
            {            
            for (i = 1; i < CF_OBSERVABLES; i++)
               {
               MAX.Q[i].expect = 1;
               MAX.Q[i].q = 1;
               }
            }
         }
      
      NOW += CF_MEASURE_INTERVAL;
      count++;
      }

   /* Output the data in a plethora of files */
   
   fprintf(FPAV,"%d ",count);
   fprintf(FPVAR,"%d ",count);
   fprintf(FPNOW,"%d ",count);

   for (i = 0; i < CF_OBSERVABLES; i++)
      {
      fprintf(FPAV,"%f ",ENTRY.Q[i].expect/MAX.Q[i].expect);
      fprintf(FPVAR,"%f ",ENTRY.Q[i].var/MAX.Q[i].var);
      fprintf(FPNOW,"%f ",ENTRY.Q[i].q/MAX.Q[i].q);
      }                        
   
   fprintf(FPAV,"\n");
   fprintf(FPVAR,"\n");
   fprintf(FPNOW,"\n");
   
   if (SEPARATE)
      {
      for (i = 0; i < CF_OBSERVABLES; i++)
         {
         fprintf(FPE[i],"%d %f %f\n",count, ENTRY.Q[i].expect, sqrt(ENTRY.Q[i].var));
         /* Use same scaling for Q so graphs can be merged */
         fprintf(FPQ[i],"%d %f 0.0\n",count, ENTRY.Q[i].q);
         }               
      }
   
   memset(&ENTRY,0,sizeof(ENTRY));
   }

DBP->close(DBP,0);

CloseFiles();
}

/*****************************************************************************/

void MagnifyNow()

{ int its,i,j,k, count = 0;
  DBT key,value;
  time_t now;

OpenMagnifyFiles();

its = 1; /* detailed view */

now = time(NULL);
NOW = now - (time_t)(4 * CF_TICKS_PER_HOUR);
 
while (NOW < now)
   {
   memset(&ENTRY,0,sizeof(ENTRY)); 

   for (j = 0; j < its; j++)
      {
      memset(&key,0,sizeof(key));       
      memset(&value,0,sizeof(value));
      
      strcpy(TIMEKEY,GenTimeKey(NOW));
      
      key.data = TIMEKEY;
      key.size = strlen(TIMEKEY)+1;

      if ((ERRNO = DBP->get(DBP,NULL,&key,&value,0)) != 0)
         {
         if (ERRNO != DB_NOTFOUND)
            {
            DBP->err(DBP,ERRNO,NULL);
            exit(1);
            }
         }

      /* Work out local average over grain size "its" */
      
      if (value.data != NULL)
         {
         memcpy(&DET,value.data,sizeof(DET));
         
         for (i = 0; i < CF_OBSERVABLES; i++)
            {
            ENTRY.Q[i].expect += DET.Q[i].expect/(double)its;
            ENTRY.Q[i].var += DET.Q[i].var/(double)its;
            ENTRY.Q[i].q += DET.Q[i].q/(double)its;
            }         
         
         if (NOSCALING)
            {            
            for (i = 1; i < CF_OBSERVABLES; i++)
               {
               MAX.Q[i].expect = 1;
               MAX.Q[i].q = 1;
               }
            }
         }
      
      NOW += CF_MEASURE_INTERVAL;
      count++;
      }

   /* Output q and E/sig data in a plethora of files */

   for (i = 0; i < CF_OBSERVABLES; i++)
      {
      fprintf(FPM[i],"%d %f %f %f\n",count, ENTRY.Q[i].expect, sqrt(ENTRY.Q[i].var),ENTRY.Q[i].q);
      }               
   }

DBP->close(DBP,0);
CloseMagnifyFiles();
}

/*****************************************************************************/

void WriteHistograms()

{ int i,j,k;
 
/* Finally, look at the histograms */
 
 for (i = 0; i < 7; i++)
    {
    for (j = 0; j < CF_OBSERVABLES; j++)
       {
       for (k = 0; k < CF_GRAINS; k++)
          {
          HISTOGRAM[j][i][k] = 0;
          }
       }
    }
 
 if (SEPARATE)
    {
    int position,day;
    int weekly[CF_OBSERVABLES][CF_GRAINS];
    FILE *fp;
    
    snprintf(FLNAME,CF_BUFSIZE,"%s/state/histograms",CFWORKDIR);
    
    if ((fp = fopen(FLNAME,"r")) == NULL)
       {
       Verbose("Unable to load histogram data\n");
       exit(1);
       }
    
    for (position = 0; position < CF_GRAINS; position++)
       {
       fscanf(fp,"%d ",&position);
       
       for (i = 0; i < CF_OBSERVABLES; i++)
          {
          for (day = 0; day < 7; day++)
             {
             fscanf(fp,"%d ",&(HISTOGRAM[i][day][position]));
             }
          
          weekly[i][position] = 0;
          }
       }
    
    fclose(fp);
    
    if (!HIRES)
       {
       /* Smooth daily and weekly histograms */
       for (k = 1; k < CF_GRAINS-1; k++)
          {
          for (j = 0; j < CF_OBSERVABLES; j++)
             {
             for (i = 0; i < 7; i++)  
                {
                SMOOTHHISTOGRAM[j][i][k] = ((double)(HISTOGRAM[j][i][k-1] + HISTOGRAM[j][i][k] + HISTOGRAM[j][i][k+1]))/3.0;
                }
             }
          }
       }
    else
       {
       for (k = 1; k < CF_GRAINS-1; k++)
          {
          for (j = 0; j < CF_OBSERVABLES; j++)
             {
             for (i = 0; i < 7; i++)  
                {
                SMOOTHHISTOGRAM[j][i][k] = (double) HISTOGRAM[j][i][k];
                }
             }
          }
       }
    
    
    for (i = 0; i < CF_OBSERVABLES; i++)
       {
       sprintf(FLNAME,"%s.distr",OBS[i][0]); 
       if ((FPQ[i] = fopen(FLNAME,"w")) == NULL)
          {
          perror("fopen");
          exit(1);
          }
       }
    
    /* Plot daily and weekly histograms */
    for (k = 0; k < CF_GRAINS; k++)
       {
       int a;
       
       for (j = 0; j < CF_OBSERVABLES; j++)
          {
          for (i = 0; i < 7; i++)  
             {
             weekly[j][k] += (int) (SMOOTHHISTOGRAM[j][i][k]+0.5);
             }
          }
       
       for (a = 0; a < CF_OBSERVABLES; a++)
          {
          fprintf(FPQ[a],"%d %d\n",k,weekly[a][k]);
          }
       }
    
    for (i = 0; i < CF_OBSERVABLES; i++)
       {
       fclose(FPQ[i]);
       }
    }
}


/*****************************************************************************/

void DiskArrivals(void)

{ DIR *dirh;
  FILE *fp; 
  struct dirent *dirp;
  int count = 0, index = 0, i;
  char filename[CF_BUFSIZE],database[CF_BUFSIZE];
  double val, maxval = 1.0, *array, grain = 0.0;
  time_t now;
  DBT key,value;
  DB *dbp = NULL;
  DB_ENV *dbenv = NULL;


if ((array = (double *)malloc((int)CF_WEEK)) == NULL)
   {
   Verbose("Memory error");
   perror("malloc");
   return;
   }
  
if ((dirh = opendir(CFWORKDIR)) == NULL)
   {
   Verbose("Can't open directory %s\n",CFWORKDIR);
   perror("opendir");
   return;
   }

Verbose("\n\nLooking for filesystem arrival process data in %s\n",CFWORKDIR); 

for (dirp = readdir(dirh); dirp != NULL; dirp = readdir(dirh))
   {
   if (strncmp(dirp->d_name,"scan:",5) == 0)
      {
      Verbose("Found %s - generating X,Y plot\n",dirp->d_name);

      snprintf(database,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,dirp->d_name);
      
      if ((ERRNO = db_create(&dbp,dbenv,0)) != 0)
         {
         Verbose("Couldn't open arrivals database %s\n",database);
         return;
         }
      
#ifdef CF_OLD_DB
      if ((ERRNO = (dbp->open)(dbp,database,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
      if ((ERRNO = (dbp->open)(dbp,NULL,database,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
         {
         Verbose("Couldn't open database %s\n",database);
         dbp->close(dbp,0);
         continue;
         }
      
      maxval = 1.0;
      grain = 0.0;
      count = 0.0;
      index = 0;
      
      for (now = CF_MONDAY_MORNING; now < CF_MONDAY_MORNING+CF_WEEK; now += CF_MEASURE_INTERVAL)
         {
         memset(&key,0,sizeof(key));       
         memset(&value,0,sizeof(value));
         
         strcpy(TIMEKEY,GenTimeKey(now));
         
         key.data = TIMEKEY;
         key.size = strlen(TIMEKEY)+1;
         
         if ((ERRNO = dbp->get(dbp,NULL,&key,&value,0)) != 0)
            {
            if (ERRNO != DB_NOTFOUND)
               {
               DBP->err(DBP,ERRNO,NULL);
               exit(1);
               }
            }
         
         if (value.data != NULL)
            {
            grain += (double)*(double *)(value.data);
            }
         else
            {
            grain = 0;
            }
         
         if (HIRES)
            {
            if (grain > maxval)
               {
               maxval = grain;
               }
            
            array[index] = grain;
            grain = 0.0;     
            index++;
            }
         else
            {
            if (count % 12 == 0)
               {
               if (grain > maxval)
                  {
                  maxval = grain;
                  }
               array[index] = grain;
               index++;
               grain = 0.0;
               }
            }            
         count++;
         }
      
      dbp->close(dbp,0);
      
      snprintf(filename,CF_BUFSIZE-1,"%s.cfenv",dirp->d_name);
      
      if ((fp = fopen(filename,"w")) == NULL)
         {
         Verbose("Unable to open %s for writing\n",filename);
         perror("fopen");
         return;
         }
      
      Verbose("Data points = %d\n",index);
      
      for (i = 0; i < index; i++)
         {
         if (i > 1 && i < index-1)
            {
            val = (array[i-1]+array[i]+array[i+1])/3.0;  /* Smoothing */
            }
         else
            {
            val = array[i];
            }
         fprintf(fp,"%d %f\n",i,val/maxval*50.0);
         }
      
      fclose(fp);      
      }
   }
 
closedir(dirh);
}

/***************************************************************/

void PeerIntermittency()

{ DBT key,value;
  DB *dbp,*dbpent;
  DBC *dbcp;
  DB_ENV *dbenv = NULL, *dbenv2 = NULL;
  int i,ret;
  FILE *fp1,*fp2;
  char name[CF_BUFSIZE],hostname[CF_BUFSIZE],timekey[CF_MAXVARSIZE];
  char out1[CF_BUFSIZE],out2[CF_BUFSIZE];
  struct QPoint entry;
  struct Item *ip, *hostlist = NULL;
  double entropy,average,var,sum,sum_av;
  time_t now = time(NULL), then, lastseen = CF_WEEK;

snprintf(name,CF_BUFSIZE-1,"%s/%s",CFWORKDIR,CF_LASTDB_FILE);

average = (double) CF_HOUR;  /* It will take a week for a host to be deemed reliable */
var = 0;

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   Verbose("Couldn't open last-seen database %s\n",name);
   return;
   }

#ifdef CF_OLD_DB
if ((errno = (dbp->open)(dbp,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = (dbp->open)(dbp,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   Verbose("Couldn't open last-seen database %s\n",name);
   dbp->close(dbp,0);
   return;
   }

if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
   {
   Verbose("Error reading from last-seen database\n");
   dbp->err(dbp, ret, "DB->cursor");
   return;
   }

memset(&key, 0, sizeof(key));
memset(&value, 0, sizeof(value));

Verbose("Examining known peers...\n");

while (dbcp->c_get(dbcp, &key, &value, DB_NEXT) == 0)
   {
   strcpy(hostname,IPString2Hostname((char *)key.data+1));

   if (!IsItemIn(hostlist,hostname))
      {
      /* Check hostname not recorded twice with +/- */
      AppendItem(&hostlist,hostname,NULL);
      Verbose("Examining intermittent host %s\n",hostname);
      }
   }

dbcp->c_close(dbcp);
dbp->close(dbp,0);


/* Now go through each host and recompute entropy */

for (ip = hostlist; ip != NULL; ip=ip->next)
   {
   snprintf(out1,CF_BUFSIZE,"lastseen-%s.q",ip->name);

   Verbose("Opening %s\n",out1);
   
   if ((fp1 = fopen(out1,"w")) == NULL)
      {
      Verbose("Unable to open %s\n",out1);
      continue;
      }

   snprintf(out2,CF_BUFSIZE,"lastseen-%s.E-sigma",hostname);
   if ((fp2 = fopen(out2,"w")) == NULL)
      {
      Verbose("Unable to open %s\n",out1);
      continue;
      }
   
   snprintf(name,CF_BUFSIZE-1,"%s/%s.%s",CFWORKDIR,CF_LASTDB_FILE,ip->name);
   Verbose("Consulting profile %s\n",name);

   if ((errno = db_create(&dbpent,dbenv2,0)) != 0)
      {
      Verbose("Couldn't init reliability profile database %s\n",name);
      return;
      }
   
#ifdef CF_OLD_DB
   if ((errno = (dbpent->open)(dbpent,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
   if ((errno = (dbpent->open)(dbpent,NULL,name,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
      {
      Verbose("Couldn't open last-seen database %s\n",name);
      continue;
      }

   for (now = CF_MONDAY_MORNING; now < CF_MONDAY_MORNING+CF_WEEK; now += CF_MEASURE_INTERVAL)
      {
      memset(&key,0,sizeof(key));       
      memset(&value,0,sizeof(value));
      
      strcpy(timekey,GenTimeKey(now));
      
      key.data = timekey;
      key.size = strlen(timekey)+1;

      if ((errno = dbpent->get(dbpent,NULL,&key,&value,0)) != 0)
         {
         if (errno != DB_NOTFOUND)
            {
            dbpent->err(dbp,errno,NULL);
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

         fprintf(fp2,"%d %lf %lf\n",now,average,sqrt(var));
         }
      else
         {
         /* If we have no data, it means no contact for whatever reason.
            It could be unable to respond unwilling to respond, policy etc.
            Assume for argument that we expect regular responses ... */
         
         lastseen += CF_MEASURE_INTERVAL; /* infer based on no data */
         }

      fprintf(fp1,"%d %d\n",now,lastseen);
      }

   fclose(fp1);
   fclose(fp2);
   dbpent->close(dbpent,0);
   }

DeleteItemList(hostlist);
}



/*****************************************************************************/
/* Level 2                                                                   */
/*****************************************************************************/

void CheckOpts(int argc,char **argv)

{ extern char *optarg;
  int optindex = 0;
  int c;

 /* XXX Initialize workdir for non privileged users */

strcpy(CFWORKDIR,WORKDIR);
FLNAME[0] = '\0';
ERASE[0] = '\0';
VERBOSE = false;

if (geteuid() > 0)
   {
   char *homedir;
   if ((homedir = getenv("HOME")) != NULL)
      {
      strcpy(CFWORKDIR,homedir);
      strcat(CFWORKDIR,"/.cfagent");
      }
   }

snprintf(FILENAME,CF_BUFSIZE,"%s/state/%s",CFWORKDIR,CF_AVDB_FILE);

while ((c=getopt_long(argc,argv,"Thtf:o:rsenNEv:",GRAPHOPTIONS,&optindex)) != EOF)
  {
  switch ((char) c)
      {
      case 'E':
          strncpy(ERASE,optarg,CF_BUFSIZE-1);
          break;
          
      case 't':
          TITLES = true;
          break;

      case 'f':
          strcpy(FILENAME,optarg);
          break;

      case 'o': strcpy(FLNAME,optarg);
          Verbose("Setting output directory to s\n",FLNAME);
          break;

      case 'T': TIMESTAMPS = true;
          break;

      case 'v': VERBOSE = true;
         break;

      case 'r': HIRES = true;
         break;

      case 's': SEPARATE = true;
          break;

      case 'e': ERRORBARS = false;
          break;

      case 'n': NOSCALING = true;
          break;
          
      case 'N': NOWOPT = true;
          break;

      default:  Syntax();
                exit(1);

      }
   }
}

/*****************************************************************************/

void Syntax()

{ int i;

printf("Cfengine Environment Graph Generator\n%s\n%s\n",VERSION,COPYRIGHT);
printf("\n");
printf("Options:\n\n");

for (i=0; GRAPHOPTIONS[i].name != NULL; i++)
   {
   printf("--%-20s    (-%c)\n",GRAPHOPTIONS[i].name,(char)GRAPHOPTIONS[i].val);
   }

printf("\nBug reports to bug-cfengine@cfengine.org\n");
printf("General help to help-cfengine@cfengine.org\n");
printf("Info & fixes at http://www.cfengine.org\n");
}

/*********************************************************************/

void OpenFiles()

{ int i;
 
sprintf(FLNAME,"cfenv-average");

if ((FPAV = fopen(FLNAME,"w")) == NULL)
   {
   perror("fopen");
   exit(1);
   }

sprintf(FLNAME,"cfenv-stddev"); 

if ((FPVAR = fopen(FLNAME,"w")) == NULL)
   {
   perror("fopen");
   exit(1);
   }

sprintf(FLNAME,"cfenv-now"); 

if ((FPNOW = fopen(FLNAME,"w")) == NULL)
   {
   perror("fopen");
   exit(1);
   }


/* Now if -s open a file foreach metric! */

if (SEPARATE)
   {
   for (i = 0; i < CF_OBSERVABLES; i++)
      {
      sprintf(FLNAME,"%s.E-sigma",OBS[i][0]);
      
      if ((FPE[i] = fopen(FLNAME,"w")) == NULL)
         {
         perror("fopen");
         exit(1);
         }
      
      sprintf(FLNAME,"%s.q",OBS[i][0]);
      
      if ((FPQ[i] = fopen(FLNAME,"w")) == NULL)
         {
         perror("fopen");
         exit(1);
         }

      }
   }
}

/*********************************************************************/

void CloseFiles()

{ int i;
 
fclose(FPAV);
fclose(FPVAR);
fclose(FPNOW); 

if (SEPARATE)
   {
   for (i = 0; i < CF_OBSERVABLES; i++)
      {
      fclose(FPE[i]);
      fclose(FPQ[i]);
      }
   }
}

/*********************************************************************/

void OpenMagnifyFiles()

{ int i;
 
for (i = 0; i < CF_OBSERVABLES; i++)
   {
   sprintf(FLNAME,"%s.mag",OBS[i][0]);
   
   if ((FPM[i] = fopen(FLNAME,"w")) == NULL)
      {
      perror("fopen");
      exit(1);
      }
   }
}

/*********************************************************************/

void CloseMagnifyFiles()

{ int i;

for (i = 0; i < CF_OBSERVABLES; i++)
   {
   fclose(FPM[i]);
   }
}



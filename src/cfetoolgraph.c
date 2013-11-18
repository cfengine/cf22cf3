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
/* File: cfetoolgraph.c                                                      */
/*                                                                           */
/* Description: Standalone UI for Long term state registry                   */
/*                                                                           */
/*****************************************************************************/

#include "../pub/getopt.h"
#include "cf.defs.h"
#include "cf.extern.h"
#include "cfetooldefs.h"
#include <math.h>
#include <db.h>

struct option GRAPHOPTIONS[] = {
  {"path", required_argument, 0, 'p'},
  {"help", no_argument, 0, 'h'},
  {"timestamps", no_argument, 0, 'T'},
  {"resolution", no_argument, 0, 'r'},
  {"no-scaling", no_argument, 0, 'n'},
  {"weekly", no_argument, 0, 'w'},
  {"daily", no_argument, 0, 'd'},
  {"yearly", no_argument, 0, 'y'},
  {NULL, 0, 0, 0}
};

int weekly = false;
int daily = false;
int yearly = false;
char *NAME;
char LOCATION[CF_BUFSIZE];
char *PROG_NAME;
char FILENAME[CF_BUFSIZE];
char PATHNAME[CF_BUFSIZE];
int INTERVAL;
int TITLES = false;
int TIMESTAMPS = false;
int HIRES = false;
int NOSCALING = true;
int STEP;
unsigned int HISTOGRAM[24][CF_GRAINS];
int SMOOTHHISTOGRAM[24][CF_GRAINS];

static struct Average ENTRY, MAX, MIN, DET;
char TIMEKEY[64], FLNAME[CF_BUFSIZE], DIRNAME[CF_BUFSIZE], *sp;
double AGE;
int errno, i, j, k, count = 0, its;
time_t NOW;
DBT key, value;
DB *DBP;
FILE *FPAV = NULL, *FPVAR = NULL, *FPHIST = NULL, *FP = NULL;


void ReadAverages (int dbtype);
void SummarizeAverages (void);
void WriteGraphFiles (int dbtype);
void WriteHistogram (int dbtype);
void FindHurstExponents (int dbtype);
struct Average FindHurstFunction
(int sameples_per_grain, int grains, int dbtype);
void Syntax (void);

/*********************************************************************/

int main(int argc, char **argv)
{
  extern char *optarg;
  int optindex = 0;
  int c;
  char temp[CF_BUFSIZE];
  memset(PATHNAME, 0, sizeof(PATHNAME));
  PROG_NAME = argv[0];

  if (argc < 2)
  {
    Syntax();
    exit(1);
  }

  NAME = argv[1];
  if(NAME[0] == '-') {
    Syntax();
    exit(1);
  }
  argv++;
  argc--;

  while ((c =
          getopt_long(argc, argv, "Thtrndwyp:", GRAPHOPTIONS, &optindex)) != EOF)
  {
    switch ((char)c)
    {
      case 't':
        TITLES = true;
        break;

      case 'T':
        TIMESTAMPS = true;
        break;

      case 'r':
        HIRES = true;
        break;

      case 'n':
        NOSCALING = true;
        break;

      case 'p':
        strcpy(PATHNAME,optarg);
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
        Syntax();
        exit(1);

    }
  }

  if(PATHNAME[0] != '\0')
    sprintf(LOCATION, "%s/%s", PATHNAME, NAME);
  else
    sprintf(LOCATION, "./%s", NAME);

  if(daily) {
    snprintf(FILENAME, CF_BUFSIZE, "%s/daily.db", LOCATION);
    ReadAverages(DAILY);
    SummarizeAverages();
    WriteGraphFiles(DAILY);
    WriteHistogram(DAILY);
    FindHurstExponents(DAILY);
  }
  if(weekly || (!daily && !yearly)) {
    snprintf(FILENAME, CF_BUFSIZE, "%s/weekly.db", LOCATION);
    ReadAverages(WEEKLY);
    SummarizeAverages();
    WriteGraphFiles(WEEKLY);
    WriteHistogram(WEEKLY);
    FindHurstExponents(WEEKLY);
  }
  if(yearly) {
    snprintf(FILENAME, CF_BUFSIZE, "%s/yearly.db", LOCATION);
    ReadAverages(YEARLY);
    SummarizeAverages();
    WriteGraphFiles(YEARLY);
    WriteHistogram(YEARLY);
    FindHurstExponents(YEARLY);
  }

  printf("Done. Graph files created in directory:\n%s\n", DIRNAME);
  return 0;
}

/*****************************************************************************/

void ReadAverages(int dbtype)
{
  int begin_time = 0;
  int total_time = 0;
  switch(dbtype)
  {
    case DAILY:
      begin_time = MONDAY_MORNING;
      total_time = ONE_DAY;
      break;
    case YEARLY:
      begin_time = JANUARY_FIRST;
      total_time = ONE_YEAR;
      break;
    default: /* weekly */
      begin_time = MONDAY_MORNING;
      total_time = ONE_WEEK;
      break;
  }
  printf("\nLooking for database %s\n", FILENAME);
  printf("\nFinding MAXimum values...\n\n");

  if ((errno = db_create(&DBP, NULL, 0)) != 0)
  {
    printf("Couldn't create average database %s\n", FILENAME);
    exit(1);
  }

#ifdef CF_OLD_DB
  if ((errno =
       (DBP->open)(DBP, FILENAME, NULL, DB_BTREE, DB_RDONLY, 0644)) != 0)
#else
  if ((errno =
       (DBP->open)(DBP, NULL, FILENAME, NULL, DB_BTREE, DB_RDONLY, 0644)) != 0)
#endif
  {
    printf("Couldn't open average database %s\n", FILENAME);
    DBP->err(DBP, errno, NULL);
    exit(1);
  }

  memset(&key, 0, sizeof(key));
  memset(&value, 0, sizeof(value));

  key.data = "STEP";
  key.size = strlen("STEP") + 1;

  if ((errno = DBP->get(DBP, NULL, &key, &value, 0)) != 0)
  {
    if (errno != DB_NOTFOUND)
    {
      DBP->err(DBP, errno, NULL);
      DBP->close(DBP, 0);
      return;
    }
  }

  if (value.data != NULL)
  {
    STEP = *(int *)(value.data);
  }
  else
  {
    fprintf(stderr, "Database has no STEP\n");
    exit(1);
  }
  INTERVAL = 60 * STEP;

  MAX.expect = 0.01;
  MAX.var = 0.01;

  MIN.expect = 9999.0;
  MIN.var = 9999.0;

  for (NOW = begin_time; NOW < begin_time + total_time;
       NOW += INTERVAL)
  {
    memset(&key, 0, sizeof(key));
    memset(&value, 0, sizeof(value));
    memset(&ENTRY, 0, sizeof(ENTRY));

    strcpy(TIMEKEY, GenTimeKey2(NOW, dbtype));

    key.data = TIMEKEY;
    key.size = strlen(TIMEKEY) + 1;

    if ((errno = DBP->get(DBP, NULL, &key, &value, 0)) != 0)
    {
      if (errno != DB_NOTFOUND)
      {
        DBP->err(DBP, errno, NULL);
        exit(1);
      }
    } 


    if (value.data != NULL)
    {
      memcpy(&ENTRY, value.data, sizeof(ENTRY));

      if (fabs(ENTRY.expect) > MAX.expect)
      {
        MAX.expect = fabs(ENTRY.expect);
      }

      if (fabs(ENTRY.expect) < MIN.expect)
      {
        MIN.expect = fabs(ENTRY.expect);
      }

      if (fabs(ENTRY.var) > MAX.var)
      {
        MAX.var = fabs(ENTRY.var);
      }
    }
  }

  DBP->close(DBP, 0);
}

/*****************************************************************************/

void SummarizeAverages()
{

  printf
    (" x  yN (Variable content)\n---------------------------------------------------------\n");
  printf(" 1. MAX = %10f - %10f u %10f\n", MIN.expect, MAX.expect,
         sqrt(MAX.var));

  if ((errno = db_create(&DBP, NULL, 0)) != 0)
  {
    printf("Couldn't open average database %s\n", FILENAME);
    exit(1);
  }

#ifdef CF_OLD_DB
  if ((errno =
       (DBP->open)(DBP, FILENAME, NULL, DB_BTREE, DB_RDONLY, 0644)) != 0)
#else
  if ((errno =
       (DBP->open)(DBP, NULL, FILENAME, NULL, DB_BTREE, DB_RDONLY, 0644)) != 0)
#endif
  {
    printf("Couldn't open average database %s\n", FILENAME);
    exit(1);
  }

  memset(&key, 0, sizeof(key));
  memset(&value, 0, sizeof(value));

  key.data = "DATABASE_AGE";
  key.size = strlen("DATABASE_AGE") + 1;

  if ((errno = DBP->get(DBP, NULL, &key, &value, 0)) != 0)
  {
    if (errno != DB_NOTFOUND)
    {
      DBP->err(DBP, errno, NULL);
      exit(1);
    }
  }

  if (value.data != NULL)
  {
    AGE = *(double *)(value.data);
    printf("\n\nDATABASE_AGE %.4f (weeks), %lf intervals\n\n", AGE / CF_WEEK * INTERVAL, AGE);
  }
}

/*****************************************************************************/

void WriteGraphFiles(int dbtype)
{
  int begin_time = 0;
  int total_time = 0;
  switch(dbtype)
  {
    case DAILY:
      begin_time = MONDAY_MORNING;
      total_time = ONE_DAY;
      break;
    case YEARLY:
      begin_time = JANUARY_FIRST;
      total_time = ONE_YEAR;
      break;
    default: /* weekly */
      begin_time = MONDAY_MORNING;
      total_time = ONE_WEEK;
      break;
  }
  if (TIMESTAMPS)
  {
    if ((NOW = time((time_t *) NULL)) == -1)
    {
      printf("Couldn't read system clock\n");
    }

    switch(dbtype)
    {
      case DAILY:
        sprintf(DIRNAME, "%s/daily-%s", LOCATION, ctime(&NOW));
        break;
      case YEARLY:
        sprintf(DIRNAME, "%s/yearly-%s", LOCATION, ctime(&NOW));
        break;
      default: /* weekly */
        sprintf(DIRNAME, "%s/weekly-%s", LOCATION, ctime(&NOW));
        break;
    }

    for (sp = DIRNAME; *sp != '\0'; sp++)
    {
      if (isspace((int)*sp))
      {
        *sp = '_';
      }
    }
  }
  else
  {
    switch(dbtype)
    {
      case DAILY:
        sprintf(DIRNAME, "%s/daily-snapshot", LOCATION);
        break;
      case YEARLY:
        sprintf(DIRNAME, "%s/yearly-snapshot", LOCATION);
        break;
      default: /* weekly */
        sprintf(DIRNAME, "%s/weekly-snapshot", LOCATION);
        break;
    }
  }

  printf("Creating directory %s\n", DIRNAME);

  if (mkdir(DIRNAME, 0755) == -1)
  {
    perror("mkdir");
    printf("Aborting\n");
    exit(0);
  }


  printf("Writing data to directory %s\n ", DIRNAME);


  sprintf(FLNAME, "%s/average", DIRNAME);

  if ((FPAV = fopen(FLNAME, "w")) == NULL)
  {
    perror("fopen");
    exit(1);
  }

  sprintf(FLNAME, "%s/stddev", DIRNAME);

  if ((FPVAR = fopen(FLNAME, "w")) == NULL)
  {
    perror("fopen");
    exit(1);
  }

  sprintf(FLNAME,"%s/graph", DIRNAME); 

  if ((FP = fopen(FLNAME,"w")) == NULL)
  {
    perror("fopen");
    exit(1);
  }

  if (HIRES)
  {
    its = 1;
  }
  else
  {
    its = 12;
  }

  count = 0;
  NOW = begin_time;
  memset(&ENTRY, 0, sizeof(ENTRY));

  while (NOW < begin_time + total_time)
  {
    for (j = 0; j < its; j++)
    {
      memset(&key, 0, sizeof(key));
      memset(&value, 0, sizeof(value));

      strcpy(TIMEKEY, GenTimeKey2(NOW, dbtype));
      key.data = TIMEKEY;
      key.size = strlen(TIMEKEY) + 1;

      if ((errno = DBP->get(DBP, NULL, &key, &value, 0)) != 0)
      {
        if (errno != DB_NOTFOUND)
        {
          DBP->err(DBP, errno, NULL);
          exit(1);
        }
      } 


      if (value.data != NULL)
      {
        memcpy(&DET, value.data, sizeof(DET));

        ENTRY.expect += DET.expect / (double)its;
        ENTRY.var += DET.var / (double)its;

        if (NOSCALING)
        {
          MAX.expect = 1;
        }

        if (j == its - 1)
        {
          fprintf(FPAV, "%d %f\n", count++, ENTRY.expect / MAX.expect);

          fprintf(FPVAR, "%d %f\n", count, sqrt(ENTRY.var) / MAX.expect);

          fprintf(FP,"%d %f %f\n",count,ENTRY.expect/MAX.expect,sqrt(ENTRY.var)/MAX.expect);

          memset(&ENTRY, 0, sizeof(ENTRY));
        }
      }

      NOW += INTERVAL;
    }
  }

  DBP->close(DBP, 0);

  fclose(FPAV);
  fclose(FPVAR);

}

/*****************************************************************************/

void WriteHistogram(int dbtype)
{
  int numdays=0;
  int position, day;
  int weekly[CF_GRAINS];

  /* Finally, look at the histogram */

  printf("Writing histogram file now!\n");

  for (i = 0; i < 24; i++)
  {
    for (k = 0; k < CF_GRAINS; k++)
    {
      HISTOGRAM[i][k] = 0;
    }
  }


  switch(dbtype)
  {
    case DAILY:
      snprintf(FLNAME, CF_BUFSIZE, "%s/daily.hist", LOCATION);
      break;
    case YEARLY:
      snprintf(FLNAME, CF_BUFSIZE, "%s/yearly.hist", LOCATION);
      break;
    default: /* weekly */
      snprintf(FLNAME, CF_BUFSIZE, "%s/weekly.hist", LOCATION);
      break;
  }

  if ((FPHIST = fopen(FLNAME, "r")) == NULL)
  {
    printf("Unable to load histogram data\n");
    return;
  }

  for (position = 0; position < CF_GRAINS; position++)
  {
    fscanf(FPHIST, "%d ", &position);

    for (day = 0; day < 24; day++)
    {
      fscanf(FPHIST, "%d ", &(HISTOGRAM[day][position]));
    }

    weekly[position] = 0;
  }

  fclose(FPHIST);

  switch(dbtype)
  {
    case DAILY:
      numdays = 24;
      break;
    case YEARLY:
      numdays = 12;
      break;
    default: /* weekly */
      numdays = 7;
      break;
  }

  if (!HIRES)
  {
    /* Smooth daily and weekly histograms */
    for (k = 1; k < CF_GRAINS - 1; k++)
    {
      for (i = 0; i < numdays; i++)
      {
        SMOOTHHISTOGRAM[i][k] = ((double)
                                 (HISTOGRAM[i][k - 1] +
                                  HISTOGRAM[i][k] +
                                  HISTOGRAM[i][k + 1])) / 3.0;
      }
    }
  }
  else
  {
    for (k = 0; k < CF_GRAINS; k++)
    {
      for (i = 0; i < numdays; i++)
      {
        SMOOTHHISTOGRAM[i][k] = (double)HISTOGRAM[i][k];
      }
    }
  }

  sprintf(FLNAME, "%s/distr", DIRNAME);
  if ((FPHIST = fopen(FLNAME, "w")) == NULL)
  {
    perror("fopen");
    exit(1);
  }

  /* Plot daily and weekly histograms */
  for (k = 0; k < CF_GRAINS; k++)
  {
    int a;
    for (i = 0; i < numdays; i++)
    {
      weekly[k] += (int)(SMOOTHHISTOGRAM[i][k] + 0.5);
    }

    fprintf(FPHIST, "%d %d\n", k, weekly[k]);
  }

  fclose(FPHIST);
  printf("Done writing histogram file (%s)\n", FLNAME);
}

/*****************************************************************************/

void FindHurstExponents(int dbtype)
{
  int delta_t[5], grains[5], i, j;
  int samples_per_grain[5];
  double dilatation, uncertainty;
  struct Average H[5], M[5], h2;

  /* Dilatation intervals */

  delta_t[0] = INTERVAL * 2;
  delta_t[1] = 3600;
  delta_t[2] = 6 * 3600;
  delta_t[3] = 24 * 3600;
  delta_t[4] = CF_WEEK;

  memset(&h2, 0, sizeof(struct Average));

  for (i = 0; i < 5; i++)
  {
    grains[i] = CF_WEEK / delta_t[i];
    samples_per_grain[i] = delta_t[i] / INTERVAL;
    H[i] = FindHurstFunction(samples_per_grain[i], grains[i], dbtype);
  }

  printf
    ("\n============================================================================\n");
  printf("Fluctuation measures - Hurst exponent estimates\n");
  printf
    ("============================================================================\n");

  for (i = 1; i < 5; i++)
  {
    dilatation = (double)delta_t[i] / (double)delta_t[0];
    M[i].expect = log(H[i].expect / H[0].expect) / log(dilatation);
    printf(" M[%d] = %f\n", i, M[i].expect);

    h2.expect += M[i].expect * M[i].expect / 4.0;

    uncertainty =
      1.0 / fabs(1.0 / H[i].expect -
                 1.0 / H[0].expect) * sqrt(MAX.var / log(dilatation)) /
      (MAX.expect * 2.0);
  }

  printf("\n\nESTIMATED RMS HURST EXPONENTS...\n\n");
  printf("Hurst exponent  = %.1f u %.2f - order of mag\n", sqrt(h2.expect),
         uncertainty);

}


/*****************************************************************************/

char *CanonifyName(char *str)
{
  static char buffer[CF_BUFSIZE];
  char *sp;

  memset(buffer, 0, CF_BUFSIZE);
  strcpy(buffer, str);

  for (sp = buffer; *sp != '\0'; sp++)
  {
    if (!isalnum((int)*sp) || *sp == '.')
    {
      *sp = '_';
    }
  }

  return buffer;
}

/*********************************************************************/

struct Average FindHurstFunction(int samples_per_grain, int grains, int dbtype)
/* Find the average of (max-min) over all intervals of width delta_t */
{
  static struct Average lmin, lmax, av;
  int control = 0;
  int begin_time = 0;
  int total_time = 0;
  switch(dbtype)
  {
    case DAILY:
      begin_time = MONDAY_MORNING;
      total_time = ONE_DAY;
      break;
    case YEARLY:
      begin_time = JANUARY_FIRST;
      total_time = ONE_YEAR;
      break;
    default: /* weekly */
      begin_time = MONDAY_MORNING;
      total_time = ONE_WEEK;
      break;
  }


  if ((errno = db_create(&DBP, NULL, 0)) != 0)
  {
    printf("Couldn't create average database %s\n", FILENAME);
    exit(1);
  }

#ifdef CF_OLD_DB
  if ((errno =
       (DBP->open)(DBP, FILENAME, NULL, DB_BTREE, DB_RDONLY, 0644)) != 0)
#else
  if ((errno =
       (DBP->open)(DBP, NULL, FILENAME, NULL, DB_BTREE, DB_RDONLY, 0644)) != 0)
#endif
  {
    printf("Couldn't open average database %s\n", FILENAME);
    DBP->err(DBP, errno, NULL);
    exit(1);
  }

  memset(&key, 0, sizeof(key));
  memset(&value, 0, sizeof(value));
  memset(&av, 0, sizeof(av));

  lmax.expect = 0.01;
  lmin.expect = 9999.0;

  count = 0;

  for (NOW = begin_time; NOW < begin_time + total_time;
       NOW += INTERVAL)
  {
    memset(&key, 0, sizeof(key));
    memset(&value, 0, sizeof(value));
    memset(&ENTRY, 0, sizeof(ENTRY));

    strcpy(TIMEKEY, GenTimeKey2(NOW, dbtype));

    key.data = TIMEKEY;
    key.size = strlen(TIMEKEY) + 1;

    if ((errno = DBP->get(DBP, NULL, &key, &value, 0)) != 0)
    {
      if (errno != DB_NOTFOUND)
      {
        DBP->err(DBP, errno, NULL);
        exit(1);
      }
    }

    count++;

    if (value.data != NULL)
    {
      memcpy(&ENTRY, value.data, sizeof(ENTRY));

      if (false)    /* This conformal scaling has no effect on the Hurst
                     * parameter expect div by zero errors! */
      {
        ENTRY.expect = ENTRY.expect / sqrt(ENTRY.var);
      }

      if (fabs(ENTRY.expect) > lmax.expect)
      {
        lmax.expect = fabs(ENTRY.expect);
      }


      if (fabs(ENTRY.expect) < lmin.expect)
      {
        lmin.expect = fabs(ENTRY.expect);
      }
    }

    /* For each grain, find the difference of the max and min values for
     * final average */

    if (count == samples_per_grain)
    {
      count = 0;
      control += samples_per_grain;

      /* av += lmax - lmin; */

      av.expect += (lmax.expect - lmin.expect) / (double)grains;

      lmax.expect = 0.01;
      lmin.expect = 9999.0;
    }
  }

  printf("Scanned %d grains of size %d for Hurst function\n", control,
         samples_per_grain);

  DBP->close(DBP, 0);
  return (av);
}

/*****************************************************************************/

void Syntax()
{
  int i;

  printf("Syntax:\n\n");

  printf("%s name", PROG_NAME);
  for (i = 0; GRAPHOPTIONS[i].name != NULL; i++)
  {
    printf(" (--%s|-%c)", GRAPHOPTIONS[i].name, (char)GRAPHOPTIONS[i].val);
  }
  printf("\n");
}


/*********************************************************************/

char *GenTimeKey2(time_t now, int dbtype)
{
  char str[64];

  sprintf(str, "%s", ctime(&now));

  return ConvTimeKey2(str, dbtype);
}

/*********************************************************************/

char *ConvTimeKey2(char *str, int dbtype)
{
  int i, hr, min, timeinmins;
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
    sprintf(minbuf, "%04d", timeinmins / STEP );
  else if (STEP < 15)
    sprintf(minbuf, "%03d", timeinmins / STEP );
  else if (STEP < 145)
    sprintf(minbuf, "%02d", timeinmins / STEP );
  else
    sprintf(minbuf, "%d", timeinmins / STEP );

  strcat(timekey, minbuf);

  return timekey;
}

/*********************************************************************/

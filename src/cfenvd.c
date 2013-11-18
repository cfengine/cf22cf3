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
/* File: cfenvd.c                                                            */
/*                                                                           */
/* Description: Long term state registry                                     */
/*                                                                           */
/* Based in part on results of the ECG project, by Mark, Sigmund Straumsnes  */
/* and Hårek Haugerud, Oslo University College 1998                          */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

struct option CFDENVOPTIONS[] =
   {
   {"help",no_argument,0,'h'},
   {"debug",optional_argument,0,'d'}, 
   {"verbose",no_argument,0,'v'},
   {"no-fork",no_argument,0,'F'},
   {"histograms",no_argument,0,'H'},
   {"tcpdump",no_argument,0,'T'},
   {NULL,0,0,0}
   };

extern int NO_FORK;
extern int HISTO;
extern short TCPDUMP;

/*******************************************************************/
/* Prototypes                                                      */
/*******************************************************************/

void CheckOpts(int argc,char **argv);
void Syntax (void);

/*******************************************************************/
/* Level 0 : Main                                                  */
/*******************************************************************/

int main (int argc,char **argv)

{
CheckOpts(argc,argv);
MonInitialize();
GetNameInfo();
GetInterfaceInfo();
GetV6InterfaceInfo();  
StartServer(argc,argv);
return 0;
}

/********************************************************************/
/* Level 1                                                          */
/********************************************************************/

void CheckOpts(int argc,char **argv)

{ extern char *optarg;
 int optindex = 0;
 int c;

umask(077);
sprintf(VPREFIX,"cfenvd"); 
openlog(VPREFIX,LOG_PID|LOG_NOWAIT|LOG_ODELAY,LOG_DAEMON);

strcpy(CFLOCK,"cfenvd");

SetContext("cfenvd");


IGNORELOCK = false; 
OUTPUT[0] = '\0';

while ((c=getopt_long(argc,argv,"d:vhHFVT",CFDENVOPTIONS,&optindex)) != EOF)
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
  
                NO_FORK = true;
                printf("cfenvd: Debug mode: running in foreground\n");
                break;

      case 'v': VERBOSE = true;
         break;

      case 'V': printf("GNU %s-%s daemon\n%s\n",PACKAGE,VERSION,COPYRIGHT);
         printf("This program is covered by the GNU Public License and may be\n");
         printf("copied free of charge. No warrenty is implied.\n\n");
         exit(0);
         break;

      case 'F': NO_FORK = true;
         break;

      case 'H': HISTO = true;
         break;

      case 'T': TCPDUMP = true;
                break;
  
      default:  Syntax();
                exit(1);

      }
   }

LOGGING = true;                    /* Do output to syslog */

SetReferenceTime(false);
SetStartTime(false);
SetSignals();
signal (SIGTERM,HandleSignal);                   /* Signal Handler */
signal (SIGHUP,HandleSignal);
signal (SIGINT,HandleSignal);
signal (SIGPIPE,HandleSignal);
signal (SIGSEGV,HandleSignal);
signal (SIGUSR1,HandleSignal);
signal (SIGUSR2,HandleSignal);
}

/*******************************************************************************/

void Syntax()

{ int i;

printf("GNU cfengine environment daemon\n%s-%s\n%s\n",PACKAGE,VERSION,COPYRIGHT);
printf("\n");
printf("Options:\n\n");

for (i=0; CFDENVOPTIONS[i].name != NULL; i++)
   {
   printf("--%-20s    (-%c)\n",CFDENVOPTIONS[i].name,(char)CFDENVOPTIONS[i].val);
   }

printf("\nBug reports to bug-cfengine@cfengine.org\n");
printf("General help to help-cfengine@cfengine.org\n");
printf("Info & fixes at http://www.cfengine.org\n");
}


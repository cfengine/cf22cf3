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
/* File: init.c                                                              */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"
#include "../pub/global.h"

/*********************************************************************/

void CheckWorkDirectories()

{ struct stat statbuf;
  int result;
  char *sp;

Debug("CheckWorkDirectories()\n");

 if (uname(&VSYSNAME) == -1)
   {
   perror("uname ");
   FatalError("Uname couldn't get kernel name info!!\n");
   }
 
snprintf(LOGFILE,CF_BUFSIZE,"%s/cfagent.%s.log",VLOGDIR,VSYSNAME.nodename);
VSETUIDLOG = strdup(LOGFILE); 
 
if (!IsPrivileged())
   {
   Verbose("\n(Non privileged user...)\n\n");
   
   if ((sp = getenv("HOME")) == NULL)
      {
      FatalError("You do not have a HOME variable pointing to your home directory");
      }

   snprintf(VLOGDIR,CF_BUFSIZE,"%s/.cfagent",sp);
   snprintf(VLOCKDIR,CF_BUFSIZE,"%s/.cfagent",sp);
   snprintf(VBUFF,CF_BUFSIZE,"%s/.cfagent/test",sp);
   MakeDirectoriesFor(VBUFF,'y');
   snprintf(VBUFF,CF_BUFSIZE,"%s/.cfagent/state/test",sp);
   MakeDirectoriesFor(VBUFF,'n');
   snprintf(CFPRIVKEYFILE,CF_BUFSIZE,"%s/.cfagent/ppkeys/localhost.priv",sp);
   snprintf(CFPUBKEYFILE,CF_BUFSIZE,"%s/.cfagent/ppkeys/localhost.pub",sp);
   }
else
   {
   snprintf(VBUFF,CF_BUFSIZE,"%s/test",VLOCKDIR);
   snprintf(VBUFF,CF_BUFSIZE,"%s/test",VLOGDIR);
   MakeDirectoriesFor(VBUFF,'n');
   snprintf(VBUFF,CF_BUFSIZE,"%s/state/test",VLOGDIR);
   MakeDirectoriesFor(VBUFF,'n');
   snprintf(CFPRIVKEYFILE,CF_BUFSIZE,"%s/ppkeys/localhost.priv",CFWORKDIR);
   snprintf(CFPUBKEYFILE,CF_BUFSIZE,"%s/ppkeys/localhost.pub",CFWORKDIR);
   }

Verbose("Checking integrity of the state database\n");
snprintf(VBUFF,CF_BUFSIZE,"%s/state",VLOCKDIR);
if (stat(VBUFF,&statbuf) == -1)
   {
   snprintf(VBUFF,CF_BUFSIZE,"%s/state",VLOCKDIR);
   MakeDirectoriesFor(VBUFF,'n');
   if (chown(VBUFF,getuid(),getgid()) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Unable to set owner on %s to %d.%d",VBUFF,getuid(),getgid());
      CfLog(cferror,OUTPUT,"chown");
      }
   chmod(VBUFF,(mode_t)0755);
   }
else 
   {
   if (statbuf.st_mode & 022)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"UNTRUSTED: State directory %s (mode %o) was not private!\n",VLOCKDIR,statbuf.st_mode & 0777);
      CfLog(cferror,OUTPUT,"");
      }
   }

Verbose("Checking integrity of the module directory\n"); 

snprintf(VBUFF,CF_BUFSIZE,"%s/modules",VLOCKDIR);
if (stat(VBUFF,&statbuf) == -1)
   {
   snprintf(VBUFF,CF_BUFSIZE,"%s/modules/test",VLOCKDIR);
   MakeDirectoriesFor(VBUFF,'n');
   snprintf(VBUFF,CF_BUFSIZE,"%s/modules",VLOCKDIR);
   if (chown(VBUFF,getuid(),getgid()) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Unable to set owner on %s to %d.%d",VBUFF,getuid(),getgid());
      CfLog(cferror,OUTPUT,"chown");
      }

   chmod(VBUFF,(mode_t)0700);
   }
else 
   {
   if (statbuf.st_mode & 022)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"UNTRUSTED: Module directory %s (mode %o) was not private!\n",VLOCKDIR,statbuf.st_mode & 0777);
      CfLog(cferror,OUTPUT,"");
      }
   }

Verbose("Checking integrity of the input data for RPC\n"); 

snprintf(VBUFF,CF_BUFSIZE,"%s/rpc_in",VLOCKDIR);

if (stat(VBUFF,&statbuf) == -1)
   {
   snprintf(VBUFF,CF_BUFSIZE,"%s/rpc_in/test",VLOCKDIR);
   MakeDirectoriesFor(VBUFF,'n');
   snprintf(VBUFF,CF_BUFSIZE,"%s/rpc_in",VLOCKDIR);
   if (chown(VBUFF,getuid(),getgid()) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Unable to set owner on %s to %d.%d",VBUFF,getuid(),getgid());
      CfLog(cferror,OUTPUT,"chown");
      }

   chmod(VBUFF,(mode_t)0700);
   }
else 
   {
   if (statbuf.st_mode & 077)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"UNTRUSTED: RPC input directory %s was not private! (%o)\n",VBUFF,statbuf.st_mode & 0777);
      FatalError(OUTPUT);
      }
   }

Verbose("Checking integrity of the output data for RPC\n"); 

snprintf(VBUFF,CF_BUFSIZE,"%s/rpc_out",VLOCKDIR);
if (stat(VBUFF,&statbuf) == -1)
   {
   snprintf(VBUFF,CF_BUFSIZE,"%s/rpc_out/test",VLOCKDIR);
   MakeDirectoriesFor(VBUFF,'n');
   snprintf(VBUFF,CF_BUFSIZE,"%s/rpc_out",VLOCKDIR);
   if (chown(VBUFF,getuid(),getgid()) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Unable to set owner on %s to %d.%d",VBUFF,getuid(),getgid());
      CfLog(cferror,OUTPUT,"chown");
      }

   chmod(VBUFF,(mode_t)0700);   
   }
else
   {
   if (statbuf.st_mode & 077)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"UNTRUSTED: RPC output directory %s was not private! (%o)\n",VBUFF,statbuf.st_mode & 0777);
      FatalError(OUTPUT);
      }
   }
 
Verbose("Checking integrity of the PKI directory\n");
snprintf(VBUFF,CF_BUFSIZE,"%s/ppkeys",VLOCKDIR);
    
if (stat(VBUFF,&statbuf) == -1)
   {
   snprintf(VBUFF,CF_BUFSIZE,"%s/ppkeys/test",VLOCKDIR);
   MakeDirectoriesFor(VBUFF,'n');
   snprintf(VBUFF,CF_BUFSIZE,"%s/ppkeys",VLOCKDIR); 
   chmod(VBUFF,(mode_t)0700); /* Keys must be immutable to others */
   }
else
   {
   if (statbuf.st_mode & 077)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"UNTRUSTED: Private key directory %s/ppkeys (mode %o) was not private!\n",VLOCKDIR,statbuf.st_mode & 0777);
      FatalError(OUTPUT);
      }
   }

Verbose("Making sure that locks are private...\n"); 
if (chown(VLOCKDIR,getuid(),getgid()) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Unable to set owner on %s to %d.%d",VLOCKDIR,getuid(),getgid());
   CfLog(cferror,OUTPUT,"chown");
   }
 
/* Locks must be immutable to others */
 if (stat(VLOCKDIR,&statbuf) != -1)
   {
     /* change permissions go-w */
     chmod(VLOCKDIR,(mode_t)(statbuf.st_mode & ~022));
   }
}

/**********************************************************************/

void ActAsDaemon(int preserve)

{ int fd, maxfd;

#ifdef HAVE_SETSID
setsid();
#endif

closelog();

fflush(NULL);
fd = open("/dev/null", O_RDWR, 0);
if (fd != -1)
   {
   dup2(fd,STDIN_FILENO);
   dup2(fd,STDOUT_FILENO);
   dup2(fd,STDERR_FILENO);
   if (fd > STDERR_FILENO) close(fd);
   }

chdir("/");
   
#ifdef HAVE_SYSCONF
maxfd = sysconf(_SC_OPEN_MAX);
#else
# ifdef _POXIX_OPEN_MAX
maxfd = _POSIX_OPEN_MAX;
# else
maxfd = 1024;
# endif
#endif

for (fd=STDERR_FILENO+1; fd < maxfd; ++fd)
   {
   if (fd != preserve) close(fd);
   }
}

/*******************************************************************/

int IsIPV6Address(char *name)

{ char *sp;
 int count,max = 0; 

Debug("IsIPV6Address(%s)\n",name);
 
if (name == NULL)
   {
   return false;   
   }

count = 0;
 
for (sp = name; *sp != '\0'; sp++)
   {
   if (isalnum((int)*sp))
      {
      count++;
      }
   else if ((*sp != ':') && (*sp != '.'))
      {
      return false;
      }

   if (*sp == 'r')
      {
      return false;
      }
   
   if (count > max)
      {
      max = count;
      }
   else
      {
      count = 0;
      }
   }

if (max <= 2)
   {
   Debug("Looks more like a MAC address");
   return false;
   }
 
if (strstr(name,":") == NULL)
   {
   return false;
   }

if (StrStr(name,"scope"))
   {
   return false;    
   }
 
return true;
}


/*******************************************************************/

int IsIPV4Address(char *name)

{ char *sp;
  int count = 0; 

Debug("IsIPV4Address(%s)\n",name);
 
if (name == NULL)
   {
   return false;   
   }
 
for (sp = name; *sp != '\0'; sp++)
   {
   if (!isdigit((int)*sp) && (*sp != '.'))
      {
      return false;
      }

   if (*sp == '.')
      {
      count++;
      }
   }
 
if (count != 3)
   {
   return false;
   }

return true;
}

/*******************************************************************/

int IsInterfaceAddress(char *adr)

 /* Does this address belong to a local interface */

{ struct Item *ip;

for (ip = IPADDRESSES; ip != NULL; ip=ip->next)
   {
   if (StrnCmp(adr,ip->name,strlen(adr)) == 0)
      {
      Debug("Identifying (%s) as one of my interfaces\n",adr);
      return true;
      }
   }

Debug("(%s) is not one of my interfaces\n",adr); 
return false; 
}

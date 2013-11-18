
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
/* File: popen.c                                                             */
/*                                                                           */
/* Created: Tue Dec  2 02:14:16 1997                                         */
/*                                                                           */
/* popen replacement for POSIX 2 avoiding shell piggyback                    */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h" 

# if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
extern pthread_attr_t PTHREADDEFAULTS;
extern pthread_mutex_t MUTEX_COUNT;
extern pthread_mutex_t MUTEX_HOSTNAME;
# endif

pid_t *CHILD;
int    MAXFD = 20; /* Max number of simultaneous pipes */

/*****************************************************************************/

void ConvertFile(char *command,struct Item **list)

{ FILE *pp;
  char line[CF_BUFSIZE];

  
if ((pp = cfpopen(command,"r")) == NULL)
   {
   printf(" \"%s\" could not be executed",command);
   return;
   }

while (!feof(pp))
   {
   line[0] = '\0';
   fgets(line,CF_BUFSIZE-1,pp);
   AppendItem(list,line,NULL);
   }

cfpclose(pp);
}

/*****************************************************************************/

FILE *cfpopen(char *command,char *type)

 { static char arg[CF_MAXSHELLARGS][CF_BUFSIZE];
   int i, argc, pd[2];
   char **argv;
   pid_t pid;
   FILE *pp = NULL;

Debug("cfpopen(%s)\n",command);

if ((*type != 'r' && *type != 'w') || (type[1] != '\0'))
   {
   errno = EINVAL;
   return NULL;
   }

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
if (pthread_mutex_lock(&MUTEX_COUNT) != 0)
   {
   CfLog(cferror,"pthread_mutex_lock failed","pthread_mutex_unlock");
   return NULL;
   }
#endif

if (CHILD == NULL)   /* first time */
   {
   if ((CHILD = calloc(MAXFD,sizeof(pid_t))) == NULL)
      {
#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
      pthread_mutex_unlock(&MUTEX_COUNT);
#endif
      return NULL;
      }
   }

#if defined HAVE_PTHREAD_H && (defined HAVE_LIBPTHREAD || defined BUILDTIN_GCC_THREAD)
if (pthread_mutex_unlock(&MUTEX_COUNT) != 0)
   {
   CfLog(cferror,"pthread_mutex_unlock failed","pthread_mutex_unlock");
   return NULL;
   }
#endif


if (pipe(pd) < 0)        /* Create a pair of descriptors to this process */
   {
   return NULL;
   }

if ((pid = fork()) == -1)
   {
   return NULL;
   }

signal(SIGCHLD,SIG_DFL);

ALARM_PID = (pid != 0 ? pid : -1);

if (pid == 0)
   {
   switch (*type)
      {
      case 'r':
          
          close(pd[0]);        /* Don't need output from parent */
          
          if (pd[1] != 1)
             {
             dup2(pd[1],1);    /* Attach pp=pd[1] to our stdout */
             dup2(pd[1],2);    /* Merge stdout/stderr */
             close(pd[1]);
             }
          
          break;
          
      case 'w':
          
          close(pd[1]);
          
          if (pd[0] != 0)
             {
             dup2(pd[0],0);
             close(pd[0]);
             }
       }
   
   for (i = 0; i < MAXFD; i++)
      {
      if (CHILD[i] > 0)
         {
         close(i);
         }
      }
   
   argc = SplitCommand(command,arg);
   argv = (char **) malloc((argc+1)*sizeof(char *));
   
   if (argv == NULL)
      {
      FatalError("Out of memory");
      }
   
   for (i = 0; i < argc; i++)
      {
      argv[i] = arg[i];
      }
   
   argv[i] = (char *) NULL;
   
   if (execv(arg[0],argv) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Couldn't run %s",arg[0]);
      CfLog(cferror,OUTPUT,"execv");
      }
   
   free((char *)argv);
   _exit(1);
   }
else
   {
   switch (*type)
      {
      case 'r':
          
          close(pd[1]);
          
          if ((pp = fdopen(pd[0],type)) == NULL)
             {
             /* Don't leave zombies. */
             cfpwait(pid);
             return NULL;
             }
          break;
          
      case 'w':
          
          close(pd[0]);
          
          if ((pp = fdopen(pd[1],type)) == NULL)
             {
             /* Don't leave zombies. */
             cfpwait(pid);
             return NULL;
             }
      }
   
   if (fileno(pp) >= MAXFD)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"File descriptor %d of child %d higher than MAXFD, check for defunct children", fileno(pp), pid);
      CfLog(cferror,OUTPUT,"");
      }
   else
      {
      CHILD[fileno(pp)] = pid;
      }
   
   return pp;
   }

return NULL; /* Cannot reach here */
}

/*****************************************************************************/

FILE *cfpopensetuid(char *command,char *type,uid_t uid,gid_t gid,char *chdirv,char *chrootv)
    
 { static char arg[CF_MAXSHELLARGS][CF_BUFSIZE];
   int i, argc, pd[2];
   char **argv;
   pid_t pid;
   FILE *pp = NULL;

Debug("cfpopensetuid(%s,%s,%d,%d)\n",command,type,uid,gid);

if ((*type != 'r' && *type != 'w') || (type[1] != '\0'))
   {
   errno = EINVAL;
   return NULL;
   }

if (CHILD == NULL)   /* first time */
   {
   if ((CHILD = calloc(MAXFD,sizeof(pid_t))) == NULL)
      {
      return NULL;
      }
   }

if (pipe(pd) < 0)        /* Create a pair of descriptors to this process */
   {
   return NULL;
   }

if ((pid = fork()) == -1)
   {
   return NULL;
   }

ALARM_PID = (pid != 0 ? pid : -1);

if (pid == 0)
   {
   switch (*type)
      {
      case 'r':
          
          close(pd[0]);        /* Don't need output from parent */
          
          if (pd[1] != 1)
             {
             dup2(pd[1],1);    /* Attach pp=pd[1] to our stdout */
             dup2(pd[1],2);    /* Merge stdout/stderr */
             close(pd[1]);
             }
          
          break;
          
      case 'w':
          
          close(pd[1]);
          
          if (pd[0] != 0)
             {
             dup2(pd[0],0);
             close(pd[0]);
             }
      }
   
   for (i = 0; i < MAXFD; i++)
      {
      if (CHILD[i] > 0)
         {
         close(i);
         }
      }
   
   argc = SplitCommand(command,arg);
   argv = (char **) malloc((argc+1)*sizeof(char *));
   
   if (argv == NULL)
      {
      FatalError("Out of memory");
      }
   
   for (i = 0; i < argc; i++)
      {
      argv[i] = arg[i];
      }
   
   argv[i] = (char *) NULL;
   
   if (chrootv && strlen(chrootv) != 0)
      {
      if (chroot(chrootv) == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Couldn't chroot to %s\n",chrootv);
         CfLog(cferror,OUTPUT,"chroot");
         free((char *)argv);
         return NULL;
         }
      }
   
   if (chdirv && strlen(chdirv) != 0)
      {
      if (chdir(chdirv) == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Couldn't chdir to %s\n",chdirv);
         CfLog(cferror,OUTPUT,"chdir");
         free((char *)argv);
         return NULL;
         }
      }
   
   if (!CfSetUid(uid,gid))
      {
      free((char *)argv);
      _exit(1);
      }
   
   if (execv(arg[0],argv) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Couldn't run %s",arg[0]);
      CfLog(cferror,OUTPUT,"execv");
      }
   
   free((char *)argv);
   _exit(1);
   }
else
   {
   switch (*type)
      {
      case 'r':
          
          close(pd[1]);
          
          if ((pp = fdopen(pd[0],type)) == NULL)
             {
             /* Don't leave zombies. */
             cfpwait(pid);
             return NULL;
             }
          break;
          
      case 'w':
          
          close(pd[0]);
          
          if ((pp = fdopen(pd[1],type)) == NULL)
             {
             /* Don't leave zombies. */
             cfpwait(pid);
             return NULL;
             }
      }
   
   if (fileno(pp) >= MAXFD)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"File descriptor %d of child %d higher than MAXFD, check for defunct children", fileno(pp), pid);
      CfLog(cferror,OUTPUT,"");
      }
   else
      {
      CHILD[fileno(pp)] = pid;
      }
   return pp;
   }
return NULL; /* cannot reach here */
}

/*****************************************************************************/
/* Shell versions of commands - not recommended for security reasons         */
/*****************************************************************************/

FILE *cfpopen_sh(char *command,char *type)
    
 { int i,pd[2];
   pid_t pid;
   FILE *pp = NULL;

Debug("cfpopen(%s)\n",command);

if ((*type != 'r' && *type != 'w') || (type[1] != '\0'))
   {
   errno = EINVAL;
   return NULL;
   }

if (CHILD == NULL)   /* first time */
   {
   if ((CHILD = calloc(MAXFD,sizeof(pid_t))) == NULL)
      {
      return NULL;
      }
   }

if (pipe(pd) < 0)        /* Create a pair of descriptors to this process */
   {
   return NULL;
   }

if ((pid = fork()) == -1)
   {
   return NULL;
   }

ALARM_PID = (pid != 0 ? pid : -1);

if (pid == 0)
   {
   switch (*type)
      {
      case 'r':
          
          close(pd[0]);        /* Don't need output from parent */
          
          if (pd[1] != 1)
             {
             dup2(pd[1],1);    /* Attach pp=pd[1] to our stdout */
             dup2(pd[1],2);    /* Merge stdout/stderr */
             close(pd[1]);
             }
          
          break;
          
      case 'w':
          
          close(pd[1]);
          
          if (pd[0] != 0)
             {
             dup2(pd[0],0);
             close(pd[0]);
             }
      }
   
   for (i = 0; i < MAXFD; i++)
      {
      if (CHILD[i] > 0)
         {
         close(i);
         }
      }
   
   execl("/bin/sh","sh","-c",command,NULL);
   _exit(1);
   }
else
   {
   switch (*type)
      {
      case 'r':
          
          close(pd[1]);
          
          if ((pp = fdopen(pd[0],type)) == NULL)
             {
             /* Don't leave zombies. */
             cfpwait(pid);
             return NULL;
             }
          break;
          
      case 'w':
          
          close(pd[0]);
          
          if ((pp = fdopen(pd[1],type)) == NULL)
             {
             /* Don't leave zombies. */
             cfpwait(pid);
             return NULL;
             }
      }
   
   if (fileno(pp) >= MAXFD)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"File descriptor %d of child %d higher than MAXFD, check for defunct children", fileno(pp), pid);
      CfLog(cferror,OUTPUT,"");
      }
   else
      {
      CHILD[fileno(pp)] = pid;
      }

   return pp;
   }

return NULL;
}

/******************************************************************************/

FILE *cfpopen_shsetuid(char *command,char *type,uid_t uid,gid_t gid,char *chdirv,char *chrootv)
    
 { int i,pd[2];
   pid_t pid;
   FILE *pp = NULL;

Debug("cfpopen_shsetuid(%s,%s,%d,%d)\n",command,type,uid,gid);

if ((*type != 'r' && *type != 'w') || (type[1] != '\0'))
   {
   errno = EINVAL;
   return NULL;
   }

if (CHILD == NULL)   /* first time */
   {
   if ((CHILD = calloc(MAXFD,sizeof(pid_t))) == NULL)
      {
      return NULL;
      }
   }

if (pipe(pd) < 0)        /* Create a pair of descriptors to this process */
   {
   return NULL;
   }

if ((pid = fork()) == -1)
   {
   return NULL;
   }

ALARM_PID = (pid != 0 ? pid : -1);

if (pid == 0)
   {
   switch (*type)
      {
      case 'r':
          
          close(pd[0]);        /* Don't need output from parent */
          
          if (pd[1] != 1)
             {
             dup2(pd[1],1);    /* Attach pp=pd[1] to our stdout */
             dup2(pd[1],2);    /* Merge stdout/stderr */
             close(pd[1]);
             }
          
          break;
          
      case 'w':
          
          close(pd[1]);
          
          if (pd[0] != 0)
             {
             dup2(pd[0],0);
             close(pd[0]);
             }
      }
   
   for (i = 0; i < MAXFD; i++)
      {
      if (CHILD[i] > 0)
         {
         close(i);
         }
      }
   
   if (chrootv && strlen(chrootv) != 0)
      {
      if (chroot(chrootv) == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Couldn't chroot to %s\n",chrootv);
         CfLog(cferror,OUTPUT,"chroot");
         return NULL;
         }
      }
   
   if (chdirv && strlen(chdirv) != 0)
      {
      if (chdir(chdirv) == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Couldn't chdir to %s\n",chdirv);
         CfLog(cferror,OUTPUT,"chroot");
         return NULL;
         }
      }
   
   if (!CfSetUid(uid,gid))
      {
      _exit(1);
      }
   
   execl("/bin/sh","sh","-c",command,NULL);
   _exit(1);
   }
else
   {
   switch (*type)
      {
      case 'r':
          
          close(pd[1]);
          
          if ((pp = fdopen(pd[0],type)) == NULL)
             {
             /* Don't leave zombies. */
             cfpwait(pid);
             return NULL;
             }
          break;
          
      case 'w':
          
          close(pd[0]);
          
          if ((pp = fdopen(pd[1],type)) == NULL)
             {
             /* Don't leave zombies. */
             cfpwait(pid);
             return NULL;
             }
      }
   
   if (fileno(pp) >= MAXFD)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"File descriptor %d of child %d higher than MAXFD, check for defunct children", fileno(pp), pid);
      CfLog(cferror,OUTPUT,"");
      /* Don't leave zombies. */
      cfpwait(pid);
      return NULL;
      }
   else
      {
      CHILD[fileno(pp)] = pid;
      }
   return pp;
   }

return NULL;
}


/******************************************************************************/
/* Close commands                                                             */
/******************************************************************************/

int cfpwait(pid_t pid)

{ int status, wait_result;

Debug("cfpwait - Waiting for process %d\n",pid); 

#ifdef HAVE_WAITPID

while(waitpid(pid,&status,0) < 0)
   {
   if (errno != EINTR)
      {
      return -1;
      }
   }

return status; 
 
#else

while ((wait_result = wait(&status)) != pid)
   {
   if (wait_result <= 0)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Wait for child failed\n");
      CfLog(cfinform,OUTPUT,"wait");
      return -1;
      }
   }
 
if (WIFSIGNALED(status))
   {
   return -1;
   }
 
if (! WIFEXITED(status))
   {
   return -1;
   }
 
return (WEXITSTATUS(status));
#endif
}

/*******************************************************************/

int cfpclose(FILE *pp)

{ int fd;
  pid_t pid;

Debug("cfpclose(pp)\n");

if (CHILD == NULL)  /* popen hasn't been called */
   {
   return -1;
   }

ALARM_PID = -1;
fd = fileno(pp);

if (fd >= MAXFD)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"File descriptor %d of child higher than "
      "MAXFD, check for defunct children", fd);
   CfLog(cferror,OUTPUT,"");
   /* We can't wait for the specific pid as it isn't stored in CHILD. */
   pid = -1;
   }
else
   {
   if ((pid = CHILD[fd]) == 0)
      {
      return -1;
      }

   CHILD[fd] = 0;
   }

if (fclose(pp) == EOF)
   {
   return -1;
   }

return cfpwait(pid);
}

/*******************************************************************/

int CfSetUid(uid_t uid,gid_t gid)

{ struct passwd *pw;
 
if (gid != (gid_t) -1)
   {
   Verbose("Changing gid to %d\n",gid);      
   
   if (setgid(gid) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Couldn't set gid to %d\n",gid);
      CfLog(cferror,OUTPUT,"setgid");
      return false;
      }

   /* Now eliminate any residual privileged groups */
   
   if ((pw = getpwuid(uid)) == NULL)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Unable to get login groups when dropping privilege to %d",uid);
      CfLog(cferror,OUTPUT,"initgroups");
      return false;
      }
   
   if (initgroups(pw->pw_name, pw->pw_gid) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Unable to set login groups when dropping privilege to %s=%d",pw->pw_name,uid);
      CfLog(cferror,OUTPUT,"initgroups");
      return false;
      }
   }

if (uid != (uid_t) -1)
   {
   Verbose("Changing uid to %d\n",uid);
   
   if (setuid(uid) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Couldn't set uid to %d\n",uid);
      CfLog(cferror,OUTPUT,"setuid");
      return false;
      }
   }

return true;
}

/*******************************************************************/
/* Command exec aids                                               */
/*******************************************************************/

int SplitCommand(char *comm,char arg[CF_MAXSHELLARGS][CF_BUFSIZE])

{ char *sp;
  int i = 0;

for (sp = comm; sp < comm+strlen(comm); sp++)
   {
   if (i >= CF_MAXSHELLARGS-1)
      {
      CfLog(cferror,"Too many arguments in embedded script","");
      FatalError("Use a wrapper");
      }
   
   while (*sp == ' ' || *sp == '\t')
      {
      sp++;
      }
   
   switch (*sp)
      {
      case '\0': return(i-1);
   
      case '\"': sscanf (++sp,"%[^\"]",arg[i]);
          break;
      case '\'': sscanf (++sp,"%[^\']",arg[i]);
          break;
      case '`':  sscanf (++sp,"%[^`]",arg[i]);
          break;
      default:   sscanf (sp,"%s",arg[i]);
          break;
      }
   
   sp += strlen(arg[i]);
   i++;
   }
 
 return (i);
}




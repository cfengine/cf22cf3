/* 

        Copyright (C) 1995,96,97
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

/*******************************************************************/
/*                                                                 */
/*  Cfengine : remote client example                               */
/*                                                                 */
/*  Mark Burgess 1997                                              */
/*  modified : Bas van der Vlies <basv@sara.nl> 1998               */
/*  modified : Andrew Mayhew <amayhew@icewire.com> 2000            */
/*  modified : Mark 2002, Bas 2002                                 */
/*  modified : Jochen Reinwand <Jochen.Reinwand@lau-net.de> 2002   */
/*                                                                 */
/*******************************************************************/

#define FileVerbose if (VERBOSE || DEBUG || D2) fprintf

#include "cf.defs.h"
#include "cf.extern.h"

enum fileoutputlevels
   {
   fopl_normal,
   fopl_error,
   fopl_inform
   };

int  MAXCHILD = 1;
int  FileFlag = 0;
int  TRUSTALL = false;
enum fileoutputlevels  OUTPUTLEVEL = fopl_normal;
char OUTPUTDIR[CF_BUFSIZE];
char INCLUDEFILE[CF_BUFSIZE];

struct Item *VCFRUNCLASSES = NULL;
struct Item *VCFRUNOPTIONHOSTS = NULL;
struct Item *VCFRUNHOSTLIST = NULL;

char VCFRUNHOSTS[CF_BUFSIZE] = "cfrun.hosts";
char CFRUNOPTIONS[CF_BUFSIZE];
char CFLOCK[CF_BUFSIZE] = "dummy";

/*******************************************************************/
/* Functions internal to cfrun.c                                   */
/*******************************************************************/

void CheckOptsAndInit  (int argc,char **argv);
int PollServer (char *host, char *options, int storeinfile);
void SendClassData (int sd, char *sendbuffer);
void CheckAccess (char *users);
void cfrunSyntax (void);
void ReadCfrunConf (char* cfg_fic);
int ParseHostname (char *hostname, char *new_hostname);
void FileOutput (FILE *fp, enum fileoutputlevels level, char *message);

/*******************************************************************/
/* Level 0 : Main                                                  */
/*******************************************************************/

int main (int argc,char **argv)

{ struct Item *ip;
  int   i=0;
  int  status;
  int  pid;

SetContext("cfrun");

CheckOptsAndInit(argc,argv);

ip = VCFRUNHOSTLIST;

while (ip != NULL)
   {
   if (i < MAXCHILD)
      {
      if (fork() == 0) /* child */
         {
         ALARM_PID = -1;
         printf("cfrun(%d):         .......... [ Hailing %s ] ..........\n",i,ip->name);
         Debug("pid = %d i = %d\n", getpid(), i);
         
         if (PollServer(ip->name,ip->classes, FileFlag))
            {
            Verbose("Connection with %s completed\n\n",ip->name);
            }
         else
            {
            Verbose("Connection refused...\n\n");
            }
         exit(0);
         }
      else
         {
         /* parent */
         i++;
         }
      }
   else 
      {
      pid = wait(&status);
      Debug("wait result pid = %d number %d\n", pid, i);
      i--;
      }

   if ( i < MAXCHILD )
      {
      ip = ip->next;
      }
   } 
 
 while (i > 0)
    {
    pid = wait(&status);
    Debug("Child pid = %d ended\n", pid);
    i--;
    }
 
 exit(0);
 return 0;
}

/********************************************************************/
/* Level 1                                                          */
/********************************************************************/

void CheckOptsAndInit(int argc,char **argv)

{ int optgroup = 0, i;
  struct Item *ip;

/* Separate command args into options and classes */
memset(CFRUNOPTIONS,0,CF_BUFSIZE);

for (i = 1; i < argc; i++) 
   {
   if (optgroup == 0)
      {  
      if (strncmp(argv[i],"-h",2) == 0)   
         {
         cfrunSyntax();
         }
      else if (strncmp(argv[i],"-f",2) == 0)
         {
         i++;
         
         if ((i >= argc) || (strncmp(argv[i],"-",1) == 0))
            {
            printf("Error: No filename listed after -f option.\n");
            cfrunSyntax();
            exit(0);
            }
         memset(VCFRUNHOSTS,0,CF_BUFSIZE);
         strncat(VCFRUNHOSTS,argv[i],CF_BUFSIZE-1-strlen(VCFRUNHOSTS));
         Debug("cfrun: cfrun file = %s\n",VCFRUNHOSTS);
         }
      else if (strncmp(argv[i],"-d",2) == 0)
         {
         DEBUG = true;
         VERBOSE = true;
         }
      else if (strncmp(argv[i],"-v",2) == 0)
         {
         VERBOSE=true;
         }
      else if (strncmp(argv[i],"-T",2) == 0)
         {
         TRUSTALL = true;
         }
      else if (strncmp(argv[i],"-S",2) == 0)
         {
         SILENT = true;
         }
      else if (strncmp(argv[i],"--",2) == 0) 
         {
         optgroup++;
         }
      else if (argv[i][0] == '-')
         {
         printf("Error: Unknown option.\n");
         cfrunSyntax();
         exit(0);
         }
      else
         {
         AppendItem(&VCFRUNOPTIONHOSTS,argv[i],NULL); /* Restrict run hosts */
         }
      }
   else if (optgroup == 1) 
      {
      if (strncmp(argv[i],"--",2) == 0)
         {
         optgroup++;
         }
      else
         {
         strncat(CFRUNOPTIONS,argv[i],CF_BUFSIZE-1-strlen(CFRUNOPTIONS));
         strncat(CFRUNOPTIONS," ",CF_BUFSIZE-1-strlen(CFRUNOPTIONS));
         }
      }
   else
      {
      AppendItem(&VCFRUNCLASSES,argv[i],"");
      }
   }
 
Debug("CFRUNOPTIONS string: %s\n",CFRUNOPTIONS);

for (ip = VCFRUNCLASSES; ip != NULL; ip=ip->next)
   {
   Debug("Class item: %s\n",ip->name);
   }

/* XXX Initialize workdir for non privileged users */

strcpy(CFWORKDIR,WORKDIR);

#ifndef NT
if (getuid() > 0)
   {
   char *homedir;
   if ((homedir = getenv("HOME")) != NULL)
      {
      strncpy(CFWORKDIR,homedir,CF_BUFSIZE-16);
      strcat(CFWORKDIR,"/.cfagent");
      }
   }
#endif

ReadCfrunConf(VCFRUNHOSTS); 

GetNameInfo();

CfenginePort();
StrCfenginePort();

Debug("FQNAME = %s, WORKDIR = %s\n",VFQNAME,WORKDIR);

sprintf(VPREFIX,"cfrun:%s",VFQNAME);


/* Read hosts file */

umask(077);
strcpy(VLOCKDIR,CFWORKDIR);
strcpy(VLOGDIR,CFWORKDIR); 

OpenSSL_add_all_algorithms();
ERR_load_crypto_strings();
CheckWorkDirectories();
LoadSecretKeys();
RandomSeed();
}


/********************************************************************/

int PollServer(char *host,char *options,int storeinfile)

{ struct hostent *hp;
  struct sockaddr_in raddr;
  char sendbuffer[CF_BUFSIZE],recvbuffer[CF_BUFSIZE];
  char filebuffer[CF_BUFSIZE],parsed_host[CF_BUFSIZE];
  char reply[8];
  struct servent *server;
  int err,n_read,first,port;
  char *sp,forceipv4='n';
  void *gotkey;
  FILE  *fp;
  struct Image addresses;
#ifdef HAVE_GETADDRINFO
  struct addrinfo query, *response=NULL, *ap;
#endif
  char strport[16];
  
CONN = NewAgentConn();

if (storeinfile)
   {
   sprintf(filebuffer, "%s/%s", OUTPUTDIR, host);
   if ((fp = fopen(filebuffer, "w")) == NULL)
      {
      return false;
      }
   }
else
   {
   fp = stdout;
   }
 
port = ParseHostname(host,parsed_host);
 
FileVerbose(fp, "Connecting to server %s to port %d with options %s %s\n",parsed_host,port,options,CFRUNOPTIONS);

#ifdef HAVE_GETADDRINFO

 Debug("Using v6 compatible lookup...\n"); 

if (port)
   {
   snprintf(strport,15,"%u",port);
   }
else
   {
   snprintf(strport,15,"%s",STR_CFENGINEPORT);
   }
 
memset(&query,0,sizeof(struct addrinfo));
query.ai_family = AF_UNSPEC;
query.ai_socktype = SOCK_STREAM;
query.ai_flags = AI_PASSIVE;
 
if ((err = getaddrinfo(parsed_host,strport,&query,&response)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Unable to lookup %s (%s)",parsed_host,gai_strerror(err));
   CfLog(cferror,OUTPUT,"");
   }
 
for (ap = response; ap != NULL; ap = ap->ai_next)
   {
   snprintf(CONN->remoteip,CF_MAX_IP_LEN,"%s",sockaddr_ntop(ap->ai_addr));
   break;
   }

if (response != NULL)
   {
   freeaddrinfo(response);
   }

#else 
 
if ((hp = gethostbyname(parsed_host)) == NULL)
   {
   printf("Unknown host: %s\n", parsed_host);
   printf("Make sure that fully qualified names can be looked up at your site!\n");
   printf("i.e. www.gnu.org, not just www. If you use NIS or /etc/hosts\n");
   printf("make sure that the full form is registered too as an alias!\n");
   FileOutput(fp, fopl_error, "Unknown host\n");
   exit(1);
   }
 
memset(&raddr,0,sizeof(raddr));
 
if (port)
   {
   raddr.sin_port = htons(port);
   }
else
   {
   CfenginePort();
   raddr.sin_port = SHORT_CFENGINEPORT;
   }
 
raddr.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
raddr.sin_family = AF_INET; 

snprintf(CONN->remoteip,CF_MAX_IP_LEN,"%s",inet_ntoa(raddr.sin_addr));

#endif
 
addresses.trustkey = 'n'; 
addresses.encrypt = 'n';
addresses.server = strdup(parsed_host);

snprintf(sendbuffer,CF_BUFSIZE,"root-%s",parsed_host);
gotkey = HavePublicKey(sendbuffer);
 
if (!gotkey)
   {
   snprintf(sendbuffer,CF_BUFSIZE,"root-%s",CONN->remoteip);
   gotkey = HavePublicKey(sendbuffer);
   }
 
if (!gotkey)
   {
   if (TRUSTALL)
      {
      printf("Accepting public key from %s\n",parsed_host);
      FileOutput(fp, fopl_inform, "Accepting public key from host\n");
      addresses.trustkey = 'y'; 
      }
   else
      {
      printf("WARNING - You do not have a public key from host %s = %s\n",host,CONN->remoteip);
      printf("          Do you want to accept one on trust? (yes/no)\n\n--> ");
      
      while (true)
         {
         fgets(reply,8,stdin);
         Chop(reply);
         
         if (strcmp(reply,"yes")==0)
            {
            addresses.trustkey = 'y';
            break;
            }
         else if (strcmp(reply,"no")==0)
            {
            break;
            }
         else
            {
            printf("Please reply yes or no...(%s)\n",reply);
            }
         }
      }
   }
 
if (!RemoteConnect(parsed_host,forceipv4,port,strport))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"No response hailing %s\n",parsed_host);
   CfLog(cferror,OUTPUT,"socket");
   FileOutput(fp, fopl_error,OUTPUT);
   if (CONN->sd != CF_NOT_CONNECTED)
      {
      close(CONN->sd);
      CONN->sd = CF_NOT_CONNECTED;
      }
   free(addresses.server);
   return false;
   }
 
if (!IdentifyForVerification(CONN->sd,CONN->localip,CONN->family))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Hail succeeded, but unable to open channel to %s\n",parsed_host);
   CfLog(cferror,OUTPUT,"socket");
   FileOutput(fp, fopl_error,OUTPUT);
   close(CONN->sd);
   errno = EPERM;
   free(addresses.server);
   return false;
   }

if (!KeyAuthentication(&addresses))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Key-authentication for %s failed\n",VFQNAME);
   CfLog(cferror,OUTPUT,"");
   FileOutput(fp, fopl_error, "Key-authentication failed\n");
   errno = EPERM;
   close(CONN->sd);
   free(addresses.server);
   return false;
   }
 
snprintf(sendbuffer,CF_BUFSIZE,"EXEC %s %s",options,CFRUNOPTIONS);

if (SendTransaction(CONN->sd,sendbuffer,0,CF_DONE) == -1)
   {
   printf("Transmission rejected");
   FileOutput(fp, fopl_error, "Transmission rejected\n");
   close(CONN->sd);
   free(addresses.server);
   return false;
   }

SendClassData(CONN->sd,sendbuffer);

FileVerbose(fp, "%s replies..\n\n",parsed_host);

first = true;
 
while (true)
   {
   memset(recvbuffer,0,CF_BUFSIZE);

   if ((n_read = ReceiveTransaction(CONN->sd,recvbuffer,NULL)) == -1)
      {
      if (errno == EINTR) 
         {
         continue;
         }

      close(CONN->sd);
      free(addresses.server);
      return true;
      }

   if ((sp = strstr(recvbuffer,CFD_TERMINATOR)) != NULL)
      {
      *sp = '\0';
      fprintf(fp,"%s",recvbuffer);
      break;
      }

   if ((sp = strstr(recvbuffer,"BAD:")) != NULL)
      {
      *sp = '\0';
      fprintf(fp,"%s",recvbuffer+4);
      }   

   if (n_read == 0)
      {
      if (!first)
         {
         fprintf(fp,"\n");
         }
      break;
      }
   
   if (strlen(recvbuffer) == 0)
      {
      continue;
      }

   if (strstr(recvbuffer,"too soon"))
      {
      FileVerbose(fp,"%s",recvbuffer);
      }

   if (strstr(recvbuffer,"cfXen"))
      {
      fprintf(fp,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
      continue;
      }

   if (first)
      {
      fprintf(fp,"\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
      }
   
   first = false;

   fprintf(fp,"%s",recvbuffer);
   }

if (!first)
   {
   fprintf(fp,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
   }
 
close(CONN->sd);
free(addresses.server);
return true;
}

/********************************************************************/
/* Level 2                                                          */
/********************************************************************/

void ReadCfrunConf(char* cfg_fic)
{ char filename[CF_BUFSIZE], *sp;
  char buffer[CF_MAXVARSIZE], options[CF_BUFSIZE], line[CF_BUFSIZE];
  FILE *fp;
  struct Item *ip; 

memset(filename,0,CF_BUFSIZE);

if (!strchr(VCFRUNHOSTS, '/'))
   {
   if ((sp=getenv(CF_INPUTSVAR)) != NULL)
      {
      strcpy(filename,sp);
      if (filename[strlen(filename)-1] != '/')
         {
         strcat(filename,"/");
         }
      }
   else
      {
      snprintf(filename, CF_BUFSIZE, "%s/inputs/", CFWORKDIR);
      }
   }
 
strcat(filename,cfg_fic);

if ((fp = fopen(filename,"r")) == NULL)      /* Open root file */
   {
   printf("Unable to open %s\n",filename);
   return;
   }

while (!feof(fp))
   {
   memset(buffer,0,CF_MAXVARSIZE);
   memset(options,0,CF_BUFSIZE);
   memset(line,0,CF_BUFSIZE);

   ReadLine(line,CF_BUFSIZE,fp);

   if (strncmp(line,"domain",6) == 0)
      {
      sscanf(line,"domain = %295[^# \n]",VDOMAIN);
      Verbose("Domain name = %s\n",VDOMAIN);
      continue;
      }

   if (strncmp(line,"hostnamekeys",6) == 0)
      {
      char buf[256];
      buf[0] = '\0';
      sscanf(line,"hostnamekeys = %255[^# \n]",buf);
      Verbose("Hostname keys\n");

      if (strcmp(buf,"yes") == 0||strcmp(buf,"true") == 0)
         {
         AddMacroValue(CONTEXTID,"HostnameKeys","true");
         }
      continue;
      }

   if (strncmp(line,"maxchild", strlen("maxchild")) == 0)
      {
      sscanf(line,"maxchild = %295[^# \n]", buffer);
      
      if ( (MAXCHILD = atoi(buffer)) == 0 )
         {
         MAXCHILD = 1;
         }
      
      Verbose("cfrun: maxchild = %d\n", MAXCHILD);
      continue;
      }
   
   if (strncmp(line,"outputdir", strlen("outputdir")) == 0)
      {
      sscanf(line,"outputdir = %295[^# \n]", OUTPUTDIR);
      Verbose("cfrun: outputdir = %s\n", OUTPUTDIR);
      if ( opendir(OUTPUTDIR) == NULL)
         {
         printf("Directory %s does not exists\n", OUTPUTDIR);
         exit(1);
         }
      
      FileFlag=1;
      continue;
      }

   if (strncmp(line,"outputlevel", strlen("outputlevel")) == 0)
      {
      sscanf(line,"outputlevel = %295[^# \n]", buffer);
      
      if (strncmp(buffer,"inform", strlen("inform")) == 0)
         {
         OUTPUTLEVEL = fopl_inform;
         Verbose("cfrun: outputlevel = inform");
         }
      else if (strncmp(buffer,"error", strlen("error")) == 0)
         {
         OUTPUTLEVEL = fopl_error;
         Verbose("cfrun: outputlevel = error");
         }
      else if (strncmp(buffer,"normal", strlen("normal")) == 0)
         {
         OUTPUTLEVEL = fopl_normal;
         Verbose("cfrun: outputlevel = normal");
         }
      else
         {
         printf("Invalid outputlevel: %s\n", OUTPUTDIR);
         }
      
      continue;
      }
   
   if (strncmp(line,"include", strlen("include")) == 0)
      {
      sscanf(line,"include = %295[^# \n]", INCLUDEFILE);
      Verbose("cfrun: include = %s\n", INCLUDEFILE);
      ReadCfrunConf(INCLUDEFILE);
      continue;
      }
   
   if (strncmp(line,"access",6) == 0)
      {
      for (sp = line; (*sp != '=') && (*sp != '\0'); sp++)
         {
         }
      
      if (*sp == '\0' || *(++sp) == '\0')
         {
         continue;
         }
      
      CheckAccess(sp);
      continue;
      }

   if (strncmp(line,"bindtointerface",strlen("bindtointerface")) == 0)
      {
      sscanf(line,"bindtointerface = %295[^# \n]",BINDINTERFACE);
      Verbose("Bind interface = %s\n",BINDINTERFACE);
      continue;
      }
   
   sscanf(line,"%295s %[^#\n]",buffer,options);
   
   if (buffer[0] == '#')
      {
      continue;
      }

   if (strlen(buffer) == 0)
      {
      continue;
      }
   


   if (VCFRUNOPTIONHOSTS != NULL && !IsItemIn(VCFRUNOPTIONHOSTS,buffer))
      {
      Verbose("Skipping host %s\n",buffer);
      continue;
      }

   if ((!strstr(buffer,".")) && (strlen(VDOMAIN) > 0))
      {
      strcat(buffer,".");
      strcat(buffer,VDOMAIN);
      }
      
   if (!IsItemIn(VCFRUNHOSTLIST,buffer))
      {
      AppendItem(&VCFRUNHOSTLIST,buffer,options);
      }
   }
 
for (ip = VCFRUNHOSTLIST; ip != NULL; ip=ip->next)
   {
   Debug("host item: %s (%s)\n",ip->name,ip->classes);
   }

fclose(fp);
 
}


/********************************************************************/
 
int ParseHostname(char *hostname,char *new_hostname)

{ int port=5308;

sscanf(hostname,"%[^:]:%d", new_hostname, &port);

return(port);
}


/********************************************************************/

void SendClassData(int sd,char *sendbuffer)

{ struct Item *ip;
  int used;
  char *sp;

sp = sendbuffer;
used = 0;
memset(sendbuffer,0,CF_BUFSIZE);
  
for (ip = VCFRUNCLASSES; ip != NULL; ip = ip->next)
   {
   if (used + strlen(ip->name) +2 > CF_BUFSIZE)
      {
      if (SendTransaction(sd,sendbuffer,0,CF_DONE) == -1)
         {
         perror("send");
         return;
         }

      used = 0;
      sp = sendbuffer;
      memset(sendbuffer,0,CF_BUFSIZE);
      }
   
   strncat(sendbuffer,ip->name,CF_BUFSIZE-CF_BUFFERMARGIN);
   strcat(sendbuffer," ");

   sp += strlen(ip->name)+1;
   used += strlen(ip->name)+1;
   }

if (used + strlen(CFD_TERMINATOR) +2 > CF_BUFSIZE)
   {
   if (SendTransaction(sd,sendbuffer,0,CF_DONE) == -1)
      {
      perror("send");
      return;
      }

   used = 0;
   sp = sendbuffer;
   memset(sendbuffer,0,CF_BUFSIZE);
   }
   
sprintf(sp, "%s", CFD_TERMINATOR);

if (SendTransaction(sd,sendbuffer,0,CF_DONE) == -1)
   {
   perror("send");
   return;
   }
}

/********************************************************************/

void CheckAccess(char *users)

{ char id[CF_MAXVARSIZE], *sp;
  struct passwd *pw;
  int uid,myuid;

myuid = getuid();

if (myuid == 0)
   {
   return;
   }

for (sp = users; *sp != '\0'; sp++)
   {
   id[0] = '\0';

   sscanf(sp,"%295[^,:]",id);

   sp += strlen(id);
   
   if (isalpha((int)id[0]))
      {
      if ((pw = getpwnam(id)) == NULL)
         {
         printf("cfrun: No such user (%s) in password database\n",id);
         continue;
         }
      
      if (pw->pw_uid == myuid)
         {
         return;
         }
      }
   else
      {
      uid = atoi(id);
      if (uid == myuid)
         {
         return;
         }
      }
   }
 
 printf("cfrun: you have not been granted permission to run cfrun\n");
 exit(0);
}

/********************************************************************/

void FileOutput(FILE *fp,enum fileoutputlevels level,char *message)

{
 switch (level)
    {
    case fopl_inform:
 if (OUTPUTLEVEL >= fopl_inform)
    {
    fprintf(fp, "cfrun: INFORM: %s", message);
    }
 break;
    case fopl_error:
 if (OUTPUTLEVEL >= fopl_error)
    {
    fprintf(fp, "cfrun: ERROR: %s", message);
    }
 break;
    /* Default is to do nothing. That's right for "normal"! */
    }
 return;
}

/********************************************************************/

void cfrunSyntax()

{
 printf("Usage: cfrun [-f cfrun.hosts|-h|-d|-S|-T|-v] [-- OPTIONS [-- CLASSES]]\n\n");
 printf("-f cfrun.hosts\tcfrun file to read in list of hosts (see below for syntax.)\n");
 printf("-h\t\tGet this help message.\n");
 printf("-d\t\tDebug mode, turns on verbose as well.\n");
 printf("-S\t\tSilent mode.\n");
 printf("-T\t\tTrust all incoming public keys.\n");
 printf("-v\t\tVerbose mode.\n");
 printf("-- OPTIONS\tArguments to be passed to host application.\n");
 printf("-- CLASSES\tClasses to be defined for the hosts.\n\n");
 printf("e.g.  cfrun -- -- linux          Run on all linux machines\n");
 printf("      cfrun -- -p                Ping and parse on all hosts\n");
 printf("      cfrun host1 host2 -- -p    Ping and parse on named hosts\n");
 printf("      cfrun -v -- -p             Ping all, local verbose\n");
 printf("      cfrun -v -- -k -- solaris  Local verbose, all solaris, but no copy\n\n");
 printf("cfrun.hosts file syntax:\n");
 printf("# starts a comment\n");
 printf("include = [file]\t# External cfrun.hosts file to include.\n");
 printf("domain = [domain]\t# Domain to use for connection(s).\n");
 printf("maxchild = [num]\t# Maximum number of children to spawn during run.\n");
 printf("outputdir = [dir]\t# Directory where to put host output files.\n");
 printf("access = [user]\t\t# User allowed to do cfrun?\n");
 printf("[host]\t\t\t# One host per line list to cycle through.\n");
 printf("\t\t\t# Only the hosts are required for cfrun to operate.\n");
 
 exit(0);
}


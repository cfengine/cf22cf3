/* 

        Copyright (C) 1994-
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
/*  Cfengine : a site configuration langugae                       */
/*                                                                 */
/*  Module: (main) cfengine.c                                      */
/*                                                                 */
/*  Mark Burgess 1994/96                                           */
/*                                                                 */
/*******************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*******************************************************************/
/* Functions internal to cfengine.c                                */
/*******************************************************************/

int main (int argc,char *argv[]);
void Initialize (int argc, char **argv);
void PreNetConfig (void);
void ReadRCFile (void);
void Convert(int argc,char **argv);
void CheckSystemVariables (void);
void SetReferenceTime (int setclasses);
void SetStartTime (int setclasses);
void DoTree (int passes, char *info);
enum aseq EvaluateAction (char *action, struct Item **classlist, int pass);
void CheckOpts (int argc, char **argv);
int GetResource (char *var);
void Syntax (void);
void EmptyActionSequence (void);
void GetEnvironment (void);
int NothingLeftToDo (void);
void SummarizeObjects (void);
void SetContext (char *id);
void DeleteCaches (void);
void QueryCheck(void);
void ExtractControls(struct Item *base,struct Item **common,struct Item **agent,struct Item **exec, struct Item **server);
void ControlPreamble(FILE *fp,struct Item *common,struct Item *agent,struct Item *exec,struct Item *server);

/*******************************************************************/
/* Command line options                                            */
/*******************************************************************/

  /* GNU STUFF FOR LATER #include "getopt.h" */
 
 struct option OPTIONS[5] =
      {
      { "file",required_argument,0,'f' },
      { "variables",no_argument,0,'v' },
      { "server",no_argument,0,'s' },
      { "bundle",no_argument,0,'b' },
      { NULL,0,0,0 }
      };


/*******************************************************************/
/* Level 0 : Main                                                  */
/*******************************************************************/

int main(int argc,char *argv[])

{ struct Item *ip;
 
SetContext("global");
SetSignals(); 

signal (SIGTERM,HandleSignal);                   /* Signal Handler */
signal (SIGHUP,HandleSignal);
signal (SIGINT,HandleSignal);
signal (SIGPIPE,HandleSignal);
signal (SIGUSR1,HandleSignal);
signal (SIGUSR2,HandleSignal);

Initialize(argc,argv); 
SetReferenceTime(true);
SetStartTime(false);
 
if (! NOHARDCLASSES)
   {
   GetNameInfo();
   GetInterfaceInfo();
   GetV6InterfaceInfo();
   GetEnvironment();
   }

LISTSEPARATOR = ':';
PreNetConfig();
ReadRCFile();   /* Should come before parsing so that it can be overridden */

SetContext("main");


ParseInputFile(VINPUTFILE,true);
CheckFilters();
Convert(argc,argv);

return 0;
}

/*******************************************************************/
/* Level 1                                                         */
/*******************************************************************/
 
void Initialize(int argc,char *argv[])

{ char *sp, **cfargv;
  int i, j, cfargc, seed;
  struct stat statbuf;
  unsigned char s[16];
  char ebuff[CF_EXPANDSIZE];
  
strcpy(VDOMAIN,CF_START_DOMAIN);

PreLockState();

ISCFENGINE = true;
VFACULTY[0] = '\0';
VSYSADM[0] = '\0';
VNETMASK[0]= '\0';
VBROADCAST[0] = '\0';
VMAILSERVER[0] = '\0';
ALLCLASSBUFFER[0] = '\0';
VREPOSITORY = strdup("\0");

strcpy(METHODNAME,"cf-nomethod"); 
METHODREPLYTO[0] = '\0';
METHODFOR[0] = '\0';
 
#ifndef HAVE_REGCOMP
re_syntax_options |= RE_INTERVALS;
#endif
 
strcpy(VINPUTFILE,"cfagent.conf");
strcpy(VNFSTYPE,"nfs");

IDClasses();
 
/* Note we need to fix the options since the argv mechanism doesn't */
/* work when the shell #!/bla/cfengine -v -f notation is used.      */
/* Everything ends up inside a single argument! Here's the fix      */

cfargc = 1;

/* Pass 1: Find how many arguments there are. */
for (i = 1, j = 1; i < argc; i++)
   {
   sp = argv[i];
   
   while (*sp != '\0')
      {
      while (*sp == ' ' && *sp != '\0') /* Skip to arg */
         {
         sp++;
         }
      
      cfargc++;
      
      while (*sp != ' ' && *sp != '\0') /* Skip to white space */
         {
         sp++;
         }
      }
   }

/* Allocate memory for cfargv. */

cfargv = (char **) malloc(sizeof(char *) * (cfargc + 1));

if (!cfargv)
   {
   FatalError("cfagent: Out of memory parsing arguments\n");
   }

/* Pass 2: Parse the arguments. */

cfargv[0] = "cfagent";

for (i = 1, j = 1; i < argc; i++)
   {
   sp = argv[i];
   
   while (*sp != '\0')
      {
      while (*sp == ' ' && *sp != '\0') /* Skip to arg */
         {
         if (*sp == ' ')
            {
            *sp = '\0'; /* Break argv string */
            }
         sp++;
         }
      
      cfargv[j++] = sp;
      
      while (*sp != ' ' && *sp != '\0') /* Skip to white space */
         {
         sp++;
         }
      }
   }

cfargv[j] = NULL;
 
VDEFAULTBINSERVER.name = "";
 
VEXPIREAFTER = VDEFAULTEXPIREAFTER;
VIFELAPSED = VDEFAULTIFELAPSED;
TRAVLINKS = false;

/* XXX Initialize workdir for non privileged users */

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

sprintf(ebuff,"%s/state/cf_procs",CFWORKDIR);

if (stat(ebuff,&statbuf) == -1)
   {
   CreateEmptyFile(ebuff);
   }

sprintf(ebuff,"%s/state/cf_rootprocs",CFWORKDIR);

if (stat(ebuff,&statbuf) == -1)
   {
   CreateEmptyFile(ebuff);
   }

sprintf(ebuff,"%s/state/cf_otherprocs",CFWORKDIR);

if (stat(ebuff,&statbuf) == -1)
   {
   CreateEmptyFile(ebuff);
   }

strcpy(VLOGDIR,CFWORKDIR); 
strcpy(VLOCKDIR,VLOGDIR);  /* Same since 2.0.a8 */

OpenSSL_add_all_algorithms();
ERR_load_crypto_strings();
CheckWorkDirectories();
RandomSeed();

RAND_bytes(s,16);
s[15] = '\0';
seed = ElfHash(s);
srand48((long)seed);  
CheckOpts(cfargc,cfargv);
free(cfargv);

AddInstallable("no_default_route");
CfenginePort();
StrCfenginePort();
}

/*******************************************************************/

void PreNetConfig()                           /* Execute a shell script */

{ struct stat buf;
  char comm[CF_BUFSIZE],ebuff[CF_EXPANDSIZE];
  char *sp;
  FILE *pp;

if (NOPRECONFIG)
   {
   CfLog(cfverbose,"Ignoring the cf.preconf file: option set","");
   return;
   }

strcpy(VPREFIX,"cfengine:");
strcat(VPREFIX,VUQNAME);
 
if ((sp=getenv(CF_INPUTSVAR)) != NULL)
   {
   snprintf(comm,CF_BUFSIZE,"%s/%s",sp,VPRECONFIG);

   if (stat(comm,&buf) == -1)
       {
       CfLog(cfverbose,"No preconfiguration file","");
       return;
       }
   
   snprintf(comm,CF_BUFSIZE,"%s/%s %s 2>&1",sp,VPRECONFIG,CLASSTEXT[VSYSTEMHARDCLASS]);
   }
else
   {
   snprintf(comm,CF_BUFSIZE,"%s/%s",CFWORKDIR,VPRECONFIG);
   
   if (stat(comm,&buf) == -1)
      {
      CfLog(cfverbose,"No preconfiguration file\n","");
      return;
      }
   
   snprintf(comm,CF_BUFSIZE,"%s/%s %s",CFWORKDIR,VPRECONFIG,CLASSTEXT[VSYSTEMHARDCLASS]);
   }

 if (S_ISDIR(buf.st_mode) || S_ISCHR(buf.st_mode) || S_ISBLK(buf.st_mode))
    {
    snprintf(OUTPUT,CF_BUFSIZE*2,"Error: %s was not a regular file\n",VPRECONFIG);
    CfLog(cferror,OUTPUT,"");
    FatalError("Aborting.");
    }
 
Verbose("\n\nExecuting Net Preconfiguration script...%s\n\n",VPRECONFIG);
 
if ((pp = cfpopen(comm,"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Failed to open pipe to %s\n",comm);
   CfLog(cferror,OUTPUT,"");
   return;
   }

while (!feof(pp))
   {
   if (ferror(pp))  /* abortable */
      {
      CfLog(cferror,"Error running preconfig\n","ferror");
      break;
      }

   ReadLine(ebuff,CF_BUFSIZE,pp);

   if (feof(pp))
      {
      break;
      }
   
   if (strstr(ebuff,"cfengine-preconf-abort"))
      {
      exit(2);
      }

   if (ferror(pp))  /* abortable */
      {
      CfLog(cferror,"Error running preconfig\n","ferror");
      break;
      }

   CfLog(cfinform,ebuff,"");
   }

cfpclose(pp);
}

/*******************************************************************/

void DeleteCaches()
{
/* DeleteItemList(VEXCLUDECACHE); ?? */
}

/*******************************************************************/

void ReadRCFile()

{ char filename[CF_BUFSIZE], buffer[CF_BUFSIZE], *mp;
  char class[CF_MAXVARSIZE], variable[CF_MAXVARSIZE], value[CF_MAXVARSIZE];
  int c;
  FILE *fp;

filename[0] = buffer[0] = class[0] = variable[0] = value[0] = '\0';
LINENUMBER = 0;

snprintf(filename,CF_BUFSIZE,"%s/inputs/%s",CFWORKDIR,VRCFILE);
if ((fp = fopen(filename,"r")) == NULL)      /* Open root file */
   {
   return;
   }

while (!feof(fp))
   {
   ReadLine(buffer,CF_BUFSIZE,fp);
   LINENUMBER++;
   class[0]='\0';
   variable[0]='\0';
   value[0]='\0';

   if (strlen(buffer) == 0 || buffer[0] == '#')
      {
      continue;
      }

   if (strstr(buffer,":") == 0)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Malformed line (missing :) in resource file %s - skipping\n",VRCFILE);
      CfLog(cferror,OUTPUT,"");
      continue;
      }

   sscanf(buffer,"%[^.].%[^:]:%[^\n]",class,variable,value);

   if (class[0] == '\0' || variable[0] == '\0' || value[0] == '\0')
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"%s:%s - Bad resource\n",VRCFILE,buffer);
      CfLog(cferror,OUTPUT,"");
      snprintf(OUTPUT,CF_BUFSIZE*2,"class=%s,variable=%s,value=%s\n",class,variable,value);
      CfLog(cferror,OUTPUT,"");
      FatalError("Bad resource");
      }

   if (strcmp(CLASSTEXT[VSYSTEMHARDCLASS],class) != 0) 
      {
      continue;  /* No point if not equal*/
      }

   if ((mp = strdup(value)) == NULL)
      {
      perror("malloc");
      FatalError("malloc in ReadRCFile");
      }

   snprintf(OUTPUT,CF_BUFSIZE*2,"Redefining resource %s as %s (%s)\n",variable,value,class);
   CfLog(cfverbose,OUTPUT,"");

   c = VSYSTEMHARDCLASS;

   switch (GetResource(variable))
      {
      case rmountcom:
                        VMOUNTCOMM[c] = mp;
                        break;

      case runmountcom:
                        VUNMOUNTCOMM[c] = mp;
                        break;
      case rethernet:
                        VIFDEV[c] = mp;
                        break;
      case rmountopts:
                        VMOUNTOPTS[c] = mp;
                        break;
      case rfstab:
                        VFSTAB[c] = mp;
                        break;
      case rmaildir:
                        VMAILDIR[c] = mp;
                        break;
      case rnetstat:
                        VNETSTAT[c] = mp;
                        break;
      case rpscomm:
                 VPSCOMM[c] = mp;
                 break;
      case rpsopts:
                 VPSOPTS[c] = mp;
                 break;
      default:
                        snprintf(OUTPUT,CF_BUFSIZE,"Bad resource %s in %s\n",variable,VRCFILE);
                        FatalError(OUTPUT);
                        break;
      }
   }

fclose(fp);
}

/*******************************************************************/

void EmptyActionSequence()

{
DeleteItemList(VACTIONSEQ);
VACTIONSEQ = NULL;
}

/*******************************************************************/

void Convert(int argc,char **argv)

{ struct Item *ip,*p2;
  FILE *fout;
  char *convert = argv[0];
  int i,lines_converted = 0;

if (VARIABLES)
   {
   ListDefinedVariables();
   ListDefinedIgnore(NULL);
   
// Classes  -- need to adde these differently
   ListDefinedClasses();
   
   ListDefinedStrategies(NULL);
   }
else if (BUNDLES)
   {
   printf("bundle agent converted_%s\n{\n",CanonifyName(VINPUTFILE));
   
   printf("\n\nfiles:\n\n");
   
   ListFiles(NULL);
   ListDefinedTidy(NULL);
   ListDefinedDisable(NULL);
   ListDefinedMakePaths(NULL);
   
   ListDefinedImages(NULL);
   ListDefinedLinks(NULL);
   ListDefinedLinkchs(NULL);
   
   ListFileEdits(NULL);
   

//ListACLs();
//ListFilters(NULL);

// Methods

   ListDefinedMethods(NULL);
   
// Packages
   
   ListDefinedPackages(NULL);
   
//Processes
   
   ListProcesses(NULL);
   
// Storage
   
   ListDefinedRequired(NULL);
   ListMiscMounts(NULL);
   ListUnmounts(NULL);
   
// commands
   
   ListDefinedScripts(NULL);
   ListDefinedAlerts(NULL);

   //ListDefinedResolvers(NULL);
   
   printf("\n}\n\n");
   }
else if (SERVER)
   {
   struct Item *promises_cf_list = NULL, *agent = NULL, *exec = NULL, *server = NULL, *common = NULL;;
   char cmd[CF_BUFSIZE];
   struct Item *partial = NULL,*ptr;
   
   snprintf(cmd,CF_BUFSIZE,"%s -v -f cfservd.conf",convert);
   ConvertFile(cmd,&promises_cf_list);

   ExtractControls(promises_cf_list,&common,&agent,&exec,&server);
   ServerControl(server);

   printf("bundle server converted_%s\n{\n",CanonifyName(VINPUTFILE));
   for (ptr = promises_cf_list; ptr != NULL; ptr=ptr->next)
      {
      printf("%s",ptr->name);
      }

   ListServerAccess();   
   printf("\n}\n\n");
   }
else
   { 
   struct Item *promises_cf_list = NULL, *agent = NULL, *exec = NULL, *server = NULL, *common = NULL;;
   struct Item *partial = NULL,*ptr;
   char filename[CF_BUFSIZE];
   char cmd[CF_BUFSIZE],*dir = "cf_conversion_output",path[CF_BUFSIZE];
   struct stat sb;
   
   // Master controller

   printf("\n\n");
   printf("Cfengine Conversion Utility (alpha 1)\n\n");

   if (stat(dir,&sb) == -1)
      {
      printf(" -> Creating output directory \"%s\"\n",dir);
      if (mkdir(dir,0700) == -1)
         {
         perror("mkdir");
         FatalError("abort");
         }
      }
   else if (!S_ISDIR(sb.st_mode))
      {
      FatalError("Directory blocking output route");
      }

   printf(" -> Matrix from %s\n",convert);
   printf(" -> INPUTS from . %s\n",getenv("CFINPUTS"));
   getcwd(path,CF_BUFSIZE);
   printf(" -> OUTPUTS at %s/%s\n",path,dir);
   
   printf(" -> Commencing pre-scan for common environment\n");

   AppendItem(&promises_cf_list,"bundle common g\n{\n",NULL);

   snprintf(cmd,CF_BUFSIZE,"%s -v",convert);

   for (i = 1; i < argc; i++)
      {
      strcat(cmd," ");
      strcat(cmd,argv[i]);
      }
   
   ConvertFile(cmd,&promises_cf_list);
   AppendItem(&promises_cf_list,"}\n\n",NULL);

   printf(" -> Pre-scan complete\n");

   printf(" -> Scanning for recognizable control settings\n");
   ExtractControls(promises_cf_list,&common,&agent,&exec,&server);
   
   printf(" -> Start main promise bundle\n");
   AppendItem(&promises_cf_list,"bundle agent main\n{\nmethods:\n\n",NULL);

   snprintf(cmd,CF_BUFSIZE,"%s -b",convert);

   for (i = 1; i < argc; i++)
      {
      strcat(cmd," ");
      strcat(cmd,argv[i]);
      }
   
   ConvertFile(cmd,&promises_cf_list);
   
   if (VIMPORT)
      {
      printf(" -> Import files detected\n");
      
      for (ip = VIMPORT; ip != NULL; ip=ip->next)
         {
         printf(" -> delta-Transformation of \"%s\"\n",ip->name);

         if (ip->classes)
            {
            snprintf(cmd,CF_BUFSIZE-1,"%s::\n",ip->classes);
            AppendItem(&promises_cf_list,cmd,NULL);
            }
         
         snprintf(cmd,CF_BUFSIZE-1," \"any\" usebundle => converted_%s;\n",CanonifyName(ip->name));
         AppendItem(&promises_cf_list,cmd,NULL);

         snprintf(cmd,CF_BUFSIZE-1,"%s -b -f %s",convert,ip->name);         
         ConvertFile(cmd,&partial);

         snprintf(filename,CF_BUFSIZE-1,"%s/%s/%s",path,dir,ip->name);
         MakeDirectoriesFor(filename,'y');

         if ((fout = fopen(filename,"w")) == NULL)
            {
            printf(" !! Unable to write transformation -> %s\n",filename);
            FatalError("abort");
            }

         for (ptr = partial; ptr != NULL; ptr=ptr->next)
            {
            fprintf(fout,"%s",ptr->name);
            lines_converted++;
            }
         
         fclose(fout);
         DeleteItemList(partial);
         partial = NULL;
         }
      
      AppendItem(&promises_cf_list,"}\n\n",NULL);
      }

   snprintf(filename,CF_BUFSIZE,"%s/%s/%s",path,dir,"promises.cf");
   
   if ((fout = fopen(filename,"w")) == NULL)
      {
      printf(" !! Unable to write transformation -> %s\n",filename);
      FatalError("abort");
      }
   
   ControlPreamble(fout,common,agent,exec,server);
   
   printf(" -> Converting cfservd.cf\n");
   
   snprintf(cmd,CF_BUFSIZE-1,"%s -s -f cfservd.conf",convert);

   for (i = 1; i < argc; i++)
      {
      strcat(cmd," ");
      strcat(cmd,argv[i]);
      }

   ConvertFile(cmd,&promises_cf_list);

   printf(" -> Writing promises.cf\n");
   
   for (ptr = promises_cf_list; ptr != NULL; ptr=ptr->next)
      {
      fprintf(fout,"%s",ptr->name);
      lines_converted++;
      }
   
   fclose(fout);

   printf(" -> %d lines of core transformed\n",lines_converted);
   }
}

/*******************************************************************/

void ExtractControls(struct Item *base,struct Item **common,struct Item **agent,struct Item **exec, struct Item **server)

{ struct Item *ip;
  char out[CF_BUFSIZE];
  char lval[CF_BUFSIZE],rval[CF_BUFSIZE],tmp[CF_BUFSIZE];
  
for (ip = base; ip != NULL; ip=ip->next)
   {
   sscanf(ip->name,"%s string => %s",tmp,rval);
   sscanf(tmp+1,"%[^\\\"]",lval);
   
   if (strcmp(ToLowerStr(lval),"emailmaxlines") == 0)
      {
      printf(" -> > convert control setting %s\n",lval);
      snprintf(out,CF_BUFSIZE-1,"mailmaxlines => %s\n",rval);
      AppendItem(exec,out,NULL);
      strcpy(ip->name,"");
      }
   
   if (strcmp(ToLowerStr(lval),"sysadm") == 0)
      {
      printf(" -> > convert control setting %s\n",lval);
      snprintf(out,CF_BUFSIZE-1,"mailto => %s\n",rval);
      AppendItem(exec,out,NULL);
      strcpy(ip->name,"");
      }
   
   if (strcmp(ToLowerStr(lval),"emailto") == 0)
      {
      printf(" -> > convert control setting %s\n",lval);
      snprintf(out,CF_BUFSIZE-1,"mailto => %s\n",rval);
      AppendItem(exec,out,NULL);
      strcpy(ip->name,"");
      }

   if (strcmp(ToLowerStr(lval),"emailfrom") == 0)
      {
      printf(" -> > convert control setting %s\n",lval);
      snprintf(out,CF_BUFSIZE-1,"mailfrom => %s\n",rval);
      AppendItem(exec,out,NULL);
      strcpy(ip->name,"");
      }

   if (strcmp(ToLowerStr(lval),"cfinputs_version") == 0)
      {
      printf(" -> > convert control setting %s\n",lval);
      snprintf(out,CF_BUFSIZE-1,"version => %s\n",rval);
      AppendItem(common,out,NULL);
      strcpy(ip->name,"");
      }

   if (strcmp(ToLowerStr(lval),"smtpserver") == 0)
      {
      printf(" -> > convert control setting %s\n",lval);
      snprintf(out,CF_BUFSIZE-1,"smtpserver => %s\n",rval);
      AppendItem(exec,out,NULL);
      strcpy(ip->name,"");
      }

   if (strcmp(ToLowerStr(lval),"splaytime") == 0)
      {
      printf(" -> > convert control setting %s\n",lval);
      snprintf(out,CF_BUFSIZE-1,"splaytime => %s\n",rval);
      AppendItem(exec,out,NULL);
      strcpy(ip->name,"");
      }

   if (strcmp(ToLowerStr(lval),"inform") == 0)
      {
      printf(" -> > convert control setting %s\n",lval);
      snprintf(out,CF_BUFSIZE-1,"inform => %s\n",rval);
      AppendItem(agent,out,NULL);
      strcpy(ip->name,"");
      }

   if (strcmp(ToLowerStr(lval),"syslog") == 0)
      {
      printf(" -> > convert control setting %s\n",lval);
      snprintf(out,CF_BUFSIZE-1,"syslog => %s\n",rval);
      AppendItem(agent,out,NULL);
      strcpy(ip->name,"");
      }
   
   if (strcmp(ToLowerStr(lval),"maxconnections") == 0)
      {
      snprintf(out,CF_BUFSIZE-1,"maxconnections => %s\n",rval);
      AppendItem(server,out,NULL);
      strcpy(ip->name,"");
      }

   if (strcmp(ToLowerStr(lval),"logallconnections") == 0)
      {
      snprintf(out,CF_BUFSIZE-1,"logallconnections => %s\n",rval);
      AppendItem(server,out,NULL);
      strcpy(ip->name,"");
      }
   
   if (strcmp(ToLowerStr(lval),"cfruncommand") == 0)
      {
      snprintf(out,CF_BUFSIZE-1,"cfruncommand => %s\n",rval);
      AppendItem(server,out,NULL);
      strcpy(ip->name,"");
      }

   if (strcmp(ToLowerStr(lval),"addinstallables") == 0)
      {
      printf(" -> > convert control setting %s\n",lval);
      snprintf(out,CF_BUFSIZE-1,"\n# addinstallables removed - no longer relevant\n");
      //AppendItem(agent,out,NULL);
      strcpy(ip->name,"");
      }

   if (strcmp(ToLowerStr(lval),"workdir") == 0)
      {
      printf(" -> > convert control setting %s\n",lval);
      snprintf(out,CF_BUFSIZE-1,"\n# workdir removed - no longer relevant\n");
      //AppendItem(common,out,NULL);
      strcpy(ip->name,"");
      }

   if (strcmp(ToLowerStr(lval),"moduledirectory") == 0)
      {
      printf(" -> > convert control setting %s\n",lval);
      snprintf(out,CF_BUFSIZE-1,"\n# moduledirectory removed - no longer relevant\n");
      //AppendItem(agent,out,NULL);
      strcpy(ip->name,"");
      }
   }
}

/*******************************************************************/

void ControlPreamble(FILE *fp,struct Item *common,struct Item *agent,struct Item *exec,struct Item *server)
{
struct Item *ip;
 
fprintf(fp,"body common control\n"
    "{\n"
    "bundlesequence => { \"g\",\"main\" };\n"
    );

for (ip = common; ip != NULL; ip=ip->next)
   {
   fprintf(fp,"%s",ip->name);
   }

fprintf(fp,"}\n\n");

fprintf(fp,"body agent control\n{\n");
for (ip = agent; ip != NULL; ip=ip->next)
   {
   fprintf(fp,"%s",ip->name);
   }

fprintf(fp,"}\n\n");

fprintf(fp,"body executor control\n{\n");
for (ip = exec; ip != NULL; ip=ip->next)
   {
   fprintf(fp,"%s",ip->name);
   }

fprintf(fp,"}\n\n");
}

/*******************************************************************/

void CheckSystemVariables()

{ char id[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];
  int time, hash, activecfs, locks;

Debug2("\n\n");

if (VACTIONSEQ == NULL)
   {
   Warning("actionsequence is empty ");
   Warning("perhaps cfagent.conf/update.conf have not yet been set up?");
   }

ACTION = none;

sprintf(id,"%d",geteuid());   /* get effective user id */

ebuff[0] = '\0';

if (!StrStr(VSYSNAME.nodename,VDOMAIN))
   {
   snprintf(VFQNAME,CF_BUFSIZE,"%s.%s",VSYSNAME.nodename,ToLowerStr(VDOMAIN));
   strcpy(VUQNAME,VSYSNAME.nodename);
   }
else
   {
   int n = 0;
   strcpy(VFQNAME,VSYSNAME.nodename);
   
   while(VSYSNAME.nodename[n++] != '.')
      {
      }
   
   strncpy(VUQNAME,VSYSNAME.nodename,n-1);        
   }
  
Verbose("Accepted domain name: %s\n\n",VDOMAIN); 


if (VACCESSLIST != NULL && !IsItemIn(VACCESSLIST,id))
   {
   FatalError("Access denied - user not listed in access list");
   }

Debug2("cfagent -d : Debugging output enabled.\n");

EDITVERBOSE = false;

if (ERRORCOUNT > 0)
   {
   FatalError("Execution terminated after parsing due to errors in program");
   }
 
VCANONICALFILE = strdup(CanonifyName(VINPUTFILE));

if (GetMacroValue(CONTEXTID,"LockDirectory"))
   {
   Verbose("\n[LockDirectory is no longer used - same as LogDirectory]\n\n");
   }

if (GetMacroValue(CONTEXTID,"LogDirectory"))
   {
   Verbose("\n[LogDirectory is no longer runtime configurable: use configure --with-workdir=CFWORKDIR ]\n\n");
   }

Verbose("LogDirectory = %s\n",VLOGDIR);
  
LoadSecretKeys();
 
if (GetMacroValue(CONTEXTID,"childlibpath"))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"LD_LIBRARY_PATH=%s",GetMacroValue(CONTEXTID,"childlibpath"));
   if (putenv(strdup(OUTPUT)) == 0)
      {
      Verbose("Setting %s\n",OUTPUT);
      }
   else
      {
      Verbose("Failed to set %s\n",GetMacroValue(CONTEXTID,"childlibpath"));
      }
   }

if (GetMacroValue(CONTEXTID,"BindToInterface"))
   {
   ExpandVarstring("$(BindToInterface)",ebuff,NULL);
   strncpy(BINDINTERFACE,ebuff,CF_BUFSIZE-1);
   Debug("$(BindToInterface) Expanded to %s\n",BINDINTERFACE);
   } 

if (GetMacroValue(CONTEXTID,"MaxCfengines"))
   {
   activecfs = atoi(GetMacroValue(CONTEXTID,"MaxCfengines"));
 
   locks = CountActiveLocks();
 
   if (locks >= activecfs)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Too many cfagents running (%d/%d)\n",locks,activecfs);
      CfLog(cferror,OUTPUT,"");
      closelog();
      exit(1);
      }
   }


if (OptionIs(CONTEXTID,"SkipIdentify",true))
   {
   SKIPIDENTIFY = true;
   }

if (OptionIs(CONTEXTID,"Verbose",true))
   {
   VERBOSE = true;
   }

if (OptionIs(CONTEXTID,"LastSeen",false))
   {
   LASTSEEN = false;
   }

if (OptionIs(CONTEXTID,"FullEncryption",true))
   {
   FULLENCRYPT = true;
   }

if (OptionIs(CONTEXTID,"Inform",true))
   {
   INFORM = true;
   } 

if (OptionIs(CONTEXTID,"Exclamation",false))
   {
   EXCLAIM = false;
   } 

if (OptionIs(CONTEXTID,"Auditing",true))
   {
   AUDIT = true;
   }

INFORM_save = INFORM;

if (OptionIs(CONTEXTID,"Syslog",true))
   {
   LOGGING = true;
   }

LOGGING_save = LOGGING;

if (OptionIs(CONTEXTID,"DryRun",true))
   {
   DONTDO = true;
   AddClassToHeap("opt_dry_run");
   }

if (GetMacroValue(CONTEXTID,"BinaryPaddingChar"))
   {
   strcpy(ebuff,GetMacroValue(CONTEXTID,"BinaryPaddingChar"));
   
   if (ebuff[0] == '\\')
      {
      switch (ebuff[1])
         {
         case '0': PADCHAR = '\0';
             break;
         case '\n': PADCHAR = '\n';
             
             break;
         case '\\': PADCHAR = '\\';
         }
      }
   else
      {
      PADCHAR = ebuff[0];
      }
   }
 
 
if (OptionIs(CONTEXTID,"Warnings",true))
   {
   WARNINGS = true;
   }

if (OptionIs(CONTEXTID,"NonAlphaNumFiles",true))
   {
   NONALPHAFILES = true;
   }

if (OptionIs(CONTEXTID,"SecureInput",true))
   {
   CFPARANOID = true;
   }

if (OptionIs(CONTEXTID,"ShowActions",true))
   {
   SHOWACTIONS = true;
   }

if (GetMacroValue(CONTEXTID,"Umask"))
   {
   mode_t val;
   val = (mode_t)atoi(GetMacroValue(CONTEXTID,"Umask"));
   if (umask(val) == (mode_t)-1)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Can't set umask to %o\n",val);
      CfLog(cferror,OUTPUT,"umask");
      }
   }

if (GetMacroValue(CONTEXTID,"DefaultCopyType"))
   {
   if (strcmp(GetMacroValue(CONTEXTID,"DefaultCopyType"),"mtime") == 0)
      {
      DEFAULTCOPYTYPE = 'm';
      }
   if (strcmp(GetMacroValue(CONTEXTID,"DefaultCopyType"),"checksum") == 0)
      {
      DEFAULTCOPYTYPE = 'c';
      }
   if (strcmp(GetMacroValue(CONTEXTID,"DefaultCopyType"),"binary") == 0)
      {
      DEFAULTCOPYTYPE = 'b';
      }
   if (strcmp(GetMacroValue(CONTEXTID,"DefaultCopyType"),"ctime") == 0)
      {
      DEFAULTCOPYTYPE = 't';
      }
   }
 
if (GetMacroValue(CONTEXTID,"ChecksumDatabase"))
   {
   FatalError("ChecksumDatabase variable is deprecated - comment it out");
   }
else
   {
   snprintf(ebuff,CF_BUFSIZE,"%s/%s",CFWORKDIR,CF_CHKDB);
   CHECKSUMDB = strdup(ebuff);
   }

if (SHOWDB)
   {
   printf("%s\n",CHECKSUMDB);
   exit(0);
   }
 
Verbose("Checksum database is %s\n",CHECKSUMDB); 
 
if (GetMacroValue(CONTEXTID,"CompressCommand"))
   {
   ExpandVarstring("$(CompressCommand)",ebuff,NULL);

   COMPRESSCOMMAND = strdup(ebuff);
   
   if (*COMPRESSCOMMAND != '/')
      {
      FatalError("$(CompressCommand) does not expand to an absolute filename\n");
      }
   }

if (OptionIs(CONTEXTID,"ChecksumUpdates",true))
   {
   CHECKSUMUPDATES = true;
   } 
 
if (GetMacroValue(CONTEXTID,"TimeOut"))
   {
   time = atoi(GetMacroValue(CONTEXTID,"TimeOut"));

   if (time < 3 || time > 60)
      {
      CfLog(cfinform,"TimeOut not between 3 and 60 seconds, ignoring.\n","");
      }
   else
      {
      CF_TIMEOUT = time;
      }
   }

/* Make sure we have a healthy binserver list so binserver expansion works, even if binserver not defined */

if (VBINSERVERS == NULL)
   {
   PrependItem(&VBINSERVERS,VUQNAME,NULL);
   }

if (VBINSERVERS->name != NULL)
   {
   VDEFAULTBINSERVER = *VBINSERVERS;
   Verbose("Default binary server seems to be %s\n",VDEFAULTBINSERVER.name);
   }

AppendItem(&VBINSERVERS,VUQNAME,NULL);

/* Done binserver massage */

if (NOSPLAY)
   {
   return;
   }

time = 0;
snprintf(ebuff,CF_BUFSIZE,"%s+%s+%d",VFQNAME,VIPADDRESS,getuid());
hash = Hash(ebuff);

if (!NOSPLAY)
   {
   if (GetMacroValue(CONTEXTID,"SplayTime"))
      {
      time = atoi(GetMacroValue(CONTEXTID,"SplayTime"));
      
      if (time < 0)
         {
         CfLog(cfinform,"SplayTime with negative value, ignoring.\n","");
         return;
         }
      
      if (!DONESPLAY)
         {
         if (!PARSEONLY)
            {
            DONESPLAY = true;
            Verbose("Sleeping for SplayTime %d seconds\n\n",(int)(time*60*hash/CF_HASHTABLESIZE));
            sleep((int)(time*60*hash/CF_HASHTABLESIZE));
            }
         }
      else
         {
         Verbose("Time splayed once already - not repeating\n");
         }
      }
   } 
 
 if (OptionIs(CONTEXTID,"LogTidyHomeFiles",false))
    {
    LOGTIDYHOMEFILES = false;
    } 
}


/*******************************************************************/

void QueryCheck()

{ char src[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];
  struct Item *ip;
 
if (QUERYVARS == NULL)
   {
   return;
   }

for (ip = QUERYVARS; ip != NULL; ip=ip->next)
   {
   snprintf(src,CF_MAXVARSIZE,"$(%s)",ip->name);
   ExpandVarstring(src,ebuff,"");
   printf("%s=%s\n",ip->name,ebuff);
   }

exit(0);
}

/*******************************************************************/

void DoTree(int passes,char *info)

{ struct Item *action;
}


/*******************************************************************/

int NothingLeftToDo()

   /* Check the *probable* source for additional action */

{ struct ShellComm *vscript;
  struct Link *vlink;
  struct File *vfile;
  struct Disable *vdisablelist;
  struct File *vmakepath;
  struct Link *vchlink;
  struct UnMount *vunmount;
  struct Edit *veditlist;
  struct Process *vproclist;
  struct Tidy *vtidy;
  struct Package *vpkg;
  struct Image *vcopy;

if (IsWildItemIn(VACTIONSEQ,"process*"))
   {
   for (vproclist = VPROCLIST; vproclist != NULL; vproclist=vproclist->next)
      {
      if ((vproclist->done == 'n') && IsDefinedClass(vproclist->classes))
         {
         Verbose("Checking for potential rule:: Proc <%s> / %s\n",vproclist->expr,vproclist->classes);
         return false;
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"shellcomman*"))
   {
   for (vscript = VSCRIPT; vscript != NULL; vscript=vscript->next)
      {
      if ((vscript->done == 'n')  && IsDefinedClass(vscript->classes))
         {
         Verbose("Checking for potential rule:: Shell <%s> / %s\n",vscript->name,vscript->classes);
         return false;
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"cop*"))
   {
   for (vcopy = VIMAGE; vcopy != NULL; vcopy=vcopy->next)
      {
      if ((vcopy->done == 'n')  && IsDefinedClass(vcopy->classes))
         {
         Verbose("Checking for potential rule:: Copy <%s> / %s\n",vcopy->path,vcopy->classes);
         return false;
         }
      }
   }


if (IsWildItemIn(VACTIONSEQ,"file*"))
   {
   for (vfile = VFILE; vfile != NULL; vfile=vfile->next)
      {
      if ((vfile->done == 'n') && IsDefinedClass(vfile->classes))
         {
         Verbose("Checking for potential rule:: File <%s>/ %s\n",vfile->path,vfile->classes);
         return false;
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"tid*"))
   {
   for (vtidy = VTIDY; vtidy != NULL; vtidy=vtidy->next)
      {
      if (vtidy->done == 'n')
         {
         struct TidyPattern *tp;
         int active = 0;
         
         for (tp = vtidy->tidylist; tp != NULL; tp=tp->next)
            {
            if (IsDefinedClass(tp->classes))
               {
               active=1;
               break;
               }
            }

         if (active)
            {
            Verbose("Checking for potential rule:: Tidy <%s>\n",vtidy->path);
            return false;
            }
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"editfile*"))
   {
   for (veditlist = VEDITLIST; veditlist != NULL; veditlist=veditlist->next) 
      { 
      struct Edlist *ep;
      int something_to_do = false;
      
      for (ep = veditlist->actions; ep != NULL; ep=ep->next)
         {
         if (IsDefinedClass(ep->classes))
            {
            something_to_do = true;
            if (ep->data)
               {
               Verbose("Defined Edit %s / %s\n",ep->data,ep->classes);
               }
            else
               {
               Verbose("Defined Edit nodata / %s\n",ep->classes);
               }
            break;
            }
         }
   
      if (veditlist->done == 'n' && something_to_do)
         {
         Verbose("Checking for potential rule:: Edit <%s>\n",veditlist->fname);
         return false;
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"process*"))
   {
   for (vdisablelist = VDISABLELIST; vdisablelist != NULL; vdisablelist=vdisablelist->next)
      {
      if (vdisablelist->done == 'n' && IsDefinedClass(vdisablelist->classes))
         {
         Verbose("Checking for potential rule:: Disable <%s> / %s\n",vdisablelist->name,vdisablelist->classes);
         return false;
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"director*"))
   {
   for (vmakepath = VMAKEPATH; vmakepath != NULL; vmakepath=vmakepath->next)
      {
      if (vmakepath->done == 'n' && IsDefinedClass(vmakepath->classes))
         {
         Verbose("Checking for potential rule:: makePath <%s>\n",vmakepath->path);
         return false;
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"link*"))
   {
   for (vlink = VLINK; vlink != NULL; vlink=vlink->next)
      {
      if (vlink->done == 'n' && IsDefinedClass(vlink->classes))
         {
         Verbose("Checking for potential rule:: Link <%s>\n",vlink->from);
         return false;
         }
      }
   
   for (vchlink = VCHLINK; vchlink != NULL; vchlink=vchlink->next)
      {
      if (vchlink->done == 'n' && IsDefinedClass(vchlink->classes))
         {
         Verbose("Checking for potential rule:: CLink <%s>\n",vchlink->from);
         return false;
         }
      }
   }


if (IsWildItemIn(VACTIONSEQ,"unmoun*"))
   {
   for (vunmount = VUNMOUNT; vunmount != NULL; vunmount=vunmount->next)
      {
      if (vunmount->done == 'n' && IsDefinedClass(vunmount->classes))
         {
         Verbose("Checking for potential rule:: Umount <%s>\n",vunmount->name);
         return false;
         }
      }
   }

if (IsWildItemIn(VACTIONSEQ,"packag*"))
   {
   for (vpkg = VPKG; vpkg != NULL; vpkg=vpkg->next)
      {
      if (vpkg->done == 'n' && IsDefinedClass(vpkg->classes))
         {
         Verbose("Checking for potential rule:: Packages <%s>\n",vpkg->name);
         return false;
         }
      }
   }


return true; 
}

/*******************************************************************/

enum aseq EvaluateAction(char *action,struct Item **classlist,int pass)

{ int i,j = 0;
  char *sp,cbuff[CF_BUFSIZE],actiontxt[CF_BUFSIZE],mod[CF_BUFSIZE],args[CF_BUFSIZE];
  struct Item *ip;

cbuff[0]='\0';
actiontxt[0]='\0';
mod[0] = args[0] = '\0'; 
sscanf(action,"%s %[^\n]",mod,args);
sp = mod;

while (*sp != '\0')
   {
   ++j;
   sscanf(sp,"%[^.]",cbuff);

   while ((*sp != '\0') && (*sp !='.'))
      {
      sp++;
      }
 
   if (*sp == '.')
      {
      sp++;
      }
 
   if (IsHardClass(cbuff))
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Error in action sequence: %s\n",action);
      CfLog(cferror,OUTPUT,"");
      FatalError("You cannot add a reserved class!");
      }
 
   if (j == 1)
      {
      VIFELAPSED = VDEFAULTIFELAPSED;
      VEXPIREAFTER = VDEFAULTEXPIREAFTER;
      strcpy(actiontxt,cbuff);
      continue;
      }
   else
      {
      if ((strncmp(actiontxt,"module:",7) != 0) && ! IsSpecialClass(cbuff))
         {
         AppendItem(classlist,cbuff,NULL);
         }
      }
   }
 
 BuildClassEnvironment();

if ((VERBOSE || DEBUG || D2) && *classlist != NULL)
   {
   printf("\n                  New temporary class additions\n");
   printf("                  --------( Pass %d )-------\n",pass);
   for (ip = *classlist; ip != NULL; ip=ip->next)
      {
      printf("                             %s\n",ip->name);
      }
   }

ACTION = none;
 
for (i = 0; ACTIONSEQTEXT[i] != NULL; i++)
   {
   if (strcmp(ACTIONSEQTEXT[i],actiontxt) == 0)
      {
      Debug("Actionsequence item %s\n",actiontxt);
      ACTION = i;
      return (enum aseq) i;
      }
   }

Debug("Checking if entry is a module\n");
 
if (strncmp(actiontxt,"module:",7) == 0)
   {
   if (pass == 1)
      {
      CheckForModule(actiontxt,args);
      }
   return plugin;
   }

Verbose("No action matched %s\n",actiontxt);
return(non);
}


/*******************************************************************/

void SummarizeObjects()

{ struct cfObject *op;

 Verbose("\n\n++++++++++++++++++++++++++++++++++++++++\n");
 Verbose("Summary of objects involved\n");
 Verbose("++++++++++++++++++++++++++++++++++++++++\n\n");
 
for (op = VOBJ; op != NULL; op=op->next)
   {
   Verbose("    %s\n",op->scope);
   }
}


/*******************************************************************/
/* Level 2                                                         */
/*******************************************************************/

void CheckOpts(int argc,char **argv)

{ extern char *optarg;
  struct Item *actionList;
  int optindex = 0;
  int c;

while ((c=getopt_long(argc,argv,"sbvf:",OPTIONS,&optindex)) != EOF)
  {
  switch ((char) c)
      {
      case 'f':
          strncpy(VINPUTFILE,optarg, CF_BUFSIZE-1);
          VINPUTFILE[CF_BUFSIZE-1] = '\0';
          MINUSF = true;
          break;
          
      case 'v':
          VARIABLES = true;
          break;

      case 'b':
          BUNDLES = true;
          break;

      case 's':
          SERVER = true;
          break;
          
      default:  Syntax();
          exit(1);
          
      }
  }
}


/*******************************************************************/

int GetResource(char *var)

{ int i;

for (i = 0; VRESOURCES[i] != '\0'; i++)
   {
   if (strcmp(VRESOURCES[i],var)==0)
      {
      return i;
      }
   }

snprintf (OUTPUT,CF_BUFSIZE,"Unknown resource %s in %s",var,VRCFILE);
FatalError(OUTPUT);
return 0;
}


/*******************************************************************/

void Syntax()

{ int i;

printf("Cfengine Conversion Utility\n%s\n%s\n",VERSION,COPYRIGHT);
printf("\n");
printf("Options:\n\n");

for (i=0; OPTIONS[i].name != NULL; i++)
   {
   printf("--%-20s    (-%c)\n",OPTIONS[i].name,(char)OPTIONS[i].val);
   }

printf("\nDebug levels: 1=parsing, 2=running, 3=summary, 4=expression eval\n");

printf("\nBug reports to bug-cfengine@cfengine.org\n");
printf("General help to help-cfengine@cfengine.org\n");
printf("Info & fixes at http://www.cfengine.org\n");
}
/* EOF */

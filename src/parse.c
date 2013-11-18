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
 

/*******************************************************************/
/*                                                                 */
/*  Parse Zone for cfengine                                        */
/*                                                                 */
/*  This is wide screen entertainment. Resize                      */
/*  your window before viewing!                                    */
/*                                                                 */
/*  The routines here are called by the lexer and the yacc parser  */
/*                                                                 */
/*******************************************************************/

#define INET

#include <stdio.h>
#include "cf.defs.h"
#include "cf.extern.h"

extern FILE *yyin;


/*******************************************************************/

int ParseInputFile(char *file,int audit)

{ char filename[CF_BUFSIZE], *sp;
  struct Item *ptr;
  struct stat s;
  struct Item *done = NULL;
   
Debug("Parse(%s)\n",file);
  
NewParser();
PARSING = true;

if (strcmp(file,"-") == 0)
   {
   Debug("(BEGIN PARSING STDIN)\n");
   ParseStdin();
   }
else
   {
   sp = FindInputFile(filename,file);
   
   Debug("(BEGIN PARSING %s)\n",filename); 
   Verbose("Looking for an input file %s\n",filename);
   
   if (stat(filename,&s) == -1)
      {
      Verbose("(No file %s)\n",filename);
      Debug("(END OF PARSING %s)\n",filename);
      Verbose("Finished with %s\n",filename);
      PARSING = false;
      DeleteParser();
      return false;
      }
   
   ParseFile(filename,sp,audit);
   }

if (!BUNDLES)
   {
   for (ptr = VIMPORT; ptr != NULL; ptr=ptr->next)
      {
      Debug("(BEGIN PARSING %s)\n",ptr->name); 
      Verbose("Looking for an input file %s\n",ptr->name);
      
      sp = FindInputFile(filename,ptr->name);
      
      if (!IsItemIn(done,filename))
         {
         ParseFile(filename,sp,audit);
         PrependItem(&done,filename,NULL);
         }
      }
   }

DeleteItemList(done);

PARSING = false;
DeleteParser();

Debug("(END OF PARSING %s)\n",file);
Verbose("Finished with %s\n\n",file); 
 
  
return true; 
}

/*******************************************************************/

void NewParser()

{
 Debug("New Parser Object::");
 FINDERTYPE = (char *) malloc(CF_BUFSIZE);
 strncpy(FINDERTYPE, "*", CF_BUFSIZE); /* "*" = no findertype set */
 VUIDNAME = (char *) malloc(CF_BUFSIZE);
 VGIDNAME = (char *) malloc(CF_BUFSIZE);
 FILTERNAME = (char *) malloc(CF_BUFSIZE);
 STRATEGYNAME = (char *) malloc(CF_BUFSIZE);
 CURRENTITEM = (char *) malloc(CF_BUFSIZE);
 GROUPBUFF = (char *) malloc(CF_BUFSIZE);
 ACTIONBUFF = (char *) malloc(CF_BUFSIZE);
 CURRENTOBJECT = (char *) malloc(CF_BUFSIZE);
 CURRENTAUTHPATH = (char *) malloc(CF_BUFSIZE);
 CLASSBUFF = (char *) malloc(CF_BUFSIZE);
 LINKFROM = (char *) malloc(CF_BUFSIZE);
 LINKTO = (char *) malloc(CF_BUFSIZE);
 ERROR = (char *) malloc(CF_BUFSIZE);
 EXPR = (char *) malloc(CF_BUFSIZE);
 MOUNTFROM = (char *) malloc(CF_BUFSIZE);
 MOUNTONTO = (char *) malloc(CF_BUFSIZE);
 *MOUNTFROM = '\0';
 *MOUNTONTO = '\0';
 MOUNTOPTS = (char *) malloc(CF_BUFSIZE);

 PKGVER = (char *) malloc(CF_BUFSIZE);
 PKGVER[0] = '\0';

 DESTINATION = (char *) malloc(CF_BUFSIZE);

 IMAGEACTION = (char *) malloc(CF_BUFSIZE);
 PARSEMETHODRETURNCLASSES = (char *) malloc(CF_BUFSIZE);

 CHDIR = (char *) malloc(CF_BUFSIZE);
 RESTART = (char *) malloc(CF_BUFSIZE);
 LOCALREPOS = (char *) malloc(CF_BUFSIZE);
 FILTERDATA = (char *) malloc(CF_BUFSIZE);
 STRATEGYDATA = (char *) malloc(CF_BUFSIZE);

 memset(CLASSBUFF,0,CF_BUFSIZE);
}

/*******************************************************************/

void DeleteParser()

{ Debug("Delete Parser Object::");

free(PARSEMETHODRETURNCLASSES);
free(FINDERTYPE);
free(VUIDNAME);
free(VGIDNAME);
free(FILTERNAME);
free(STRATEGYNAME);
free(CURRENTITEM);
free(GROUPBUFF);
free(ACTIONBUFF);
free(CURRENTOBJECT);
free(CURRENTAUTHPATH);
free(CLASSBUFF);
free(LINKFROM);
free(LINKTO);
free(ERROR);
free(EXPR);
free(RESTART); 
free(MOUNTFROM);
free(MOUNTONTO);
free(MOUNTOPTS);
free(DESTINATION);
free(IMAGEACTION);
free(PKGVER);
free(CHDIR);
free(LOCALREPOS);
free(FILTERDATA);
free(STRATEGYDATA);
}

/*******************************************************************/

int RemoveEscapeSequences(char *from,char *to)

{ char *sp,*cp;
  char start = *from;
  int len = strlen(from);

if (len == 0)
   {
   return 0;
   }

 for (sp=from+1,cp=to; (sp-from) < len; sp++,cp++)
    {
    if ((*sp == start))
       {
       *(cp) = '\0';
       if (*(sp+1) != '\0')
          {
          return (2+(sp - from));
          }
       return 0;
       }
    if (*sp == '\n')
       {
       LINENUMBER++;
       }
    *cp = *sp;    
    }
 
 yyerror("Runaway string");
 *(cp) = '\0';
 return 0;
}

/*******************************************************************/

void SetAction (enum actions action) 

{
InstallPending(ACTION);   /* Flush any existing actions */

SetStrategies(); 

Debug1("\n\n==============================BEGIN NEW ACTION %s=============\n\n",ACTIONTEXT[action]);

ACTION = action;
strcpy(ACTIONBUFF,ACTIONTEXT[action]);

switch (ACTION)
   {
   case interfaces:
   case files:
   case makepath:
   case tidy:
   case disable:
   case rename_disable:
   case filters:
   case strategies:     
   case image:
   case links:
   case required:
   case disks:
   case shellcommands:
   case scli:
   case alerts:
   case unmounta:
   case admit:
   case deny:
   case methods:
   case processes:  InitializeAction();
   }

Debug1("\nResetting CLASS to ANY\n\n"); 
strcpy(CLASSBUFF,CF_ANYCLASS);    /* default class */
}

/*******************************************************************/

void HandleLValue(char *id)     /* Record name of a variable assignment */

{
Debug1("HandleLVALUE(%s) in action %s\n",id,ACTIONTEXT[ACTION]);

 switch(ACTION)   /* Check for IP names in cfd */
    {
    case control:
        
        if ((CONTROLVAR = ScanVariable(id)) != nonexistentvar)
           {
           strcpy(CURRENTITEM,id);
           return;
           }
        else
           {
//           if (IsDefinedClass(CLASSBUFF))
//              {
              RecordMacroId(id);
//              }
           return;
           }
        break;
        
    case groups:
        { int count = 1;
        char *cid = id;
        
        while (*cid != '\0')
           {
           if (*cid++ == '.')
              {
              count++;
              }
           }
        
        if (strcmp(id,CF_ANYCLASS) == 0)
           {
           yyerror("Reserved class <any>");
           }
        
        if (count > 1)                              /* compound class */
           {
           yyerror("Group with compound identifier");
           FatalError("Dots [.] not allowed in group identifiers");
           }
        
        if (IsHardClass(id))
           {
           yyerror("Reserved class name (choose a different name)");
           }
        
        strcpy(GROUPBUFF,id);
        PrependLoL(&WORKLIST,id);
        }
        
    break;
    
    }
}


/*******************************************************************/

void HandleBraceObjectID(char *id)   /* Record identifier for braced object */

{
Debug1("HandleBraceObjectID(%s) in action %s\n",id,ACTIONTEXT[ACTION]);

switch (ACTION)
   {
   case acls:
       strcpy(CURRENTOBJECT,id);
       InstallACL(id,CLASSBUFF);
       break;
       
   case strategies:
       if (strlen(STRATEGYNAME) == 0)
          {
          strcpy(STRATEGYNAME,id);
          InstallStrategy(id,CLASSBUFF);
          }
       else
          {
          yyerror("Multiple identifiers or forgotten quotes in strategy");
          }
       break;
       
   case editfiles:
       if (strlen(CURRENTOBJECT) == 0)
          {
          strcpy(CURRENTOBJECT,id);
          EDITGROUPLEVEL = 0;
          FOREACHLEVEL = 0;
          SEARCHREPLACELEVEL = 0;
          }
       else
          {
          yyerror("Multiple filenames in editfiles");
          }
       break; 
       
   case filters:
       if (strlen(FILTERNAME)==0)
          {
          strcpy(FILTERNAME,id);
          InstallFilter(id);
          }
       else
          {
          yyerror("Multiple identifiers in filter");
          }
       break;
   }
}

/*******************************************************************/

void HandleBraceObjectClassifier(char *id)      /* Record LHS item in braced object */

{
Debug1("HandleClassifier(%s) in action %s\n",id,ACTIONTEXT[ACTION]);

switch (ACTION)
   {
   case acls:
       AddACE(CURRENTOBJECT,id,CLASSBUFF);
       break;

   case filters:
   case strategies:
                 strcpy(CURRENTITEM,id);
   break;

   case editfiles:
                 if (strcmp(id,"EndGroup") == 0 ||
                     strcmp(id,"GotoLastLine") == 0 ||
                     strcmp(id,"AutoCreate") == 0  ||
                     strcmp(id,"WarnIfFileMissing") == 0  ||
                     strcmp(id,"EndLoop") == 0  ||
                     strcmp(id,"CatchAbort") == 0  ||
                     strcmp(id,"ExpandVariables") == 0  ||
                     strcmp(id,"EmptyEntireFilePlease") == 0)
                        {
                        HandleEdit(CURRENTOBJECT,id,NULL);
                        }

   strcpy(CURRENTITEM,id);
       break;
   }
}

/*******************************************************************/

void HandleClass (char *id)

{ int members;
 char *sp;

 Debug("HandleClass(%s)\n",id);

 for (sp = id; *sp != '\0'; sp++)
   {
   switch (*sp)
      {
      case '-':
      case '*':
          snprintf(OUTPUT,CF_BUFSIZE,"Illegal character (%c) in class %s",*sp,id);
          yyerror(OUTPUT);
      }
   }
 
InstallPending(ACTION);

if ((members = CompoundId(id)) > 1)             /* Parse compound id */
   {
   Debug1("Compound class = (%s) with %d members\n",id,members);
   }
else
   {
   Debug1("Simple class = (%s)\n",id);
   }

strcpy(CLASSBUFF,id); 
}

/*******************************************************************/

void HandleQuotedString(char *qstring)

{
Debug1("HandleQuotedString %s\n",qstring);

switch (ACTION)
    {
    case editfiles: 
        HandleEdit(CURRENTOBJECT,CURRENTITEM,qstring);
        break;
        
    case filters:
        strcpy(FILTERDATA,qstring);
        ACTIONPENDING = true;
        InstallPending(ACTION); 
        break;
        
    case strategies:
        strcpy(STRATEGYDATA,qstring);
        ACTIONPENDING = true;
        InstallPending(ACTION); 
        break;
        
    case processes: /* Handle anomalous syntax of restart/setoptonstring */
                
        if (strcmp(CURRENTOBJECT,"SetOptionString") == 0)
           {    
           if (HAVE_RESTART)
              {
              yyerror("Processes syntax error");
              }
           strcpy(RESTART,qstring);
           ACTIONPENDING = true;
           InstallPending(ACTION);
           
           return;
           }
        
        if (strlen(RESTART) > 0 || (!HAVE_RESTART && strlen(EXPR) > 0)) /* Any string must be new rule */
           {
           if (!ACTIONPENDING)
              {
              yyerror("Insufficient or incomplete processes statement");
              }
           InstallPending(ACTION);
           InitializeAction();
           }
        
        if (EXPR[0] == '\0')
           {
           if (HAVE_RESTART)
              {
              yyerror("Missing process search expression");
              }
           Debug1("Installing expression %s\n",qstring);
           strcpy(EXPR,qstring);
           HAVE_RESTART = false;
           }
        else if (HAVE_RESTART)
           {
           Debug1("Installing restart expression\n");
           strncpy(RESTART,qstring,CF_BUFSIZE-1);
           ACTIONPENDING = true;
           } 
        break;
        
    case interfaces:
        strncpy(VIFNAME,qstring,15);
        break;
        
    default:
        InstallPending(ACTION); 
        strncpy(CURRENTOBJECT,qstring,CF_BUFSIZE-1);
        ACTIONPENDING = true;
        return;
    } 
}

/*******************************************************************/

void HandleGroupRValue(char *rval)        /* Assignment in groups/classes */

{ 
Debug1("\nHandleGroupRvalue(%s)\n",rval);

if (ACTION == control)
   {
   if (CountParentheses(rval) > 0)
      {
      yyerror("Variables in function with $() rather than ${} - bug workaround");
      }
   
   if (NestedParentheses(rval) > 1)
      {
      yyerror("Variables in function with $() rather than ${} - bug workaround");
      }
   }

AddToLoL(&WORKLIST,GROUPBUFF,rval);
}

/***************************************************************************/

void HandleFunctionObject(char *fn) /* Function in main body */

{ char local[CF_BUFSIZE];

if (ACTION == methods)
   {
   strncpy(CURRENTOBJECT,fn,CF_BUFSIZE-1);
   ACTIONPENDING = true; 
   return;
   }

/*if ((ACTION != alerts) && (!IsDefinedClass(CLASSBUFF)))
   {
   Debug("Function object class was beyond current environment (%s)\n",CLASSBUFF);
   return;
   }
*/

if (IsBuiltinFunction(fn))
   {
   local[0] = '\0';
   strcpy(local,fn);
   
   switch (ACTION)
      {
      case groups:
          AddToLoL(&WORKLIST,GROUPBUFF,fn);
          break;
          
      case control:          

          printf(" # NB - check var \"%s\" this HandleFunctionObject(%s)\n",CURRENTITEM,fn); 
          InstallControlRValue(CURRENTITEM,local);
          break;
          
      case alerts:
          
          if (strcmp(local,"noinstall") != 0)
             {
             Debug("Recognized function %s\n",fn);
             strncpy(CURRENTOBJECT,fn,CF_BUFSIZE-1);
             ACTIONPENDING = true;
             }
          else
             {
             Debug("Not recognized %s\n",fn);
             }
          break;
          
      default: snprintf(OUTPUT,CF_BUFSIZE,"Function call %s out of place",fn);
          yyerror(OUTPUT);
      }
   }
}

/***************************************************************************/

void HandleVarObject(char *object)

{
Debug1("Handling Object = (%s)\n",object);

strncpy(CURRENTOBJECT,object,CF_BUFSIZE-1);        /* Yes this must be here */

if (object[strlen(object)-1] == ':')
   {
   yyerror("There seems to be an action or class syntax error");
   }

ACTIONPENDING = true;                         /* we're parsing an action */

if (ACTION_IS_LINK || ACTION_IS_LINKCHILDREN)      /* to-link (after ->) */
   {
   strncpy(LINKTO,CURRENTOBJECT,CF_BUFSIZE-1);
   return;
   }

switch (ACTION)
   {
   case control:

       switch (CONTROLVAR)
          {
          case cfmountpat:
              SetMountPath(object);
              break;
              
          case cfrepos:
              SetRepository(object);
              break;
              
          case cfrepchar:
              if (strlen(object) > 1)
                 {
                 yyerror("reposchar can only be a single letter");
                 }
              if (object[0] == '/')
                 {
                 yyerror("illegal value for reposchar");
                 }
              REPOSCHAR = object[0];
              break;
              
          case cflistsep:
              if (strlen(object) > 1)
                 {
                 yyerror("listseparator can only be a single letter");
                 }
              if (object[0] == '/')
                 {
                 yyerror("illegal value for listseparator");
                 }
              LISTSEPARATOR = object[0];
              break;
              
          case cfhomepat:
              yyerror("Path relative to mountpath required");
              FatalError("Absolute path was specified\n");
              break;
              
          case nonexistentvar:
              
              AddMacroValue(CONTEXTID,CURRENTITEM,object);

              break;
          }
       break;
       
   case import:
       AppendImport(object); /* Recursion */
       break;
       
   case links:      /* from link (see cf.l) */
       break;
       
   case filters:
       if (strlen(FILTERNAME) == 0)
          {
          yyerror("empty or broken filter");
          }
       else
          {
          strncpy(CURRENTITEM,object,CF_BUFSIZE-1);
          ACTIONPENDING = true;
          }
       break;
       
   case disks:
       
   case required:   strncpy(CURRENTOBJECT,object,CF_BUFSIZE-1);
       break;
       
   case shellcommands:
   case scli:
       break;
       
       /* HvB : Bas van der Vlies */
   case mountables: 
       break;
       
   case defaultroute:InstallDefaultRouteItem(object);
       break;
       
   case resolve: 
       break;
       
   case broadcast:   InstallBroadcastItem(object);
       break;

   case mailserver: InstallMailserverPath(object);
       break;
   case tidy:       strncpy(CURRENTITEM,object,CF_BUFSIZE-1);
       break;

   case rename_disable:
   case disable:    strncpy(CURRENTOBJECT,object,CF_BUFSIZE-1);
                    ACTIONPENDING = true;
                    break;
      
   case makepath:   strncpy(CURRENTOBJECT,object,CF_BUFSIZE-1);
                    break;
      
   case ignore:     strncpy(CURRENTOBJECT,object,CF_BUFSIZE-1);
                    break;
       
   case misc_mounts:
       if (! MOUNT_FROM)
          {
          MOUNT_FROM = true;
          strncpy(MOUNTFROM,CURRENTOBJECT,CF_BUFSIZE-1);
          }
       else
          {
          if (MOUNT_ONTO)
             {
             yyerror ("Path not expected");
             FatalError("miscmounts: syntax error");
             }
          MOUNT_ONTO = true;
          strncpy(MOUNTONTO,CURRENTOBJECT,CF_BUFSIZE-1);
          }
       break;
       
   case unmounta:
       strncpy(CURRENTOBJECT,object,CF_BUFSIZE-1);
       break;
       
   case image:
   case files:      
       break;
       
   case editfiles:  /* file recorded in CURRENTOBJECT */
       strncpy(CURRENTOBJECT,object,CF_BUFSIZE-1);
       break;
       
   case processes:
       if (strcmp(ToLowerStr(object),"restart") == 0)
          {
          if (HAVE_RESTART)
             {
             yyerror("Repeated Restart option in processes");
             }
          HAVE_RESTART = true;
          Debug1("Found restart directive\n");
          }
       else if (strcmp(ToLowerStr(object),"setoptionstring") == 0)
          {
          InstallPending(ACTION);
          InitializeAction();
          Debug1("\nFound SetOptionString\n");
          strcpy(CURRENTOBJECT,"SetOptionString");
          strcpy(EXPR,"SetOptionString");
          }
       else if (HAVE_RESTART)
          {
          Debug1("Installing restart expression\n");
          strncpy(RESTART,object,CF_BUFSIZE-1);
          ACTIONPENDING = true;
          }
       else
          {
          Debug1("Dropped %s :(\n",object);
          }
       break;
       
   case binservers: InstallBinserverItem(object);
       break;
       
   case homeservers: InstallHomeserverItem(object);
       break;

   case packages:
       break;

   case methods:    if (strlen(object) > CF_BUFSIZE-1)
                       {
         yyerror("Method argument string is too long");
                       }
                    break;
       
   default:         yyerror("Unknown command or name out of context");
   }
}

/*******************************************************************/

void HandleServerRule(char *object)

{ char buffer[CF_EXPANDSIZE];

//ExpandVarstring(object,buffer,"");
 strcpy(buffer,object);
Debug("HandleServerRule(%s=%s)\n",object,buffer);

 if (*buffer == '/')
    {
    Debug("\n\nNew admit/deny object=%s\n",buffer);
    strcpy(CURRENTAUTHPATH,object);
    }
 else
    {
    switch(ACTION)   /* Check for IP names in cfservd */
       {
       case admit:
           FuzzyMatchParse(buffer);       
           InstallAuthItem(CURRENTAUTHPATH,object,&VADMIT,&VADMITTOP,CLASSBUFF);
           break;
       case deny:
           FuzzyMatchParse(buffer);       
           InstallAuthItem(CURRENTAUTHPATH,object,&VDENY,&VDENYTOP,CLASSBUFF);
           break;
       }
    }
}

/*******************************************************************/

void HandleOption(char *option)

{
Debug("HandleOption(%s)\n",option);
 
switch (ACTION)
   {   
   case files:
                 HandleOptionalFileAttribute(option);
   break;
   case image:
                 HandleOptionalImageAttribute(option);
   break;
   case tidy:
                 HandleOptionalTidyAttribute(option);
                 break;
   
   case makepath:
                 HandleOptionalDirAttribute(option);
   break;

   case disable:
   case rename_disable:
                 HandleOptionalDisableAttribute(option);
                 break;
   case links:
                 HandleOptionalLinkAttribute(option);
                 break;
   case processes:
                 HandleOptionalProcessAttribute(option);
                 break;

   case misc_mounts:

             HandleOptionalMiscMountsAttribute(option);
   break;

   case mountables:
                 HandleOptionalMountablesAttribute(option);
   break;

   case unmounta:
                 HandleOptionalUnMountAttribute(option);
   break;
   
   case shellcommands:
   case scli:
                 HandleOptionalScriptAttribute(option);
   break;

   case alerts:  HandleOptionalAlertsAttribute(option);
                 break;

   case disks:
   case required:
                 HandleOptionalRequired(option);
   break;

   case interfaces:
                 HandleOptionalInterface(option);
                 if (strlen(VIFNAME) && strlen(CURRENTOBJECT) && strlen(DESTINATION))
                    {
                    ACTIONPENDING = true;
                    }
                 break;
                 
   case admit:
                 InstallAuthItem(CURRENTAUTHPATH,option,&VADMIT,&VADMITTOP,CLASSBUFF);
   break;
   case deny:
                 InstallAuthItem(CURRENTAUTHPATH,option,&VDENY,&VDENYTOP,CLASSBUFF);
   break;

   case import:  /* Need option for private modules... */
                 break;

   case packages:
                 HandleOptionalPackagesAttribute(option);
   break;

   case methods: HandleOptionalMethodsAttribute(option);
                 break;

   default:
                 yyerror("Options cannot be used in this context:");
   } 
}


/*******************************************************************/
/* Level 2                                                         */
/*******************************************************************/

void ParseFile(char *filename,char *env,int audit)

{ FILE *save_yyin = yyin;
 
signal(SIGALRM,(void *)TimeOut);
alarm(RPCTIMEOUT);

if (!FileSecure(filename))
   {
   FatalError("Security exception");
   }
 
if ((yyin = fopen(filename,"r")) == NULL)      /* Open root file */
   {
   printf("%s: Can't open file %s\n",VPREFIX,filename);

   if (env == NULL)
      {
      printf("%s: (%s is set to <nothing>)\n",VPREFIX,CF_INPUTSVAR);
      }
   else
      {
      printf("%s: (%s is set to %s)\n",VPREFIX,CF_INPUTSVAR,env);
      }
   
   exit (1);
   }
 
strcpy(VCURRENTFILE,filename);

Debug("\n##########################################################################\n"); 
Debug("# BEGIN PARSING %s\n",VCURRENTFILE);
Debug("##########################################################################\n\n"); 
 
LINENUMBER = 1;

if (audit)
   {
   PrependAuditFile(filename);
   }

while (!feof(yyin))
   { 
   yyparse();
   
   if (ferror(yyin))  /* abortable */
      {
      printf("%s: Error reading %s\n",VPREFIX,VCURRENTFILE);
      perror("cfengine");
      exit(1);
      }
   }

if (audit)
   {
   VersionAuditFile();
   }

fclose (yyin);
yyin = save_yyin;
 
alarm(0);
signal(SIGALRM,SIG_DFL);
InstallPending(ACTION);
}

/*******************************************************************/

void ParseStdin()

{
yyin = stdin;
 
Debug("\n##########################################################################\n"); 
Debug("# BEGIN PARSING stdin\n");
Debug("##########################################################################\n\n"); 
 
LINENUMBER=1;
 
while (!feof(yyin))
   { 
   yyparse();
   
   if (ferror(yyin))  /* abortable */
      {
      printf("%s: Error reading %s\n",VPREFIX,VCURRENTFILE);
      perror("cfengine");
      exit(1);
      }
   }
 
InstallPending(ACTION);
}

/*******************************************************************/

int CompoundId(char *id)                   /* check for dots in the name */

{ int count = 1;
  char *cid = id;

for (cid = id; *cid != '\0'; cid++)
   {
   if (*cid == '.' || *cid == '|')
      {
      count++;
      }
   }

return(count);
}


/*******************************************************************/

void RecordMacroId(char *name)

{
Debug("RecordMacroId(%s)\n",name);
strcpy(CURRENTITEM,name); 

if (strcmp(name,"this") == 0)
   {
   yyerror("$(this) is a reserved variable");
   }
}

/*******************************************************************/
/* Level 2                                                         */
/*******************************************************************/

char *FindInputFile(char *result,char *filename)

{ char *sp;
 
result[0] = '\0';
 
if (MINUSF && (*filename == '.' || *filename == '/'))
   {
   Verbose("Manually overidden relative path (%s)\n",filename);
   strncpy(result,filename,CF_BUFSIZE-1);
   return result;
   }
 
if ((sp=getenv(CF_INPUTSVAR)) != NULL)
   {
   if (!IsAbsoluteFileName(filename))     /* Don't prepend to absolute names */
      { 
      strcpy(result,sp);
      
      if (! IsAbsoluteFileName(result))
         {
         Verbose("CFINPUTS was not an absolute path, overriding with %s\n",CFWORKDIR);
         snprintf(result,CF_BUFSIZE,"%s/inputs",CFWORKDIR);
         }
      
      AddSlash(result);
      }
   }
 else
    {
    if (!IsAbsoluteFileName(filename))     /* Don't prepend to absolute names */
       { 
       strcpy(result,CFWORKDIR);
       AddSlash(result);
       strcat(result,"inputs/");
       }
    }
 
strcat(result,filename);
return result;
}


/*******************************************************************/
/* Toolkits Misc                                                   */
/*******************************************************************/

void InitializeAction()                                   /* Set defaults */

 {
 Debug1("InitializeAction()\n");
 
 ACTIONPENDING = false;

 PLUSMASK = (mode_t)0;
 MINUSMASK = (mode_t)0;
 PLUSFLAG = (u_long)0;
 MINUSFLAG = (u_long)0;
 VRECURSE = 0;
 HAVE_RESTART = false;
 VAGE = 99999;
 strncpy(FINDERTYPE,"*",CF_BUFSIZE);
 strcpy(VUIDNAME,"*");
 strcpy(VGIDNAME,"*");
 HAVE_RESTART = 0;
 FILEACTION=warnall;
 PIFELAPSED=-1;
 PEXPIREAFTER=-1;

 *CURRENTAUTHPATH = '\0';
 *CURRENTOBJECT = '\0';
 *CURRENTITEM = '\0';
 *DESTINATION = '\0';
 *IMAGEACTION = '\0';
 *LOCALREPOS = '\0';
 *EXPR = '\0';
 *RESTART = '\0';
 *FILTERDATA = '\0';
 *STRATEGYDATA = '\0';
 *CHDIR ='\0';
  *THISMODE ='\0';
 METHODFILENAME[0] = '\0';
 PARSEMETHODRETURNCLASSES[0] = '\0';
 METHODFORCE[0] = '\0';
 CHROOT[0] = '\0';
 strcpy(VIFNAME,"");
 PTRAVLINKS = (short) '?';
 IMAGEBACKUP = 'y';
 ENCRYPT = 'n';
 VERIFY = 'n';
 ROTATE=0;
 TIDYSIZE=0;
 PROMATCHES=-1;
 PROCOMP = '!';
 PROACTION = 'd';
 PROSIGNAL=0;
 COMPRESS='n';
 AGETYPE='a';
 COPYTYPE = DEFAULTCOPYTYPE; /* 't' */
 LINKDIRS = 'k';
 USESHELL = 'y';
 LOGP = 'd';
 INFORMP = 'd';
 PURGE = 'n';
 TRUSTKEY = 'n';
 NOABSPATH = 'n';
 CHKROOT = 'n';
 SCAN = 'n';
 CHECKSUM = 'n';
 TIDYDIRS = 'n';
 VEXCLUDEPARSE = NULL;
 VINCLUDEPARSE = NULL;
 VIGNOREPARSE = NULL;
 VACLBUILD = NULL;
 VFILTERBUILD = NULL;
 VSTRATEGYBUILD = NULL;
 VCPLNPARSE = NULL;
 VTIMEOUT=0;

 PKGMGR = DEFAULTPKGMGR; /* pkgmgr_none */
 PKGACTION = pkgaction_none;
 CMPSENSE = cmpsense_eq;
 PKGVER[0] = '\0';

 STRATEGYNAME[0] = '\0';
 FILTERNAME[0] = '\0';
 memset(ALLCLASSBUFFER,0,CF_BUFSIZE);
 memset(DEFINECLASSBUFFER,0,CF_BUFSIZE);
 memset(ELSECLASSBUFFER,0,CF_BUFSIZE);
 
 strcpy(CFSERVER,"localhost");
 
 IMGCOMP = DISCOMP='>';
 IMGSIZE = DISABLESIZE=CF_NOSIZE;
 DELETEDIR = 'y';   /* t=true */
 DELETEFSTAB = 'y';
 FORCE = 'n';
 FORCEDIRS = 'n';
 STEALTH = 'n';
 XDEV = 'n';
 PRESERVETIMES = 'n';
 TYPECHECK = 'y';
 UMASK = 077;     /* Default umask for scripts/files */
 FORK = 'n';
 PREVIEW = 'n';
 COMPATIBILITY = 'n';

 if (MOUNT_FROM && MOUNT_ONTO)
    {
    Debug("Resetting miscmount data\n");
    MOUNT_FROM = false;
    MOUNT_ONTO = false;
    MOUNTMODE='w';
    *MOUNTFROM = '\0';
    *MOUNTONTO = '\0';
    }

 /* 
  * HvB: Bas van der Vlies
 */
 CF_MOUNT_RO=false;
 MOUNTOPTS[0]='\0';
 
 /* Make sure we don't clean the buffer in the middle of a link! */

 if ( ! ACTION_IS_LINK && ! ACTION_IS_LINKCHILDREN)
    {
    memset(LINKFROM,0,CF_BUFSIZE);
    memset(LINKTO,0,CF_BUFSIZE);  /* ALSO RESTART */
    LINKSILENT = false;
    LINKTYPE = 's';
    FORCELINK = 'n';
    DEADLINKS = false;
    }
 }

/*********************************************************************/

void SetMountPath (char *value)

{ char buff[CF_EXPANDSIZE];

//ExpandVarstring(value,buff,"");
 
Debug("Appending [%s] to mountlist\n",buff);
 
AppendItem(&VMOUNTLIST,value,CLASSBUFF);
}

/*********************************************************************/

void SetRepository (char *value)

{ char ebuff[CF_EXPANDSIZE];
 
if (*value != '/')
   {
   yyerror("File repository must be an absolute directory name");
   }
 
if (strlen(VREPOSITORY) != 0)
   {
   yyerror("Redefinition of system variable repository");
   }

//ExpandVarstring(value,ebuff,"");
VREPOSITORY = strdup(value);
}

/* EOF */

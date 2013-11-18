/* cfengine for GNU
 
        Copyright (C) 1995-
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
/* File: modules.c                                                           */
/*                                                                           */
/* Created: Sun Jan 25 13:27:33 2004                                         */
/*                                                                           */
/* Author:                                           >                       */
/*                                                                           */
/* Revision: $Id$                                                            */
/*                                                                           */
/* Description:                                                              */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

void CheckClass(char *class, char *source);

/*******************************************************************/

int CheckForModule(char *actiontxt,char *args)

{ struct stat statbuf;
  char line[CF_BUFSIZE],command[CF_EXPANDSIZE],name[CF_MAXVARSIZE],content[CF_BUFSIZE],ebuff[CF_EXPANDSIZE],*sp;
  FILE *pp;
  int print;

if (NOMODULES)
   {
   return false;
   }

if (*actiontxt == '/')
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Absolute module path (%s) should be named relative to the authorized module directory",actiontxt);
   CfLog(cferror,OUTPUT,"");
   }

if (GetMacroValue(CONTEXTID,"moduledirectory"))
   {
   ExpandVarstring("$(moduledirectory)",ebuff,NULL);
   }
else
   {
   snprintf(ebuff,CF_BUFSIZE,"%s/modules",VLOCKDIR);
   }

AddSlash(ebuff);
strcat(ebuff,actiontxt);
 
if (stat(ebuff,&statbuf) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"(Plug-in %s not found)",ebuff);
   Banner(OUTPUT);
   return false;
   }

if ((statbuf.st_uid != 0) && (statbuf.st_uid != getuid()))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Module %s was not owned by uid=%d executing cfagent\n",ebuff,getuid());
   CfLog(cferror,OUTPUT,"");
   return false;
   }
  
snprintf(OUTPUT,CF_BUFSIZE*2,"Plug-in `%s\'",actiontxt);
Banner(OUTPUT);

strcat(ebuff," ");

if (BufferOverflow(ebuff,args))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Culprit: class list for module (shouldn't happen)\n" );
   CfLog(cferror,OUTPUT,"");
   return false;
   }

strcat(ebuff,args); 
ExpandVarstring(ebuff,command,NULL); 
 
Verbose("Exec module [%s]\n",command); 
   
if ((pp = cfpopen(command,"r")) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open pipe from %s\n",actiontxt);
   CfLog(cferror,OUTPUT,"cfpopen");
   return false;
   }

while (!feof(pp))
   {
   if (ferror(pp))  /* abortable */
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Shell command pipe %s\n",actiontxt);
      CfLog(cferror,OUTPUT,"ferror");
      break;
      }
   
   ReadLine(line,CF_BUFSIZE,pp);

   if (strlen(line) > CF_BUFSIZE - 80)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Line from module %s is too long to be sensible\n",actiontxt);
      CfLog(cferror,OUTPUT,"");
      break;
      }
   
   if (ferror(pp))  /* abortable */
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Shell command pipe %s\n",actiontxt);
      CfLog(cferror,OUTPUT,"ferror");
      break;
      }  
   
   print = false;
   
   for (sp = line; *sp != '\0'; sp++)
      {
      if (! isspace((int)*sp))
         {
         print = true;
         break;
         }
      }
   
   switch (*line)
      {
      case '+':
          Verbose("Activated classes: %s\n",line+1);
          CheckClass(line+1,command);
          AddMultipleClasses(line+1);
          break;
      case '-':
          Verbose("Deactivated classes: %s\n",line+1);
          CheckClass(line+1,command);
          NegateCompoundClass(line+1,&VNEGHEAP);
          break;
      case '=':
          content[0] = '\0';
          sscanf(line+1,"%[^=]=%[^\n]",name,content);
          Verbose("Defined Macro: %s, Value: %s\n",name,content);
          AddMacroValue(CONTEXTID,name,content);
          break;
          
      default:
          if (print)
             {
             snprintf(OUTPUT,CF_BUFSIZE,"%s: %s\n",actiontxt,line);
             CfLog(cferror,OUTPUT,"");
             }
      }
   }

cfpclose(pp);
return true; 
}



/******************************************************************/

void CheckClass(char *name,char *source)

{ char *sp;

 for (sp = name; *sp != '\0'; sp++)
    {
    if (!isalnum((int)*sp) && (*sp != '_'))
       {
       snprintf(OUTPUT,CF_BUFSIZE,"Module class contained an illegal character (%c). Only alphanumeric or underscores in classes.",*sp);
       CfLog(cferror,OUTPUT,"");
       }
    }
 
}

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
/*  Class string evaluation toolkit for cfengine                   */
/*                                                                 */
/*  Dependency: item.c toolkit                                     */
/*                                                                 */
/*******************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"


/*********************************************************************/
/* Object variables                                                  */
/*********************************************************************/

char *DAYTEXT[] =
   {
   "Monday",
   "Tuesday",
   "Wednesday",
   "Thursday",
   "Friday",
   "Saturday",
   "Sunday"
   };

char *MONTHTEXT[] =
   {
   "January",
   "February",
   "March",
   "April",
   "May",
   "June",
   "July",
   "August",
   "September",
   "October",
   "November",
   "December"
   };

/*********************************************************************/

int ShowClass(char *c1,char *c2)
    
{ regex_t rx,rxcache;
  regmatch_t pmatch;
  char buf[CF_BUFSIZE];
  int code;

if (c1 == NULL || c2 == NULL)
   {
   return true;
   }

code = regcomp(&rx,c1,REG_EXTENDED);
 
if (code != 0)
   {
   regerror(code,&rx,buf,CF_BUFSIZE-1);
   printf("Regular expression error %d for %s: %s\n", code,c1,buf);
   FatalError("Cannot use this for matching");
   }
else
   {
   if (regexec(&rx,c2,1,&pmatch,0) != 0)
      {
      return false;
      }
   }

return true;
}

/*********************************************************************/
/* Level 1                                                           */
/*********************************************************************/

int Day2Number(char *datestring)

{ int i = 0;

for (i = 0; i < 7; i++)
   {
   if (strncmp(datestring,DAYTEXT[i],3) == 0)
      {
      return i;
      }
   }

return -1;
}

/*********************************************************************/

int CountParentheses(char *str)

{ char *sp;
  int count = 0;

for (sp = str; *sp != '\0'; sp++)
   {
   if (*sp == '(')
      {
      count++;
      }

   if (*sp == ')')
      {
      count--;
      }
   }

return count;
}

/*********************************************************************/

int NestedParentheses(char *str)

{ char *sp;
  int count = 0;

for (sp = str; *sp != '\0'; sp++)
   {
   if (*sp == '(')
      {
      count++;
      }
   }

return count;
}

/*********************************************************************/

void AddInstallable(char *classlist)

{ char *sp, currentitem[CF_MAXVARSIZE];

if (classlist == NULL)
   {
   return;
   }

Debug("AddInstallable(%s)\n",classlist);
  
 for (sp = classlist; *sp != '\0'; sp++)
   {
   currentitem[0] = '\0';

   sscanf(sp,"%[^,:.]",currentitem);

   sp += strlen(currentitem);

   if (! IsItemIn(VALLADDCLASSES,currentitem))
      {
      AppendItem(&VALLADDCLASSES,currentitem,NULL);
      }

   if (*sp == '\0')
      {
      break;
      }
   }
}

/*********************************************************************/

void AddPrefixedMultipleClasses(char *name,char *classlist)

{ char *sp, currentitem[CF_MAXVARSIZE],local[CF_MAXVARSIZE],pref[CF_BUFSIZE];
 
if ((classlist == NULL) || strlen(classlist) == 0)
   {
   return;
   }

memset(local,0,CF_MAXVARSIZE);
strncpy(local,classlist,CF_MAXVARSIZE-1);

Debug("AddPrefixedMultipleClasses(%s,%s)\n",name,local);

for (sp = local; *sp != '\0'; sp++)
   {
   memset(currentitem,0,CF_MAXVARSIZE);

   sscanf(sp,"%250[^.:,]",currentitem);

   sp += strlen(currentitem);

   pref[0] = '\0';
   snprintf(pref,CF_BUFSIZE,"%s_%s",name,currentitem);

   
   if (IsHardClass(pref))
      {
      FatalError("cfengine: You cannot use -D to define a reserved class!");
      }

   AddClassToHeap(CanonifyName(pref));
   }
}

/*********************************************************************/

void AddMultipleClasses(char *classlist)

{ char *sp, currentitem[CF_MAXVARSIZE],local[CF_MAXVARSIZE];
 
if ((classlist == NULL) || strlen(classlist) == 0)
   {
   return;
   }

memset(local,0,CF_MAXVARSIZE);
strncpy(local,classlist,CF_MAXVARSIZE-1);

Debug("AddMultipleClasses(%s)\n",local);

for (sp = local; *sp != '\0'; sp++)
   {
   memset(currentitem,0,CF_MAXVARSIZE);

   sscanf(sp,"%250[^.:,]",currentitem);

   sp += strlen(currentitem);
      
   if (IsHardClass(currentitem))
      {
      FatalError("cfengine: You cannot use -D to define a reserved class!");
      }

   AddClassToHeap(CanonifyName(currentitem));
   }
}

/*********************************************************************/

void AddTimeClass(char *str)

{ int i;
  char buf2[10], buf3[10], buf4[10], buf5[10], buf[10], out[10];
  time_t now = time(NULL);
  struct tm *tmv = gmtime(&now);
  
snprintf(buf,9,"GMT_Hr%d\n",tmv->tm_hour);
AddClassToHeap(buf);

for (i = 0; i < 7; i++)
   {
   if (strncmp(DAYTEXT[i],str,3)==0)
      {
      AddClassToHeap(DAYTEXT[i]);
      break;
      }
   }

sscanf(str,"%*s %s %s %s %s",buf2,buf3,buf4,buf5);

/* Hours */

sscanf(buf4,"%[^:]",buf);
sprintf(out,"Hr%s",buf);
AddClassToHeap(out);
memset(VHR,0,3);
strncpy(VHR,buf,2); 

/* Minutes */

sscanf(buf4,"%*[^:]:%[^:]",buf);
sprintf(out,"Min%s",buf);
AddClassToHeap(out);
memset(VMINUTE,0,3);
strncpy(VMINUTE,buf,2); 
 
sscanf(buf,"%d",&i);

switch ((i / 5))
   {
   case 0: AddClassToHeap("Min00_05");
           break;
   case 1: AddClassToHeap("Min05_10");
           break;
   case 2: AddClassToHeap("Min10_15");
           break;
   case 3: AddClassToHeap("Min15_20");
           break;
   case 4: AddClassToHeap("Min20_25");
           break;
   case 5: AddClassToHeap("Min25_30");
           break;
   case 6: AddClassToHeap("Min30_35");
           break;
   case 7: AddClassToHeap("Min35_40");
           break;
   case 8: AddClassToHeap("Min40_45");
           break;
   case 9: AddClassToHeap("Min45_50");
           break;
   case 10: AddClassToHeap("Min50_55");
            break;
   case 11: AddClassToHeap("Min55_00");
            break;
   }

/* Add quarters */ 

switch ((i / 15))
   {
   case 0: AddClassToHeap("Q1");
           sprintf(out,"Hr%s_Q1",VHR);
    AddClassToHeap(out);
           break;
   case 1: AddClassToHeap("Q2");
           sprintf(out,"Hr%s_Q2",VHR);
    AddClassToHeap(out);
           break;
   case 2: AddClassToHeap("Q3");
           sprintf(out,"Hr%s_Q3",VHR);
    AddClassToHeap(out);
           break;
   case 3: AddClassToHeap("Q4");
           sprintf(out,"Hr%s_Q4",VHR);
    AddClassToHeap(out);
           break;
   }
 

/* Day */

sprintf(out,"Day%s",buf3);
AddClassToHeap(out);
memset(VDAY,0,3);
strncpy(VDAY,buf3,2);
 
/* Month */

for (i = 0; i < 12; i++)
   {
   if (strncmp(MONTHTEXT[i],buf2,3)==0)
      {
      AddClassToHeap(MONTHTEXT[i]);
      memset(VMONTH,0,4);
      strncpy(VMONTH,MONTHTEXT[i],3);
      break;
      }
   }

/* Year */

strcpy(VYEAR,buf5); 

sprintf(out,"Yr%s",buf5);
AddClassToHeap(out);
}

/*******************************************************************/

int Month2Number(char *string)

{ int i;

if (string == NULL)
   {
   return -1;
   }
 
for (i = 0; i < 12; i++)
   {
   if (strncmp(MONTHTEXT[i],string,strlen(string))==0)
      {
      return i+1;
      break;
      }
   }

return -1;
}

/*******************************************************************/

void AddClassToHeap(char *class)

{
Chop(class);
Debug("AddClassToHeap(%s)\n",class);

if (strlen(class) == 0)
   {
   return;
   }

if (IsItemIn(ABORTHEAP,class))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Cfagent aborted on defined class [%s]\n",class);
   CfLog(cferror,OUTPUT,"");
   exit(1);
   }

if (IsItemIn(VHEAP,class))
   {
   return;
   }

AppendItem(&VHEAP,class,CONTEXTID);
}

/*********************************************************************/

void DeleteClassFromHeap(char *class)

{
DeleteItemLiteral(&VHEAP,class);
}

/*********************************************************************/

void DeleteClassesFromContext(char *context)

{ struct Item *ip,*np;

Verbose("Purging private classes from context %s\n",context);
 
for (ip = VHEAP; ip != NULL; ip = np)
   {
   np = ip->next;
   
   if (strcmp(ip->classes,context) == 0)
      {
      Verbose(" - Deleting %s \n",ip->name);
      DeleteItem(&VHEAP,ip);
      }
   }
}

/*********************************************************************/

int IsHardClass(char *sp)  /* true if string matches a hardwired class e.g. hpux */

{ int i;

for (i = 2; CLASSTEXT[i] != '\0'; i++)
   {
   if (strcmp(CLASSTEXT[i],sp) == 0)
      {
      return(true);
      }
   }

for (i = 0; i < 7; i++)
   {
   if (strcmp(DAYTEXT[i],sp)==0)
      {
      return(false);
      }
   }

return(false);
}

/*******************************************************************/

int IsSpecialClass(char *class)

{ int value = -1;

if (strncmp(class,"IfElapsed",strlen("IfElapsed")) == 0)
   {
   sscanf(class,"IfElapsed%d",&value);

   if (value < 0)
      {
      Silent("%s: silly IfElapsed parameter in action sequence, using default...\n",VPREFIX);
      return true;
      }

   if (!PARSING)
      {
      VIFELAPSED = value;
      
      Verbose("                  IfElapsed time: %d minutes\n",VIFELAPSED);
      return true;
      }
   }

if (strncmp(class,"ExpireAfter",strlen("ExpireAfter")) == 0)
   {
   sscanf(class,"ExpireAfter%d",&value);

   if (value <= 0)
      {
      Silent("%s: silly ExpireAter parameter in action sequence, using default...\n",VPREFIX);
      return true;
      }

   if (!PARSING)
      {
      VEXPIREAFTER = value;
      Verbose("\n                  ExpireAfter time: %d minutes\n",VEXPIREAFTER); 
      return true;
      }
   }

return false;
}

/*********************************************************************/

int IsExcluded(char *exception)

{
if (!IsDefinedClass(exception))
   {
   Debug2("%s is excluded!\n",exception);
   return true;
   }  

return false;
}

/*********************************************************************/

int IsDefinedClass(char *class) 

  /* Evaluates a.b.c|d.e.f etc and returns true if the class */
  /* is currently true, given the defined heap and negations */

{ int ret;
Debug4("IsDefinedClass(%s,VADDCLASSES)\n",class);

if (CfShow())
   {
   return true;
   }

if (class == NULL)
   {
   return true;
   }

ret = EvaluateORString(class,VADDCLASSES,0);

return ret;
}


/*********************************************************************/

int IsInstallable(char *class)

  /* Evaluates to true if the class string COULD become true in */
  /* the course of the execution - but might not be true now    */

{ char buffer[CF_BUFSIZE], *sp;
 int i = 0, val;

Debug1("IsInstallable(%s) -",class);

if (CfShow())
   {
   return true;
   }

for (sp = class; *sp != '\0'; sp++)
   {
   if (*sp == '!')
      {
      continue;         /* some actions might be presented as !class */
      }
   buffer[i++] = *sp; 
   }

buffer[i] = '\0';
 
/* return (EvaluateORString(buffer,VALLADDCLASSES)||EvaluateORString(class,VADDCLASSES));*/

val = (EvaluateORString(buffer,VALLADDCLASSES,0)||EvaluateORString(class,VALLADDCLASSES,1)||EvaluateORString(class,VADDCLASSES,0));

if (val)
   {
   Debug1(" true\n");
   }
else
   {
   Debug1(" false\n");
   }

return val;
}


/*********************************************************************/

void NegateCompoundClass(char *class,struct Item **heap)

{ char *sp = class;
  char cbuff[CF_MAXVARSIZE];

Debug1("NegateCompoundClass(%s)",class);

while(*sp != '\0')
   {
   sscanf(sp,"%255[^.]",cbuff);

   while ((*sp != '\0') && ((*sp !='.')||(*sp == '&')))
      {
      sp++;
      }

   if ((*sp == '.') || (*sp == '&'))
      {
      sp++;
      }

   if (IsHardClass(cbuff))
      { char err[CF_BUFSIZE];
      yyerror("Illegal exception");
      sprintf (err,"Cannot negate the reserved class [%s]\n",cbuff);
      FatalError(err);
      }

   AppendItem(heap,cbuff,NULL);
   }
}

/*********************************************************************/
/* Level 2                                                           */
/*********************************************************************/

int EvaluateORString(char *class,struct Item *list,int fromIsInstallable)

{ char *sp, cbuff[CF_BUFSIZE];
  int result = false;

if (class == NULL)
   {
   return false;
   }

Debug4("\n--------\nEvaluateORString(%s)\n",class);
 
for (sp = class; *sp != '\0'; sp++)
   {
   while (*sp == '|')
      {
      sp++;
      }

   memset(cbuff,0,CF_BUFSIZE);

   sp += GetORAtom(sp,cbuff);

   if (strlen(cbuff) == 0)
      {
      break;
      }


   if (IsBracketed(cbuff)) /* Strip brackets */
      {
      cbuff[strlen(cbuff)-1] = '\0';

      result |= EvaluateORString(cbuff+1,list,fromIsInstallable);
      Debug4("EvalORString-temp-result-y=%d (%s)\n",result,cbuff+1);
      }
   else
      {
      result |= EvaluateANDString(cbuff,list,fromIsInstallable);
      Debug4("EvalORString-temp-result-n=%d (%s)\n",result,cbuff);
      }

   if (*sp == '\0')
      {
      break;
      }
   }

Debug4("EvaluateORString(%s) returns %d\n",class,result); 
return result;
}

/*********************************************************************/
/* Level 3                                                           */
/*********************************************************************/

int EvaluateANDString(char *class,struct Item *list,int fromIsInstallable)

{ char *sp, *atom;
  char cbuff[CF_BUFSIZE];
  int count = 1;
  int negation = false;

Debug4("EvaluateANDString(%s)\n",class);

count = CountEvalAtoms(class);
sp = class;
 
while(*sp != '\0')
   {
   negation = false;

   while (*sp == '!')
      {
      negation = !negation;
      sp++;
      }

   memset(cbuff,0,CF_BUFSIZE);

   sp += GetANDAtom(sp,cbuff) + 1;

   atom = cbuff;

     /* Test for parentheses */
   
   if (IsBracketed(cbuff))
      {
      atom = cbuff+1;

      Debug4("Checking AND Atom %s?\n",atom);
      
      cbuff[strlen(cbuff)-1] = '\0';
      
      if (EvaluateORString(atom,list,fromIsInstallable))
         {
         if (negation)
            {
            Debug4("EvalANDString-temp-result-neg1=false\n");
            return false;
            }
         else
            {
            Debug4("EvalORString-temp-result count=%d\n",count);
            count--;
            }
         }
      else
         {
         if (negation)
            {
            Debug4("EvalORString-temp-result2 count=%d\n",count);
            count--;
            }
         else
            {
            return false;
            }
         }

      continue;
      }
   else
      {
      atom = cbuff;
      }
   
   /* End of parenthesis check */
   
   if (*sp == '.' || *sp == '&')
      {
      sp++;
      }

   Debug4("Checking OR atom (%s)?\n",atom);

   CheckCommonErrors(atom);

   if (IsItemIn(VNEGHEAP,atom))
      {
      if (negation)
         {
         Debug4("EvalORString-temp-result3 count=%d\n",count);
         count--;
         }
      else
         {
         return false;
         }
      } 
   else if (IsItemIn(VHEAP,atom))
      {
      if (negation)
         {
         Debug4("EvaluateANDString(%s) returns false by negation 1\n",class);
         return false;
         }
      else
         {
         Debug4("EvalORString-temp-result3.5 count=%d\n",count);
         count--;
         }
      } 
   else if (IsItemIn(list,atom))
      {
      if (negation && !fromIsInstallable)
         {
         Debug4("EvaluateANDString(%s) returns false by negation 2\n",class);
         return false;
         }
      else
         {
         Debug4("EvalORString-temp-result3.6 count=%d\n",count);
         count--;
         }
      } 
   else if (negation)    /* ! (an undefined class) == true */
      {
      Debug4("EvalORString-temp-result4 count=%d\n",count);
      count--;
      }
   else       
      {
      Debug4("EvaluateANDString(%s) returns false ny negation 3\n",class);
      return false;
      }
   }

 
if (count == 0)
   {
   Debug4("EvaluateANDString(%s) returns true\n",class);
   return(true);
   }
else
   {
   Debug4("EvaluateANDString(%s) returns false\n",class);
   return(false);
   }
}

/*********************************************************************/

int GetORAtom(char *start,char *buffer)

{ char *sp = start;
  char *spc = buffer;
  int bracklevel = 0, len = 0;

while ((*sp != '\0') && !((*sp == '|') && (bracklevel == 0)))
   {
   if (*sp == '(')
      {
      Debug4("+(\n");
      bracklevel++;
      }

   if (*sp == ')')
      {
      Debug4("-)\n");
      bracklevel--;
      }

   Debug4("(%c)",*sp);
   *spc++ = *sp++;
   len++;
   }

*spc = '\0';

Debug4("\nGetORATom(%s)->%s\n",start,buffer); 
return len;
}

/*********************************************************************/
/* Level 4                                                           */
/*********************************************************************/

int GetANDAtom(char *start,char *buffer)

{ char *sp = start;
  char *spc = buffer;
  int bracklevel = 0, len = 0;

while ((*sp != '\0') && !(((*sp == '.')||(*sp == '&')) && (bracklevel == 0)))
   {
   if (*sp == '(')
      {
      Debug4("+(\n");
      bracklevel++;
      }

   if (*sp == ')')
      {
      Debug("-)\n");
      bracklevel--;
      }

   *spc++ = *sp++;

   len++;
   }

*spc = '\0';
Debug4("\nGetANDATom(%s)->%s\n",start,buffer);  

return len;
}

/*********************************************************************/

int CountEvalAtoms(char *class)

{ char *sp;
  int count = 0, bracklevel = 0;
  
for (sp = class; *sp != '\0'; sp++)
   {
   if (*sp == '(')
      {
      Debug4("+(\n");
      bracklevel++;
      continue;
      }

   if (*sp == ')')
      {
      Debug4("-)\n");
      bracklevel--;
      continue;
      }
   
   if ((bracklevel == 0) && ((*sp == '.')||(*sp == '&')))
      {
      count++;
      }
   }

if (bracklevel != 0)
   {
   sprintf(OUTPUT,"Bracket mismatch, in [class=%s], level = %d\n",class,bracklevel);
   yyerror(OUTPUT);
   FatalError("Aborted");
   }

return count+1;
}

/*********************************************************************/
/* TOOLKIT : actions                                                 */
/*********************************************************************/

enum actions ActionStringToCode (char *str)

{ char *sp;
  int i;
  enum actions action;

action = none;

for (sp = str; *sp != '\0'; sp++)
   {
   *sp = ToLower(*sp);
   if (*sp == ':')
      {
      *sp = '\0';
      }
   }

for (i = 1; ACTIONID[i] != '\0'; i++)
   {
   if (strcmp(ACTIONID[i],str) == 0)
      {
      action = (enum actions) i;
      break;
      }
   }

if (action == none)
  {
  yyerror("Indexed macro specified no action");
  FatalError("Could not compile action");
  }

return (enum actions) i;
}

/*********************************************************************/

int IsBracketed(char *s)

 /* return true if the entire string is bracketed, not just if
    if contains brackets */

{ int i, level= 0;

if (*s != '(')
   {
   return false;
   }

for (i = 0; i < strlen(s)-1; i++)
   {
   if (s[i] == '(')
      {
      level++;
      }
   
   if (s[i] == ')')
      {
      level--;
      }

   if (level == 0)
      {
      return false;  /* premature ) */
      }
   }

return true;
}

/****************************************************************/

int CfShow() /* is the this cfshow -r parser ? */

{
switch (ACTION)
   {
   case control:
   case defaultroute:
   case import:
       return false;
   }

if (INSTALLALL)
   {
   return true;
   }
else
   {
   return false;
   }
}

/****************************************************************/

void CheckCommonErrors(char *atom)

{
}

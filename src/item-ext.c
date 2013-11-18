/* cfengine for GNU
 
        Copyright (C) 1995,1999
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


/*********************************************************************/
/*                                                                   */
/*  TOOLKIT: the "item extension" object library for cfengine        */
/*                                                                   */
/*********************************************************************/

/* The functions in this file are mostly used by "editfiles" commands;
 *  DoEditFile() may need to reset more globals before returning for
 *  them to always work as expected. */

#include "cf.defs.h"
#include "cf.extern.h"


/*********************************************************************************/

struct Item *ListFromArgs(char *string)

/* Splits a string with quoted components etc */

{ struct Item *ip = NULL;
 int inquotes_level = 0,i = 0,argc=0;
  int inquote_level = 0;
  int paren_level = 0;
  char *sp,lastch = '\0';
  char item[CF_BUFSIZE];

memset(item,0,CF_BUFSIZE);
     
for (sp = string; *sp != '\0'; sp++)
   {
   switch (*sp)
      {
      case '\"':
          
          if (lastch == '\\')  /* Escaped quote */
             {
             if (inquotes_level == 0)
                {
                yyerror("Quoting error - escaped quote outside string");
                FatalError("Unrecoverable");
                }
             
             i--;
             }
          else
             {
             inquotes_level = 1 - inquotes_level; /* toggle */
             continue;
             }
          break;

      case '\'':
          
          if (lastch == '\\')  /* Escaped quote */
             {
             if (inquotes_level == 0)
                {
                yyerror("Quoting error - escaped quote outside string");
                FatalError("Unrecoverable");
                }
             
             i--;
             }
          else
             {
             inquote_level = 1 - inquote_level; /* toggle */
             continue;
             }
          break;

      case '(':

          if (inquotes_level == 0)
             {
             paren_level++;
             }
          break;
          
      case ')':
          
          if (inquotes_level == 0)
             {          
             paren_level--;
             }
          break;
          
      case ',':
          
          if ((inquotes_level == 0) && (paren_level == 0))
             {
             item[i] = '\0';
             i = 0;
             AppendItem(&ip,item,"");
             Debug("ListArg[%d]=(%s)\n",argc++,item);
             memset(item,0,CF_BUFSIZE);
             continue;
             }
      }

   item[i++] = *sp;
   lastch = *sp;
   }

item[i] = '\0';
AppendItem(&ip,item,"");
Debug("ListArg[%d]=(%s)\n",argc,item);
return ip;
}

/*********************************************************************/

int OrderedListsMatch(struct Item *list1,struct Item *list2)

{ struct Item *ip1,*ip2;

for (ip1 = list1,ip2 = list2; (ip1!=NULL)&&(ip2!=NULL); ip1=ip1->next,ip2=ip2->next)
   {
   if (strcmp(ip1->name,ip2->name) != 0)
      {
      Debug("OrderedListMatch failed on (%s,%s)\n",ip1->name,ip2->name);
      return false;
      }
   }

if (ip1 != ip2)
   {
   return false;
   }
 
return true; 
}


/*********************************************************************/

int IsClassedItemIn(struct Item *list,char *item)

{ struct Item *ptr; 

if ((item == NULL) || (strlen(item) == 0))
   {
   return true;
   }
 
for (ptr = list; ptr != NULL; ptr=ptr->next)
   {
   if (strcmp(ptr->name,item) == 0)
      {
      if (IsExcluded(ptr->classes))
         {
         continue;
         }   

      return(true);
      }
   }
 
return(false);
}

/*********************************************************************/

int IsWildItemIn(struct Item *list,char *wildcard)

   /* Checks whether item matches a list of wildcards */

{ struct Item *ptr;
 
for (ptr = list; ptr != NULL; ptr=ptr->next)
   {
   if (IsExcluded(ptr->classes))
      {
      continue;
      }

   if (WildMatch(wildcard,ptr->name) || WildMatch(ptr->name,wildcard))
      {
      Debug("IsWildItem(%s,%s)\n",wildcard,ptr->name);
      return(true);
      }
   }
return(false);
}

/*********************************************************************/

void InsertItemAfter (struct Item **filestart,struct Item *ptr,char *string)

{ struct Item *ip;
  char *sp;

EditVerbose("Inserting %s \n",string);

if ((ip = (struct Item *)malloc(sizeof(struct Item))) == NULL)
   {
   CfLog(cferror,"","Can't allocate memory in InsertItemAfter()");
   FatalError("");
   }

if ((sp = malloc(strlen(string)+1)) == NULL)
   {
   CfLog(cferror,"","Can't allocate memory in InsertItemAfter()");
   FatalError("");
   }

if (CURRENTLINEPTR == NULL)   /* File is empty */
   {
   if (*filestart == NULL)
      {
      *filestart = ip;
      ip->next = NULL;
      }
   else
      {
      ip->next = (*filestart)->next;
      (*filestart)->next = ip;
      }
   
   strcpy(sp,string);
   ip->name = sp;
   ip->classes = NULL;
   CURRENTLINEPTR = ip;
   CURRENTLINENUMBER = 1;
   }
else
   {
   ip->next = CURRENTLINEPTR->next;
   CURRENTLINENUMBER++;
   CURRENTLINEPTR->next = ip;
   CURRENTLINEPTR = ip;
   strcpy(sp,string);
   ip->name = sp;
   ip->classes = NULL;
   }

NUMBEROFEDITS++;

return;
}



/*********************************************************************/

struct Item *LocateNextItemContaining(struct Item *list,char *string) 

{ struct Item *ip;

for (ip = list; ip != NULL; ip=ip->next)
   {
   if (ip->name == NULL)
      {
      continue;
      }
   
   if (EDABORTMODE && ItemMatchesRegEx(ip->name,VEDITABORT))
      {
      EditVerbose("Aborting search, regex %s matches line\n",VEDITABORT);
      return NULL;
      }
   
   if (strstr(ip->name,string))
      {
      return ip;
      }
   }

return NULL;
}

/*********************************************************************/

int RegexOK(char *string)

{ regex_t rx;

if (CfRegcomp(&rx,string, REG_EXTENDED) != 0)
   {
   return false;
   }

regfree(&rx); 
return true;
}

/*********************************************************************/ 

struct Item *LocateNextItemMatching(struct Item *list,char *string) 

{ struct Item *ip;
  regex_t rx,rxcache;
  regmatch_t pmatch;

if (CfRegcomp(&rxcache,string, REG_EXTENDED) != 0)
   {
   return NULL;
   }

for (ip = list; ip != NULL; ip=ip->next)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   if (EDABORTMODE && ItemMatchesRegEx(ip->name,VEDITABORT))
      {
      Verbose("Aborting search, regex %s matches line\n",VEDITABORT);
      regfree(&rx);
      return NULL;
      }

   memcpy(&rx,&rxcache,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */   
   if (regexec(&rx,ip->name,1,&pmatch,0) == 0)
      {
      if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(ip->name)))
         {
         regfree(&rx);
         return ip;
         }
      }
   }

/* regfree(&rx); */
return NULL;
}

/*********************************************************************/

struct Item *LocateNextItemStarting(struct Item *list,char *string) 

{ struct Item *ip;

for (ip = list; (ip != NULL); ip=ip->next)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   if (EDABORTMODE && ItemMatchesRegEx(ip->name,VEDITABORT))
      {
      Verbose("Aborting search, regex %s matches line\n",VEDITABORT);
      return NULL;
      }
      
   if (strncmp(ip->name,string,strlen(string)) == 0)
      {
      return ip;
      }
   }

return NULL;
}

/*********************************************************************/

struct Item *LocateItemMatchingRegExp(struct Item *list,char *string) 

{ struct Item *ip;
  regex_t rx,rxcache;
  regmatch_t pmatch;
  int line = CURRENTLINENUMBER;

if (list != NULL)
   {
   Debug("LocateItemMatchingRexExp(%s,%s)\n",list->name,string);
   }
  
if (CfRegcomp(&rxcache,string, REG_EXTENDED) != 0)
   {
   return NULL;
   }

for (ip = list; (ip != NULL); ip=ip->next, line++)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   memcpy(&rx,&rxcache,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */      
   if (EDABORTMODE && ItemMatchesRegEx(ip->name,VEDITABORT))
      {
      Verbose("Aborting search, regex %s matches line\n",VEDITABORT);
      regfree(&rx);
      return NULL;
      }
   
   if (regexec(&rx,ip->name,1,&pmatch,0) == 0)
      {
      if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(ip->name)))
         {
         EditVerbose("Edit: Search ended at line %d\n",line);
         EditVerbose("Edit: (Found %s)\n",ip->name);
         CURRENTLINENUMBER = line;
         CURRENTLINEPTR = ip;
         regfree(&rx);
         return ip;
         }
      }
   }

EditVerbose("Edit: Search for %s failed. Current line still %d\n",string,CURRENTLINENUMBER);
/* regfree(&rx); */
return NULL;
}

/*********************************************************************/

struct Item *LocateItemContainingRegExp(struct Item *list,char *string) 

{ struct Item *ip;
  regex_t rx,rxcache;
  regmatch_t pmatch;
  int line = 1;

if (CfRegcomp(&rxcache,string, REG_EXTENDED) != 0)
   {
   return NULL;
   }

for (ip = list; (ip != NULL); ip=ip->next)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   if (EDABORTMODE && ItemMatchesRegEx(ip->name,VEDITABORT))
      {
      Verbose("Aborting search, regex %s matches line\n",VEDITABORT);
      regfree(&rx);
      return NULL;
      }

   memcpy(&rx,&rxcache,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */   
   if (regexec(&rx,ip->name,1,&pmatch,0) == 0)
      {
      EditVerbose("Search ended at line %d\n",line);
      CURRENTLINENUMBER = line;
      CURRENTLINEPTR = ip;
      regfree(&rx);
      return ip;
      }
   }

EditVerbose("Search for %s failed. Current line still %d\n",string,CURRENTLINENUMBER);

/* regfree(&rx);*/
return NULL;
}

/********************************************************************/

int DeleteToRegExp(struct Item **filestart,char *string)

  /* Delete up to but not including a line matching the regex */

{ struct Item *ip, *ip_prev, *ip_end = NULL;
  int linefound = false;
  regex_t rx,rxcache;
  regmatch_t pmatch;

Debug2("DeleteToRegExp(list,%s)\n",string);

if (CURRENTLINEPTR == NULL)  /* Shouldn't happen */
   {
   CfLog(cferror,"File line-pointer undefined during editfile action\n","");
   return true;
   }

if (CfRegcomp(&rxcache,string, REG_EXTENDED) != 0)
   {
   return false;
   }

for (ip = CURRENTLINEPTR; (ip != NULL); ip=ip->next)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   if (EDABORTMODE && ItemMatchesRegEx(ip->name,VEDITABORT))
      {
      Verbose("Aborting search, regex %s matches line\n",VEDITABORT);
      regfree(&rx);
      return false;
      }

   memcpy(&rx,&rxcache,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */   
   if (regexec(&rx,ip->name,1,&pmatch,0) == 0)
      {
      if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(ip->name)))
         {
         linefound = true;
         ip_end = ip;
         break;
         }
      }
   }

 if (! linefound)
    {
    return false;
    }


for (ip_prev = *filestart; ip_prev != CURRENTLINEPTR && ip_prev->next != CURRENTLINEPTR; ip_prev=ip_prev->next)
   {
   }

for (ip = CURRENTLINEPTR; ip != NULL; ip = CURRENTLINEPTR)
   {
   if (ip->name == NULL)
      {
      continue;
      }


   if (ip == ip_end)
      {
      EditVerbose("Edit: terminating line: %s (Done)\n",ip->name);
      return true;
      }

   if (EDABORTMODE && ItemMatchesRegEx(ip->name,VEDITABORT))
      {
      Verbose("Aborting search, regex %s matches line\n",VEDITABORT);
      regfree(&rx);
      return false;
      }

   EditVerbose("Edit: delete line %s\n",ip->name);
   NUMBEROFEDITS++;
   CURRENTLINEPTR = ip->next;

   if (ip == *filestart)
      {
      *filestart = ip->next;
      }
   else
      {
      ip_prev->next = ip->next;
      }

   free (ip->name);
   free ((char *)ip);
   }

/* regfree(&rx); */
return true;
}

/*********************************************************************/

/* DeleteItem* function notes:
 * -They all take an item list and an item specification ("string" argument.)
 * -Some of them treat the item spec as a literal string, while others
 *  treat it as a regular expression.
 * -They all delete the first item meeting their criteria, as below.
 *  function   deletes item
 *  ------------------------------------------------------------------------
 *  DeleteItemStarting  start is literally equal to string item spec
 *  DeleteItemLiteral  literally equal to string item spec
 *  DeleteItemMatching  fully matched by regex item spec
 *  DeleteItemContaining containing string item spec
 */

/*********************************************************************/

int DeleteItemGeneral(struct Item **list,char *string,enum matchtypes type)

{ struct Item *ip,*last = NULL;
  int match = 0, matchlen = 0;
  regex_t rx,rxcache;
  regmatch_t pmatch;

if (list == NULL)
   {
   return false;
   }
 
switch (type)
   {
   case literalStart:
       matchlen = strlen(string);
       break;
   case regexComplete:
   case NOTregexComplete:
       if (CfRegcomp(&rxcache,string, REG_EXTENDED) != 0)
          {
          return false;
          }
       break;
   }
 
 for (ip = *list; ip != NULL; ip=ip->next)
    {
    if (ip->name == NULL)
       {
       continue;
       }
    
    if (EDABORTMODE && ItemMatchesRegEx(ip->name,VEDITABORT))
       {
       Verbose("Aborting search, regex %s matches line\n",VEDITABORT);
       return false;
       }
    
    switch(type)
       {
       case NOTliteralStart:
           match = (strncmp(ip->name, string, matchlen) != 0);
           break;
       case literalStart:
           match = (strncmp(ip->name, string, matchlen) == 0);
           break;
       case NOTliteralComplete:
           match = (strcmp(ip->name, string) != 0);
           break;
       case literalComplete:
           match = (strcmp(ip->name, string) == 0);
           break;
       case NOTliteralSomewhere:
           match = (strstr(ip->name, string) == NULL);
           break;
       case literalSomewhere:
           match = (strstr(ip->name, string) != NULL);
           break;
       case NOTregexComplete:
       case regexComplete:
           /* To fix a bug on some implementations where rx gets emptied */
           memcpy(&rx,&rxcache,sizeof(rx));
           match = (regexec(&rx,ip->name,1,&pmatch,0) == 0) && (pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(ip->name));
           
           if (type == NOTregexComplete)
              {
              match = !match;
              }
           break;
       }
    
    if (match)
       {
       if (type == regexComplete || type == NOTregexComplete)
          {
          regfree(&rx);
          }
       
       EditVerbose("Deleted item %s\n",ip->name);
       
       if (ip == *list)
          {
          free((*list)->name);
          if (ip->classes != NULL) 
             {
             free(ip->classes);
             }
          *list = ip->next;
          free((char *)ip);
          
          NUMBEROFEDITS++;
          return true;
          }
       else
          {
          if (ip != NULL)
             {
             if (last != NULL)
                {
                last->next = ip->next;
                }
             
             free(ip->name);
             if (ip->classes != NULL) 
                {
                free(ip->classes);
                }
             free((char *)ip);
             }
          NUMBEROFEDITS++;
          return true;
          }
       
       }
    last = ip;
    }
 
 return false;
}


/*********************************************************************/

int DeleteItemStarting(struct Item **list,char *string)  /* delete 1st item starting with string */

{
return DeleteItemGeneral(list,string,literalStart);
}

/*********************************************************************/

int DeleteItemNotStarting(struct Item **list,char *string)  /* delete 1st item starting with string */

{
return DeleteItemGeneral(list,string,NOTliteralStart);
}

/*********************************************************************/

int DeleteItemLiteral(struct Item **list,char *string)  /* delete 1st item which is string */

{
return DeleteItemGeneral(list,string,literalComplete);
}

/*********************************************************************/

int DeleteItemMatching(struct Item **list,char *string)  /* delete 1st item fully matching regex */

{
return DeleteItemGeneral(list,string,regexComplete);
}

/*********************************************************************/

int DeleteItemNotMatching(struct Item **list,char *string)  /* delete 1st item fully matching regex */

{
return DeleteItemGeneral(list,string,NOTregexComplete);
}

/*********************************************************************/

int DeleteItemContaining(struct Item **list,char *string) /* delete first item containing string */

{
return DeleteItemGeneral(list,string,literalSomewhere);
}

/*********************************************************************/

int DeleteItemNotContaining(struct Item **list,char *string) /* delete first item containing string */

{
return DeleteItemGeneral(list,string,NOTliteralSomewhere);
}


/*********************************************************************/

int CommentItemStarting(struct Item **list,char *string,char *comm,char *end)

{ struct Item *ip;
  char buff[CF_BUFSIZE];

for (ip = *list; ip != NULL; ip=ip->next)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   if (EDABORTMODE && ItemMatchesRegEx(ip->name,VEDITABORT))
      {
      Verbose("Aborting search, regex %s matches line\n",VEDITABORT);
      return false;
      }
      
   if (strncmp(ip->name,string,strlen(string)) == 0)
      {
      if (strlen(ip->name)+strlen(comm)+strlen(end)+2 > CF_BUFSIZE)
         {
         CfLog(cferror,"Bufsize overflow while commenting line - abort\n","");
         return false;
         }

      if (strncmp(ip->name,comm,strlen(comm))== 0)
         {
         continue;
         }

      EditVerbose("Commenting %s%s%s\n",comm,ip->name,end);

      snprintf(buff,CF_BUFSIZE,"%s%s%s",comm,ip->name,end);
      free(ip->name);

      if ((ip->name = malloc(strlen(buff)+1)) == NULL)
         {
         CfLog(cferror,"malloc in CommentItemStarting\n","");
         FatalError("");
         }

      strcpy(ip->name,buff);
      NUMBEROFEDITS++;

      return true;
      }
   }

return false;
}

/*********************************************************************/

int CommentItemContaining(struct Item **list,char *string,char *comm,char *end)

{ struct Item *ip;
  char buff[CF_BUFSIZE];

for (ip = *list; ip != NULL; ip=ip->next)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   if (EDABORTMODE && ItemMatchesRegEx(ip->name,VEDITABORT))
      {
      Verbose("Aborting search, regex %s matches line\n",VEDITABORT);
      return false;
      }
      
   if (strstr(ip->name,string))
      {
      if (strlen(ip->name)+strlen(comm)+strlen(end)+2 > CF_BUFSIZE)
         {
         CfLog(cferror,"Bufsize overflow while commenting line - abort\n","");
         return false;
         }

      if (strncmp(ip->name,comm,strlen(comm))== 0)
         {
         continue;
         }

      EditVerbose("Commenting %s%s%s\n",comm,ip->name,end);

      snprintf(buff,CF_BUFSIZE,"%s%s%s",comm,ip->name,end);
      free(ip->name);

      if ((ip->name = malloc(strlen(buff)+1)) == NULL)
         {
         CfLog(cferror,"malloc in CommentItemContaining\n","");
         FatalError("");
         }

      strcpy(ip->name,buff);
      NUMBEROFEDITS++;

      return true;
      }
   }

return false;
}

/*********************************************************************/

int CommentItemMatching(struct Item **list,char *string,char *comm,char *end)

{ struct Item *ip;
  char buff[CF_BUFSIZE];
  regex_t rx,rxcache;
  regmatch_t pmatch;

if (CfRegcomp(&rxcache,string, REG_EXTENDED) != 0)
   {
   return false;
   }

for (ip = *list; ip != NULL; ip=ip->next)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   if (EDABORTMODE && ItemMatchesRegEx(ip->name,VEDITABORT))
      {
      Verbose("Aborting search, regex %s matches line\n",VEDITABORT);
      regfree(&rx);
      return false;
      }

   memcpy(&rx,&rxcache,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */
   
   if (regexec(&rx,ip->name,1,&pmatch,0) == 0)
      {
      if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(ip->name)))
         {
         if (strlen(ip->name)+strlen(comm)+strlen(end)+2 > CF_BUFSIZE)
            {
            CfLog(cferror,"Bufsize overflow while commenting line - abort\n","");
            regfree(&rx);
            return false;
            }
         
         if (strncmp(ip->name,comm,strlen(comm)) == 0) /* Already commented */
            {
            continue;
            }
         
         EditVerbose("Commenting %s%s%s\n",comm,ip->name,end);
         
         snprintf(buff,CF_BUFSIZE,"%s%s%s",comm,ip->name,end);
         free(ip->name);
         
         if ((ip->name = malloc(strlen(buff)+1)) == NULL)
            {
            CfLog(cferror,"malloc in CommentItemContaining\n ","");
            FatalError("");
            }
         
         strcpy(ip->name,buff);
         NUMBEROFEDITS++;
         
         regfree(&rx);
         return true;
         }
      }
   }
 
/* regfree(&rx); */
return false;
}

/********************************************************************/

int UnCommentItemMatching(struct Item **list,char *string,char *comm,char *end)

{ struct Item *ip;
  char *sp, *sp1, *sp2, *spc;
  regex_t rx,rxcache;
  regmatch_t pmatch;

Debug("UnCommentItemMatching %s/%s\n",string,comm);
  
if (CfRegcomp(&rxcache,string, REG_EXTENDED) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Failed to compile expression %s",string);
   CfLog(cferror,OUTPUT,"");
   return false;
   }

for (ip = *list; ip != NULL; ip=ip->next)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   memcpy(&rx,&rxcache,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */

   Debug("Compare %s/%s\n",string,ip->name);

   if (regexec(&rx,ip->name,1,&pmatch,0) == 0)
      {
      if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(ip->name)))
         {
         Debug("Compare %s/%s - ok FOUND\n",string,ip->name);
         
         if (strlen(ip->name)+strlen(comm)+strlen(end)+2 > CF_BUFSIZE)
            {
            CfLog(cferror,"Bufsize overflow while commenting line - abort\n","");
            regfree(&rx);
            return false;
            }

         if (strstr(ip->name,comm) == NULL)
            {
            CURRENTLINEPTR = ip->next;
            continue;
            }

         EditVerbose("Uncomment line %s\n",ip->name);
         CURRENTLINEPTR = ip->next;
         
         if ((sp = malloc(strlen(ip->name)+2)) == NULL)
            {
            CfLog(cferror,"No Memory in UnCommentNLines\n","malloc");
            regfree(&rx);
            return false;
            }
         
         spc = sp;
         
         for (sp1 = ip->name; isspace((int)*sp1); sp1++)
            {
            *spc++ = *sp1;
            }
         
         *spc = '\0';
         
         sp2 = ip->name+strlen(ip->name);
         
         if ((strlen(end) != 0) && (strstr(ip->name,end) != NULL))
            {
            for (sp2 = ip->name+strlen(ip->name); strncmp(sp2,end,strlen(end)) != 0; sp2--)
               {
               }
            
            *sp2 = '\0';
            }
         
         strcat(sp,sp1+strlen(comm));
         
         if (sp2 != ip->name+strlen(ip->name))
            {
            strcat(sp,sp2+strlen(end));
            }
         
         if (strcmp(sp,ip->name) != 0)
            {
            NUMBEROFEDITS++;
            }
         
         free(ip->name);
         ip->name = sp;
         regfree(&rx);
         return true;
         }
      }
   }
 
/* regfree(&rx); */
return false;
}

/********************************************************************/

int UnCommentItemContaining(struct Item **list,char *string,char *comm,char *end)

{ struct Item *ip;
  char *sp, *sp1, *sp2, *spc;

for (ip = *list; ip != NULL; ip=ip->next)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   if (strstr(ip->name,string))
      {
      if (strstr(ip->name,comm) == NULL)
         {
         CURRENTLINEPTR = ip->next;
         continue;
         }

      EditVerbose("Uncomment line %s\n",ip->name);
      CURRENTLINEPTR = ip->next;

      if ((sp = malloc(strlen(ip->name)+2)) == NULL)
         {
         CfLog(cferror,"No memory in UnCommentNLines\n","malloc");
         return false;
         }

      spc = sp;
   
      for (sp1 = ip->name; isspace((int)*sp1); sp1++)
         {
         *spc++ = *sp1;
         }

      *spc = '\0';

      sp2 = ip->name+strlen(ip->name);

      if ((strlen(end) != 0) && (strstr(ip->name,end) != NULL))
         {
         for (sp2 = ip->name+strlen(ip->name); strncmp(sp2,end,strlen(end)) != 0; sp2--)
            {
            }

         *sp2 = '\0';
         }

      strcat(sp,sp1+strlen(comm));

      if (sp2 != ip->name+strlen(ip->name))
         {
         strcat(sp,sp2+strlen(end));
         }

      if (strcmp(sp,ip->name) != 0)
         {
         NUMBEROFEDITS++;
         }
   
      free(ip->name);
      ip->name = sp;
      return true;
      }
   }

return false;
}


/********************************************************************/

int CommentToRegExp(struct Item **filestart,char *string,char *comm,char *end)

  /* Comment up to and including a line matching the regex */

{ struct Item *ip, *ip_end = NULL;
  int linefound = false, done;
  char *sp;
  regex_t rx,rxcache;
  regmatch_t pmatch;

Debug2("CommentToRegExp(list,%s %s)\n",comm,string);

if (CURRENTLINEPTR == NULL)  /* Shouldn't happen */
   {
   CfLog(cferror,"File line-pointer undefined during editfile action\n","");
   return true;
   }

if (CfRegcomp(&rxcache,string, REG_EXTENDED) != 0)
   {
   return false;
   }

for (ip = CURRENTLINEPTR; (ip != NULL); ip=ip->next)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   if (EDABORTMODE && ItemMatchesRegEx(ip->name,VEDITABORT))
      {
      Verbose("Aborting search, regex %s matches line\n",VEDITABORT);
      regfree(&rx);
      return false;
      }

   memcpy(&rx,&rxcache,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */   
   if (regexec(&rx,ip->name,1,&pmatch,0) == 0)
      {
      if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(ip->name)))
         {
         linefound = true;
         ip_end = ip;
         break;
         }
      }
   }
 
 if (! linefound)
    {
    return false;
    }
 
done = false;

for (ip = CURRENTLINEPTR; ip != NULL; ip = CURRENTLINEPTR)
   {
   if (ip == ip_end)
      {
      EditVerbose("Terminating line: %s (Done)\n",ip->name);
      done = true;
      }

   EditVerbose("Comment line %s%s%s\n",comm,ip->name, end);
   NUMBEROFEDITS++;
   CURRENTLINEPTR = ip->next;

   if ((sp = malloc(strlen(ip->name)+strlen(comm)+strlen(end)+2)) == NULL)
      {
      CfLog(cferror,"No memory in CommentToRegExp\n","malloc");
      regfree(&rx);
      return false;
      }

   strcpy (sp,comm);
   strcat (sp,ip->name);
   strcat (sp,end);

   free (ip->name);
   ip->name = sp;

   if (done)
      {
      break;
      }
   }

/* regfree(&rx); */
return true;
}

/********************************************************************/

int UnCommentToRegExp(struct Item **filestart,char *string,char *comm,char *end)

  /* Comment up to and including a line matching the regex */

{ struct Item *ip, *ip_end = NULL;
  int linefound = false, done;
  char *sp, *sp2;
  regex_t rx,rxcache;
  regmatch_t pmatch;

Debug2("CommentToRegExp(list,%s %s)\n",comm,string);

if (CURRENTLINEPTR == NULL)  /* Shouldn't happen */
   {
   CfLog(cferror,"File line-pointer undefined during editfile action\n","");
   return true;
   }

if (CfRegcomp(&rxcache,string, REG_EXTENDED) != 0)
   {
   return false;
   }

for (ip = CURRENTLINEPTR; (ip != NULL); ip=ip->next)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   if (EDABORTMODE && ItemMatchesRegEx(ip->name,VEDITABORT))
      {
      Verbose("Aborting search, regex %s matches line\n",VEDITABORT);
      regfree(&rx);
      return false;
      }

   memcpy(&rx,&rxcache,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */   
   if (regexec(&rx,ip->name,1,&pmatch,0) == 0)
      {
      if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(ip->name)))
         {
         linefound = true;
         ip_end = ip;
         break;
         }
      }
   }
 
if (!linefound)
   {
   return false;
   }
 
for (ip = CURRENTLINEPTR; ip != NULL; ip = CURRENTLINEPTR)
   {
   if (ip == ip_end)
      {
      EditVerbose("Terminating line: %s (Done)\n",ip->name);
      done = true;
      }

   if (strncmp(ip->name,comm,strlen(comm)) != 0)
      {
      CURRENTLINEPTR = ip->next;
      continue;
      }
         
   EditVerbose("Uncomment line %s\n",ip->name);
   CURRENTLINEPTR = ip->next;
   
   if ((sp = malloc(strlen(ip->name))) == NULL)
      {
      CfLog(cferror,"No Memory in UnCommentToRegexp\n","malloc");
      regfree(&rx);
      return false;
      }
   
   strcpy(sp,ip->name+strlen(comm));
   
   if ((strlen(end) != 0) && (strstr(sp,end) != NULL))
      {
      for (sp2 = sp+strlen(sp); strncmp(sp2,end,strlen(end)) != 0; sp2--)
         {
         }
      
      *sp2 = '\0';
      }
   
   if (strcmp(sp,ip->name) != 0)
      {
      NUMBEROFEDITS++;
      }
   
   free(ip->name);
   ip->name = sp;
   }

/* regfree(&rx); */
return true;
}

/********************************************************************/

int DeleteSeveralLines (struct Item **filestart,char *string)

  /* Deletes up to N lines from current position */

{ struct Item *ip, *ip_prev;
  int ctr, N = -99, done = false;

Debug2("DeleteNLines(list,%s)\n",string);

sscanf(string,"%d", &N);

if (N < 1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Illegal number value in DeleteNLines: %s\n",string);
   CfLog(cferror,OUTPUT,"");
   return false;
   }


if (CURRENTLINEPTR == NULL)  /* Shouldn't happen */
   {
   CfLog(cferror,"File line-pointer undefined during editfile action\n","");
   return true;
   }

for (ip_prev = *filestart; ip_prev && ip_prev->next != CURRENTLINEPTR; ip_prev=ip_prev->next)
   {
   }

ctr = 1;

for (ip = CURRENTLINEPTR; ip != NULL; ip = CURRENTLINEPTR)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   if (EDABORTMODE && ItemMatchesRegEx(ip->name,VEDITABORT))
      {
      Verbose("Aborting search, regex %s matches line\n",VEDITABORT);
      return false;
      }
      
   if (ctr == N)
      {
      EditVerbose("Terminating line: %s (Done)\n",ip->name);
      done = true;
      }

   EditVerbose("Delete line %s\n",ip->name);
   NUMBEROFEDITS++;
   CURRENTLINEPTR = ip->next;

   if (ip_prev == NULL)
      {
      *filestart = ip->next;
      }
   else
      {
      ip_prev->next = ip->next;
      }

   free (ip->name);
   free ((char *)ip);
   ctr++;

   if (done)
      {
      break;
      }
   }

if (ctr-1 < N)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"DeleteNLines deleted only %d lines (not %d)\n",ctr-1,N);
   CfLog(cfsilent,OUTPUT,"");
   }

return true;
}

/********************************************************************/

struct Item *GotoLastItem (struct Item *list)

{ struct Item *ip;

CURRENTLINENUMBER=1;
CURRENTLINEPTR=list;

for (ip = list; ip != NULL && ip->next != NULL; ip=ip->next)
   {
   CURRENTLINENUMBER++;
   }

CURRENTLINEPTR = ip;

return ip;
}

/********************************************************************/

int LineMatches (char *line,char *regexp)

{ regex_t rx,rxcache;
  regmatch_t pmatch;

if (CfRegcomp(&rxcache,regexp, REG_EXTENDED) != 0)
   {
   return false;
   }

memcpy(&rx,&rxcache,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */ 

if (regexec(&rx,line,1,&pmatch,0) == 0)
   {
   /* Exact match of whole line */
   
   if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(line)))
      {
      regfree(&rx);
      return true;
      }
   }

regfree(&rx);
return false;
}

/********************************************************************/

int GlobalReplace(struct Item **liststart,char *search,char *replace)

{ int i;
  char *sp, *start = NULL;
  struct Item *ip;
  struct Item *oldCurrentLinePtr;
  regex_t rx,rxcache;
  regmatch_t match,matchcheck;

EditVerbose("Checking for global replace/%s/%s\n",search,replace);

if (CfRegcomp(&rxcache,search,REG_EXTENDED) != 0)
   {
   return false;
   }

for (ip = *liststart; ip != NULL; ip=ip->next)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   memcpy(&rx,&rxcache,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */

   if (regexec(&rx,ip->name,1,&match,0) == 0)
      {
      start = ip->name + match.rm_so;
      }
   else
      {
      continue;
      }

   memset(VBUFF,0,CF_BUFSIZE);
   
   i = 0;

   for (sp = ip->name; *sp != '\0'; sp++)
      {
      if (sp != start)
         {
         VBUFF[i] = *sp;
         }
      else
         {
         sp += match.rm_eo - match.rm_so - 1;
         VBUFF[i] = '\0';
         strcat(VBUFF,replace);
         i += strlen(replace)-1;
         
         memcpy(&rx,&rxcache,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */

         if (regexec(&rx,sp,1,&match,0) == 0)
            {
            start = sp + match.rm_so;
            }
         else
            {
            start = 0;
            }
         }
      
      i++;
      }
   
   Debug("Replace:\n  (%s)\nwith      (%s)\n",ip->name,VBUFF);
   
   if (regexec(&rx,VBUFF,1,&matchcheck,0) == 0)
      {
      if ((matchcheck.rm_so == 0) && (matchcheck.rm_eo == strlen(VBUFF)))
         {
         Verbose("Edit operation replaces [%s] with whole line [%s], so it appears convergent.\n",search,replace);
         }
      else
         {
         if (EDITGROUPLEVEL <= 0)
            {
            snprintf(OUTPUT,CF_BUFSIZE*2,"WARNING: Non-convergent edit operation ReplaceAll [%s] With [%s]",search,replace);
            CfLog(cferror,OUTPUT,"");
            snprintf(OUTPUT,CF_BUFSIZE*2,"Line begins [%.40s]",ip->name);
            CfLog(cferror,OUTPUT,"");
            CfLog(cferror,"Replacement matches search string and will thus replace every time - edit was not done","");
            return false;
            }
         else
            {
            /* This needs to be a smarter check - to see if the group fixes the problem or not*/
            snprintf(OUTPUT,CF_BUFSIZE*2,"WARNING: Possible non-convergent edit operation ReplaceAll [%s] With [%s]",search,replace);
            CfLog(cfinform,OUTPUT,"");
            snprintf(OUTPUT,CF_BUFSIZE*2,"Line begins [%.40s]",ip->name);
            CfLog(cfinform,OUTPUT,"");
            CfLog(cfinform,"Replacement, although predicated, could match search string and might thus replace every time - edit was not done","");
            }
         }
      }

   CURRENTLINEPTR = ip;
   InsertItemAfter(liststart,ip,VBUFF);
   oldCurrentLinePtr = CURRENTLINEPTR;                   /* set by Insert */
   DeleteItem(liststart,ip);
   ip = oldCurrentLinePtr;
   }
 
regfree(&rxcache);
return true;
}

/********************************************************************/
/* part of ReplaceFirst regexp With string                          */
/* written by steve rader <rader@hep.wisc.edu>                      */
/********************************************************************/

int SingleReplace(liststart,search,replace)

struct Item **liststart;
char *search, *replace;

{ int i;
  char *sp, *start = NULL;
  struct Item *ip;
  struct Item *oldCurrentLinePtr;
  regex_t rx,rxcache;
  regmatch_t match;

  EditVerbose("SRDEBUG Checking for SingleReplace s/%s/%s/\n",search,replace);

  if (CfRegcomp(&rxcache,search,REG_EXTENDED) != 0) {
    EditVerbose("SRDEBUG SingleReplace ab-ended: could not compile regex! (this should not happen?)\n");
    return false;
  }

  i = 0;
  for (ip = *liststart; ip != NULL; ip=ip->next) {
    i++;
    if (ip->name == NULL) {
      continue;
    }
    bcopy(&rxcache,&rx,sizeof(rx)); /* workaround for regexec()s that empty rx */
    if (regexec(&rx,ip->name,1,&match,0) == 0) {
      start = ip->name + match.rm_so;
      EditVerbose("Doing SingleReplace of \"%s\" with \"%s\" on line %d\n",start,replace,i);
      bzero(VBUFF,CF_BUFSIZE);
      strcpy(VBUFF,ip->name);
      VBUFF[match.rm_so] = '\0';  /* ...head of string */
      strcat(VBUFF,replace);      /* ...replacement string */
      sp = ip->name;
      sp += match.rm_eo;
      strcat(VBUFF,sp);           /* ...tail of string */
      Debug("SRDEBUG old line num %d is: \"%s\"\n",i,ip->name);
      Debug("SRDEBUG new line num %d is: \"%s\"\n",i,VBUFF);
      CURRENTLINEPTR = ip;
      InsertItemAfter(liststart,ip,VBUFF);
      oldCurrentLinePtr = CURRENTLINEPTR; 
      DeleteItem(liststart,ip);
      ip = oldCurrentLinePtr;
    } 
  }
  regfree(&rxcache);
  return true;
}

/********************************************************************/

int CommentSeveralLines(struct Item **filestart,char *string,char *comm,char *end)

  /* Comments up to N lines from current position */

{ struct Item *ip;
  int ctr, N = -99, done = false;
  char *sp;

Debug2("CommentNLines(list,%s)\n",string);

sscanf(string,"%d", &N);

if (N < 1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Illegal number value in CommentNLines: %s\n",string);
   CfLog(cferror,OUTPUT,"");
   return false;
   }


if (CURRENTLINEPTR == NULL)  /* Shouldn't happen */
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"File line-pointer undefined during editfile action\n");
   CfLog(cferror,OUTPUT,"");
   return true;
   }

ctr = 1;

for (ip = CURRENTLINEPTR; ip != NULL; ip = CURRENTLINEPTR)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   if (ctr > N)
      {
      break;
      }
   
   if (ctr == N)
      {
      EditVerbose("Terminating line: %s (Done)\n",ip->name);
      done = true;
      }

   for (sp = ip->name; isspace((int)*sp); sp++)
      {
      }
   
   if (strncmp(sp,comm,strlen(comm)) == 0)
      {
      CURRENTLINEPTR = ip->next;
      ctr++;
      continue;
      }

   EditVerbose("Comment line %s\n",ip->name);
   NUMBEROFEDITS++;
   CURRENTLINEPTR = ip->next;

   if ((sp = malloc(strlen(ip->name)+strlen(comm)+strlen(end)+2)) == NULL)
      {
      CfLog(cferror,"No memory in CommentNLines\n","malloc");
      return false;
      }

   strcpy (sp,comm);
   strcat (sp,ip->name);
   strcat (sp,end);

   free (ip->name);
   ip->name = sp;
   ctr++;

   if (done)
      {
      break;
      }
   }

if (ctr-1 < N)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"CommentNLines commented only %d lines (not %d)\n",ctr-1,N);
   CfLog(cfinform,OUTPUT,"");
   }

return true;
}

/********************************************************************/

int UnCommentSeveralLines (struct Item **filestart,char *string,char *comm,char *end)

  /* Comments up to N lines from current position */

{ struct Item *ip;
  int ctr, N = -99, done = false;
  char *sp, *sp1, *sp2, *spc;

Debug2("UnCommentNLines(list,%s)\n",string);

sscanf(string,"%d", &N);

if (N < 1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Illegal number value in CommentNLines: %s\n",string);
   CfLog(cferror,OUTPUT,"");
   return false;
   }


if (CURRENTLINEPTR == NULL)  /* Shouldn't happen */
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"File line-pointer undefined during editfile action\n");
   CfLog(cferror,OUTPUT,"");
   return true;
   }

ctr = 1;

for (ip = CURRENTLINEPTR; ip != NULL; ip = CURRENTLINEPTR)
   {
   if (ip->name == NULL)
      {
      continue;
      }

   if (ctr > N)
      {
      break;
      }
   
   if (ctr == N)
      {
      EditVerbose("Terminating line: %s (Done)\n",ip->name);
      done = true;
      }

   if (strstr(ip->name,comm) == NULL)
      {
      CURRENTLINEPTR = ip->next;
      ctr++;
      continue;
      }

   EditVerbose("Uncomment line %s\n",ip->name);
   CURRENTLINEPTR = ip->next;

   if ((sp = malloc(strlen(ip->name)+2)) == NULL)
      {
      CfLog(cferror,"No memory in UnCommentNLines\n","malloc");
      return false;
      }

   spc = sp;
   
   for (sp1 = ip->name; isspace((int)*sp1); sp1++)
      {
      *spc++ = *sp1;
      }

   *spc = '\0';

   sp2 = ip->name+strlen(ip->name);

   if ((strlen(end) != 0) && (strstr(ip->name,end) != NULL))
      {
      for (sp2 = ip->name+strlen(ip->name); strncmp(sp2,end,strlen(end)) != 0; sp2--)
         {
         }

      *sp2 = '\0';
      }

   strcat(sp,sp1+strlen(comm));

   if (sp2 != ip->name+strlen(ip->name))
      {
      strcat(sp,sp2+strlen(end));
      }

   ctr++;

   if (strcmp(sp,ip->name) != 0)
      {
      NUMBEROFEDITS++;
      }
   
   free(ip->name);
   ip->name = sp;

   if (done)
      {
      break;
      }
   }

if (ctr-1 < N)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"CommentNLines commented only %d lines (not %d)\n",ctr-1,N);
   CfLog(cfinform,OUTPUT,"");
   }

return true;
}

/********************************************************************/

int ItemMatchesRegEx(char *item,char *regex)

{ regex_t rx,rxcache;
  regmatch_t pmatch; 

Debug("ItemMatchesRegEx(%s %s)\n",item,regex);

if (CfRegcomp(&rxcache,regex, REG_EXTENDED) != 0)
   {
   return true;
   }

memcpy(&rx,&rxcache,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */
 
if (regexec(&rx,item,1,&pmatch,0) == 0)
   {
   if ((pmatch.rm_so == 0) && (pmatch.rm_eo == strlen(item)))
      {
      regfree(&rx);
      return true;
      }
   }

regfree(&rx);
return false;
}

/********************************************************************/

void ReplaceWithFieldMatch(struct Item **filestart,char *field,char *replace,char split,char *filename)

{ struct Item *ip;
  char match[CF_BUFSIZE], linefield[CF_BUFSIZE], *sp, *sps, *spe;
  int matching_field = 0, fcount, i, linenum, count = 0;

Debug("ReplaceWithFieldMatch(%s,%s,%c)\n",field,replace,split);

if ((replace == NULL) || (strlen(replace) == 0))
   {
   EditVerbose("Ignoring empty line which doing ReplaceLinesMatchingField\n");
   return;
   }
  
matching_field = atoi(field);
memset(match,0,CF_BUFSIZE);

fcount = 1;
sps = spe = NULL;

for (sp = replace; *sp != '\0'; sp++)
   {
   if (*sp == split)
      {
      if (fcount == matching_field)
         {
         spe = sp;
         break;
         }
      fcount++;
      }

   if ((fcount == matching_field) && (sps == NULL))
      {
      sps = sp;
      }
   }

if (fcount < matching_field)
   {
   CfLog(cfsilent,"File formats did not completely match in ReplaceLinesMatchingField\n","");
   snprintf(OUTPUT,CF_BUFSIZE*2,"while editing %s\n",filename);
   CfLog(cfsilent,OUTPUT,"");
   return;
   }

if (spe == NULL)
   {
   spe = sp;
   }

for (i = 0, sp = sps; sp != spe; i++, sp++)
   {
   match[i] = *sp;
   }
  
Debug2("Edit: Replacing lines matching field %d == \"%s\"\n",matching_field,match);

linenum = 1;

for (ip = *filestart; ip != NULL; ip=ip->next, linenum++)
   {
   memset(linefield,0,CF_BUFSIZE);
   fcount = 1;
   sps = spe = NULL;

   if (ip->name == NULL)
      {
      continue;
      }

   for (sp = ip->name; *sp != '\0'; sp++)
      {
      if (*sp == split)
         {
         if (fcount == matching_field)
            {
            spe = sp;
            break;
            }
         fcount++;
         }
      
      if ((fcount == matching_field) && (sps == NULL))
         {
         sps = sp;
         }
      }
   
   if (spe == NULL)
      {
      spe = sp;
      }

   if (sps == NULL)
      {
      sps = sp;
      }

   for (i = 0, sp = sps; sp != spe; i++, sp++)
      {
      linefield[i] = *sp;
      }

   if (strcmp(linefield,match) == 0)
      {
      EditVerbose("Replacing line %d (key %s)\n",linenum,match);

      count++;

      if (strcmp(replace,ip->name) == 0)
         {
         continue;
         }
      
      if (count > 1)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Several lines in %s matched key %s\n",filename,match);
         CfLog(cfsilent,OUTPUT,"");
         }
      
      NUMBEROFEDITS++;
      
      free(ip->name);
      
      ip->name = (char *) malloc(strlen(replace)+1);
      strcpy(ip->name,replace);
      EditVerbose("Edit:   With (%s)\n",replace);
      }
   }
}

/********************************************************************/

void AppendToLine(struct Item *current,char *text,char *filename)

{ char *new;
 int wasblank = false;

if (current == NULL)
   {
   FatalError("Software error -- attempt to append to a non-existent line (shouldn't happen)");
   }
 
if (current->name != NULL)
   {
   wasblank = false;
   
   if (strstr(current->name,text))
      {
      return;
      }
   }

EditVerbose("Appending %s to line %-60s...\n",text,current->name);

new = malloc(strlen(current->name)+strlen(text)+1);
strcpy(new,current->name);
strcat(new,text);
NUMBEROFEDITS++;

if (!wasblank)
   {
   free(current->name);
   }

current->name = new;
}

/**************************************************************/

int CfRegcomp(regex_t *preg,const char *regex,int cflags)


{ int code;
  char buf[CF_BUFSIZE];

if (regex == NULL || *regex == '\0')
   {
   return -1;
   }

code = regcomp(preg,regex,cflags);
 
if (code != 0)
   {
   snprintf(buf,CF_BUFSIZE,"Regular expression error %d for %s\n", code, regex);
   CfLog(cferror,buf,"");

   regerror(code,preg,buf,CF_BUFSIZE);
   CfLog(cferror,buf,"");
   return -1;
   }
return 0;
}

/* EOF */

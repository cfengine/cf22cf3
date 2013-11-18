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
 

#include "cf.defs.h"
#include "cf.extern.h"


/*******************************************************************/
/* Macro substitution based on HASH-table                          */
/*******************************************************************/

void SetContext(char *id)

{
InstallObject(id);
 
Verbose("\n$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
Verbose(" * (Changing context state to: %s) *",id);
Verbose("\n$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n\n"); 
strncpy(CONTEXTID,id,31);
}

/*******************************************************************/

void InstallObject(char *name)

{ struct cfObject *ptr;
  
Debug1("Adding object %s", name);

/*
  if ( ! IsInstallable(CLASSBUFF))
   {
   return;
   }
*/

for (ptr = VOBJ; ptr != NULL; ptr=ptr->next)
   {
   if (strcmp(ptr->scope,name) == 0)
      {
      Debug("Object %s already exists\n",name);
      return;
      }
   }

if ((ptr = (struct cfObject *)malloc(sizeof(struct cfObject))) == NULL)
   {
   FatalError("Memory Allocation failed for cfObject");
   }

if (VOBJTOP == NULL)
   {
   VOBJ = ptr;
   }
else
   {
   VOBJTOP->next = ptr;
   }

InitHashTable((char **)ptr->hashtable);
 
ptr->next = NULL;
ptr->scope = strdup(name);
VOBJTOP = ptr; 
}

/*******************************************************************/

int ScopeIsMethod()

{
 if (strcmp(CONTEXTID,"private-method") == 0)
    {
    return true;
    }
 else
    {
    return false;
    }
}

/*******************************************************************/

void InitHashTable(char **table)

{ int i;

for (i = 0; i < CF_HASHTABLESIZE; i++)
   {
   table[i] = NULL;
   }
}

/*******************************************************************/

void BlankHashTable(char *scope)

{ int i;
  struct cfObject *ptr;

for (ptr = VOBJ; ptr != NULL; ptr=ptr->next)
   {
   if (strcmp(ptr->scope,scope) == 0)
      {
      Verbose("Clearing macros in scope(%s)\n",scope);
      
      for (i = 0; i < CF_HASHTABLESIZE; i++)
         {
         if (ptr->hashtable[i] != NULL)
            {
            free(ptr->hashtable[i]);
            ptr->hashtable[i] = NULL;
            }
         }
      }
   }
}

/*******************************************************************/

void PrintHashTable(char **table)

{ int i;
 char lval[CF_BUFSIZE],rval[CF_BUFSIZE]; 
 
for (i = 0; i < CF_HASHTABLESIZE; i++)
   {
   if (table[i] != NULL)
      {
      sscanf(table[i],"%[^=]=%[^\n]",lval,rval);

      if (strchr(rval,LISTSEPARATOR))
         {
         struct Item *ip,*list = NULL;

         list = SplitStringAsItemList(rval,LISTSEPARATOR);

         printf ("  \"%s\" slist => { ",lval);
         
         for (ip = list; ip != NULL; ip=ip->next)
            {
            printf("\"%s\", ",ip->name);   
            }

         printf("};\n");
         DeleteItemList(list);
         }
      else
         {
         printf ("  \"%s\" string => \"%s\";\n",lval,rval);
         }      
      }
   }
}

/*******************************************************************/

int Hash(char *name)

{ int i, slot = 0;

 for (i = 0; name[i] != '\0'; i++)
   {
   slot = (CF_MACROALPHABET * slot + name[i]) % CF_HASHTABLESIZE;
   }

return slot;
}

/*******************************************************************/

int ElfHash(char *key)

{ unsigned int h = 0;
  unsigned int g;

while (*key)
  {
  h = (h << 4) + *key++;

  g = h & 0xF0000000;         /* Assumes int is 32 bit */

  if (g) 
     {
     h ^= g >> 24;
     }

  h &= ~g;
  }

return (h % CF_HASHTABLESIZE);
}

/*******************************************************************/

void AddMacroValue(char *scope,char *name,char *value)

{ char *sp, buffer[CF_BUFSIZE],exp[CF_EXPANDSIZE];
  struct cfObject *ptr; 
  int slot;

Debug("AddMacroValue(%s.%s=%s)\n",scope,name,value);

if (name == NULL || value == NULL || scope == NULL)
   {
   yyerror("Bad macro or scope");
   CfLog(cferror,"Empty macro","");
   }

if (strlen(name) > CF_MAXVARSIZE)
   {
   yyerror("macro name too long");
   return;
   }

snprintf(exp,CF_BUFSIZE,"${%s}",name);

if (strstr(value,exp))
   {
   yyerror("Macro contains itself and is previously undefined");
   return;
   }

snprintf(exp,CF_BUFSIZE,"$(%s)",name);

if (strstr(value,exp))
   {
   yyerror("Macro contains itself and is previously undefined");
   return;
   }

ExpandVarstring(value,exp,NULL);

ptr = ObjectContext(scope);
 
snprintf(buffer,CF_BUFSIZE,"%s=%s",name,exp);

if ((sp = malloc(strlen(buffer)+1)) == NULL)
   {
   perror("malloc");
   FatalError("aborting");
   }

strcpy(sp,buffer);

slot = Hash(name);
 
if (ptr->hashtable[slot] != 0)
   {
   Debug("Macro Collision!\n");
   if (CompareMacro(name,ptr->hashtable[slot]) == 0)
      {
      if (PARSING && !IsItemIn(VREDEFINES,name))
         {
         snprintf(VBUFF,CF_BUFSIZE,"Redefinition of macro %s=%s (or perhaps missing quote)",name,exp);
         Warning(VBUFF);
         }
      free(ptr->hashtable[slot]);
      ptr->hashtable[slot] = sp;
      Debug("Stored %s in context %s\n",sp,name);
      return;
      }
   
   while ((ptr->hashtable[++slot % CF_HASHTABLESIZE] != 0))
      {
      if (slot == CF_HASHTABLESIZE-1)
         {
         slot = 0;
         }
      if (slot == Hash(name))
         {
         FatalError("AddMacroValue - internal error #1");
         }
      
      if (CompareMacro(name,ptr->hashtable[slot]) == 0)
         {
          if (PARSING && !IsItemIn(VREDEFINES,name))
             {
             snprintf(VBUFF,CF_BUFSIZE,"Redefinition of macro %s=%s",name,exp);
             Warning(VBUFF);
             }
         free(ptr->hashtable[slot]);
         ptr->hashtable[slot] = sp;
         return;
         }
      }
   }
 
 ptr->hashtable[slot] = sp;

Debug("Added Macro at hash address %d to object %s with value %s\n",slot,scope,sp);
}

/****************************************************************************/

/*
 * HvB: Bas van der Vlies
 *  This function checks if the given name has
 *  the requested value:
 *    1 --> check for values on or true
 *    0 --> check for values off or false
 *  return true if the name has the requested value
*/

int OptionIs(char *scope,char *name,short on)

{ char *result;

Debug("OptionIs(%s,%s,%d)\n",scope,name,on);
 
result = GetMacroValue(scope,name);

if (result == NULL)
   {
   return(false);
   }

if (on)
   {
   if ( (strcmp(result, "on") == 0) || ( strcmp(result, "true") == 0) )
      {
      return(true);
      }
   }
else
   {
   if ( (strcmp(result, "off") == 0) || (strcmp(result, "false") == 0) )
      {
      return(true);
      }
   }

return(false);
}


/*******************************************************************/

char *GetMacroValue(char *scope,char *name)

{ char *sp;
  int slot,i;
  struct cfObject *ptr = NULL;
  char objscope[CF_MAXVARSIZE],vname[CF_MAXVARSIZE];
  
/* Check the class.id identity to see if this is private ..
    and replace

     HASH[slot] by objectptr->hash[slot]
*/

Debug("GetMacroValue(%s,%s)\n",scope,name);

if (strstr(name,"."))
   {
   objscope[0] = '\0';
   sscanf(name,"%[^.].%s",objscope,vname);
   Debug("Macro identifier %s is prefixed with an object %s\n",vname,objscope);
   ptr = ObjectContext(objscope);
   }
 
if (ptr == NULL)
   {
   strcpy(vname,name);
   ptr = ObjectContext(scope);
   }
 
Debug("GetMacroValue(%s,%s): using scope '%s' for variable '%s'\n",
      scope,name,ptr->scope,vname);
 
i = slot = Hash(vname);
 
if (CompareMacro(vname,ptr->hashtable[slot]) != 0)
   {
   while (true)
      {
      i++;

      if (i >= CF_HASHTABLESIZE-1)
         {
         i = 0;
         }

      if (CompareMacro(vname,ptr->hashtable[i]) == 0)
         {
         for(sp = ptr->hashtable[i]; *sp != '='; sp++)
            {
            }

         return(sp+1);
         }

      if (i == slot-1)
         {
         return(getenv(vname));  /* Look in environment if not found */
         }
      }
   }
else
   {
   for(sp = ptr->hashtable[slot]; *sp != '='; sp++)
      {
      }

   Debug1("Return Macrovalue={%s}\n",sp+1);
   return(sp+1);
   }   
}

/*******************************************************************/

void DeleteMacro(char *scope,char *id)

{ int slot,i;
  struct cfObject *ptr;
  
i = slot = Hash(id);
ptr = ObjectContext(scope);
 
if (CompareMacro(id,ptr->hashtable[slot]) != 0)
   {
   while (true)
      {
      i++;
      
      if (i == slot)
         {
         Debug("No macro matched\n");
         break;
         }
      
      if (i >= CF_HASHTABLESIZE-1)
         {
         i = 0;
         }
      
      if (CompareMacro(id,ptr->hashtable[i]) == 0)
         {
         free(ptr->hashtable[i]);
         ptr->hashtable[i] = NULL;
         }
      }
   }
 else
    {
    free(ptr->hashtable[i]);
    ptr->hashtable[i] = NULL;
    }   
}


/*******************************************************************/

int CompareMacro(char *name,char *macro)

{ char buffer[CF_BUFSIZE];

if (macro == NULL || name == NULL)
   {
   return 1;
   }

sscanf(macro,"%[^=]",buffer);
return(strcmp(buffer,name));
}

/*******************************************************************/

void DeleteMacros(char *scope)

{ int i;
  struct cfObject *ptr;
  
ptr = ObjectContext(scope);
 
for (i = 0; i < CF_HASHTABLESIZE; i++)
   {
   if (ptr->hashtable[i] != NULL)
      {
      free(ptr->hashtable[i]);
      ptr->hashtable[i] = NULL;
      }
   }
}

/*******************************************************************/

struct cfObject *ObjectContext(char *scope)

{ struct cfObject *cp = NULL;

 for (cp = VOBJ; cp != NULL; cp=cp->next)
    {
    if (strcmp(cp->scope,scope) == 0)
       {
       break;
       }
    }

return cp;
}


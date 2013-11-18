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

/**************************************************************************/
/*                                                                        */
/* File: strategies.c                                                     */
/*                                                                        */
/**************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*****************************************************************************/

void InstallStrategy(char *alias,char *classes)

{ struct Strategy *ptr;
  char ebuff[CF_EXPANDSIZE];
 
 Debug1("InstallStrategy(%s,%s)\n",alias,classes);
 
/* if (! IsInstallable(classes))
    {
    Debug1("Not installing Strategy no match\n");
    return;
    }
*/
 
 ExpandVarstring(alias,ebuff,"");
 
 if ((ptr = (struct Strategy *)malloc(sizeof(struct Strategy))) == NULL)
    {
    FatalError("Memory Allocation failed for InstallStrategy() #1");
    }
 
 if ((ptr->name = strdup(ebuff)) == NULL)
    {
    FatalError("Memory Allocation failed in InstallStrategy");
    }

 ExpandVarstring(classes,ebuff,"");

 if ((ptr->classes = strdup(ebuff)) == NULL)
    {
    FatalError("Memory Allocation failed in InstallStrategy");
    }
 
 if (VSTRATEGYLISTTOP == NULL)                 /* First element in the list */
    {
    VSTRATEGYLIST = ptr;
    }
 else
    {
    VSTRATEGYLISTTOP->next = ptr;
    }
 
 ptr->next = NULL;
 ptr->type = 'r';
 ptr->done = 'n';
 ptr->strategies = NULL;

 VSTRATEGYLISTTOP = ptr;
}

/*****************************************************************************/

void AddClassToStrategy(char *alias,char *class,char *value)

{ struct Strategy *sp;
  char buf[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];
  int val = -1;

if (class[strlen(class)-1] != ':')
   {
   yyerror("Strategic class definition doesn't end in colon");
   return;
   }
  
memset(buf,0,CF_MAXVARSIZE);
sscanf(class,"%[^:]",&buf);
 
ExpandVarstring(value,ebuff,"");
Debug("AddClassToStrategy(%s,%s,%s)\n",alias,class,ebuff);

sscanf(ebuff,"%d",&val);

if (val <= 0)
   {
   yyerror("strategy distribution weight must be an integer");
   return;
   }
 
for (sp = VSTRATEGYLIST; sp != NULL; sp=sp->next)
   {
   if (strcmp(alias,sp->name) == 0)
      {
      if (!IsItemIn(sp->strategies,buf))
         {
         AppendItem(&(sp->strategies),buf,ebuff);
         AddInstallable(buf);
         }
      }
   }
}

/*****************************************************************************/

void SetStrategies()

{ struct Strategy *ptr;
  struct Item *ip; 
  int total,count;
  double prob,cum,fluct;

  
for (ptr = VSTRATEGYLIST; ptr != NULL; ptr=ptr->next)
   {
   if (ptr->done == 'y')
      {
      continue;
      }
   else
      {
      ptr->done = 'y';
      }
   
   Verbose("\n  Evaluating strategy %s (type=%c)\n",ptr->name,ptr->type);

   if (ptr->strategies)
      {
      total = 0;
      
      for (ip = ptr->strategies; ip !=NULL; ip=ip->next)
         {
         total += atoi(ip->classes);
         }
      
      /* Get random number 0-1 */
      
      fluct = drand48();
      
      count = 0;
      cum = 0.0;
      
      for (ip = ptr->strategies; ip != NULL; ip=ip->next)
         {
         prob = ((double)atoi(ip->classes))/((double)total);
         cum += prob;
         Debug("%s : %f cum %f\n",ip->name,prob,cum);
         Verbose("    Class %d: %f-%f\n",count,cum-prob,cum);
         if ((fluct < cum) || ip->next == NULL)
            {
            Verbose("   - Choosing %s (%f)\n",ip->name,fluct);
            AddClassToHeap(ip->name);
            break;
            }
         count++;
         }
      }
   }
}

/*****************************************************************************/



/* END */

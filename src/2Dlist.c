/* cfengine for GNU
 
        Copyright (C) 1995/6
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
/*  TOOLKIT: the "2Dlist" object library for cfengine                */
/*           uses (inherits) item.c                                  */
/*                                                                   */
/*********************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/* private */

#define TD_wrapped   1
#define TD_nowrap    2

/*********************************************************************/
/* TOOLKIT : 2D list                                                 */
/*********************************************************************/

/* The 2D structure is a list of varstring fragments each of which
   contains one variable. Each variable could be a list, and the
   list variables are expanded into the ilist members of the 2D list.

   The trick in extracting data is how the increment function works..*/

/*********************************************************************/

void Set2DList(struct TwoDimList *list)

{ struct TwoDimList *tp;

Debug1("Set2DLIst()\n");

for (tp = list; tp != NULL; tp=tp->next)
   {
   tp->current = tp->ilist;
   }
}

/*********************************************************************/

char *Get2DListEnt(struct TwoDimList *list)

   /* return a path string in static data, like getent in NIS */

{ static char entry[CF_EXPANDSIZE];
  struct TwoDimList *tp;
  char seps[2];

if (EndOfTwoDimList(list))
   {
   return NULL;
   }

Debug1("Get2DListEnt()\n");

memset(entry,0,CF_BUFSIZE);

for (tp = list; tp != NULL; tp=tp->next)
   {
   if (tp->current != NULL)
      {
      if (strlen(entry)+strlen((tp->current)->name) < CF_EXPANDSIZE - CF_BUFFERMARGIN)
         {
         strcat(entry,(tp->current)->name);
         }
      else
         {
         FatalError("Buffer overflow during variable expansion");
         }
      }
   }

Debug("Get2DListEnt returns %s\n",entry);

if (list->tied)
   {
   TieIncrementTwoDimList(list);
   }
else
   {
   IncrementTwoDimList(list);
   }

return entry;
}

/*********************************************************************/

void Build2DListFromVarstring(struct TwoDimList **TwoDimlist, char *varstring, char sep, short tied)

{ struct Item *ip, *basis;
  char *sp;

sep = '¶';
  
Debug1("Build2DListFromVarstring([%s],sep=[%c])\n",varstring,sep);

if (varstring == NULL)
   {
   AppendTwoDimItem(TwoDimlist,NULL);
   return;
   }

if (sp = strchr(varstring,sep))
   {
   if (sp == varstring || (sp > varstring && *(sp-1) != '\\'))
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Warning: varstring \"%s\" contains the list iterator \'%c\' - you should escape these close to non-separator characters so they don't get lost! (e.g. /bin/echo\\%c )",varstring,sep,sep);
      CfLog(cferror,OUTPUT,"");
      }
   }

basis = SplitVarstring(varstring);

for (ip = basis; ip != NULL; ip=ip->next)
   {
   /* Expand the list variables in each slot */
   
   AppendTwoDimItem(TwoDimlist,SplitString(ip->name,sep));
   }

if (TwoDimlist && *TwoDimlist)
   {
   /* Policy for expansion of lists */
   (*TwoDimlist)->tied = tied; 
   }
}

/*********************************************************************/

int IncrementTwoDimList (struct TwoDimList *from)

/* This works be recursive descent from left to right, like a mileometer */
    
{ struct TwoDimList *tp;

Debug1("IncrementTwoDimList()\n");

for (tp = from; tp != NULL; tp=tp->next)
   {
   if (tp->is2d)
      {
      break;  /* first fragment with more than one value in list */
      }
   }

if (tp == NULL)
   {
   return TD_wrapped;  /* End of cycle */
   }

/* Current points to our position in the sublist of each fragment */

if (IncrementTwoDimList(tp->next) == TD_wrapped)
   {
   tp->current = (tp->current)->next; 

   if (tp->current == NULL)     
      {
      /* Wrap to beginning again, count round robins to check eolist */
      tp->current = tp->ilist;
      tp->rounds++;            
      return TD_wrapped;
      }
   else
      {
      return TD_nowrap;
      }
   }

return TD_nowrap; /* Shouldn't get here */
}

/*********************************************************************/

int TieIncrementTwoDimList (struct TwoDimList *from)

{ struct TwoDimList *tp;

 /* March all pointers in time */

for (tp = from; tp != NULL; tp=tp->next)
   {
   if (tp->is2d)
      {
      tp->current = tp->current->next;

      if (tp->current == NULL)
         {
         /* Signal the round robin cycle ends */
         tp->rounds = 1;  
         }
      }
   else
      {
      tp->rounds = 1;
      }
   }

return TD_wrapped; /* Means nothing here */
}

/*********************************************************************/

int EndOfTwoDimList(struct TwoDimList *list)       /* bool */

   /* returns true if the leftmost list variable has cycled */
   /* i.e. rounds is > 0 for the first is-2d list item      */

{ struct TwoDimList *lp, *tp = NULL;
  int i = 0, done;

switch (ACTION)
   {
   case shellcommands:
       done = false;
       break;

   default:
       done = true;
   }
 
for (lp = list; lp != NULL; lp=lp->next)
   {
   if (lp->is2d)
      {
      tp = lp;
      }

   if (lp->rounds == 0)
      {
      if (!done)
         {
         break;
         }
      else
         {
         done = false;
         }
      }
   }

if (done || (list == NULL))
   {
   return true;
   }

if (tp == NULL)             /* Need a case when there are no lists! */
   {
   if (list->rounds == 0)
      {
      list->rounds = 1;
      return false;
      }
   else
      {
      return true;
      }
   }

if (tp->rounds > 0)
   {
   return true;
   }
else
   {
   return false;
   }
}

/*********************************************************************/

void AppendTwoDimItem(struct TwoDimList **liststart,struct Item *itemlist)

{ struct TwoDimList *ip, *lp;

 Debug("\nAppendTwoDimItem()\n");

if (liststart == NULL)
   {
   Debug("SOFTWARE ERROR in AppendTwoDimItem()\n ");
   return;
   }

if ((ip = (struct TwoDimList *)malloc(sizeof(struct TwoDimList))) == NULL)
   {
   CfLog(cferror,"AppendTwoDimItem","malloc");
   FatalError("");
   }

if (*liststart == NULL)
   {
   *liststart = ip;
   }
else
   {
   for (lp = *liststart; lp->next != NULL; lp=lp->next)
      {
      }
   
   lp->next = ip;
   }

ip->ilist = itemlist;
ip->current = itemlist; /* init to start of list - steps through these */
ip->next = NULL;
ip->rounds = 0;
ip->tied = 0;

if (itemlist == NULL || itemlist->next == NULL)
   {
   ip->is2d = false;
   }
else
   {
   ip->is2d = true; /* List has more than one element */
   }
}

/*********************************************************************/

void Delete2DList(struct TwoDimList *item)

{
if (item != NULL)
   {
   Delete2DList(item->next);
   item->next = NULL;

   DeleteItemList(item->ilist);
   
   free((char *)item);
   }
}

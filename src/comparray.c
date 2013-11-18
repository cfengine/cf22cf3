/* 

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
/* Compressed Arrays                                               */
/*                                                                 */
/*******************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*******************************************************************/

int FixCompressedArrayValue(int i,char *value,struct CompressedArray **start)

{ struct CompressedArray *ap;
  char *sp;

for (ap = *start; ap != NULL; ap = ap->next)
   {
   if (ap->key == i) 
      {
      /* value already fixed */
      return false;
      }
   }

Debug("FixCompressedArrayValue(%d,%s)\n",i,value);

if ((ap = (struct CompressedArray *)malloc(sizeof(struct CompressedArray))) == NULL)
   {
   CfLog(cferror,"Can't allocate memory in SetCompressedArray()","malloc");
   FatalError("");
   }

if ((sp = malloc(strlen(value)+2)) == NULL)
   {
   CfLog(cferror,"Can't allocate memory in SetCompressedArray()","malloc");
   FatalError("");
   }

strcpy(sp,value);
ap->key = i;
ap->value = sp;
ap->next = *start;
*start = ap;
return true;
}


/*******************************************************************/

void DeleteCompressedArray(struct CompressedArray *start)

{
if (start != NULL)
   {
   DeleteCompressedArray(start->next);
   start->next = NULL;

   if (start->value != NULL)
      {
      free(start->value);
      }

   free(start);
   }
}

/*******************************************************************/

int CompressedArrayElementExists(struct CompressedArray *start,int key)

{ struct CompressedArray *ap;

Debug("CompressedArrayElementExists(%d)\n",key);

for (ap = start; ap !=NULL; ap = ap->next)
   {
   if (ap->key == key)
      {
      return true;
      }
   }

return false;
}

/*******************************************************************/

char *CompressedArrayValue(struct CompressedArray *start,int key)

{ struct CompressedArray *ap;

for (ap = start; ap != NULL; ap = ap->next)
   {
   if (ap->key == key)
      {
      return ap->value;
      }
   }

return NULL;
}

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
/* Errors                                                          */
/*                                                                 */
/*******************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

void FatalError(s)

char *s;

{
fprintf (stderr,"%s:%s:%s\n",VPREFIX,VCURRENTFILE,s);
SILENT = true;
ReleaseCurrentLock();
closelog(); 
exit(1);
}

/*********************************************************************/

void Warning(char *s)

{
if (WARNINGS)
   { 
   fprintf (stderr," # %s\n",VCURRENTFILE,LINENUMBER,s);
   }
}

/*********************************************************************/

void ResetLine(char *s)

{ int c;
  int v;
  char *p;

v = 0;
while (isdigit((int)*s))
   {
   v = 10*v + *s++ - '0';
   }
LINENUMBER = v-1;

c = *s++;
while (c == ' ')
   {
   c = *s++;
   }

if (c == '"')
   {
   p = VCURRENTFILE;
   c = *s++;
   while (c && c != '"')
      {
      *p++ = c;
      c = *s++;
      }
   *p = '\0';
   }
}

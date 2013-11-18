
/* 

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
/* File: dtypes.c                                                            */
/*                                                                           */
/* Created: Sun Apr 18 17:16:51 2004                                         */
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
#include <math.h>

/*****************************************************************/

int IsSocketType(char *s)

{ int i;

for (i = 0; i < ATTR; i++)
   {
   if (strstr(s,ECGSOCKS[i].name))
      {
      Debug("IsSocketType(%s=%s)\n",s,ECGSOCKS[i].name);
      
      return true;
      }
   }
return false;
}

/*****************************************************************/

int IsTCPType(char *s)

{ int i;

for (i = 0; i < CF_NETATTR; i++)
   {
   if (strstr(s,TCPNAMES[i]))
      {
      Debug("IsTCPType(%s)\n",s); 
      return true;
      }
   }
return false;
}

/*****************************************************************/

int IsProcessType(char *s)

{ int i;

 if (strcmp(s,"procs") == 0)
    {
    return true;
    }
 
 return false;
}

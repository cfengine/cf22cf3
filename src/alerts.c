/* 

        Copyright (C) 1995-2000
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
/* File: alerts.c                                                            */
/*                                                                           */
/* Created: Sun Feb  2 20:26:21 2003                                         */
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

/*******************************************************************/

void DoAlerts()

{ struct Item *ip;
  char ebuffer[CF_EXPANDSIZE]; 

Banner("Alerts");

 
for (ip = VALERTS; ip != NULL; ip=ip->next)
   {
   if (IsDefinedClass(ip->classes))
      {
      if (!GetLock(ASUniqueName("alert"),CanonifyName(ip->name),ip->ifelapsed,ip->expireafter,VUQNAME,CFSTARTTIME))
         {
         continue;
         }
      
      if (IsBuiltinFunction(ip->name))
         {
         memset(ebuffer,0,CF_EXPANDSIZE);
         strncpy(ebuffer,EvaluateFunction(ip->name,ebuffer),CF_EXPANDSIZE-1);

         if (strlen(ebuffer) > 0)
            {
            printf("%s: %s\n",VPREFIX,ebuffer);
            }
         }
      else
         {
         ExpandVarstring(ip->name,ebuffer,NULL);
         CfLog(cferror,ebuffer,"");
         }
      
      ReleaseCurrentLock();
      }
   else
      {
      Debug("Not evaluating %s::%s\n",ip->name,ip->classes);
      }
   }
}

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


/***************************************************************/
/* Modestring toolkit                                          */
/***************************************************************/

void ParseModeString(char *modestring,mode_t *plusmask,mode_t *minusmask)

{ char *sp;
 int affected = 0, value = 0, gotaction;
 char action = '=';
 enum modestate state = unknown;
 enum modesort found_sort = unknown; /* Already found "sort" of mode */
 enum modesort sort = unknown; /* Sort of started but not yet finished mode */


if (modestring == NULL)
   {
   return;
   }

Debug1("ParseModeString(%s)\n",modestring);

gotaction = false;
*plusmask = *minusmask = 0;

for (sp = modestring; true ; sp++)
   {
   switch (*sp)
      {
      case 'a': CheckModeState(who,state,symbolic,sort,*sp);
                affected |= 07777;
                sort = symbolic;
                break;

      case 'u': CheckModeState(who,state,symbolic,sort,*sp);
                affected |= 04700;
                sort = symbolic;
                break;

      case 'g': CheckModeState(who,state,symbolic,sort,*sp);
                affected |= 02070;
                sort = symbolic;
                break;

      case 'o': CheckModeState(who,state,symbolic,sort,*sp);
                affected |= 00007;
                sort = symbolic;
                break;

      case '+':
      case '-':
      case '=': if (gotaction)
                   {
                   yyerror("Too many +-= in mode string");
                   }

        CheckModeState(who,state,symbolic,sort,*sp);
                action = *sp;
                state = which;
                gotaction = true;
                sort = unknown;
                break;

      case 'r': CheckModeState(which,state,symbolic,sort,*sp);
                value |= 0444 & affected;
                sort = symbolic;
                break;

      case 'w': CheckModeState(which,state,symbolic,sort,*sp);
                value |= 0222 & affected;
                sort = symbolic;
                break;

      case 'x': CheckModeState(which,state,symbolic,sort,*sp);
                value |= 0111 & affected;
                sort = symbolic;
                break;

      case 's': CheckModeState(which,state,symbolic,sort,*sp);
                value |= 06000 & affected;
                sort = symbolic;
                break;

      case 't': CheckModeState(which,state,symbolic,sort,*sp);
                value |= 01000;
                sort = symbolic;
                break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7': CheckModeState(which,state,numeric,sort,*sp);
                sort = numeric;
                gotaction = true;
                state = which;
                affected = 07777; /* TODO: Hard-coded; see below */
                sscanf(sp,"%o",&value);
                if (value > 07777) /* TODO: Hardcoded !
                                      Is this correct for all sorts of Unix ?
                                      What about NT ?
                                      Any (POSIX)-constants ??
                                   */
                   {
                   yyerror("Mode-Value too big !\n");
                   }
                while (isdigit((int)*sp) && (*sp != '\0'))
                   {
                   sp++;
                   }
                sp--;
                break;
                
      case ',':
          SetMask(action,value,affected,plusmask,minusmask);
          if (found_sort != unknown && found_sort != sort)
             {
             Warning("Symbolic and numeric form for modes mixed");
             }
          found_sort = sort;
          sort = unknown;
          action = '=';
          affected = 0;
          value = 0;
          gotaction = false;
          state = who;
          break;
          
      case '\0':
          if (state == who || value == 0)
             {
             if (strcmp(modestring,"0000") != 0 && strcmp(modestring,"000") != 0)
                {
                Warning("mode string is incomplete");
                }
             }
          
          SetMask(action,value,affected,plusmask,minusmask);
          if (found_sort != unknown && found_sort != sort)
             {
             Warning("Symbolic and numeric form for modes mixed");
             }
          Debug1("[PLUS=%o][MINUS=%o]\n",*plusmask,*minusmask);
          return;
          
      default: snprintf(OUTPUT,CF_BUFSIZE,"Invalid mode string (%s)",modestring);  
          yyerror (OUTPUT);
          break;
      }
   }
}

/*********************************************************/

void CheckModeState(enum modestate stateA,enum modestate stateB,enum modesort sortA,enum modesort sortB,char ch)

{
if ((stateA != wild) && (stateB != wild) && (stateA != stateB))
   {
   sprintf(VBUFF,"Mode string constant (%c) used out of context",ch);
   yyerror(VBUFF);
   }

if ((sortA != unknown) && (sortB != unknown) && (sortA != sortB))
   {
   yyerror("Symbolic and numeric filemodes mixed within expression");
   }
}

/*********************************************************/

void SetMask(char action,int value,int affected,mode_t *p,mode_t *m)

{
Debug1("SetMask(%c%o,%o)\n",action,value,affected);

switch(action)
   {
   case '+':
             *p |= value;
             *m |= 0;
             return;
   case '-':
             *p |= 0;
             *m |= value;
             return;
   case '=':
             *p |= value;
             *m |= ((~value) & 07777 & affected);
             return;
   default:
             sprintf(VBUFF,"Mode directive %c is unknown",action);
             yyerror(VBUFF);
             return;
   }
}


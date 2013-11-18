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


/*************************************************************************/
/* WILDCARD TOOLKIT : Level 0                                            */
/*************************************************************************/

#define nomatch         0
#define match           1
#define maxlen          20
#define startofstrings  10
#define middleofstrings 11
#define endofstrings    12

#define Wild(c)  ((c == '*' || c == '?') ? true : false)

#include <string.h>


/*************************************************************************/

char *FixWildcards(char *s)

{ char *spf,*spt;
  static char replace[CF_BUFSIZE];

memset(replace,0,CF_BUFSIZE);
  
for (spf = s,spt = replace; *spf != '\0'; spf++)
   {
   switch(*spf)
      {
      case '*':
          *spt++ = '.';
          *spt++ = *spf;          
          break;
      case '?':
          *spt++ = '.';
          break;
      case '.':
          *spt++ = '\\';
          *spt++ = '.';
          break;
      default:
          *spt++ = *spf;
          break;
      }
   }

return replace;
}

/*************************************************************************/

int IsWildCard (char *str)

{
return (strchr(str,'?') || strchr(str,'*'));
} 

/*************************************************************************/

int WildMatch (char *wildptr,char *cmpptr)

{ char buffer[CF_BUFSIZE];
  int i, status = startofstrings;
  char lastwild = '\0';

Debug("WildMatch(%s,%s)\n",wildptr,cmpptr);
     
if (strstr(wildptr,"*") == NULL && strstr(wildptr,"?") == NULL)
   {
   return (! strcmp(wildptr,cmpptr));
   }

while (true)
   {
   while (*wildptr == '?')                                /* 1 */
      {
      wildptr++;
      cmpptr++;
      if ((*cmpptr == '\0') && (*wildptr != '\0'))        /* 2 */
         {
         return(nomatch);
         }
      lastwild = '?';
      status = middleofstrings;
      }
   
   if (*wildptr == '\0' && *cmpptr == '\0')                /* 3 */
      {
      return(match);
         }
   else if (*wildptr == '\0')                              /* 4 */
      {
      return(nomatch);
      }
   
   if (*wildptr == '*')                                    /* 5 */
      {
      while (*wildptr == '*')                              /* 6 */
         {
         wildptr++;
         }
      if (*wildptr == '\0')                                /* 7 */
         {
         if (*cmpptr == '\0')                              /* 8 */
            {
            return(nomatch);
            }
         else
            {
            return(match);
            }
         }
      
      cmpptr++;                                            /* 9 */
      status = middleofstrings;
      lastwild = '*';
      }
   
   for (i = 0; !(Wild(*wildptr) || *wildptr == '\0'); i++) /* 10 */
      {
      buffer[i] = *wildptr++;
      if (*wildptr == '\0')                                /* 11 */
         {
         status = endofstrings;
         }
      }
   
   buffer[i] = '\0';
   
   if ((cmpptr = AfterSubString(cmpptr,buffer,status,lastwild)) == NULL)
      {
      return(nomatch);                                      /* 12 */
      }
   
   status = middleofstrings;
   }
}

/******************************************************************/
/* Wildcard Toolkit : Level 1                                     */
/******************************************************************/

char *AfterSubString(char *big,char *small,int status,char lastwild)

   /* If the last wildcard was a ? then this just tries to      */
   /* match the substrings from the present position, otherwise */
   /* looks for next occurrance of small within big and returns */
   /* a pointer to the next character in big after small or     */
   /* NULL if there is no string found to match                 */
   /* If end of strings is signalled, make sure that the string */
   /* is tied to the end of big. This makes sure that there is  */
   /* correct alignment with end of string marker.              */

{ char *bigptr;

if (strlen(small) > strlen(big))                       /* 13 */
   {
   return(NULL);
   }

if (lastwild == '?')                                   /* 14 */
   {
   if (strncmp(big,small,strlen(small)) == 0)
      {
      return(big+strlen(small));
      }
   else
      {
      return(NULL);
      }
   }
 
if (status == endofstrings)                             /* 15 */
   {
   big = big + strlen(big) - strlen(small);
   }

for (bigptr = big; *bigptr != '\0'; ++bigptr)           /* 16 */
   {
   if (strncmp(bigptr,small,strlen(small)) == 0)
      {
      return(bigptr+strlen(small));
      }
   
   if (status == startofstrings)                        /* 17 */
      {
      return(NULL);
      }
   }
 
return(NULL);                                           /* 18 */
}

/******************************************************************

Comments :

1. ? Matches a single character. Skip over with pointers.
2. Check that the compare string hasn't run out too soon.
3. If the wild-match is complete return MATCH!
4. other wise check that the wild card string hasn't run out
   too soon.

5. * Matches any string.
6. Skip over any *'s in the wildcard string and leave pointer
   on the first character after.
7. If the last character in the wildstring is *, match any
   remaining characters...
8. ...except no string at all. e.g. xyz* doesn't match xyz
9. Advancing this pointer by one, prevents * from matching
   the null string even when the pointers haven't reached
   the end of the string (so that the match is not complete).
   This works because it screws up the pattern match in
   the routine `AfterSubString()' by removing the first
   character from a string which might otherwise match.
   e.g. it stops *.c from matching .c because the strings
   which get passed to AfterSubString are `.c' (from *.c)
   and `c' (from .c).
10.Isolate the next string sandwhiched between wild-cards
   or string delimiters. Copy to buffer.
11.Make a note if we hit the end of wildstring in the process.
   i.e. is this the end of the strings we're matching?
12.The call to this routine checks whether a match is
   allowed between the isolated string and the comparison
   string. If a match is allowed it updates the pointers
   so that they point to the next characters after the
   match. Otherwise it returns NULL.

AfterSubString()

13. You can't find a string larger than big inside big!
    (This could occur because of points 15. and 9.)
14. If the last wildcard was just to match a single
    character then the string match must be anchored to
    the current pointer locations.
15. If this is the end of the strings to be matched, then
    make sure the match also correctly identifies which
    pattern matches the END of the string. e.g.
    to avoid confusion over *.abc in .abc.abc.abc
16. If the last wildcard was a * or none, then
    use a forward floating comparision which can
    skip over anyjunk characters looking for the next
    occurrance of the string.
17. If this is the first two characters in the string
    and there is no match, give up. This anchors the
    floating match to the start to avoid
    a*b matching xaxb by skipping over the first x!
18. If there is no match yet, there's no chance!

Test Data
----------

Wild String        Valid Match           No Match
------------       -----------          ----------

abcd                abcd

*mmm*               abcmmmabc            abcmmm
                                         mmm

*j                  jjjj                 jjjjx

*.c                  a.c                  .c
                     .c.c
*y                   ayyyyyyy             ya
                                           y

?a                    aa                   aaa
                      ba                   ab

???a                  aaaa                 ab
                      xyza                 xyzaa
???                   xyz                  ab
                                           abcd
*                     anything
*.*                   anything.anything
a*                    abc                   a
                                            bad
?mmm?                 ammmb                 abcmmmd
                                            mmm
a*b                   axyzb                 xyz
                      abbb                  axab

*******************************************************************/


/* EOF */

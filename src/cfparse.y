%{
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
 

/*******************************************************************/
/*                                                                 */
/*  PARSER for cfengine                                            */
/*                                                                 */
/*******************************************************************/

#include <stdio.h>
#include "cf.defs.h"
#include "cf.extern.h"

extern char *yytext;

%}

%token LVALUE ID VAROBJ LBRACK RBRACK CONTROL GROUPS
%token ARROW EQUALS EDITFILES QSTRING RVALUE BCLASS
%token LBRACE RBRACE PARSECLASS LARROW OPTION FUNCTION
%token ACL ADMIT DENY FILTERS STRATEGIES ACTIONTYPE ACCESSOBJ



%%

specification:       { yyerror("Warning: invalid statement"); }
                     | statements;

statements:            statement
                     | statements statement;

statement:             CONTROL controllist
                     | CONTROL
                     | GROUPS controllist
                     | GROUPS
                     | ACTIONTYPE classlist
                     | ACTIONTYPE
                     | EDITFILES
                     | EDITFILES objects
                     | ACL objects
                     | ACL
                     | FILTERS objects
                     | FILTERS
                     | STRATEGIES objects
                     | STRATEGIES
                     | ADMIT classaccesslist
                     | ADMIT
                     | DENY classaccesslist
                     | DENY;

controllist:           declarations
                     | PARSECLASS declarations
                     | PARSECLASS
                     | controllist PARSECLASS
                     | controllist PARSECLASS declarations; 

declarations:          declaration
                     | declarations declaration;

classlist:             entries
                     | PARSECLASS entries
                     | PARSECLASS
                     | classlist PARSECLASS
                     | classlist PARSECLASS entries;

classaccesslist:       accessentries
                     | PARSECLASS accessentries
                     | PARSECLASS
                     | classaccesslist PARSECLASS
                     | classaccesslist PARSECLASS accessentries;

declaration:           LVALUE EQUALS bracketlist;

bracketlist:           LBRACK rvalues RBRACK;

rvalues:               RVALUE
                     | FUNCTION
                     | rvalues FUNCTION
                     | rvalues RVALUE;

entries:               entry
                     | entries entry;

accessentries:         accessentry
                     | accessentries accessentry;

entry:                 FUNCTION
                     | FUNCTION options
                     | VAROBJ
                     | VAROBJ options
                     | VAROBJ ARROW VAROBJ options
                     | VAROBJ ARROW VAROBJ
                     | VAROBJ LARROW VAROBJ options
                     | VAROBJ LARROW VAROBJ
                     | QSTRING
                     | QSTRING options;

accessentry:           ACCESSOBJ
                     | ACCESSOBJ options;

options:               options OPTION
                     | OPTION;

objects:               objectbrackets
                     | PARSECLASS
                     | PARSECLASS objectbrackets
                     | objects PARSECLASS
                     | objects PARSECLASS objectbrackets;

objectbrackets:        objectbracket
                     | objectbrackets objectbracket;

objectbracket:         LBRACE VAROBJ objlist RBRACE
                     | LBRACE ID objlist RBRACE;

objlist:               obj
                     | objlist obj;

obj:                   BCLASS QSTRING
                     | ID QSTRING
                     | ID 
                     | VAROBJ;

%%

/*****************************************************************/

void yyerror(s)

char *s;

{
fprintf (stderr, "cf:%s:%s:%d: %s \n",VPREFIX,VCURRENTFILE,LINENUMBER,s);

ERRORCOUNT++;

if (ERRORCOUNT > 10)
   {
   FatalError("Too many errors");
   }
}

/*****************************************************************/

/* EOF */

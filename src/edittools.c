
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
 

/********************************************************************/
/*                                                                  */
/* EDITING of simple textfiles (Toolkit)                            */
/*                                                                  */
/********************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/********************************************************************/
/* EDIT Data structure routines                                     */
/********************************************************************/

void WrapDoEditFile(struct Edit *ptr,char *filename)

{ struct stat statbuf,statbuf2;
  char linkname[CF_BUFSIZE];
  char realname[CF_BUFSIZE];

Debug("WrapDoEditFile(%s,%s)\n",ptr->fname,filename);
  
if (lstat(filename,&statbuf) != -1)
   {
   if (S_ISLNK(statbuf.st_mode))
      {
      EditVerbose("File %s is a link, editing real file instead\n",filename);
      
      memset(linkname,0,CF_BUFSIZE);
      memset(realname,0,CF_BUFSIZE);
      
      if (readlink(filename,linkname,CF_BUFSIZE-1) == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Cannot read link %s\n",filename);
         CfLog(cferror,OUTPUT,"readlink");
         return;
         }
      
      if (linkname[0] != '/')
         {
         strcpy(realname,filename);
         ChopLastNode(realname);
         AddSlash(realname);
         }
      
      if (BufferOverflow(realname,linkname))
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"(culprit %s in editfiles)\n",filename);
         CfLog(cferror,OUTPUT,"");
         return;
         }
      
      if (stat(filename,&statbuf2) != -1)
         {
         if (statbuf2.st_uid != statbuf.st_uid)
            {
            /* Link to /etc/passwd? ouch! */
            snprintf(OUTPUT,CF_BUFSIZE*2,"Forbidden to edit a link to another user's file with privilege (%s)",filename);
            CfLog(cfinform,OUTPUT,"");
            return;
            }
         }
      
      strcat(realname,linkname);
      
      if (!FileObjectFilter(realname,&statbuf2,ptr->filters,editfiles))
         {
         Debug("Skipping filtered editfile %s\n",filename);
         return;
         }
      DoEditFile(ptr,realname);
      return;
      }
   else
      {
      if (!FileObjectFilter(filename,&statbuf,ptr->filters,editfiles))
         {
         Debug("Skipping filtered editfile %s\n",filename);
         return;
         }
      DoEditFile(ptr,filename);
      return;
      }
   }
 else
    {
    if (!FileObjectFilter(filename,&statbuf,ptr->filters,editfiles))
       {
       Debug("Skipping filtered editfile %s\n",filename);
       return;
       }
    DoEditFile(ptr,filename);
    }
}

/********************************************************************/

void DoEditFile(struct Edit *ptr,char *filename)

   /* Many of the functions called here are defined in the */
   /* item.c toolkit since they operate on linked lists    */

{ struct Edlist *ep, *loopstart, *loopend, *ThrowAbort();
  struct Item *filestart = NULL, *newlineptr = NULL;
  char currenteditscript[CF_BUFSIZE], searchstr[CF_BUFSIZE], expdata[CF_EXPANDSIZE];
  char *sp, currentitem[CF_MAXVARSIZE];
  struct stat tmpstat;
  char spliton = ':';
  int todo = 0, potentially_outstanding = false;
  FILE *loop_fp = NULL;
  int DeleteItemNotContaining(),DeleteItemNotStarting(),DeleteItemNotMatching();
  int global_replace = -1, ifel=-1,expaf=-1;
  int ifelapsed=VIFELAPSED,expireafter=VEXPIREAFTER;

Debug("DoEditFile(%s)\n",filename);
filestart = NULL;
currenteditscript[0] = '\0';
searchstr[0] = '\0';
memset(EDITBUFF,0,CF_BUFSIZE);
AUTOCREATED = false;
IMAGEBACKUP = 's';

if (IgnoredOrExcluded(editfiles,filename,ptr->inclusions,ptr->exclusions))
   {
   Debug("Skipping excluded file %s\n",filename);
   return;
   }

for (ep = ptr->actions; ep != NULL; ep=ep->next)
   {
   if (!IsExcluded(ep->classes))
      {
      todo++;
      }

   switch (ep->code)
      {
      case EditIfElapsed:
          ifel = atoi(ep->data);
          if (ifel > 0)
             {
             ifelapsed = ifel;
             }
          break;
      case EditExpireAfter:
          expaf = atoi(ep->data);
          if (expaf > 0)
             {
             expireafter = expaf;
             }
          break;
      }
   }

if (todo == 0)   /* Because classes are stored per edit, not per file */
   {
   return;
   }

if (!GetLock(ASUniqueName("editfile"),CanonifyName(filename),ifelapsed,expireafter,VUQNAME,CFSTARTTIME))
   {
   ptr->done = 'y';
   return;
   }
 
if (ptr->binary == 'y')
   {
   BinaryEditFile(ptr,filename);
   ReleaseCurrentLock();
   return;
   }

CheckEditSwitches(filename,ptr);

if (! LoadItemList(&filestart,filename))
   {
   if (ptr->warn == 'y')
      {
      CfLog(cfverbose,"File was marked for editing\n","");
      }
   
   ReleaseCurrentLock();
   return;
   }

NUMBEROFEDITS = 0;
EDITVERBOSE = VERBOSE;
CURRENTLINENUMBER = 1;
CURRENTLINEPTR = filestart;
strcpy(COMMENTSTART,"# ");
strcpy(COMMENTEND,"");
EDITGROUPLEVEL = 0;
SEARCHREPLACELEVEL = 0;
FOREACHLEVEL = 0;
loopstart = NULL;

Verbose("Begin editing %s\n",filename);
ep = ptr->actions;

while (ep != NULL)
   {
   if (IsExcluded(ep->classes))
      {
      ep = ep->next;
      potentially_outstanding = true;
      continue;
      }
   
   ExpandVarstring(ep->data,expdata,NULL);
   
   Debug2("Edit action: %s\n",VEDITNAMES[ep->code]);

   switch(ep->code)
      {
      case NoEdit:
      case EditInform:
      case LogAudit:
      case EditBackup:
      case EditLog:
      case EditUmask:
      case AutoCreate:
      case WarnIfFileMissing:
      case EditInclude:
      case EditExclude:
      case EditFilter:
      case DefineClasses:
      case ElseDefineClasses:
      case EditIfElapsed:
      case EditExpireAfter:
      case EditSplit:
      case EditRecurse:
        break;

      case EditUseShell:
        if (strcmp(expdata,"false") == 0)
           {
           ptr->useshell = 'n';
           }
        break;
        
      case DefineInGroup:
          if (EDITGROUPLEVEL < 1)
             {
             yyerror("DefineInGroup used outside edit-group");
             break;
             }
          
          for (sp = expdata; *sp != '\0'; sp++)
             {
             currentitem[0] = '\0';
             sscanf(sp,"%[^,:.]",currentitem);
             sp += strlen(currentitem);
             AddClassToHeap(currentitem);
             }
          break;
          
      case CatchAbort:
           EditVerbose("Caught Exception\n");
           break;
           
      case SplitOn:
          spliton = *(expdata);
          EditVerbose("Split lines by %c\n",spliton);
          break;
          
      case DeleteLinesStarting:
          while (DeleteItemStarting(&filestart,expdata))
             {
             }
          break;
          
      case DeleteLinesContaining:
          while (DeleteItemContaining(&filestart,expdata))
             {
             }
          break;
          
      case DeleteLinesNotStarting:
          while (DeleteItemNotStarting(&filestart,expdata))
             {
             }
          break;
          
      case DeleteLinesNotContaining:
          while (DeleteItemNotContaining(&filestart,expdata))
             {
             }
          break;
          
      case DeleteLinesStartingFileItems:
      case DeleteLinesContainingFileItems:
      case DeleteLinesMatchingFileItems:
      case DeleteLinesNotStartingFileItems:
      case DeleteLinesNotContainingFileItems:
      case DeleteLinesNotMatchingFileItems:
          
          if (!DeleteLinesWithFileItems(&filestart,expdata,ep->code))
             {
             goto abort;
             }
          break;
          
      case DeleteLinesAfterThisMatching:
          
          if ((filestart == NULL) || (CURRENTLINEPTR == NULL))
             {
             break;
             }
          else if (CURRENTLINEPTR->next != NULL)
             {
             while (DeleteItemMatching(&(CURRENTLINEPTR->next),expdata))
                {
                }
             }
          break;        
          
      case DeleteLinesMatching:
          while (DeleteItemMatching(&filestart,expdata))
             {
             }
          break;
          
      case DeleteLinesNotMatching:
          while (DeleteItemNotMatching(&filestart,expdata))
             {
             }
          break;
          
      case Append:
          AppendItem(&filestart,expdata,NULL);
          break;
          
      case AppendIfNoSuchLine:
          if (!IsItemIn(filestart,expdata))
             {
             AppendItem(&filestart,expdata,NULL);
             }
          break;
          
      case AppendIfNoSuchLinesFromFile:
          if (!AppendLinesFromFile(&filestart,expdata))
             {
             goto abort;
             }
          break;
          
      case SetLine:
          strncpy(EDITBUFF,expdata,CF_BUFSIZE);
          EditVerbose("Set current line to %s\n",EDITBUFF);
          break;
          
      case AppendIfNoLineMatching:
          
          Debug("AppendIfNoLineMatching : %s\n",EDITBUFF);
          
          if (strcmp(EDITBUFF,"") == 0)
             {
             snprintf(OUTPUT,CF_BUFSIZE*2,"SetLine not set when calling AppendIfNoLineMatching %s\n",expdata);
             CfLog(cferror,OUTPUT,"");
             break;
             }
          
          if (strcmp(expdata,"ThisLine") == 0)
             {
             if (LocateNextItemMatching(filestart,EDITBUFF) == NULL)
                {
                AppendItem(&filestart,EDITBUFF,NULL);
                }   
             
             break;
             }
          
          if (LocateNextItemMatching(filestart,expdata) == NULL)
             {
             AppendItem(&filestart,EDITBUFF,NULL);
             }
          break;
          
      case Prepend:
          PrependItem(&filestart,expdata,NULL);
          break;
          
      case PrependIfNoSuchLine:
          if (! IsItemIn(filestart,expdata))
             {
             PrependItem(&filestart,expdata,NULL);
             }
          break;
          
      case PrependIfNoLineMatching:
          
          if (strcmp(EDITBUFF,"") == 0)
             {
             snprintf(OUTPUT,CF_BUFSIZE,"SetLine not set when calling PrependIfNoLineMatching %s\n",expdata);
             CfLog(cferror,OUTPUT,"");
             break;
             }
          
          if (LocateNextItemMatching(filestart,expdata) == NULL)
             {
             PrependItem(&filestart,EDITBUFF,NULL);
             }
          break;
          
      case WarnIfNoSuchLine:
          if ((PASS == 1) && (LocateNextItemMatching(filestart,expdata) == NULL))
             {
             printf("Warning, file %s has no line matching %s\n",filename,expdata);
             }        
          break;
          
      case WarnIfLineMatching:
          if ((PASS == 1) && (LocateNextItemMatching(filestart,expdata) != NULL))
             {
             printf("Warning, file %s has a line matching %s\n",filename,expdata);
             }
          break;
          
      case WarnIfNoLineMatching:
          if ((PASS == 1) && (LocateNextItemMatching(filestart,expdata) == NULL))
             {
             printf("Warning, file %s has a no line matching %s\n",filename,expdata);
             }
          break;
          
      case WarnIfLineStarting:
          if ((PASS == 1) && (LocateNextItemStarting(filestart,expdata) != NULL))
             {
             printf("Warning, file %s has a line starting %s\n",filename,expdata);
             }
          break;
          
      case WarnIfNoLineStarting:
          if ((PASS == 1) && (LocateNextItemStarting(filestart,expdata) == NULL))
             {
             printf("Warning, file %s has no line starting %s\n",filename,expdata);
             }
          break;
          
      case WarnIfLineContaining:
          if ((PASS == 1) && (LocateNextItemContaining(filestart,expdata) != NULL))
             {
             printf("Warning, file %s has a line containing %s\n",filename,expdata);
             }
          break;
          
      case WarnIfNoLineContaining:
          if ((PASS == 1) && (LocateNextItemContaining(filestart,expdata) == NULL))
             {
             printf("Warning, file %s has no line containing %s\n",filename,expdata);
             }
          break;
          
      case SetCommentStart:
          strncpy(COMMENTSTART,expdata,CF_MAXVARSIZE);
          COMMENTSTART[CF_MAXVARSIZE-1] = '\0';
          break;
          
      case SetCommentEnd:
          strncpy(COMMENTEND,expdata,CF_MAXVARSIZE);
          COMMENTEND[CF_MAXVARSIZE-1] = '\0';
          break;
          
      case CommentLinesMatching:
          while (CommentItemMatching(&filestart,expdata,COMMENTSTART,COMMENTEND))
             {
             }
          break;
          
      case CommentLinesStarting:
          while (CommentItemStarting(&filestart,expdata,COMMENTSTART,COMMENTEND))
             {
             }
          break;
          
      case CommentLinesContaining:
          while (CommentItemContaining(&filestart,expdata,COMMENTSTART,COMMENTEND))
             {
             }
          break;
          
      case HashCommentLinesContaining:
          while (CommentItemContaining(&filestart,expdata,"# ",""))
             {
             }
          break;
          
      case HashCommentLinesStarting:
          while (CommentItemStarting(&filestart,expdata,"# ",""))
             {
             }
          break;
          
      case HashCommentLinesMatching:
          while (CommentItemMatching(&filestart,expdata,"# ",""))
             {
             }
          break;
          
      case SlashCommentLinesContaining:
          while (CommentItemContaining(&filestart,expdata,"//",""))
             {
             }
          break;
          
      case SlashCommentLinesStarting:
          while (CommentItemStarting(&filestart,expdata,"//",""))
             {
             }
          break;
          
      case SlashCommentLinesMatching:
          while (CommentItemMatching(&filestart,expdata,"//",""))
             {
             }
          break;
          
      case PercentCommentLinesContaining:
          while (CommentItemContaining(&filestart,expdata,"%",""))
             {
             }
          break;
          
      case PercentCommentLinesStarting:
               while (CommentItemStarting(&filestart,expdata,"%",""))
                  {
                  }
               break;
               
      case PercentCommentLinesMatching:
          while (CommentItemMatching(&filestart,expdata,"%",""))
             {
             }
          break;
          
      case ResetSearch:
          if (!ResetEditSearch(expdata,filestart))
             {
             printf("ResetSearch Failed in %s, aborting editing\n",filename);
             goto abort;
             }
          break;
          
      case LocateLineMatching:
          
          if (CURRENTLINEPTR == NULL)
             {
             newlineptr = NULL;
             }
          else
             {
             newlineptr = LocateItemMatchingRegExp(CURRENTLINEPTR,expdata);
             }
          
          if (newlineptr == NULL)
             {
             EditVerbose("LocateLineMatchingRegexp failed in %s, aborting editing\n",filename);
             ep = ThrowAbort(ep);
             }
          break;
          
      case InsertLine:
          if (filestart == NULL)
             {
             AppendItem(&filestart,expdata,NULL);
             }
          else
             {
             InsertItemAfter(&filestart,CURRENTLINEPTR,expdata);
             }
          break;
          
      case InsertFile:
          InsertFileAfter(&filestart,CURRENTLINEPTR,expdata);
          break;
          
      case IncrementPointer:
          if (! IncrementEditPointer(expdata,filestart))     /* edittools */
             {
             printf ("IncrementPointer failed in %s, aborting editing\n",filename);
             ep = ThrowAbort(ep);
             }
          
          break;
          
      case ReplaceLineWith:
          if (!ReplaceEditLineWith(expdata))
             {
             printf("Aborting edit of file %s\n",filename);
             continue;
             }
          break;
          
      case ExpandVariables:
          if (!ExpandAllVariables(filestart))
             {
             printf("Aborting edit of file %s\n",filename);
             continue;
             }
          break;
          
      case DeleteToLineMatching:
          if (! DeleteToRegExp(&filestart,expdata))
             {
             EditVerbose("Nothing matched DeleteToLineMatching regular expression\n");
             EditVerbose("Aborting file editing of %s.\n" ,filename);
             ep = ThrowAbort(ep);
             }
          break;
          
      case DeleteNLines:
          if (! DeleteSeveralLines(&filestart,expdata))
             {
             EditVerbose("Could not delete %s lines from file\n",expdata);
             EditVerbose("Aborting file editing of %s.\n",filename);
             ep = ThrowAbort(ep);
             }
          break;
          
      case HashCommentToLineMatching:
          if (! CommentToRegExp(&filestart,expdata,"#",""))
             {
             EditVerbose("Nothing matched HashCommentToLineMatching regular expression\n");
             EditVerbose("Aborting file editing of %s.\n",filename);
             ep = ThrowAbort(ep);
             }
          break;
          
      case PercentCommentToLineMatching:
          if (! CommentToRegExp(&filestart,expdata,"%",""))
             {
             EditVerbose("Nothing matched PercentCommentToLineMatching regular expression\n");
             EditVerbose("Aborting file editing of %s.\n",filename);
             ep = ThrowAbort(ep);
             }
          break;
          
      case CommentToLineMatching:
          if (! CommentToRegExp(&filestart,expdata,COMMENTSTART,COMMENTEND))
             {
             EditVerbose("Nothing matched CommentToLineMatching regular expression\n");
             EditVerbose("Aborting file editing of %s.\n",filename);
             ep = ThrowAbort(ep);
             }
          break;

      case UnCommentToLineMatching:
          if (! UnCommentToRegExp(&filestart,expdata,COMMENTSTART,COMMENTEND))
             {
             EditVerbose("Nothing matched UnCommentToLineMatching regular expression\n");
             EditVerbose("Aborting file editing of %s.\n",filename);
             ep = ThrowAbort(ep);
             }
          break;

      case CommentNLines:
          if (! CommentSeveralLines(&filestart,expdata,COMMENTSTART,COMMENTEND))
             {
             EditVerbose("Could not comment %s lines from file\n",expdata);
             EditVerbose("Aborting file editing of %s.\n",filename);
             ep = ThrowAbort(ep);
             }
          break;
          
      case UnCommentNLines:
          if (! UnCommentSeveralLines(&filestart,expdata,COMMENTSTART,COMMENTEND))
             {
             EditVerbose("Could not comment %s lines from file\n",expdata);
             EditVerbose("Aborting file editing of %s.\n",filename);
             ep = ThrowAbort(ep);
             }
          break;
          
      case UnCommentLinesContaining:
          while (UnCommentItemContaining(&filestart,expdata,COMMENTSTART,COMMENTEND))
             {
             }
          break;
          
      case UnCommentLinesMatching:
          while (UnCommentItemMatching(&filestart,expdata,COMMENTSTART,COMMENTEND))
             {
             }
          break;
          
      case SetScript:
          strncpy(currenteditscript, expdata, CF_BUFSIZE);
          currenteditscript[CF_BUFSIZE-1] = '\0';
          break;

      case RunScript:
          if (! RunEditScript(expdata,filename,&filestart,ptr))
             {
             printf("Aborting further edits to %s\n",filename);
             ep = ThrowAbort(ep);
             }
          break;
          
      case RunScriptIfNoLineMatching:
          if (! LocateNextItemMatching(filestart,expdata))
             {
             if (! RunEditScript(currenteditscript,filename,&filestart,ptr))
                {
                printf("Aborting further edits to %s\n",filename);
                ep = ThrowAbort(ep);
                }
             }
          break;
          
      case RunScriptIfLineMatching:
          if (LocateNextItemMatching(filestart,expdata))
             {
             if (! RunEditScript(currenteditscript,filename,&filestart,ptr))
                {
                printf("Aborting further edits to %s\n",filename);
                ep = ThrowAbort(ep);
                }
             }
          break;
          
      case EmptyEntireFilePlease:
          EditVerbose("Emptying entire file\n");
          DeleteItemList(filestart);
          filestart = NULL;
          CURRENTLINEPTR = NULL;
          CURRENTLINENUMBER=0;
          NUMBEROFEDITS++;
          break;
          
      case GotoLastLine:
          GotoLastItem(filestart);
          break;
          
      case BreakIfLineMatches:
          if (CURRENTLINEPTR == NULL || CURRENTLINEPTR->name == NULL )
             {
             EditVerbose("(BreakIfLIneMatches - no match for %s - file empty)\n",expdata);
             break;
             }
          
          if (LineMatches(CURRENTLINEPTR->name,expdata))
             {
             EditVerbose("Break! %s\n",expdata);
             goto abort;
             }
          break;
          
      case BeginGroupIfNoMatch:
          
          if (CURRENTLINEPTR == NULL || CURRENTLINEPTR->name == NULL )
             {
             EditVerbose("(Begin Group - no match for %s - file empty)\n",expdata);
             break;
             }
          
          if (LineMatches(CURRENTLINEPTR->name,expdata))
             {
             EditVerbose("(Begin Group - skipping %s)\n",expdata);
             ep = SkipToEndGroup(ep,filename);
             }
          else
             {
             EditVerbose("(Begin Group - no match for %s)\n",expdata);
             EDITGROUPLEVEL++;
             }
          break;

      case BeginGroupIfMatch:
          
          if (CURRENTLINEPTR == NULL || CURRENTLINEPTR->name == NULL )
             {
             EditVerbose("(Begin Group - no match for %s - file empty)\n",expdata);
             break;
             }
          
          if (LineMatches(CURRENTLINEPTR->name,expdata))
             {
             EditVerbose("(Begin Group - match for %s)\n",expdata);
             EDITGROUPLEVEL++;
             }
          else
             {
             EditVerbose("(Begin Group - skipping %s)\n",expdata);
             ep = SkipToEndGroup(ep,filename);
             }

          break;

          
      case BeginGroupIfNoLineMatching:
          if (LocateItemMatchingRegExp(filestart,expdata) != 0)
             {
             EditVerbose("(Begin Group - skipping %s)\n",expdata);
             ep = SkipToEndGroup(ep,filename);
             }
          else
             {
             EditVerbose("(Begin Group - no line matching %s)\n",expdata);
             EDITGROUPLEVEL++;
             }
          break;

      case BeginGroupIfLineMatching:
          if (LocateItemMatchingRegExp(filestart,expdata) == 0)
             {
             EditVerbose("(Begin Group - skipping %s)\n",expdata);
             ep = SkipToEndGroup(ep,filename);
             }
          else
             {
             EditVerbose("(Begin Group - line matching %s)\n",expdata);
             EDITGROUPLEVEL++;
             }
          break;

          
      case BeginGroupIfNoLineContaining:
          if (LocateNextItemContaining(filestart,expdata) != 0)
             {
             EditVerbose("(Begin Group - skipping, string matched)\n");
             ep = SkipToEndGroup(ep,filename);
             }
          else
             {
             EditVerbose("(Begin Group - no line containing %s)\n",expdata);
             EDITGROUPLEVEL++;
             }
          break;
          
      case BeginGroupIfLineContaining:
          if (LocateNextItemContaining(filestart,expdata) == 0)
             {
             EditVerbose("(Begin Group - skipping, string matched)\n");
             ep = SkipToEndGroup(ep,filename);
             }
          else
             {
             EditVerbose("(Begin Group - line containing %s)\n",expdata);
             EDITGROUPLEVEL++;
             }
          break;
          
      case BeginGroupIfNoSuchLine:
          if (IsItemIn(filestart,expdata))
             {
             EditVerbose("(Begin Group - skipping, line exists)\n");
             ep = SkipToEndGroup(ep,filename);
             }
          else
             {
             EditVerbose("(Begin Group - no line %s)\n",expdata);
             EDITGROUPLEVEL++;
             }
          break;
          
          
      case BeginGroupIfFileIsNewer:
          if ((!AUTOCREATED) && (!FileIsNewer(filename,expdata)))
             {
             EditVerbose("(Begin Group - skipping, file is older)\n");
             while(ep->code != EndGroup)
                {
                ep=ep->next;
                }
             }
          else
             {
             EditVerbose("(Begin Group - new file %s)\n",expdata);
             EDITGROUPLEVEL++;
             }
          break;
          
          
      case BeginGroupIfDefined:
          if (!IsExcluded(expdata))
             {
             EditVerbose("(Begin Group - class %s defined)\n", expdata);
             EDITGROUPLEVEL++;
             }
          else
             {
             EditVerbose("(Begin Group - class %s not defined - skipping)\n", expdata);
             ep = SkipToEndGroup(ep,filename);
             }
          break;
          
      case BeginGroupIfNotDefined:
          if (IsExcluded(expdata))
             {
             EDITGROUPLEVEL++;
             EditVerbose("(Begin Group - class %s not defined)\n", expdata);
             }
          else
             {
             EditVerbose("(Begin Group - class %s defined - skipping)\n", expdata);
             ep = SkipToEndGroup(ep,filename);
             }
          break;
          
      case BeginGroupIfFileExists:
          if (stat(expdata,&tmpstat) == -1)
             {
             EditVerbose("(Begin Group - file unreadable/no such file - skipping)\n");
             ep = SkipToEndGroup(ep,filename);
             }
          else
             {
             EditVerbose("(Begin Group - found file %s)\n",expdata);
             EDITGROUPLEVEL++;
             }
          break;
          
      case EndGroup:
          EditVerbose("(End Group)\n");
          EDITGROUPLEVEL--;
          break;
          
      case ReplaceAll:
          strncpy(searchstr,expdata,CF_BUFSIZE);
          global_replace = true;
          break;
          

      case ReplaceFirst:
          strncpy(searchstr,expdata,CF_BUFSIZE);
          global_replace = false;
          break;
  
      case With:
          switch(global_replace)
             {
             case true: 
                 if (!GlobalReplace(&filestart,searchstr,expdata))
                    {
                    snprintf(OUTPUT,CF_BUFSIZE*2,"Error editing file %s",filename);
                    CfLog(cferror,OUTPUT,"");
                    }
                 break;
                 
             case false: 
                 if (!SingleReplace(&filestart,searchstr,expdata))
                    {
                    snprintf(OUTPUT,CF_BUFSIZE*2,"Error editing file %s",filename);
                    CfLog(cferror,OUTPUT,"");
                    }
                 break;
                 
             default:
                 snprintf(OUTPUT,CF_BUFSIZE*2,"Internal error editing file %s: found With without finding ReplaceAll or ReplaceFirst first!!?",filename);
                 CfLog(cferror,OUTPUT,"");
             }

          global_replace = -1;
          break;
          
      case FixEndOfLine:
          DoFixEndOfLine(filestart,expdata);
          break;
          
      case AbortAtLineMatching:
          EDABORTMODE = true;
          strncpy(VEDITABORT,expdata,CF_BUFSIZE);
          break;
          
      case UnsetAbort:
          EDABORTMODE = false;
          break;
          
      case AutoMountDirectResources:
          HandleAutomountResources(&filestart,expdata);
          break;
          
      case ForEachLineIn:
          if (loopstart == NULL)
             {
             loopstart = ep;
             
             if ((loop_fp = fopen(expdata,"r")) == NULL)
                {
                EditVerbose("Couldn't open %s\n",expdata);
                while(ep->code != EndLoop) /* skip over loop */
                   {
                   ep = ep->next;
                   }
                break;
                }
             
             EditVerbose("Starting ForEach loop with %s\n",expdata);
             continue;
             }
          else
             {
             if (!feof(loop_fp))
                {
                memset(EDITBUFF,0,CF_BUFSIZE);
                
                while (ReadLine(EDITBUFF,CF_BUFSIZE,loop_fp)) /* Like SetLine */
                   {
                   if (strlen(EDITBUFF) == 0)
                      {
                      EditVerbose("ForEachLineIn skipping blank line");
                      continue;
                      }
                   break;
                   }
                if (strlen(EDITBUFF) == 0)
                   {
                   EditVerbose("EndForEachLineIn\n");
                   fclose(loop_fp);
                   loopstart = NULL;
                   while(ep->code != EndLoop)
                      {
                      ep = ep->next;
                      }
                   EditVerbose("EndForEachLineIn, set current line to: %s\n",EDITBUFF);
                   }
                
                Debug("ForeachLine: %s\n",EDITBUFF);
                }
             else
                {
                EditVerbose("EndForEachLineIn");
                
                fclose(loop_fp);
                loopstart = NULL;
                
                while(ep->code != EndLoop)
                   {
                   ep = ep->next;
                   }
                }
             }
          
          break;
          
      case EndLoop:
          loopend = ep;
          ep = loopstart;
          continue;
          
      case ReplaceLinesMatchingField:
          ReplaceWithFieldMatch(&filestart,expdata,EDITBUFF,spliton,filename);
          break;
          
      case AppendToLineIfNotContains:
          
          if (CURRENTLINEPTR == NULL)
             {
             PrependItem(&filestart,expdata,NULL);
             }
          else
             {
             AppendToLine(CURRENTLINEPTR,expdata,filename);
             }
          break;
          
      default: snprintf(OUTPUT,CF_BUFSIZE*2,"Unknown action in editing of file %s\n",filename);
          CfLog(cferror,OUTPUT,"");
          break;
      }
   
   ep = ep->next;
   }
 
abort :  
     
EditVerbose("End editing %s\n",filename);
EditVerbose(".....................................................................\n");
 
EDITVERBOSE = false;
EDABORTMODE = false;
 
if (DONTDO || CompareToFile(filestart,filename))
   {
   EditVerbose("Unchanged file: %s\n",filename);
   NUMBEROFEDITS = 0;
   }
 
if ((! DONTDO) && (NUMBEROFEDITS > 0))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Saving edit changes to file %s",filename);
   CfLog(cfinform,OUTPUT,"");
   AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_CHG);
   SaveItemList(filestart,filename,ptr->repository);
   AddEditfileClasses(ptr,true);
   }
else
   {
   AddEditfileClasses(ptr,false);
   }

ResetOutputRoute('d','d');
ReleaseCurrentLock();

DeleteItemList(filestart);

if (!potentially_outstanding)
   {
   ptr->done = 'y';
   }
}

/********************************************************************/

int IncrementEditPointer(char *str,struct Item *liststart)

{ int i,n = 0;
  struct Item *ip;

sscanf(str,"%d", &n);

if (n == 0)
   {
   printf("Illegal increment value: %s\n",str);
   return false;
   }

Debug("IncrementEditPointer(%d)\n",n);

if (CURRENTLINEPTR == NULL)  /* is prev undefined, set to line 1 */
   {
   if (liststart == NULL)
      {
      EditVerbose("cannot increment line pointer in empty file\n");
      return true;
      }
   else
      {
      CURRENTLINEPTR=liststart;
      CURRENTLINENUMBER=1;
      }
   }


if (n < 0)
   {
   if (CURRENTLINENUMBER + n < 1)
      {
      EditVerbose("pointer decrements to before start of file!\n");
      EditVerbose("pointer stopped at start of file!\n");
      CURRENTLINEPTR=liststart;
      CURRENTLINENUMBER=1;      
      return true;
      }

   i = 1;

   for (ip = liststart; ip != CURRENTLINEPTR; ip=ip->next, i++)
      {
      if (i == CURRENTLINENUMBER + n)
         {
         CURRENTLINENUMBER += n;
         CURRENTLINEPTR = ip;
  Debug2("Current line (%d) starts: %20.20s ...\n",CURRENTLINENUMBER,CURRENTLINEPTR->name);

         return true;
         }
      }
   }

for (i = 0; i < n; i++)
   {
   if (CURRENTLINEPTR->next != NULL)
      {
      CURRENTLINEPTR = CURRENTLINEPTR->next;
      CURRENTLINENUMBER++;

      EditVerbose("incrementing line pointer to line %d\n",CURRENTLINENUMBER);
      }
   else
      {
      EditVerbose("inc pointer failed, still at %d\n",CURRENTLINENUMBER);
      }
   }

Debug2("Current line starts: %20s ...\n",CURRENTLINEPTR->name);

return true;
}

/********************************************************************/

int ResetEditSearch (char *str,struct Item *list)

{ int i = 1 ,n = -1;
  struct Item *ip;

sscanf(str,"%d", &n);

if (n < 1)
   {
   printf("Illegal reset value: %s\n",str);
   return false;
   }

for (ip = list; (i < n) && (ip != NULL); ip=ip->next, i++)
   {
   }

if (i < n || ip == NULL)
   {
   printf("Search for (%s) begins after end of file!!\n",str);
   return false;
   }

EditVerbose("resetting pointers to line %d\n",n);

CURRENTLINENUMBER = n;
CURRENTLINEPTR = ip;

return true;
}

/********************************************************************/

int ReplaceEditLineWith (char *string)

{ char *sp;

if (strcmp(string,CURRENTLINEPTR->name) == 0)
   {
   EditVerbose("ReplaceLineWith - line does not need correction.\n");
   return true;
   }

if ((sp = malloc(strlen(string)+1)) == NULL)
   {
   printf("Memory allocation failed in ReplaceEditLineWith, aborting edit.\n");
   return false;
   }

EditVerbose("Replacing line %d with %10s...\n",CURRENTLINENUMBER,string);
strcpy(sp,string);
free (CURRENTLINEPTR->name);
CURRENTLINEPTR->name = sp;
NUMBEROFEDITS++;
return true;
}

/********************************************************************/

int ExpandAllVariables (struct Item *list)

{ char *sp;
  int i = 1;
  struct Item *ip;

  for (ip = list; ip != NULL; ip=ip->next, i++)
    {

    if ((sp = malloc(CF_EXPANDSIZE)) == NULL)
      {
      printf("Memory allocation failed in ExpandEditLineVariables, aborting edit.\n");
      return false;
      }

    memset( sp, 0, CF_EXPANDSIZE);
    ExpandVarstring(ip->name,sp,NULL);

    if (strcmp(sp,ip->name) == 0)
      {
      continue;
      }

    EditVerbose("Expanded line %d to %10s...\n",i,sp);
    free (ip->name);
    ip->name = sp;
    NUMBEROFEDITS++;
  }

return true;
}

/********************************************************************/

int RunEditScript (char *script,char *fname,struct Item **filestart,struct Edit *ptr)

{ FILE *pp;
  char buffer[CF_BUFSIZE];

if (script == NULL)
   {
   printf("No script defined for with SetScript\n");
   return false;
   }

if (DONTDO)
   {
   return true;
   }

if (NUMBEROFEDITS > 0)
   {
   SaveItemList(*filestart,fname,ptr->repository);
   snprintf(OUTPUT,CF_BUFSIZE,"Saved file changes to %s",fname);
   AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_CHG);
   AddEditfileClasses(ptr,true);
   }
else
   {
   AddEditfileClasses(ptr,false);
   }

DeleteItemList(*filestart);

snprintf(buffer,CF_BUFSIZE,"%s %s %s  2>&1",script,fname,CLASSTEXT[VSYSTEMHARDCLASS]);

EditVerbose("Running command: %s\n",buffer);
      
switch (ptr->useshell)
   {
   case 'y':  pp = cfpopen_sh(buffer,"r");
              break;
   default:   pp = cfpopen(buffer,"r");
              break;      
   }
  
if (pp == NULL)
   {
   printf("Edit script %s failed to open.\n",buffer);
   snprintf(OUTPUT,CF_BUFSIZE,"Failed edit script %s",buffer);
   AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_FAIL);
   return false;
   }

while (!feof(pp))   
   {
   char buffer[CF_BUFSIZE];
   
   ReadLine(buffer,CF_BUFSIZE-1,pp);

   if (!feof(pp))
      {
      EditVerbose("%s\n",buffer);
      }
   }

cfpclose(pp);

*filestart = 0;

if (! LoadItemList(filestart,fname))
   {
   if (ptr->warn == 'y')
      {
      snprintf(OUTPUT,CF_BUFSIZE,"File %s did not exist and was marked for editing");
      CfLog(cferror,OUTPUT,"");
      }
   return false;
   }

NUMBEROFEDITS = 0;
CURRENTLINENUMBER = 1;
CURRENTLINEPTR = *filestart;
return true;
}

/************************************************************/

void DoFixEndOfLine(struct Item *list,char *type)

  /* Assumes that CF_EXTRASPC macro allows enough space */
  /* in the allocated strings to add a few characters */

{ struct Item *ip;
  char *sp;
  int gotCR;

EditVerbose("Checking end of line conventions: type = %s\n",type);

if (strcmp("unix",type) == 0 || strcmp("UNIX",type) == 0)
   {
   for (ip = list; ip != NULL; ip=ip->next)
      {
      for (sp = ip->name; *sp != '\0'; sp++)
  {
  if (*sp == (char)13)
     {
     *sp = '\0';
     NUMBEROFEDITS++;
     }
  }
      }
   return;
   }

if (strcmp("dos",type) == 0 || strcmp("DOS",type) == 0)
   {
   for (ip = list; ip != NULL; ip = ip->next)
      {
      gotCR = false;
      
      for (sp = ip->name; *sp !='\0'; sp++)
         {
         if (*sp == (char)13)
            {
            gotCR = true;
            }
         }
      
      if (!gotCR)
         {
         *sp = (char)13;
         *(sp+1) = '\0';
         NUMBEROFEDITS++;
         }
      }
   return;
   }
 
printf("Unknown file format: %s\n",type);
}

/**************************************************************/

void HandleAutomountResources(struct Item **filestart,char *opts)

{ struct Mountables *mp;
  char buffer[CF_BUFSIZE];
  char *sp;

for (mp = VMOUNTABLES; mp != NULL; mp=mp->next)
   {
   for (sp = mp->filesystem; *sp != ':'; sp++)
      {
      }

   sp++;
   snprintf(buffer,CF_BUFSIZE,"%s\t%s\t%s",sp,opts,mp->filesystem);

   if (LocateNextItemContaining(*filestart,sp) == NULL)
      {
      AppendItem(filestart,buffer,"");
      NUMBEROFEDITS++;
      }
   else
      {
      EditVerbose("have a server for %s\n",sp);
      }
   }
}

/**************************************************************/

void CheckEditSwitches(char *filename,struct Edit *ptr)

{ struct stat statbuf;
  struct Edlist *ep;
  char inform, log;
  char expdata[CF_EXPANDSIZE];
  int fd;
  struct Edlist *actions = ptr->actions;
#ifdef WITH_SELINUX
  int selinux_enabled=0;
  security_context_t scontext=NULL;

  selinux_enabled = (is_selinux_enabled()>0);
#endif
  
PARSING = true;

if (INFORM)
   {
   inform = 'y';
   }
else
   {
   inform = 'n';
   }

if (LOGGING)
   {
   log = 'y';
   }
else
   {
   log = 'n';
   }


for (ep = actions; ep != NULL; ep=ep->next)
   {
   if (IsExcluded(ep->classes))
      {
      continue;
      }
   
   ExpandVarstring(ep->data,expdata,NULL);
   
   switch(ep->code)
      {
      case AutoCreate:
          if (!DONTDO)
             { mode_t mask;
             
             if (stat(filename,&statbuf) == -1)
                {
                Debug("Setting umask to %o\n",ptr->umask);
                mask=umask(ptr->umask);
#ifdef WITH_SELINUX
                if(selinux_enabled)
                    {
                    /* use default security context when creating destination file */
                    matchpathcon(filename,0,&scontext);
                    Debug("Setting SELinux context to: %s\n", scontext);
                    setfscreatecon(scontext);
                    }
#endif
                
                if ((fd = creat(filename,0644)) == -1)
                   {
                   snprintf(OUTPUT,CF_BUFSIZE*2,"Unable to create file %s\n",filename);
                   CfLog(cfinform,OUTPUT,"creat");
                   }
                else
                   {
                   AUTOCREATED = true;
                   close(fd);
                   }
                snprintf(OUTPUT,CF_BUFSIZE*2,"Creating file %s, mode %o\n",filename,(0644 & ~ptr->umask));
                CfLog(cfinform,OUTPUT,"");
                umask(mask);
#ifdef WITH_SELINUX
                if (selinux_enabled)
                    {
                    /* set create context back to default */
                    setfscreatecon(NULL);
                    freecon(scontext);
                    }
#endif
                break;
                }
             }
          else
             {
             if (stat(filename,&statbuf) == -1)
                {
                snprintf(OUTPUT,CF_BUFSIZE*2,"Should create and edit file %s, mode %o\n",filename,(0644 & ~ptr->umask));
                CfLog(cfinform,OUTPUT,"");
                }
             }
          
          break;
          
      case EditBackup:
          if (strcmp("false",ToLowerStr(expdata)) == 0 || strcmp("off",ToLowerStr(expdata)) == 0)
             {
             IMAGEBACKUP = 'n';
             }
          
          if (strcmp("single",ToLowerStr(expdata)) == 0 || strcmp("one",ToLowerStr(expdata)) == 0)
             {
             IMAGEBACKUP = 'y';
             }
          
          if (strcmp("timestamp",ToLowerStr(expdata)) == 0 || strcmp("stamp",ToLowerStr(expdata)) == 0)
             {
             IMAGEBACKUP = 's';
             }          
          break;
          
      case EditLog:
          if (strcmp(ToLowerStr(expdata),"true") == 0 || strcmp(ToLowerStr(expdata),"on") == 0)
             {
             log = 'y';
             break;
             }
          
          if (strcmp(ToLowerStr(expdata),"false") == 0 || strcmp(ToLowerStr(expdata),"off") == 0)
             {
             log = 'n';
             break;
             }

          break;
          
      case EditInform:

          if (strcmp(ToLowerStr(expdata),"true") == 0 || strcmp(ToLowerStr(expdata),"on") == 0)
             {
             inform = 'y';
             break;
             }
          
          if (strcmp(ToLowerStr(expdata),"false") == 0 || strcmp(ToLowerStr(expdata),"off") == 0)
             {
             inform = 'n';
             break;
             }

      case LogAudit:
          
          if (strcmp(ToLowerStr(expdata),"true") == 0 || strcmp(ToLowerStr(expdata),"on") == 0)
             {
             ptr->logaudit = 'y';
             break;
             }
          
          if (strcmp(ToLowerStr(expdata),"false") == 0 || strcmp(ToLowerStr(expdata),"off") == 0)
             {
             ptr->logaudit = 'n';
             break;
             }

          break;
      }
   }

PARSING = false; 
ResetOutputRoute(log,inform);
}


/**************************************************************/
/* Level 3                                                    */
/**************************************************************/

void AddEditfileClasses (struct Edit *list,int editsdone)

{ char *sp, currentitem[CF_MAXVARSIZE];
  struct Edlist *ep;

if (editsdone)
   {
   for (ep = list->actions; ep != NULL; ep=ep->next)
      {
      if (IsExcluded(ep->classes))
         {
         continue;
         }
      
      if (ep->code == DefineClasses)
         {
         Debug("AddEditfileClasses(%s)\n",ep->data);
         
         for (sp = ep->data; *sp != '\0'; sp++)
            {
            currentitem[0] = '\0';
            
            sscanf(sp,"%[^,:.]",currentitem);
            
            sp += strlen(currentitem);
            
            AddClassToHeap(currentitem);
            }
         }
      }
   }
 else
    {
    for (ep = list->actions; ep != NULL; ep=ep->next)
       {
       if (IsExcluded(ep->classes))
          {
          continue;
          }
       
       if (ep->code == ElseDefineClasses)
          {
          Debug("Entering AddEditfileClasses(%s)\n",ep->data);

          sp = ep->data; 
          while(*sp != '\0')
             {
             currentitem[0] = '\0';
             
             sscanf(sp,"%[^,:.]",currentitem);
             
             sp += strlen(currentitem);
             
             AddClassToHeap(currentitem);

             if (strlen(sp) > 0)
                {
                sp++;
                }
             }
          }
       }
    }
 
 if (ep == NULL)
    {
    return;
    }
 
}

/**************************************************************/

int DeleteLinesWithFileItems(struct Item **filestart,char *infile,enum editnames code)

{ struct Item *ip,*ipc,*infilelist = NULL; 
  int slen, matches=0;
  int positive=false;
  
if (!LoadItemList(&infilelist,infile))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Cannot open file iterator %s in editfiles - aborting editing",infile);
   CfLog(cferror,OUTPUT,"");
   return false;
   }
 
 for (ip = *filestart; ip != NULL; ip = ip->next)
   {
   Verbose("Looking at line with %s\n",ip->name);
   positive = false;
   
   switch (code)
      {
      case DeleteLinesStartingFileItems:
          positive = true;

      case DeleteLinesNotStartingFileItems:
          matches = 0;
          
          for (ipc = infilelist; ipc != NULL; ipc=ipc->next)
             {
             Chop(ipc->name);
             slen = IntMin(strlen(ipc->name),strlen(ip->name));  
             if (strncmp(ipc->name,ip->name,slen) == 0)
                {
                Debug("Matched with %s\n",ipc->name);
                matches++;
                }
             }
          
          if (positive && (matches > 0))
             {
             Debug("%s POS matched %s\n",VEDITNAMES[code],ip->name);
             DeleteItem(filestart,ip);      
             }
          else if (!positive && matches == 0)
             {
             Debug("%s NEG matched %s\n",VEDITNAMES[code],ip->name);
             DeleteItem(filestart,ip);
             }
          
          break;
          
      case DeleteLinesContainingFileItems:
          positive = true;

      case DeleteLinesNotContainingFileItems:
          matches = 0;
          
          for (ipc = infilelist; ipc != NULL; ipc=ipc->next)
             {
             Chop(ipc->name);
             
             if (strstr(ipc->name,ip->name) == 0)
                {
                matches++;
                }
             else if(strstr(ip->name,ipc->name) == 0)
                {
                matches++;
                }
             }
          
          if (positive && (matches > 0))
             {
             Debug("%s matched %s\n",VEDITNAMES[code],ip->name);
             DeleteItem(filestart,ip);      
             }
          else if (!positive &&matches == 0)
             {
             Debug("%s matched %s\n",VEDITNAMES[code],ip->name);
             DeleteItem(filestart,ip);
             }
          
          break;
          
      case DeleteLinesMatchingFileItems:
          positive = true;
      case DeleteLinesNotMatchingFileItems:
          Verbose("%s not implemented (yet)\n",VEDITNAMES[code]);
          break;
          
      }
   }
 
 DeleteItemList(infilelist); 
 return true;
}

/**************************************************************/

int AppendLinesFromFile(struct Item **filestart,char *filename)

{ FILE *fp;
  char buffer[CF_BUFSIZE]; 

 if ((fp=fopen(filename,"r")) == NULL)
    {
    snprintf(OUTPUT,CF_BUFSIZE,"Editfiles could not read file %s\n",filename);
    CfLog(cferror,OUTPUT,"fopen");
    return false;
    }

 while (!feof(fp))
    {
    buffer[0] = '\0';
    fgets(buffer,CF_BUFSIZE-1,fp);
    Chop(buffer);
    if (!IsItemIn(*filestart,buffer))
       {
       AppendItem(filestart,buffer,NULL);
       }
    }

 fclose(fp);
 return true;
}

/**************************************************************/

struct Edlist *ThrowAbort(struct Edlist *from)

{ struct Edlist *ep, *last = NULL;
 
for (ep = from; ep != NULL; ep=ep->next)
   {
   if (ep->code == CatchAbort)
      {
      return ep;
      }
   
   last = ep;
   }

return last; 
}

/**************************************************************/

struct Edlist *SkipToEndGroup(struct Edlist *ep,char *filename)

{ int level = -1;
 
while(ep != NULL)
   {
   switch (ep->code)
      {
      case BeginGroupIfMatch:
      case BeginGroupIfNoMatch:
      case BeginGroupIfNoLineMatching:
      case BeginGroupIfLineMatching:
      case BeginGroupIfNoSuchLine:
      case BeginGroupIfFileIsNewer:
      case BeginGroupIfFileExists:
      case BeginGroupIfLineContaining:
      case BeginGroupIfNoLineContaining:
      case BeginGroupIfDefined:
      case BeginGroupIfNotDefined:
          level ++;
      }

   Debug("   skip: %s (%d)\n",VEDITNAMES[ep->code],level);

   if (ep->code == EndGroup)
       {
       if (level == 0)
          {
          return ep;
          }
       level--;
       }
   
   if (ep->next == NULL)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Missing EndGroup in %s",filename);
      CfLog(cferror,OUTPUT,"");
      break;
      }
   
   ep=ep->next;
   }
 
 return ep;
}

/**************************************************************/

int BinaryEditFile(struct Edit *ptr,char *filename)

{ char expdata[CF_EXPANDSIZE],search[CF_BUFSIZE];
  struct Edlist *ep;
  struct stat statbuf;
  void *memseg;

EditVerbose("Begin (binary) editing %s\n",filename);

NUMBEROFEDITS = 0;
search[0] = '\0'; 
 
if (stat(filename,&statbuf) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't stat %s\n",filename);
   CfLog(cfverbose,OUTPUT,"stat");
   return false;
   }

if ((EDITBINFILESIZE != 0) &&(statbuf.st_size > EDITBINFILESIZE))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"File %s is bigger than the limit <editbinaryfilesize>\n",filename);
   CfLog(cfinform,OUTPUT,"");
   return(false);
   }

if (! S_ISREG(statbuf.st_mode))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"%s is not a plain file\n",filename);
   CfLog(cfinform,OUTPUT,"");
   return false;
   }

if ((memseg = malloc(statbuf.st_size+CF_BUFFERMARGIN)) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Unable to load file %s into memory",filename);
   CfLog(cferror,OUTPUT,"malloc");
   return false;
   }
 
LoadBinaryFile(filename,statbuf.st_size,memseg);

ep = ptr->actions;

while (ep != NULL)
   {
   if (IsExcluded(ep->classes))
      {
      ep = ep->next;
      continue;
      }

   ExpandVarstring(ep->data,expdata,NULL);
   
   Debug2("Edit action: %s\n",VEDITNAMES[ep->code]);

   switch(ep->code)
      {
      case WarnIfContainsString:
          WarnIfContainsRegex(memseg,statbuf.st_size,expdata,filename);
          break;
          
      case WarnIfContainsFile:
          WarnIfContainsFilePattern(memseg,statbuf.st_size,expdata,filename);
          break;
          
      case EditMode:
          break;
          
      case ReplaceAll:
          Debug("Replace %s\n",expdata);
          strncpy(search,expdata,CF_BUFSIZE);
          break;
          
      case With:
          if (strcmp(expdata,search) == 0)
             {
             Verbose("Search and replace patterns are identical in binary edit %s\n",filename);
             break;
             }
          
          if (BinaryReplaceRegex(memseg,statbuf.st_size,search,expdata,filename))
             {
             NUMBEROFEDITS++;
             }
          break;
          
      default:
          snprintf(OUTPUT,CF_BUFSIZE*2,"Cannot use %s in a binary edit (%s)",VEDITNAMES[ep->code],filename);
          CfLog(cferror,OUTPUT,"");
      }
   
   ep=ep->next;
   }
 
 if ((! DONTDO) && (NUMBEROFEDITS > 0))
    {
    SaveBinaryFile(filename,statbuf.st_size,memseg,ptr->repository);
    AddEditfileClasses(ptr,true);
    }
 else
    {
    AddEditfileClasses(ptr,false);
    }
 
 free(memseg);
 EditVerbose("End editing %s\n",filename);
 EditVerbose(".....................................................................\n"); 
 return true;
}


/**************************************************************/
/* Level 4                                                    */
/**************************************************************/

int LoadBinaryFile(char *source,off_t size,void *memseg)

{ int sd,n_read;
  char buf[CF_BUFSIZE];
  off_t n_read_total = 0;
  char *ptr;

Debug("LoadBinaryFile(%s,%d)\n",source,size);
  
if ((sd = open(source,O_RDONLY)) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Can't copy %s!\n",source);
   CfLog(cfinform,OUTPUT,"open");
   return false;
   }

ptr = memseg;
 
while (true)
   {
   if ((n_read = read (sd,buf,CF_BUFSIZE)) == -1)
      {
      if (errno == EINTR) 
         {
         continue;
         }

      close(sd);
      return false;
      }

   memcpy(ptr,buf,n_read);
   ptr += n_read;

   if (n_read < CF_BUFSIZE)
      {
      break;
      }

   n_read_total += n_read;
   }

close(sd);
return true;
}

/**************************************************************/

int SaveBinaryFile(char *file,off_t size,void *memseg,char *repository)

 /* If we do this, we screw up checksums anyway, so no need to
    preserve unix holes here ...*/

{ int dd;
  char new[CF_BUFSIZE],backup[CF_BUFSIZE];

Debug("SaveBinaryFile(%s,%d)\n",file,size);
Verbose("Saving %s\n",file);
 
strcpy(new,file);
strcat(new,CF_NEW);
strcpy(backup,file);
strcat(backup,CF_EDITED);
 
unlink(new);  /* To avoid link attacks */
 
if ((dd = open(new,O_WRONLY|O_CREAT|O_TRUNC|O_EXCL, 0600)) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Copy %s security - failed attempt to exploit a race? (Not copied)\n",file);
   CfLog(cfinform,OUTPUT,"open");
   return false;
   }
 
cf_full_write (dd,(char *)memseg,size);

close(dd);

if (! IsItemIn(VREPOSLIST,new))
   {
   if (rename(file,backup) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Error while renaming backup %s\n",file);
      CfLog(cferror,OUTPUT,"rename ");
      unlink(new);
      return false;
      }
   else if (Repository(backup,repository))
      {
      if (rename(new,file) == -1)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Error while renaming %s\n",file);
         CfLog(cferror,OUTPUT,"rename");
         return false;
         }       
      unlink(backup);
      return true;
      }
   }
 
if (rename(new,file) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Error while renaming %s\n",file);
   CfLog(cferror,OUTPUT,"rename");
   return false;
   }       

return true;
}

/**************************************************************/

void WarnIfContainsRegex(void *memseg,off_t size,char *data,char *filename)

{ off_t sp;
  regex_t rx,rxcache;
  regmatch_t pmatch;

Debug("WarnIfContainsRegex(%s)\n",data); 

for (sp = 0; sp < (off_t)(size-strlen(data)); sp++)
   {
   if (memcmp((char *) memseg+sp,data,strlen(data)) == 0)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"WARNING! File %s contains literal string %.255s",filename,data);
      CfLog(cferror,OUTPUT,"");
      return;
      }
   }

if (CfRegcomp(&rxcache,data,REG_EXTENDED) != 0)
   {
   return;
   }

for (sp = 0; sp < (off_t)(size-strlen(data)); sp++)
   {
   memcpy(&rx,&rxcache,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */

   if (regexec(&rx,(char *)memseg+sp,1,&pmatch,0) == 0)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"WARNING! File %s contains regular expression %.255s",filename,data);
      CfLog(cferror,OUTPUT,"");
      regfree(&rx);
      return;
      }
   }
}

/**************************************************************/

void WarnIfContainsFilePattern(void *memseg,off_t size,char *data,char *filename)

{ off_t sp;
  struct stat statbuf; 
  char *pattern;
  
Debug("WarnIfContainsFile(%s)\n",data); 

if (stat(data,&statbuf) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"File %s cannot be opened",data);
   CfLog(cferror,OUTPUT,"stat");
   return;
   }

if (! S_ISREG(statbuf.st_mode))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"File %s cannot be used as a binary pattern",data);
   CfLog(cferror,OUTPUT,"");
   return;
   }

if (statbuf.st_size > size)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"File %s is larger than the search file, ignoring",data);
   CfLog(cfinform,OUTPUT,"");
   return;
   }

if ((pattern = malloc(statbuf.st_size+CF_BUFFERMARGIN)) == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"File %s cannot be loaded",data);
   CfLog(cferror,OUTPUT,"");
   return;
   }

if (!LoadBinaryFile(data,statbuf.st_size,pattern))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"File %s cannot be opened",data);
   CfLog(cferror,OUTPUT,"stat");
   return;
   }
 
for (sp = 0; sp < (off_t)(size-statbuf.st_size); sp++)
   {
   if (memcmp((char *) memseg+sp,pattern,statbuf.st_size-1) == 0)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"WARNING! File %s contains the contents of reference file %s",filename,data);
      CfLog(cferror,OUTPUT,"");
      free(pattern);
      return;
      }
   }
 
free(pattern);
}


/**************************************************************/

int BinaryReplaceRegex(void *memseg,off_t size,char *search,char *replace,char *filename)

{ off_t sp,spr;
  regex_t rx,rxcache;
  regmatch_t pmatch;
  int match = false;
  
Debug("BinaryReplaceRegex(%s,%s,%s)\n",search,replace,filename); 

if (CfRegcomp(&rxcache,search,REG_EXTENDED) != 0)
   {
   return false;
   }

for (sp = 0; sp < (off_t)(size-strlen(replace)); sp++)
   {
   memcpy(&rx,&rxcache,sizeof(rx)); /* To fix a bug on some implementations where rx gets emptied */

   if (regexec(&rx,(char *)(sp+(char *)memseg),1,&pmatch,0) == 0)
      {
      if (pmatch.rm_eo-pmatch.rm_so < strlen(replace))
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Cannot perform binary replacement: string doesn't fit in %s",filename);
         CfLog(cfverbose,OUTPUT,"");
         }
      else
         {
         Verbose("Replacing [%s] with [%s] at %d\n",search,replace,sp);
         match = true;
         
         strncpy((char *)memseg+sp+(off_t)pmatch.rm_so,replace,strlen(replace));
         
         Verbose("Padding character is %c\n",PADCHAR);

 	 if ( pmatch.rm_so + strlen(replace) - pmatch.rm_eo >= pmatch.rm_eo )
            {
            Verbose("ReplaceAll: replacement is smaller than match: padding replacement with %d chars!! (pad char is \"%c\")\n",pmatch.rm_eo - ( pmatch.rm_so + strlen(replace)), PADCHAR);
            }
         
         for (spr = (pmatch.rm_so+strlen(replace)); spr < pmatch.rm_eo; spr++)
            {
            *((char *)memseg+spr+sp) = PADCHAR; /* default space */
            }
         
         sp += pmatch.rm_eo - pmatch.rm_so - 1;
         }
      }
   else
      {
      sp += strlen(replace);
      }
   }
 
regfree(&rx);
return match; 
}


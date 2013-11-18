/* cfengine for GNU
 
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


/*******************************************************************/
/*                                                                 */
/* Routines which install actions parsed by the parser             */
/*                                                                 */
/* Derived from parse.c (Parse object)                             */
/*                                                                 */
/*******************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*******************************************************************/

void InstallControlRValue(char *lvalue,char *varvalue)

{ int number = -1;
  char buffer[CF_MAXVARSIZE], command[CF_MAXVARSIZE], *sp;
  char value[CF_EXPANDSIZE];

//ExpandVarstring(varvalue,value,NULL);
  strcpy(value,varvalue);

if (ScanVariable(lvalue) == cfautodef)
   {
   AppendItems(&VAUTODEFINE,value,CLASSBUFF);
   return;
   }

/*
if (!IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s=%s, no match (%s)\n",lvalue,varvalue,CLASSBUFF);
   return;
   }
*/

if (strcmp(varvalue,CF_NOCLASS) == 0)
   {
   Debug1("Not installing %s, evaluated to false\n",varvalue);
   return;
   }


/* end version 1 compat */ 
 
/* Actionsequence needs to be dynamical here, so make exception - should be IsInstallable?? */
 
if ((ScanVariable(lvalue) != cfactseq) && !IsDefinedClass(CLASSBUFF))
   {
   Debug("Class %s not defined, not defining\n",CLASSBUFF);
   //return;
   }
else
   {
   Debug1("Assign variable [%s=%s] when %s)\n",lvalue,value,CLASSBUFF);
   }
 
 switch (ScanVariable(lvalue))
    {
    case cfsite:
    case cffaculty:
        if (!IsDefinedClass(CLASSBUFF))
           {
           break;
           }
        
        if (VFACULTY[0] != '\0')
           {
//           yyerror("Multiple declaration of variable faculty / site");
//           FatalError("Redefinition of basic system variable");
           printf(" # vars: \"faculty\" string => \"%s\"\n",VFACULTY);
           }
        
        strcpy(VFACULTY,value);
        break;
        
    case cfdomain:
        if (!IsDefinedClass(CLASSBUFF))
           {
           break;
           }
        
        if (strlen(value) > 0)
           {
           strcpy(VDOMAIN,value);
           DeleteClassFromHeap("undefined_domain");
           }
        else
           {
           yyerror("domain is empty");
           }
        
        if (!StrStr(VSYSNAME.nodename,VDOMAIN))
           {
           snprintf(VFQNAME,CF_BUFSIZE,"%s.%s",VSYSNAME.nodename,ToLowerStr(VDOMAIN));
           strcpy(VUQNAME,VSYSNAME.nodename);
           }
        else
           {
           int n = 0;
           strcpy(VFQNAME,VSYSNAME.nodename);
           
           while(VSYSNAME.nodename[n++] != '.')
              {
              }
           
           strncpy(VUQNAME,VSYSNAME.nodename,n-1);        
           }
        
        if (! NOHARDCLASSES)
           {
           char *ptr;
           if (strlen(VFQNAME) > CF_MAXVARSIZE-1)
              {
              FatalError("The fully qualified name is longer than CF_MAXVARSIZE!!");
              }
           
           strcpy(buffer,VFQNAME);
           
           AddClassToHeap(CanonifyName(buffer));
           /* Add some domain hierarchy classes */
           for (ptr=VFQNAME; *ptr != '\0'; ptr++)
              {
              if (*ptr == '.')
                 {
                 if (*(ptr+1) != '\0')
                    {
                    Debug("Defining domain #%s#\n",(ptr+1));
                    AddClassToHeap(CanonifyName(ptr+1));
                    }
                 else
                    {
                    Debug("Domain rejected\n");
                    }      
                 }
              }
           }
        
        break;

        
    case cfsysadm:  /* Can be redefined */

        printf(" # vars: %s:: \"email_from\" string => \"%s\";\n",CLASSBUFF,value);
        strcpy(VSYSADM,value);
        break;
                  
    case cfnetmask:

        if (!IsDefinedClass(CLASSBUFF))
           {
           break;
           }
        
        if (VNETMASK[0] != '\0')
           {
//           yyerror("Multiple declaration of variable netmask");
//           FatalError("Redefinition of basic system variable");
           printf(" # vars: \"netmask\" string => \"%s\"\n",VFACULTY);
           }
        strcpy(VNETMASK,value);
        AddNetworkClass(VNETMASK);
        break;
        
        
    case cfmountpat:
        SetMountPath(value);
        break;
        
    case cfrepos:
        SetRepository(value);
        break;
        
    case cfhomepat:  
        Debug1("Installing %s as home pattern\n",value);
        AppendItem(&VHOMEPATLIST,value,CLASSBUFF);
        break;
        
    case cfextension:
        AppendItems(&EXTENSIONLIST,value,CLASSBUFF);
        break;
        
    case cfsuspicious:
        AppendItems(&SUSPICIOUSLIST,value,CLASSBUFF);
        break;
        
    case cfschedule:
        AppendItems(&SCHEDULE,value,CLASSBUFF);
        break;
        
    case cfspooldirs:
        AppendItem(&SPOOLDIRLIST,value,CLASSBUFF);
        break;
        
    case cfmethodpeers:
        AppendItems(&VRPCPEERLIST,value,CLASSBUFF);
        break;
        
    case cfnonattackers:
        AppendItems(&NONATTACKERLIST,value,CLASSBUFF);
        break;
    case cfmulticonn:
        AppendItems(&MULTICONNLIST,value,CLASSBUFF);
        break;
    case cftrustkeys:
        AppendItems(&TRUSTKEYLIST,value,CLASSBUFF);
        break;

    case cfabortclasses:
        AppendItem(&ABORTHEAP,value,CLASSBUFF);
        break;

    case cfignoreinterfaceregex:
        DeleteInterfaceInfo(value);
        break;
        
    case cfdynamic:
        AppendItems(&DHCPLIST,value,CLASSBUFF);
        break;
    case cfallowusers:
        AppendItems(&ALLOWUSERLIST,value,CLASSBUFF);
        break;    
    case cfskipverify:
        AppendItems(&SKIPVERIFY,value,CLASSBUFF);
        break;
    case cfredef:
        AppendItems(&VREDEFINES,value,CLASSBUFF);
        break;  
    case cfattackers:
        AppendItems(&ATTACKERLIST,value,CLASSBUFF);
        break;
        
    case cftimezone:
        if (!IsDefinedClass(CLASSBUFF))
           {
           break;
           }
        
        AppendItem(&VTIMEZONE,value,NULL);
        break;
        
    case cfssize: 
        sscanf(value,"%d",&number);
        if (number >= 0)
           {
           SENSIBLEFSSIZE = number;
           }
        else
           {
           yyerror("Silly value for sensiblesize (must be positive integer)");
           }
        break;
        
    case cfscount:
        sscanf(value,"%d",&number);
        if (number > 0)
           {
           SENSIBLEFILECOUNT = number;
           }
        else
           {
           yyerror("Silly value for sensiblecount (must be positive integer)");
           }
        
        break;
        
    case cfeditsize:
        sscanf(value,"%d",&number);
        
        if (number >= 0)
           {
           EDITFILESIZE = number;
           }
        else
           {
           yyerror("Silly value for editfilesize (must be positive integer)");
           }
        
        break;
        
    case cfbineditsize:
        sscanf(value,"%d",&number);
        
        if (number >= 0)
           {
           EDITBINFILESIZE = number;
           }
        else
           {
           yyerror("Silly value for editbinaryfilesize (must be positive integer)");
           }
        
        break;
        
    case cfifelapsed:
        sscanf(value,"%d",&number);
        
        if (number >= 0)
           {
           VDEFAULTIFELAPSED = VIFELAPSED = number;
           }
        else
           {
           yyerror("Silly value for IfElapsed");
           }
        
        break;
        
    case cfexpireafter:
        sscanf(value,"%d",&number);
        
        if (number > 0)
           {
           VDEFAULTEXPIREAFTER = VEXPIREAFTER = number;
           }
        else
           {
           yyerror("Silly value for ExpireAfter");
           }
        
        break;
        
    case cfactseq:
        AppendToActionSequence(value);
        break;
        
    case cfaccess:
        AppendToAccessList(value);
        break;
        
    case cfnfstype:
        strcpy(VNFSTYPE,value); 
        break;
        
    case cfmethodname:
        if (strcmp(METHODNAME,"cf-nomethod") != 0)
           {
           yyerror("Redefinition of method name");
           }
        strncpy(METHODNAME,value,CF_BUFSIZE-1);
        SetContext("private-method");
        break;
        
    case cfarglist:
        AppendItem(&METHODARGS,value,CLASSBUFF);
        break;
        
    case cfaddclass:
        AddMultipleClasses(value);
        break;
        
    case cfinstallclass:
        AddInstallable(value);
        break;
        
    case cfexcludecp:
        PrependItem(&VEXCLUDECOPY,value,CLASSBUFF);
        break;
        
    case cfsinglecp:
        PrependItem(&VSINGLECOPY,value,CLASSBUFF);

        if ((strcmp(value,"on")==0) || (strcmp(value,"true")==0) || (strcmp(value,"*")==0))
           {
           ALL_SINGLECOPY = true;
           }
       break; 

        break;
        
    case cfexcludeln:
        PrependItem(&VEXCLUDELINK,value,CLASSBUFF);
        
        break;
        
    case cfcplinks:
        PrependItem(&VCOPYLINKS,value,CLASSBUFF);
        break;
    case cflncopies:
        PrependItem(&VLINKCOPIES,value,CLASSBUFF);
        break;
        
    case cfrepchar:
        if (strlen(value) > 1)
           {
           yyerror("reposchar can only be a single letter");
           break;
           }
        if (value[0] == '/')
           {
           yyerror("illegal value for reposchar");
           break;
           }
        REPOSCHAR = value[0];
        break;
        
    case cflistsep:
        if (strlen(value) > 1)
           {
           yyerror("listseparator can only be a single letter");
           break;
           }
        if (value[0] == '/')
           {
           yyerror("illegal value for listseparator");
           break;
           }
        LISTSEPARATOR = value[0];
        break;
        
    case cfunderscore:
        if (strcmp(value,"on") == 0)
           { char rename[CF_MAXVARSIZE];
           UNDERSCORE_CLASSES=true;
           Verbose("Resetting classes using underscores...\n");
           while(DeleteItemContaining(&VHEAP,CLASSTEXT[VSYSTEMHARDCLASS]))
              {
              }
           
           sprintf(rename,"_%s",CLASSTEXT[VSYSTEMHARDCLASS]);
           
           AddClassToHeap(rename);
           break;
           }
        
        if (strcmp(value,"off") == 0)
           {
           UNDERSCORE_CLASSES=false;
           break;
           }
        
        yyerror("illegal value for underscoreclasses");
        break;
        
    case cfifname:
        if (strlen(value)>15)
           {
           yyerror("Silly interface name, (should be something link eth0)");
           }
        
        strncpy(VIFNAMEOVERRIDE,value,15);
        VIFDEV[VSYSTEMHARDCLASS] = VIFNAMEOVERRIDE; /* override */
        Debug("Overriding interface with %s\n",VIFDEV[VSYSTEMHARDCLASS]);
        break;
        
    case cfdefcopy:
        if (strcmp(value,"ctime") == 0)
           {
           DEFAULTCOPYTYPE = 't';
           return;
           }
        else if (strcmp(value,"mtime") == 0)
           {
           DEFAULTCOPYTYPE = 'm';
           return;
           }
        else if (strcmp(value,"checksum")==0 || strcmp(value,"sum") == 0)
           {
           DEFAULTCOPYTYPE = 'c';
           return;
           }
        else if (strcmp(value,"byte")==0 || strcmp(value,"binary") == 0)
           {
           DEFAULTCOPYTYPE = 'b';
           return;
           }
        yyerror("Illegal default copy type");
        break;
        
    case cfdefpkgmgr:
        DEFAULTPKGMGR = GetPkgMgr(value);
        break;
        
    default:

        AddMacroValue(CONTEXTID,lvalue,value);
        break;
    }
}

/*******************************************************************/

void HandleEdit(char *file,char *edit,char *string)      /* child routines in edittools.c */

{
/*if (! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing Edit no match\n");
   return;
   }
*/
 
if (string == NULL)
   {
   Debug1("Handling Edit of %s, action [%s] with no data if (%s)\n",file,edit,CLASSBUFF);
   }
else
   {
   Debug1("Handling Edit of %s, action [%s] with data <%s> if (%s)\n",file,edit,string,CLASSBUFF);
   }

if (EditFileExists(file))
   {
   AddEditAction(file,edit,string);
   }
else
   {
   InstallEditFile(file,edit,string);
   }
}

/********************************************************************/

void HandleOptionalFileAttribute(char *item)

{ char value[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];

value[0] = '\0';

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(ebuff,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("File attribute with no value");
   }

Debug1("HandleOptionalFileAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfrecurse: HandleRecurse(value);
                   break;
   case cfmode:    ParseModeString(value,&PLUSMASK,&MINUSMASK);
                   strcpy(THISMODE,value);
                   break;                   
   case cfflags:   ParseFlagString(value,&PLUSFLAG,&MINUSFLAG);
                   break;     
   case cfowner:
       if (strlen(value) < CF_BUFSIZE)
          {
          strcpy(VUIDNAME,value);
          }
       else
          {
          yyerror("Too many owners");
          }
       break;

   case cfgroup:
       if (strlen(value) < CF_BUFSIZE)
          {
          strcpy(VGIDNAME,value);
          }
       else
          {
          yyerror("Too many groups");
          }
       break;

   case cfaction:  FILEACTION = (enum fileactions) GetFileAction(value);
                   break;
   case cflinks:   HandleTravLinks(value);
                   break;
   case cfexclude: DeleteSlash(value);
                   PrependItem(&VEXCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cfinclude: DeleteSlash(value);
                   PrependItem(&VINCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cfignore:  PrependItem(&VIGNOREPARSE,value,CF_ANYCLASS);
                   break;
   case cfacl:     PrependItem(&VACLBUILD,value,CF_ANYCLASS);
                   break;
   case cffilter:  PrependItem(&VFILTERBUILD,value,CF_ANYCLASS);
                   break;     
   case cfdefine:  HandleDefine(value);
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;
   case cfsetaudit: HandleCharSwitch("audit",value,&AUDITP);
                   break;
   case cfchksum:  HandleChecksum(value);
                   break;
   case cfxdev:    HandleCharSwitch("xdev",value,&XDEV);
                   break;
   case cfrxdirs:  HandleCharSwitch("rxdirs",value,&RXDIRS);
                   break;
   case cfifelap:  HandleIntSwitch("ifelapsed",value,&PIFELAPSED,0,999999);
                   break;
   case cfexpaft:  HandleIntSwitch("expireafter",value,&PEXPIREAFTER,0,999999);
                   break;
   default:        yyerror("Illegal file attribute");
   }
}


/*******************************************************************/

void HandleOptionalImageAttribute(char *item)

{ char value[CF_EXPANDSIZE],ebuff[CF_EXPANDSIZE];

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(ebuff,"%*[^=]=%[^\n]",value);

if (value[0] == '\0')
   {
   yyerror("Copy attribute with no value");
   }

Debug1("HandleOptionalImageAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
#ifdef DARWIN
   case cffindertype:
        if (strlen(value) == 4)
           {
           strncpy(FINDERTYPE,value,CF_BUFSIZE);
           }
        else
           {
           yyerror("Attribute findertype must be exactly 4 characters");
           }
        break;
#endif
   case cfmode:    ParseModeString(value,&PLUSMASK,&MINUSMASK);
                   strcpy(THISMODE,value);
                   break;
   case cfflags:   ParseFlagString(value,&PLUSFLAG,&MINUSFLAG);
                   break;
   case cfowner:   strcpy(VUIDNAME,value);
                   break;
   case cfgroup:   strcpy(VGIDNAME,value);
                   break;
   case cfdest:    strcpy(DESTINATION,value);
                   break;
   case cfaction:  strcpy(IMAGEACTION,value);
                   break;
   case cfcompat:  HandleCharSwitch("oldserver",value,&COMPATIBILITY);
                   break;            
   case cfcheckroot: HandleCharSwitch("checkroot",value,&CHKROOT);
                   break;
   case cfforce:   HandleCharSwitch("force",value,&FORCE);
                   break;
   case cfforcedirs: HandleCharSwitch("forcedirs",value,&FORCEDIRS);
                   break;     
   case cfforceipv4: HandleCharSwitch("forceipv4",value,&FORCEIPV4);
                   break;     
   case cfbackup:  HandleCharSwitch("backup",value,&IMAGEBACKUP);
                   break;
   case cfrecurse: HandleRecurse(value);
                   break;
   case cftype:    HandleCopyType(value);
                   break;
   case cfexclude: PrependItem(&VEXCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cfsymlink: PrependItem(&VCPLNPARSE,value,CF_ANYCLASS);
                   break;
   case cfinclude: PrependItem(&VINCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cfignore:  PrependItem(&VIGNOREPARSE,value,CF_ANYCLASS);
                   break;
   case cflntype:  HandleLinkType(value);
                   break;
   case cfserver:  HandleServer(value);
                   break;
   case cfencryp:  HandleCharSwitch("encrypt",value,&ENCRYPT);
                   break;
   case cfverify:  HandleCharSwitch("verify",value,&VERIFY);
                   break;
   case cfdefine:  HandleDefine(value);       
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;
   case cffailover: HandleFailover(value);
                   break;          
   case cfsize:    HandleCopySize(value);
                   break;
   case cfacl:     PrependItem(&VACLBUILD,value,CF_ANYCLASS);
                   break;
   case cfpurge:   HandleCharSwitch("purge",value,&PURGE);
                   break;
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;
   case cfsetaudit: HandleCharSwitch("audit",value,&AUDITP);
                   break;                   
   case cfstealth: HandleCharSwitch("stealth",value,&STEALTH);
                   break;
   case cftypecheck: HandleCharSwitch("typecheck",value,&TYPECHECK);
                   break;
   case cfrepository: strncpy(LOCALREPOS,value,CF_BUFSIZE-CF_BUFFERMARGIN);
                   break;
   case cffilter:  PrependItem(&VFILTERBUILD,value,CF_ANYCLASS);
                   break;

   case cftrustkey: HandleCharSwitch("trustkey",value,&TRUSTKEY);
                   break;
   case cftimestamps:
                   HandleTimeStamps(value);
                   break;
   case cfxdev:    HandleCharSwitch("xdev",value,&XDEV);
                   break;
   case cfifelap:  HandleIntSwitch("ifelapsed",value,&PIFELAPSED,0,999999);
                   break;
   case cfexpaft:  HandleIntSwitch("expireafter",value,&PEXPIREAFTER,0,999999);
                   break;

   default:        yyerror("Illegal copy attribute");
   }
}

/******************************************************************/

void HandleOptionalRequired(char *item)

{ char value[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];

value[0] = '\0';

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(ebuff,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Required/disk attribute with no value");
   }

Debug1("HandleOptionalRequiredAttribute(%s)\n",value);

switch(GetCommAttribute(ebuff))
   {
   case cffree:    HandleRequiredSize(value);
                   break;
   case cfscan:    HandleCharSwitch("scanarrivals",value,&SCAN);
                   break;
   case cfdefine:  HandleDefine(value);
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;    
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;
   case cfsetaudit: HandleCharSwitch("audit",value,&AUDITP);
                   break;
   case cfifelap:  HandleIntSwitch("ifelapsed",value,&PIFELAPSED,0,999999);
                   break;
   case cfexpaft:  HandleIntSwitch("expireafter",value,&PEXPIREAFTER,0,999999);
                   break;

   /* HvB: Bas van der Vlies */
   case cfforce:   HandleCharSwitch("force",value,&FORCE);
                   break;

   default:        yyerror("Illegal disk/required attribute");
   }

}

/******************************************************************/

void HandleOptionalInterface(char *item)

{ char value[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];

value[0] = '\0';

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(ebuff,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("intefaces attribute with no value");
   }

Debug1("HandleOptionalInterfaceAttribute(value=%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfsetnetmask:   HandleNetmask(value);
                        break;
   case cfsetbroadcast: HandleBroadcast(value);
                        break;
   case cfsetipaddress: HandleIPAddress(value);
                        break;

   default:        yyerror("Illegal interfaces attribute");
   }

}

/***********************************************************************/

void HandleOptionalMountablesAttribute(char *item) /* HvB: Bas van der Vlies */

{ char value[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];

value[0] = '\0';
//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(ebuff,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("mount attribute with no value");
   }

Debug1("HandleOptionalMountItem(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfmountoptions: 
      strcpy(MOUNTOPTS, value);
      break; 

   case cfreadonly: 
       if ((strcmp(value,"on")==0) || (strcmp(value,"true")==0))
          {
          CF_MOUNT_RO=true;
          }
       break; 
       
   default:         yyerror("Illegal mount option"
                            "(mountoptions/readonly)");
   }
}


/******************************************************************/

void HandleOptionalUnMountAttribute(char *item)

{ char value[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];

value[0] = '\0';

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(ebuff,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Unmount attribute with no value");
   }

Debug1("HandleOptionalUnMountsItem(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfdeldir:
       if (strcmp(value,"true") == 0 || strcmp(value,"yes") == 0)
          {
          DELETEDIR = 'y';
          break;
          }
       
       if (strcmp(value,"false") == 0 || strcmp(value,"no") == 0)
          {
          DELETEDIR = 'n';
          break;
          }
       break;
       
   case cfdelfstab:
       if (strcmp(value,"true") == 0 || strcmp(value,"yes") == 0)
          {
          DELETEFSTAB = 'y';
          break;
          }
       
       if (strcmp(value,"false") == 0 || strcmp(value,"no") == 0)
          {
          DELETEFSTAB = 'n';
          break;
          }   
       break;
       
   case cfforce:
       if (strcmp(value,"true") == 0 || strcmp(value,"yes") == 0)
          {
          FORCE = 'y';
          break;
          }
       
       if (strcmp(value,"false") == 0 || strcmp(value,"no") == 0)
          {
          FORCE = 'n';
          break;
          }
       break;
              
   case cfifelap:
       HandleIntSwitch("ifelapsed",value,&PIFELAPSED,0,999999);
       break;
   case cfexpaft:
       HandleIntSwitch("expireafter",value,&PEXPIREAFTER,0,999999);
       break;

   default:
       yyerror("Illegal unmount option (deletedir/deletefstab/force)");
   }
 
}

/******************************************************************/

void HandleOptionalMiscMountsAttribute(char *item)

{ char value[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];

value[0] = '\0';

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(ebuff,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Miscmounts attribute with no value");
   }

Debug1("HandleOptionalMiscMOuntsItem(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfmode:
        { struct Item *op;
        struct Item *next;
        
        if (MOUNTOPTLIST)
           {/* just in case */
           DeleteItemList(MOUNTOPTLIST);
           }

        MOUNTOPTLIST = SplitStringAsItemList(value,',');
        
        for (op = MOUNTOPTLIST; op != NULL; op = next)
           {
           next = op->next;  /* in case op is deleted */
           Debug1("miscmounts option: %s\n", op->name);
           
           if (strcmp(op->name,"rw") == 0)
              {
              MOUNTMODE='w';
              DeleteItem(&MOUNTOPTLIST,op);
              }
           else if (strcmp(op->name,"ro") == 0)
              {
              MOUNTMODE='o';
              DeleteItem(&MOUNTOPTLIST,op);
              }
           /* validate other mount options here */
           }
        }
        break;
   case cfifelap:  HandleIntSwitch("ifelapsed",value,&PIFELAPSED,0,999999);
       break;
   case cfexpaft:  HandleIntSwitch("expireafter",value,&PEXPIREAFTER,0,999999);
       break;
       
   default:        yyerror("Illegal miscmounts attribute (rw/ro)");
   }
 
}

/******************************************************************/

void HandleOptionalTidyAttribute(char *item)

{ char value[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];

value[0] = '\0';

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(ebuff,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Tidy attribute with no value");
   }

Debug1("HandleOptionalTidyAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfrecurse: HandleRecurse(value);
                   break;

   case cfexclude: PrependItem(&VEXCLUDEPARSE,value,CF_ANYCLASS);
                   break;

   case cfignore:  PrependItem(&VIGNOREPARSE,value,CF_ANYCLASS);
                   break;

   case cfinclude: 
   case cfpattern:

       strcpy(CURRENTITEM,value);
       if (*value == '/')
          {
          yyerror("search pattern begins with / must be a relative name");
          }
                   break;

   case cfifelap:  HandleIntSwitch("ifelapsed",value,&PIFELAPSED,0,999999);
                   break;
   case cfexpaft:  HandleIntSwitch("expireafter",value,&PEXPIREAFTER,0,999999);
                   break;

     
   case cfage:     HandleAge(value);
                   break;

   case cflinks:   HandleTravLinks(value);
                   break;

   case cfsize:    HandleTidySize(value);
                   break;

   case cftype:    HandleTidyType(value);
                   break;

   case cfdirlinks:
                   HandleTidyLinkDirs(value);
     break;

   case cfrmdirs:  HandleTidyRmdirs(value);
                   break;

   case cfdefine:  HandleDefine(value);
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;     
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;
   case cfsetaudit: HandleCharSwitch("audit",value,&AUDITP);
                   break;
   case cfcompress: HandleCharSwitch("compress",value,&COMPRESS);
                   break;
   case cffilter:  PrependItem(&VFILTERBUILD,value,CF_ANYCLASS);
                   break;
   case cfxdev:    HandleCharSwitch("xdev",value,&XDEV);
                   break;

   default:        yyerror("Illegal tidy attribute");
   }
}

/******************************************************************/

void HandleOptionalDirAttribute(char *item)

{ char value[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];

value[0] = '\0';

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(item,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Directory attribute with no value");
   }

Debug1("HandleOptionalDirAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfmode:    ParseModeString(value,&PLUSMASK,&MINUSMASK);
                   strcpy(THISMODE,value);
                   break;
   case cfflags:   ParseFlagString(value,&PLUSFLAG,&MINUSFLAG);
                   break;
   case cfowner:   strcpy(VUIDNAME,value);
                   break;
   case cfgroup:   strcpy(VGIDNAME,value);
                   break;
   case cfdefine:  HandleDefine(value);
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;     
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;
   case cfsetaudit: HandleCharSwitch("audit",value,&AUDITP);
                   break;
   case cfifelap:  HandleIntSwitch("ifelapsed",value,&PIFELAPSED,0,999999);
                   break;
   case cfexpaft:  HandleIntSwitch("expireafter",value,&PEXPIREAFTER,0,999999);
                   break;
   case cfrxdirs:  HandleCharSwitch("rxdirs",value,&RXDIRS);
                   break;

   default:        yyerror("Illegal directory attribute");
   }
}


/*******************************************************************/

void HandleOptionalDisableAttribute(char *item)

{ char value[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];

value[0] = '\0';

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(ebuff,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Disable attribute with no value");
   }

Debug1("HandleOptionalDisableAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfaction:
       if (strcmp(value,"warn") == 0)
          {
          PROACTION = 'w';
          }
       else if (strcmp(value,"delete") == 0)
          {
          PROACTION = 'd';
          }
       else
          {
          yyerror("Unknown action for disable");
          }
       break;
       
   case cftype:    HandleDisableFileType(value);
                   break;

   case cfrotate:  HandleDisableRotate(value);
                   break;
     
   case cfsize:    HandleDisableSize(value);
                   break;
   case cfdefine:  HandleDefine(value);
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;     
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;
   case cfsetaudit: HandleCharSwitch("audit",value,&AUDITP);
                   break;
   case cfrepository: strncpy(LOCALREPOS,value,CF_BUFSIZE-CF_BUFFERMARGIN);
                   break;
   case cfifelap:  HandleIntSwitch("ifelapsed",value,&PIFELAPSED,0,999999);
                   break;
   case cfexpaft:  HandleIntSwitch("expireafter",value,&PEXPIREAFTER,0,999999);
                   break;
   case cfdest:    strncpy(DESTINATION,value,CF_BUFSIZE-1);
                   break;

   default:        yyerror("Illegal disable attribute");
   }
}


/*******************************************************************/

void HandleOptionalLinkAttribute(char *item)

{ char value[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];

value[0] = '\0';

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(ebuff,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Link attribute with no value");
   }

Debug1("HandleOptionalLinkAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfaction:  HandleLinkAction(value);
                   break;
   case cftype:    HandleLinkType(value);
                   break;
   case cfexclude: PrependItem(&VEXCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cfinclude: PrependItem(&VINCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cffilter:  PrependItem(&VFILTERBUILD,value,CF_ANYCLASS);
                   break;     
   case cfignore:  PrependItem(&VIGNOREPARSE,value,CF_ANYCLASS);
                   break;
   case cfcopy:    PrependItem(&VCPLNPARSE,value,CF_ANYCLASS);
                   break;
   case cfrecurse: HandleRecurse(value);
                   break;
   case cfcptype:  HandleCopyType(value);
                   break;
   case cfnofile:  HandleDeadLinks(value);
                   break;
   case cfdefine:  HandleDefine(value);
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;     
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;
   case cfsetaudit: HandleCharSwitch("audit",value,&AUDITP);
                   break;
   case cfifelap:  HandleIntSwitch("ifelapsed",value,&PIFELAPSED,0,999999);
                   break;
   case cfexpaft:  HandleIntSwitch("expireafter",value,&PEXPIREAFTER,0,999999);
                   break;

   default:        yyerror("Illegal link attribute");
   }
}

/*******************************************************************/

void HandleOptionalProcessAttribute(char *item)

{ char value[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];

value[0] = '\0';

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(ebuff,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Process attribute with no value");
   }

Debug1("HandleOptionalProcessAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfaction:
       if (strcmp(value,"signal") == 0 || strcmp(value,"do") == 0)
          {
          PROACTION = 's';
          }
       else if (strcmp(value,"bymatch") == 0)
          {
          PROACTION = 'm';
          }
       else if (strcmp(value,"warn") == 0)
          {
          PROACTION = 'w';
          ACTIONPENDING = true;
          }
       else
          {
          yyerror("Unknown action for processes");
          }
       break;

   case cfmatches: HandleProcessMatches(value);
                   ACTIONPENDING = true;
                   break;
   case cfsignal:  HandleProcessSignal(value);
                   ACTIONPENDING = true;
                   break;
   case cfdefine:  HandleDefine(value);
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;
   case cfuseshell:HandleUseShell(value);
                   break;
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;
   case cfsetaudit: HandleCharSwitch("audit",value,&AUDITP);
                   break;
   case cfexclude: PrependItem(&VEXCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cfinclude: PrependItem(&VINCLUDEPARSE,value,CF_ANYCLASS);
                   break;
   case cffilter:  PrependItem(&VFILTERBUILD,value,CF_ANYCLASS);
                   break;
   case cfowner:   strcpy(VUIDNAME,value);
                   break;
   case cfgroup:   strcpy(VGIDNAME,value);
                   break;
   case cfchdir:   HandleChDir(value);
                   break;
   case cfchroot:  HandleChRoot(value);
                   break;
   case cfumask:   HandleUmask(value);
                   break;
   case cfifelap:  HandleIntSwitch("ifelapsed",value,&PIFELAPSED,0,999999);
                   break;
   case cfexpaft:  HandleIntSwitch("expireafter",value,&PEXPIREAFTER,0,999999);
                   break;
     
   default:        yyerror("Illegal process attribute");
   }

 
}

/*******************************************************************/

void HandleOptionalPackagesAttribute(char *item)

{ char value[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];

value[0] = '\0';

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(ebuff,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Packages attribute with no value");
   }

Debug1("HandleOptionalPackagesAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfversion: strcpy(PKGVER,value);
                   break;
   case cfcmp:     CMPSENSE = (enum cmpsense) GetCmpSense(value);
                   break;
   case cfpkgmgr:  PKGMGR = (enum pkgmgrs) GetPkgMgr(value);
                   break;
   case cfdefine:  HandleDefine(value);
                   break;
   case cfelsedef: HandleElseDefine(value);
                   break;     
   case cfsetlog:  HandleCharSwitch("log",value,&LOGP);
                   break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                   break;
   case cfsetaudit: HandleCharSwitch("audit",value,&AUDITP);
                   break;
   case cfifelap:  HandleIntSwitch("ifelapsed",value,&PIFELAPSED,0,999999);
                   break;
   case cfexpaft:  HandleIntSwitch("expireafter",value,&PEXPIREAFTER,0,999999);
                   break;
   case cfaction:
                   PKGACTION = (enum pkgactions) GetPkgAction(value);
                   break;
   default:        yyerror("Illegal packages attribute");
   }
}

/*******************************************************************/

void HandleOptionalMethodsAttribute(char *item)

{ char value[CF_BUFSIZE],ebuff[CF_EXPANDSIZE];

value[0] = '\0';

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(ebuff,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Methods attribute with no value");
   }

 switch(GetCommAttribute(item))
    {
    case cfserver:
        HandleServer(value);
        break;        
    case cfaction:
        strncpy(ACTIONBUFF,value,CF_BUFSIZE-1);
        break;        
    case cfretvars:
        strncpy(METHODFILENAME,value,CF_BUFSIZE-1);
        break;
    case cfretclasses:
        if (strlen(PARSEMETHODRETURNCLASSES) > 0)
           {
           yyerror("Redefinition of method return_classes");
           }
        else
           {
           strncpy(PARSEMETHODRETURNCLASSES,value,CF_BUFSIZE-1);
           }
        break;
        
    case cfforcereplyto:
        strncpy(METHODFORCE,value,CF_BUFSIZE-1);
        break;
        
    case cfsendclasses:
        if (strlen(METHODREPLYTO) > 0)
           {
           yyerror("Redefinition of method send_classes");
           }
        else
           {
           strncpy(METHODREPLYTO,value,CF_MAXVARSIZE-1);
           }
        break;
    case cfifelap:
        HandleIntSwitch("ifelapsed",value,&PIFELAPSED,0,999999);
        break;
    case cfexpaft:
        HandleIntSwitch("expireafter",value,&PEXPIREAFTER,0,999999);
        break;        
    case cfowner:
        strncpy(VUIDNAME,value,CF_BUFSIZE-1);
        break;
    case cfgroup:
        strncpy(VGIDNAME,value,CF_BUFSIZE-1);
        break;
    case cfchdir:
        HandleChDir(value);
        break;
    case cfchroot:
        HandleChRoot(value);
        break;       
               
    default:
        yyerror("Illegal methods attribute");
    }
 
ACTIONPENDING = true; 
}


/*******************************************************************/

void HandleOptionalScriptAttribute(char *item)

{ char value[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];

value[0] = '\0';

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(ebuff,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Shellcommand attribute with no value");
   }

Debug1("HandleOptionalScriptAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cftimeout:   HandleTimeOut(value);
                     break;
   case cfuseshell:  HandleUseShell(value);
                     break;
   case cfsetlog:    HandleCharSwitch("log",value,&LOGP);
                     break;
   case cfsetinform: HandleCharSwitch("inform",value,&INFORMP);
                     break;
   case cfsetaudit:  HandleCharSwitch("audit",value,&AUDITP);
                     break;
   case cfowner:     strcpy(VUIDNAME,value);
                     break;
   case cfgroup:     strcpy(VGIDNAME,value);
                     break;
   case cfdefine:    HandleDefine(value);
                     break;
   case cfelsedef:   HandleElseDefine(value);
                     break;
   case cfumask:     HandleUmask(value);
                     break;
   case cffork:      HandleCharSwitch("background",value,&FORK);
                     break;
   case cfchdir:     HandleChDir(value);
                     break;
   case cfchroot:    HandleChRoot(value);
                     break;       
   case cfpreview:   HandleCharSwitch("preview",value,&PREVIEW);
                     break;       
   case cfnoabspath: HandleCharSwitch("noabspath",value,&NOABSPATH);
                     break;
   case cfifelap:  HandleIntSwitch("ifelapsed",value,&PIFELAPSED,0,999999);
                   break;
   case cfexpaft:  HandleIntSwitch("expireafter",value,&PEXPIREAFTER,0,999999);
                   break;
   default:         yyerror("Illegal shellcommand attribute");
   }

}

/*******************************************************************/

void HandleOptionalAlertsAttribute(char *item)

{ char value[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE];

value[0] = '\0';

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

sscanf(ebuff,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   yyerror("Alerts attribute with no value");
   }

Debug1("HandleOptionalAlertsAttribute(%s)\n",value);

switch(GetCommAttribute(item))
   {
   case cfifelap:  HandleIntSwitch("ifelapsed",value,&PIFELAPSED,0,999999);
                   break;
   case cfexpaft:  HandleIntSwitch("expireafter",value,&PEXPIREAFTER,0,999999);
                   break;
   case cfsetaudit:  HandleCharSwitch("audit",value,&AUDITP);
                     break;
   default:         yyerror("Illegal alerts attribute");
   }
}

/*******************************************************************/

void HandleChDir(char *value)

{
if (!IsAbsoluteFileName(value))
   {
   yyerror("chdir is not an absolute directory name");
   }

strcpy(CHDIR,value); 
}

/*******************************************************************/

void HandleChRoot(char *value)

{
if (!IsAbsoluteFileName(value))
   {
   yyerror("chdir is not an absolute directory name");
   }
 
strcpy(CHROOT,value);  
}

/*******************************************************************/

void HandleFileItem(char *item)

{
if (strcmp(item,"home") == 0)
   {
   ACTIONPENDING=true;
   strcpy(CURRENTOBJECT,"home");
   return;
   }

snprintf(OUTPUT,100,"Unknown attribute %s",item);
yyerror(OUTPUT);
}


/*******************************************************************/

void InstallBroadcastItem(char *item)

{
Debug1("Install broadcast mode (%s)\n",item);

/*if ( ! IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",item);
   return;
   }
*/

if (VBROADCAST[0] != '\0')
   {
   return;
   yyerror("Multiple declaration of variable broadcast");
   FatalError("Redefinition of basic system variable");
   }

if (strcmp("ones",item) == 0)
   {
   strcpy(VBROADCAST,"one");
   return;
   }

if (strcmp("zeroes",item) == 0)
   {
   strcpy(VBROADCAST,"zero");
   return;
   }

if (strcmp("zeros",item) == 0)
   {
   strcpy(VBROADCAST,"zero");
   return;
   }

yyerror ("Unknown broadcast mode (should be ones, zeros or zeroes)");
FatalError("Unknown broadcast mode");
}

/*******************************************************************/

void InstallDefaultRouteItem(char *item)

{ struct hostent *hp;
  struct in_addr inaddr;
  char ebuff[CF_EXPANDSIZE];

Debug1("Install defaultroute mode (%s)\n",item);

/*
if (!IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",item);
   return;
   }
*/

if (VDEFAULTROUTE != NULL)
   {
   return;
   yyerror("Multiple declaration of variable defaultroute");
   FatalError("Redefinition of basic system variable");
   }

//ExpandVarstring(item,ebuff,NULL);
strcpy(ebuff,item);

if (inet_addr(ebuff) == -1)
   {
   if ((hp = gethostbyname(ebuff)) == NULL)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Bad address/host (%s) in default route\n",ebuff);
      CfLog(cferror,OUTPUT,"gethostbyname");
      yyerror ("Bad specification of default packet route: hostname or decimal IP address");
      }
   else
      {
      memcpy(&inaddr,hp->h_addr, hp->h_length);
      PrependItem(&VDEFAULTROUTE,inet_ntoa(inaddr),CLASSBUFF);
      }
   }
else
   {
   PrependItem(&VDEFAULTROUTE,ebuff,CLASSBUFF);
   }
}

/*******************************************************************/

void InstallGroupRValue(char *item,enum itemtypes type)

{ char *machine, *user, *domain;
  char ebuff[CF_EXPANDSIZE];

if (!IsDefinedClass(CLASSBUFF))
   {
   Debug("Not defining class (%s) - no match of container class (%s)\n",item,CLASSBUFF);
   return;
   }

/*if (*item == '\'' || *item == '"' || *item == '`')
   {
   ExpandVarstring(item+1,ebuff,NULL);
   }
else
   {
   ExpandVarstring(item,ebuff,NULL);
   }
*/

strcpy(ebuff,item);

Debug1("HandleGroupRVal(%s) group (%s), type=%d with fqname=%s,uqname=%s\n",ebuff,GROUPBUFF,type,VFQNAME,VUQNAME);

switch (type)
   {
   case simple:    if (strcmp(ebuff,VUQNAME) == 0)
                      {
                      AddClassToHeap(GROUPBUFF);
                      break;
                      }

                   if (strcmp(ebuff,VFQNAME) == 0)
                      {
                      AddClassToHeap(GROUPBUFF);
                      break;
                      }

                   if (IsDefinedClass(ebuff))
                      {
                      AddClassToHeap(GROUPBUFF);
                      break;
                      }

                   Debug("[No match of class %s]\n\n",ebuff);

                   break;

   case netgroup:  setnetgrent(ebuff);

                   while (getnetgrent(&machine,&user,&domain))
                      {
                      if (strcmp(machine,VUQNAME) == 0)
                         {
                         Debug1("Matched %s in netgroup %s\n",machine,ebuff);
                         AddClassToHeap(GROUPBUFF);
                         break;
                         }

                      if (strcmp(machine,VFQNAME) == 0)
                         {
                         Debug1("Matched %s in netgroup %s\n",machine,ebuff);
                         AddClassToHeap(GROUPBUFF);
                         break;
                         }
                      }
                   
                   endnetgrent();
                   break;


   case groupdeletion: 

                   setnetgrent(ebuff);

                   while (getnetgrent(&machine,&user,&domain))
                      {
                      if (strcmp(machine,VUQNAME) == 0)
                         {
                         Debug1("Matched delete item %s in netgroup %s\n",machine,ebuff);
                         DeleteItemLiteral(&VHEAP,GROUPBUFF);
                         break;
                         }
        
                      if (strcmp(machine,VFQNAME) == 0)
                         {
                         Debug1("Matched delete item %s in netgroup %s\n",machine,ebuff);
                         DeleteItemLiteral(&VHEAP,GROUPBUFF);
                         break;
                         }
        
                      }
                   
                   endnetgrent();
                   break;

   case classscript:

                   if (ebuff[0] != '/')
                      {
                      yyerror("Quoted scripts must begin with / for absolute path");
                      break;
                      }

                   SetClassesOnScript(ebuff,GROUPBUFF,NULL,false);
                   break;

   case deletion:  if (IsDefinedClass(ebuff))
                      {
                      DeleteItemLiteral(&VHEAP,GROUPBUFF);
                      }
                   break;

   default:        yyerror("Software error");
                   FatalError("Unknown item type");
   }
}

/*******************************************************************/

void HandleHomePattern(char *pattern)

{ char ebuff[CF_EXPANDSIZE];

//ExpandVarstring(pattern,ebuff,"");
strcpy(ebuff,pattern);
AppendItem(&VHOMEPATLIST,ebuff,CLASSBUFF);
}

/*******************************************************************/

void AppendNameServer(char *item)

{ char ebuff[CF_EXPANDSIZE];
 struct Item *list = NULL, *ip;
 
Debug1("Installing item (%s) in the nameserver list\n",item);

/*
if (!IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",item);
   return;
   }
*/

//ExpandVarstring(item,ebuff,"");
strcpy(ebuff,item);

list = SplitStringAsItemList(ebuff,LISTSEPARATOR);

for (ip = list; ip != NULL; ip=ip->next)
   {
   PrependItem(&VRESOLVE,ip->name,CLASSBUFF);
   }
}

/*******************************************************************/

void AppendImport(char *item)

{ char ebuff[CF_EXPANDSIZE];

/*
if (!IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",item);
   return;
   }
*/
 
if (strcmp(item,VCURRENTFILE) == 0)
   {
   yyerror("A file cannot import itself");
   FatalError("Infinite self-reference in class inheritance");
   }

Debug1("\n\n [Installing item (%s) in the import list]\n\n",item);

// Keep this one
ExpandVarstring(item,ebuff,"");
   
InstallItem(&VIMPORT,ebuff,CLASSBUFF,0,0);
}

/*******************************************************************/

void InstallHomeserverItem(char *item)

{ char ebuff[CF_EXPANDSIZE];
 
/*if (!IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",item);
   return;
   }
*/
 
//ExpandVarstring(item,ebuff,"");  
strcpy(ebuff,item);
AppendItem(&VHOMESERVERS,ebuff,CLASSBUFF);
}

/*******************************************************************/

void InstallBinserverItem(char *item)

{ char ebuff[CF_EXPANDSIZE];
 
/*
 if (!IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",item);
   return;
   }
*/
 
//ExpandVarstring(item,ebuff,""); 
strcpy(ebuff,item);
AppendItem(&VBINSERVERS,ebuff,CLASSBUFF);
}

/*******************************************************************/

void InstallMailserverPath(char *path)

{
/*if (!IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",path);
   return;
   }
*/
 
if (VMAILSERVER[0] != '\0')
   {
   FatalError("Redefinition of mailserver");
   }

strcpy(VMAILSERVER,path);

Debug1("Installing mailserver (%s) for group (%s)",path,GROUPBUFF);
}

/*******************************************************************/

void InstallLinkItem(char *from,char *to)

{ struct Link *ptr;
  char buffer[CF_EXPANDSIZE], buffer2[CF_EXPANDSIZE];
  char ebuff[CF_EXPANDSIZE];
  
Debug1("Storing Link: (From)%s->(To)%s\n",from,to);

/*if (!IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing link no match\n");
   return;
   }
*/

//ExpandVarstring(from,ebuff,"");
strcpy(ebuff,from);

 if (strlen(ebuff) > 1)
   {
   DeleteSlash(ebuff);
   }

//ExpandVarstring(to,buffer,"");
strcpy(buffer,to);
 
if (strlen(buffer) > 1)
   { 
   DeleteSlash(buffer);
   }

//ExpandVarstring(DEFINECLASSBUFFER,buffer2,""); 
strcpy(DEFINECLASSBUFFER,buffer2);

if ((ptr = (struct Link *)malloc(sizeof(struct Link))) == NULL)
   {
   FatalError("Memory Allocation failed for InstallListItem() #1");
   }
 
if ((ptr->from = strdup(ebuff)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallListItem() #2");
   }

if ((ptr->to = strdup(buffer)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallListItem() #3");
   }

if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallListItem() #4");
   }

if ((ptr->defines = strdup(buffer2)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallListItem() #4a");
   }

//ExpandVarstring(ELSECLASSBUFFER,buffer,"");
strcpy(buffer,ELSECLASSBUFFER);

if ((ptr->elsedef = strdup(buffer)) == NULL)
   {
   FatalError("Memory Allocation failed xxx");
   }
 
AddInstallable(ptr->defines);
AddInstallable(ptr->elsedef); 
 
if (VLINKTOP == NULL)                 /* First element in the list */
   {
   VLINK = ptr;
   }
else
   {
   VLINKTOP->next = ptr;
   }

if (strlen(ptr->from) > 1)
   {
   DeleteSlash(ptr->from);
   }

if (strlen(ptr->to) > 1)
   {
   DeleteSlash(ptr->to);
   }

   ptr->ifelapsed = PIFELAPSED;
   ptr->expireafter = PEXPIREAFTER;


ptr->audit = AUDITPTR;
ptr->lineno = LINENUMBER;
ptr->force = FORCELINK;
ptr->silent = LINKSILENT;
ptr->type = LINKTYPE;
ptr->copytype = COPYTYPE;
ptr->next = NULL;
ptr->copy = VCPLNPARSE;
ptr->exclusions = VEXCLUDEPARSE;
ptr->inclusions = VINCLUDEPARSE;
ptr->ignores = VIGNOREPARSE;
ptr->filters = VFILTERBUILD;
ptr->recurse = VRECURSE;
ptr->nofile = DEADLINKS;
ptr->log = LOGP;
ptr->inform = INFORMP;
ptr->logaudit = AUDITP;
ptr->done = 'n';
ptr->scope = strdup(CONTEXTID);
 
VLINKTOP = ptr;

if (ptr->recurse != 0)
   {
   yyerror("Recursion can only be used with +> multiple links");
   }

InitializeAction();
}

/*******************************************************************/

void InstallLinkChildrenItem (char *from,char *to)

{ struct Link *ptr;
  char *sp, buffer[CF_EXPANDSIZE],ebuff[CF_EXPANDSIZE];
  struct TwoDimList *tp = NULL;

Debug1("Storing Linkchildren item: %s -> %s\n",from,to);

/*if (!IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing linkchildren no match\n");
   return;
   }
*/

//ExpandVarstring(from,ebuff,"");
strcpy(ebuff,from);

//ExpandVarstring(DEFINECLASSBUFFER,buffer,""); 
strcpy(buffer,DEFINECLASSBUFFER);

Build2DListFromVarstring(&tp,to,LISTSEPARATOR,false);
Set2DList(tp);

for (sp = Get2DListEnt(tp); sp != NULL; sp = Get2DListEnt(tp))
   {
   if ((ptr = (struct Link *)malloc(sizeof(struct Link))) == NULL)
      {
      FatalError("Memory Allocation failed for InstallListChildrenItem() #1");
      }

   if ((ptr->from = strdup(ebuff)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallLinkchildrenItem() #2");
      }

   if ((ptr->to = strdup(sp)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallLinkChildrenItem() #3");
      }

   if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallLinkChildrenItem() #3");
      }

   if ((ptr->defines = strdup(buffer)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallListItem() #4a");
      }
   
   //ExpandVarstring(ELSECLASSBUFFER,buffer,"");
   strcpy(buffer,ELSECLASSBUFFER);
   
   if ((ptr->elsedef = strdup(buffer)) == NULL)
      {
      FatalError("Memory Allocation failed for Installrequied() #2");
      }
   
   AddInstallable(ptr->defines);
   AddInstallable(ptr->elsedef); 
   
   if (VCHLINKTOP == NULL)                 /* First element in the list */
      {
      VCHLINK = ptr;
      }
   else
      {
      VCHLINKTOP->next = ptr;
      }

      ptr->ifelapsed = PIFELAPSED;
      ptr->expireafter = PEXPIREAFTER;

   ptr->audit = AUDITPTR;
   ptr->lineno = LINENUMBER;
   ptr->force = FORCELINK;
   ptr->silent = LINKSILENT;
   ptr->type = LINKTYPE;
   ptr->next = NULL;
   ptr->copy = VCPLNPARSE;
   ptr->nofile = DEADLINKS;
   ptr->exclusions = VEXCLUDEPARSE;
   ptr->inclusions = VINCLUDEPARSE;
   ptr->ignores = VIGNOREPARSE;
   ptr->copytype = COPYTYPE;
   ptr->filters = VFILTERBUILD;
   ptr->recurse = VRECURSE;
   ptr->log = LOGP;
   ptr->inform = INFORMP;
   ptr->logaudit = AUDITP;
   ptr->done = 'n';
   ptr->scope = strdup(CONTEXTID);
 
   VCHLINKTOP = ptr;

   if (ptr->recurse != 0 && strcmp(ptr->to,"linkchildren") == 0)
      {
      yyerror("Sorry don't know how to recurse with linkchildren keyword");
      }
   }

Delete2DList(tp);

InitializeAction();
}


/*******************************************************************/

void InstallRequiredPath(char *path,int freespace)

{ struct Disk *ptr;
  char buffer[CF_EXPANDSIZE],ebuff[CF_EXPANDSIZE],*sp;
  struct TwoDimList *tp = NULL;

Build2DListFromVarstring(&tp,path,LISTSEPARATOR,false);    
Set2DList(tp);

for (sp = Get2DListEnt(tp); sp != NULL; sp = Get2DListEnt(tp))
   {
   Debug1("Installing item (%s) in the required list\n",sp);

   /*
   if (!IsInstallable(CLASSBUFF))
      {
      InitializeAction();
      Debug1("Not installing %s, no match\n",sp);
      return;
      }
   */
   
   //ExpandVarstring(sp,ebuff,"");
   strcpy(ebuff,sp);
   
   if ((ptr = (struct Disk *)malloc(sizeof(struct Disk))) == NULL)
      {
      FatalError("Memory Allocation failed for InstallRequired() #1");
      }
   
   if ((ptr->name = strdup(ebuff)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallRequired() #2");
      }
   
   if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallRequired() #2");
      }
   
   //ExpandVarstring(DEFINECLASSBUFFER,buffer,"");
   strcpy(buffer,DEFINECLASSBUFFER);
   
   if ((ptr->define = strdup(buffer)) == NULL)
      {
      FatalError("Memory Allocation failed for Installrequied() #2");
      }
   
   //ExpandVarstring(ELSECLASSBUFFER,buffer,"");
   strcpy(buffer,ELSECLASSBUFFER);
   
   if ((ptr->elsedef = strdup(buffer)) == NULL)
      {
      FatalError("Memory Allocation failed for Installrequied() #2");
      }
   
   if (VREQUIRED == NULL)                 /* First element in the list */
      {
      VREQUIRED = ptr;
      }
   else
      {
      VREQUIREDTOP->next = ptr;
      }
   
   AddInstallable(ptr->define);
   AddInstallable(ptr->elsedef);

      ptr->ifelapsed = PIFELAPSED;
      ptr->expireafter = PEXPIREAFTER;

   ptr->audit = AUDITPTR;
   ptr->lineno = LINENUMBER;
   ptr->freespace = freespace;
   ptr->next = NULL;
   ptr->log = LOGP;
   ptr->inform = INFORMP;
   ptr->logaudit = AUDITP;
   ptr->done = 'n';
   ptr->scanarrivals = SCAN;
   ptr->scope = strdup(CONTEXTID);
 
/* HvB : Bas van der Vlies */
   ptr->force = FORCE;
   
   VREQUIREDTOP = ptr;
   }

Delete2DList(tp);
InitializeAction(); 
}

/*******************************************************************/

void InstallMountableItem(char *path,char *mnt_opts,flag readonly)

{ struct Mountables *ptr;
  char ebuff[CF_EXPANDSIZE]; 

Debug1("Adding mountable %s to list\n",path);

if (!IsDefinedClass(CLASSBUFF))
   {
   return;
   }
 
//ExpandVarstring(path,ebuff,"");
strcpy(ebuff,path);

Debug1("Adding mountable %s to list\n",ebuff);

/* 
 * Check if mount entry already exists
 */
if (VMOUNTABLES != NULL)
   {
   for (ptr = VMOUNTABLES; ptr != NULL; ptr = ptr->next)
      {
      if ( strcmp(ptr->filesystem, VBUFF) == 0 )
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Only one definition per mount allowed: %s\n",ptr->filesystem);
         yyerror(OUTPUT); 
         return;
         }
      }
   }
 
if ((ptr = (struct Mountables *)malloc(sizeof(struct Mountables))) == NULL)
   {
   FatalError("Memory Allocation failed for InstallMountableItem() #1");
   }
 
if ((ptr->filesystem = strdup(ebuff)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallMountableItem() #2");
   }

if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallMountableItem() #3");
   }

if ( mnt_opts[0] != '\0' )
   {
   if ( (ptr->mountopts = strdup(mnt_opts)) == NULL )
      {
      FatalError("Memory Allocation failed for InstallMountableItem() #4");
      }
   }
else
   {
   ptr->mountopts = NULL;
   }

 ptr->audit = AUDITPTR;
 ptr->lineno = LINENUMBER;
 ptr->readonly = readonly;
 ptr->next = NULL;
 ptr->done = 'n';
 ptr->scope = strdup(CONTEXTID);
 
if (VMOUNTABLESTOP == NULL)                 /* First element in the list */
   {
   VMOUNTABLES = ptr;
   }
else
   {
   VMOUNTABLESTOP->next = ptr;
   }

VMOUNTABLESTOP = ptr;
}

/*******************************************************************/

void AppendUmount(char *path,char deldir,char delfstab,char force)

{ struct UnMount *ptr;
  char ebuff[CF_EXPANDSIZE]; 

/*
if (!IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing %s, no match\n",path);
   return;
   }
  */
  
//ExpandVarstring(path,ebuff,"");
strcpy(ebuff,path);

 if ((ptr = (struct UnMount *)malloc(sizeof(struct UnMount))) == NULL)
   {
   FatalError("Memory Allocation failed for AppendUmount() #1");
   }

if ((ptr->name = strdup(ebuff)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendUmount() #2");
   }
 
if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendUmount() #5");
   }

if (VUNMOUNTTOP == NULL)                 /* First element in the list */
   {
   VUNMOUNT = ptr;
   }
else
   {
   VUNMOUNTTOP->next = ptr;
   }

   ptr->ifelapsed = PIFELAPSED;
   ptr->expireafter = PEXPIREAFTER;
 
ptr->next = NULL;
ptr->deletedir = deldir;  /* t/f - true false */
ptr->deletefstab = delfstab;
ptr->force = force;
ptr->done = 'n';
ptr->scope = strdup(CONTEXTID);

VUNMOUNTTOP = ptr;
}

/*******************************************************************/

void AppendMiscMount(char *from,char *onto,char *mode, char *opts)

{ struct MiscMount *ptr;
  char ebuff[CF_EXPANDSIZE]; 

Debug1("Adding misc mountable %s %s (%s) to list\n",from,onto,opts);

/*
if (!IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",from);
   return;
   }
*/

if ((ptr = (struct MiscMount *)malloc(sizeof(struct MiscMount))) == NULL)
   {
   FatalError("Memory Allocation failed for AppendMiscMount #1");
   }

//ExpandVarstring(from,ebuff,"");
 strcpy(ebuff,from);
 
if ((ptr->from = strdup(ebuff)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendMiscMount() #2");
   }

//ExpandVarstring(onto,ebuff,"");
strcpy(ebuff,onto);

if ((ptr->onto = strdup(ebuff)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendMiscMount() #3");
   }

if ((ptr->options = strdup(opts)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendMiscMount() #4");
   }

if ((ptr->mode = strdup(mode)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendMiscMount() #4a");
   }

if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendMiscMount() #5");
   }

if (VMISCMOUNTTOP == NULL)                 /* First element in the list */
   {
   VMISCMOUNT = ptr;
   }
else
   {
   VMISCMOUNTTOP->next = ptr;
   }

   ptr->ifelapsed = PIFELAPSED;
   ptr->expireafter = PEXPIREAFTER;
 
ptr->audit = AUDITPTR;
ptr->lineno = LINENUMBER;
ptr->next = NULL;
ptr->done = 'n';
ptr->scope = strdup(CONTEXTID); 
VMISCMOUNTTOP = ptr;
}


/*******************************************************************/

void AppendIgnore(char *path)

{ struct TwoDimList *tp = NULL;
  char *sp;

Debug1("Installing item (%s) in the ignore list\n",path);

/*
if (!IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing %s, no match\n",path);
   return;
   }
*/

Build2DListFromVarstring(&tp,path,LISTSEPARATOR,false);
Set2DList(tp);

for (sp = Get2DListEnt(tp); sp != NULL; sp = Get2DListEnt(tp))
   {
   InstallItem(&VIGNORE,sp,CLASSBUFF,0,0);
   }

Delete2DList(tp);
}

/*******************************************************************/

void InstallPending(enum actions action)

{
if (ACTIONPENDING)
   {
   Debug1("\n   [BEGIN InstallPending %s\n",ACTIONTEXT[action]);
   }
else
   {
   Debug1("   (No actions pending in %s)\n",ACTIONTEXT[action]);
   return;
   }

switch (action)
   {
   case filters:
       InstallFilterTest(FILTERNAME,CURRENTITEM,FILTERDATA);
       CURRENTITEM[0] = '\0';
       FILTERDATA[0] = '\0'; 
       break;
       
   case strategies:
       AddClassToStrategy(STRATEGYNAME,CURRENTITEM,STRATEGYDATA);
       break;

   case resolve:
       AppendNameServer(CURRENTOBJECT);
       break;

   case files:
       InstallFileListItem(CURRENTOBJECT,PLUSMASK,MINUSMASK,FILEACTION,VUIDNAME,
                           VGIDNAME,VRECURSE,(char)PTRAVLINKS,CHECKSUM);
       break;

   case processes:

       InstallProcessItem(EXPR,RESTART,PROMATCHES,PROCOMP,
                          PROSIGNAL,PROACTION,CLASSBUFF,USESHELL,VUIDNAME,VGIDNAME);
       break;
       
   case image:
       InstallImageItem(FINDERTYPE,CURRENTOBJECT,PLUSMASK,MINUSMASK,DESTINATION,
                        IMAGEACTION,VUIDNAME,VGIDNAME,IMGSIZE,IMGCOMP,
                        VRECURSE,COPYTYPE,LINKTYPE,CFSERVER);
       break;
       
   case ignore:
       AppendIgnore(CURRENTOBJECT);
       break;

   case tidy:

       if (VAGE >= 99999)
          {
          yyerror("Must specify an age for tidy actions");
          return;
          }
       InstallTidyItem(CURRENTOBJECT,CURRENTITEM,VRECURSE,VAGE,(char)PTRAVLINKS,
                       TIDYSIZE,AGETYPE,LINKDIRS,TIDYDIRS,CLASSBUFF);
       break;
       
   case makepath:
       InstallMakePath(CURRENTOBJECT,PLUSMASK,MINUSMASK,VUIDNAME,VGIDNAME);
       break;

   case methods:
       InstallMethod(CURRENTOBJECT,ACTIONBUFF);
       break;

   case rename_disable:
   case disable:
       
       AppendDisable(CURRENTOBJECT,CURRENTITEM,ROTATE,DISCOMP,DISABLESIZE);
       break;

   case shellcommands:

       AppendScript(CURRENTOBJECT,VTIMEOUT,USESHELL,VUIDNAME,VGIDNAME);
       InitializeAction();
       break;

   case scli:
       
       AppendSCLI(CURRENTOBJECT,VTIMEOUT,USESHELL,VUIDNAME,VGIDNAME);
       InitializeAction();
       break;

   case alerts:

       if (strcmp(CLASSBUFF,"any") == 0)
          {
          yyerror("Alerts cannot be in class any - probably a mistake");
          }
       else
          {
          Debug("Install %s if %s\n",CURRENTOBJECT,CLASSBUFF);
          InstallItem(&VALERTS,CURRENTOBJECT,CLASSBUFF,0,0);
          }

       InitializeAction();
       break;
       
   case interfaces:
       AppendInterface(VIFNAME,LINKTO,DESTINATION,CURRENTOBJECT);
       break;

   case disks:
   case required:

       InstallRequiredPath(CURRENTOBJECT,IMGSIZE);
       break;

   /* HvB: Bas van der Vlies */
   case mountables:
       InstallMountableItem(CURRENTOBJECT,MOUNTOPTS,CF_MOUNT_RO);
       break;

   case misc_mounts:
       if ((strlen(MOUNTFROM) != 0) && (strlen(MOUNTONTO) != 0))
          {          
          struct Item *op;
          
          /* Reconcat list */
          
          for (op = MOUNTOPTLIST; op != NULL; op = op->next)
             {
             if (BufferOverflow(MOUNTOPTS,op->name))
                {
                printf(" culprit: InstallPending, skipping miscmount %s %s\n",
                       MOUNTFROM, MOUNTONTO);
                return;
                }
             
             strcat(MOUNTOPTS,op->name);
             strcat(MOUNTOPTS,",");
             }
          
          MOUNTOPTS[strlen(MOUNTOPTS)-1] = '\0';

          switch (MOUNTMODE)
             {
             case 'o':
                 AppendMiscMount(MOUNTFROM,MOUNTONTO,"ro",MOUNTOPTS);
                 break;
             case 'w':
                 AppendMiscMount(MOUNTFROM,MOUNTONTO,"rw",MOUNTOPTS);
                 break;
             default:  printf("Install pending, miscmount, shouldn't happen\n");
                 MOUNTOPTS[0] = '\0'; /* no mount mode set! */
             }
          }
       
       InitializeAction();
       break;
       
   case unmounta:
       AppendUmount(CURRENTOBJECT,DELETEDIR,DELETEFSTAB,FORCE);
       break;
       
   case links:
       if (LINKTO[0] == '\0')
          {
          return;
          }
       
       if (ACTION_IS_LINKCHILDREN)
          {
          InstallLinkChildrenItem(LINKFROM,LINKTO);
          ACTION_IS_LINKCHILDREN = false;
          }
       else if (ACTION_IS_LINK)
          {
          InstallLinkItem(LINKFROM,LINKTO);
          ACTION_IS_LINK = false;
          }
       else
          {
          return;                                   /* Don't have whole command */
          }
       
       break;
       
   case packages:
       InstallPackagesItem(CURRENTOBJECT,PKGVER,CMPSENSE,PKGMGR,PKGACTION);
       break;
   }

 
Debug1("   END InstallPending]\n\n");
}

/*******************************************************************/
/* Level 3                                                         */
/*******************************************************************/

void HandleCharSwitch(char *name,char *value,char *pflag)

{
Debug1("HandleCharSwitch(%s=%s)\n",name,value);
  
if ((strcmp(value,"true") == 0) || (strcmp(value,"on") == 0))
   {
   *pflag = 'y';
   return;
   }
 
if ((strcmp(value,"false") == 0) || (strcmp(value,"off") == 0))
   {
   *pflag = 'n';
   return;
   }

if ((strcmp(value,"timestamp") == 0))
   {
   *pflag = 's';
   return;
   }
 
 if (ACTION == image)
    {
    printf("Switch %s=(true/false/timestamp)|(on/off)",name); 
    yyerror("Illegal switch value");
    }
 else
    {
    printf("Switch %s=(true/false)|(on/off)",name); 
    yyerror("Illegal switch value");
    }
}

/*******************************************************************/

void HandleIntSwitch(char *name,char *value,int *pflag,int min,int max)

{ int numvalue = -17267592; /* silly number, never happens */
 
Debug1("HandleIntSwitch(%s=%s,%d,%d)\n",name,value,min,max);

sscanf(value,"%d",&numvalue);

if (numvalue == -17267592)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Integer expected as argument to %s",name);
   yyerror(OUTPUT);
   return;
   }
 
if ((numvalue <= max) && (numvalue >= min))
   {
   *pflag = numvalue;
   return;
   }
else
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Integer %s out of range (%d <= %s <= %d)",name,min,name,max);
   yyerror(OUTPUT);
   }
}

/*******************************************************************/

int EditFileExists(char *file)

{ struct Edit *ptr;
  char ebuff[CF_EXPANDSIZE]; 

//ExpandVarstring(file,ebuff,"");
strcpy(ebuff,file);

for (ptr = VEDITLIST; ptr != NULL; ptr=ptr->next)
   {
   if (strcmp(ptr->fname,ebuff) == 0)
      {
      return true;
      }
   }
 
return false;
}

/********************************************************************/

int GetExecOutput(char *command,char *buffer,int useshell)

/* Buffer initially contains whole exec string */

{ int offset = 0;
  char line[CF_BUFSIZE], *sp; 
  FILE *pp;

Debug1("GetExecOutput(%s,%s)\n",command,buffer);
  
if (useshell)
   {
   pp = cfpopen_sh(command,"r");
   }
else
   {
   pp = cfpopen(command,"r");
   }

if (pp == NULL)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open pipe to command %s\n",command);
   CfLog(cfinform,OUTPUT,"pipe");
   return false;
   }

memset(buffer,0,CF_BUFSIZE);
  
while (!feof(pp))
   {
   if (ferror(pp))  /* abortable */
      {
      fflush(pp);
      break;
      }

   ReadLine(line,CF_BUFSIZE,pp);

   if (ferror(pp))  /* abortable */
      {
      fflush(pp);
      break;
      }  
   
   for (sp = line; *sp != '\0'; sp++)
      {
      if (*sp == '\n')
         {
         *sp = ' ';
         }
      }
   
   if (strlen(line)+offset > CF_BUFSIZE-10)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Buffer exceeded %d bytes in exec %s\n",CF_MAXVARSIZE,command);
      CfLog(cferror,OUTPUT,"");
      break;
      }

   snprintf(buffer+offset,CF_BUFSIZE,"%s ",line);
   offset += strlen(line)+1;
   }

if (offset > 0)
   {
   Chop(buffer); 
   }

Debug("GetExecOutput got: [%s]\n",buffer);
 
cfpclose(pp);
return true;
}

/********************************************************************/

void InstallEditFile(char *file,char *edit,char *data)

{ struct Edit *ptr;
  char ebuff[CF_EXPANDSIZE]; 

if (data == NULL)
   {
   Debug1("InstallEditFile(%s,%s,-) with classes %s\n",file,edit,CLASSBUFF);
   }
else
   {
   Debug1("InstallEditFile(%s,%s,%s) with classes %s\n",file,edit,data,CLASSBUFF);
   }

/*
if (!IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing Edit no match\n");
   return;
   }
*/

//ExpandVarstring(file,ebuff,"");
 strcpy(ebuff,file);
 
if ((ptr = (struct Edit *)malloc(sizeof(struct Edit))) == NULL)
   {
   FatalError("Memory Allocation failed for InstallEditFile() #1");
   }
 
if ((ptr->fname = strdup(ebuff)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallEditFile() #2");
   }
 
if (VEDITLISTTOP == NULL)                 /* First element in the list */
   {
   VEDITLIST = ptr;
   }
else
   {
   VEDITLISTTOP->next = ptr;
   }
 
if (strncmp(ebuff,"home",4) == 0 && strlen(ebuff) < 6)
   {
   yyerror("Can't edit home directories: missing a filename after home");
   }

if (strlen(LOCALREPOS) > 0)
   {
   //ExpandVarstring(LOCALREPOS,ebuff,"");
   strcpy(ebuff,LOCALREPOS);
   ptr->repository = strdup(ebuff);
   }
else
   {
   ptr->repository = NULL;
   }

   ptr->ifelapsed = PIFELAPSED;
   ptr->expireafter = PEXPIREAFTER;

ptr->audit = AUDITPTR;
ptr->logaudit = AUDITP;
ptr->lineno = LINENUMBER; /* This might not be true if several stanzas */
ptr->done = 'n';
ptr->split = CF_UNUSED_CHAR;
ptr->scope = strdup(CONTEXTID);
ptr->recurse = 0;
ptr->useshell = 'y';
ptr->binary = 'n';
ptr->next = NULL;
ptr->actions = NULL;
ptr->filters = NULL;
ptr->ignores = NULL;
ptr->umask = UMASK;
ptr->exclusions = NULL;
ptr->inclusions = NULL;
ptr->warn = 'n';
VEDITLISTTOP = ptr;
AddEditAction(file,edit,data);
}

/********************************************************************/

void AddEditAction(char *file,char *edit,char *data)

{ struct Edit *ptr;
  struct Edlist *top,*new, *ep;
  struct TwoDimList *tp = NULL;
  char varbuff[CF_EXPANDSIZE];
  mode_t saved_umask;
  char *sp;

if (data == NULL)
   {
   Debug2("AddEditAction(%s,%s,-)\n",file,edit);
   }
else
   {
   Debug2("AddEditAction(%s,%s,%s)\n",file,edit,data);
   }

/*
if (!IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing Edit no match\n");
   return;
   }
*/

for (ptr = VEDITLIST; ptr != NULL; ptr=ptr->next)
   {
   //ExpandVarstring(file,varbuff,"");
   strcpy(varbuff,file);

   if (strcmp(ptr->fname,varbuff) == 0)
      {
      /* 2D list wrapper start - variable is data */

      if (EditActionsToCode(edit) == EditSplit)
         {
         if (data != NULL)
            {
            ptr->split = *(data);
            Verbose("Found new editfiles list separator \"%c\"",ptr->split);
            return;
            }
         else
            {
            yyerror("Bad list separator for editfiles entry");
            return;
            }
         }

      Build2DListFromVarstring(&tp,data,ptr->split,false);
      Set2DList(tp);
      
      for (sp = Get2DListEnt(tp); sp != NULL; sp = Get2DListEnt(tp))
         {
         if ((new = (struct Edlist *)malloc(sizeof(struct Edlist))) == NULL)
            {
            FatalError("Memory Allocation failed for AddEditAction() #1");
            }

         if (ptr->actions == NULL)
            {
            ptr->actions = new;
            }
         else
            {
            for (top = ptr->actions; top->next != NULL; top=top->next)
               {
               }
            top->next = new;
            }
                  
         if ((new->code = EditActionsToCode(edit)) == NoEdit)
            {
            snprintf(OUTPUT,CF_BUFSIZE,"Unknown edit action \"%s\"",edit);
            yyerror(OUTPUT);
            }

         if (data == NULL)
            {
            new->data = NULL;
            }
         else
            {
            if ((new->data = strdup(sp)) == NULL)
               {
               FatalError("Memory Allocation failed for AddEditAction() #1");
               }
            }
         
         new->next = NULL;

         if ((new->classes = strdup(CLASSBUFF)) == NULL)
            {
            FatalError("Memory Allocation failed for InstallEditFile() #3");
            }
         
         switch(new->code)
            {
            case EditUmask:
                saved_umask = UMASK;
                HandleUmask(sp);
                ptr->umask = UMASK;
                UMASK = saved_umask;
                break;
                
            case WarnIfFileMissing: ptr->warn = 'y';
                break;
            case EditIgnore:
                PrependItem(&(ptr->ignores),sp,CF_ANYCLASS);
                break;
            case EditExclude:
                PrependItem(&(ptr->exclusions),sp,CF_ANYCLASS);
                break;
            case EditInclude:
                PrependItem(&(ptr->inclusions),sp,CF_ANYCLASS);
                break;
            case EditRecurse:
                if (strcmp(data,"inf") == 0)
                   {
                   ptr->recurse = CF_INF_RECURSE;
                   }
                else
                   {
                   ptr->recurse = atoi(data);
                   if (ptr->recurse < 0)
                      {
                      yyerror("Illegal recursion value");
                      }
                   }
                break;
                
            case Append:
                if (EDITGROUPLEVEL == 0)
                   {
                   yyerror("Append used outside of Group - non-convergent");
                   }
                break;
                
            case EditMode:
                if (strcmp(data,"Binary") == 0)
                   {
                   ptr->binary = 'y';
                   }
                break;
                
            case BeginGroupIfNoMatch:
            case BeginGroupIfMatch:
            case BeginGroupIfNoLineMatching:
            case BeginGroupIfLineMatching:
            case BeginGroupIfNoLineContaining:
            case BeginGroupIfLineContaining:
            case BeginGroupIfNoSuchLine:
            case BeginGroupIfDefined:
            case BeginGroupIfNotDefined: 
            case BeginGroupIfFileIsNewer:
            case BeginGroupIfFileExists:
                if (IsListVar(data,ptr->split))
                   {
                   yyerror("List variables are not currently supported in BeginGroup*");
                   }

                EDITGROUPLEVEL++;
                break;
                
            case EndGroup:
                EDITGROUPLEVEL--;
                if (EDITGROUPLEVEL < 0)
                   {
                   yyerror("EndGroup without Begin");
                   }
                break;
                
            case ReplaceAll:
                if (SEARCHREPLACELEVEL > 0)
                   {
                   yyerror("ReplaceAll without With before or at line");
                   }
                
                if (IsListVar(data,ptr->split))
                   {
                   yyerror("List variables are not currently supported in ReplaceAll/With");
                   }
            
                SEARCHREPLACELEVEL++;
                break;
                
            case ReplaceFirst:
                if (SEARCHREPLACELEVEL > 0)
                   {
                   yyerror("ReplaceFirst without With before or at line");
                   }
                
                SEARCHREPLACELEVEL++;
                break;
                
            case With:

                if (IsListVar(data,ptr->split))
                   {
                   yyerror("List variables are not currently supported in ReplaceAll/With");
                   }

                SEARCHREPLACELEVEL--;
                break;
                
            case ForEachLineIn:
                if (FOREACHLEVEL > 0)
                   {
                   yyerror("Nested ForEach loops not allowed");
                   }
                
                FOREACHLEVEL++;
                break;
                
            case EndLoop:
                FOREACHLEVEL--;
                if (FOREACHLEVEL < 0)
                   {
                   yyerror("EndLoop without ForEachLineIn");
                   }
                break;
                
            case DefineInGroup:
                if (EDITGROUPLEVEL <= 0)
                   {
                   yyerror("DefineInGroup outside a group");
                   }
                AddInstallable(new->data);
                break;
                
            case SetLine:
                if (FOREACHLEVEL > 0)
                   {
                   yyerror("SetLine inside ForEachLineIn loop");
                   }
                break;
                
            case FixEndOfLine:
                if (strlen(sp) > CF_EXTRASPC - 1)
                   {
                   yyerror("End of line type is too long!");
                   printf("          (max %d characters allowed)\n",CF_EXTRASPC);
                   }
                break;
            case ReplaceLinesMatchingField:
                if (atoi(sp) == 0)
                   {
                   yyerror("Argument must be an integer, greater than zero");
                   }
                break;
            case EditFilter:
                PrependItem(&(ptr->filters),data,NULL);
                break;
            case ElseDefineClasses:
            case DefineClasses:
                if (EDITGROUPLEVEL > 0 || FOREACHLEVEL > 0)
                   {
                   yyerror("Class definitions inside conditionals or loops are not allowed. Did you mean DefineInGroup?");
                   }
                AddInstallable(new->data);
                break;
            case EditRepos:
                ptr->repository = strdup(sp);
                break;
            }
         
         /* 2d wrapper end */
         }
      
      Delete2DList(tp);
      return;
      }
   }

printf("cfengine: software error - no file matched installing %s edit\n",file);
}

/********************************************************************/

enum editnames EditActionsToCode(char *edit)

{ int i;

Debug2("EditActionsToCode(%s)\n",edit);

for (i = 0; VEDITNAMES[i] != '\0'; i++)
   {
   if (strcmp(VEDITNAMES[i],edit) == 0)
      {
      return (enum editnames) i;
      }
   }

return (NoEdit);
}


/********************************************************************/

void PrependAuditFile(char *file)

{ struct stat statbuf;;

if ((AUDITPTR = (struct Audit *)malloc(sizeof(struct Audit))) == NULL)
   {
   CfLog(cferror,"","malloc audit");
   FatalError("");
   }

if (stat(file,&statbuf) == -1)
   {
   /* shouldn't happen */
   return;
   }

ChecksumFile(file,AUDITPTR->digest,'m');   

AUDITPTR->next = VAUDIT;
AUDITPTR->filename = strdup(file);
AUDITPTR->date = strdup(ctime(&statbuf.st_mtime));
Chop(AUDITPTR->date);
AUDITPTR->version = NULL;
VAUDIT = AUDITPTR;
}

/********************************************************************/

void VersionAuditFile()

{ char *sp;
 
if (sp = GetMacroValue(CONTEXTID,"cfinputs_version"))
   {
   AUDITPTR->version = strdup(sp);
   }
else
   {
   Verbose("Cfengine input file had no explicit version string\n");
   }
}

/********************************************************************/

void AppendInterface(char *ifname,char *address,char *netmask,char *broadcast)

{ struct Interface *ifp;
 
Debug1("Installing item (%s:%s:%s) in the interfaces list\n",ifname,netmask,broadcast);

/*
if (!IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing %s, no match\n",ifname);
   return;
   }
*/

if (strlen(netmask) < 7)
   {
   yyerror("illegal or missing netmask");
   InitializeAction();
   return;
   }

if (strlen(broadcast) < 3)
   {
   yyerror("illegal or missing broadcast address");
   InitializeAction();
   return;
   }
 
if ((ifp = (struct Interface *)malloc(sizeof(struct Interface))) == NULL)
   {
   FatalError("Memory Allocation failed for AppendInterface() #1");
   }

if ((ifp->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for Appendinterface() #2");
   }

if ((ifp->ifdev = strdup(ifname)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendInterface() #3");
   }
 
if ((ifp->netmask = strdup(netmask)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendInterface() #3");
   }

if ((ifp->broadcast = strdup(broadcast)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendInterface() #3");
   } 

if ((ifp->ipaddress = strdup(address)) == NULL)
   {
   FatalError("Memory Allocation failed for AppendInterface() #3");
   } 
 
if (VIFLISTTOP == NULL)                 /* First element in the list */
   {
   VIFLIST = ifp;
   }
 else
    {
    VIFLISTTOP->next = ifp;
    }

ifp->audit = AUDITPTR;
ifp->lineno = LINENUMBER;
ifp->next = NULL;
ifp->done = 'n';
ifp->scope = strdup(CONTEXTID); 
 
VIFLISTTOP = ifp;
 
InitializeAction(); 
}

/*******************************************************************/

void AppendScript(char *item,int timeout,char useshell,char *uidname,char *gidname)

{ struct TwoDimList *tp = NULL;
  struct ShellComm *ptr;
  struct passwd *pw;
  struct group *gw;
  char *sp, ebuff[CF_EXPANDSIZE];
  int uid = CF_NOUSER; 
  int gid = CF_NOUSER;
  
Debug1("Installing shellcommand (%s) in the script list\n",item);

/*
if (!IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing (%s), no class match (%s)\n",item,CLASSBUFF);
   InitializeAction();
   return;
   }
*/

Build2DListFromVarstring(&tp,item,LISTSEPARATOR,false); /* Must be at least one space between each var */

Set2DList(tp);

for (sp = Get2DListEnt(tp); sp != NULL; sp = Get2DListEnt(tp))
   {
   if ((ptr = (struct ShellComm *)malloc(sizeof(struct ShellComm))) == NULL)
      {
      FatalError("Memory Allocation failed for AppendScript() #1");
      }

   if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for Appendscript() #2");
      }

   if ((ptr->name = strdup(sp)) == NULL)
      {
      FatalError("Memory Allocation failed for Appendscript() #3");
      }

   //ExpandVarstring(CHROOT,ebuff,"");
   strcpy(ebuff,CHROOT);

   if ((ptr->chroot = strdup(ebuff)) == NULL)
      {
      FatalError("Memory Allocation failed for Appendscipt() #4b");
      }
   
   //ExpandVarstring(CHDIR,ebuff,"");
   strcpy(ebuff,CHDIR);
   
   if ((ptr->chdir = strdup(ebuff)) == NULL)
      {
      FatalError("Memory Allocation failed for Appendscript() #4c");
      }
   
   if (VSCRIPTTOP == NULL)                 /* First element in the list */
      {
      VSCRIPT = ptr;
      }
   else
      {
      VSCRIPTTOP->next = ptr;
      }

   ptr->uid = strdup(uidname);
   ptr->gid = strdup(gidname);

      ptr->ifelapsed = PIFELAPSED;
      ptr->expireafter = PEXPIREAFTER;

   ptr->audit = AUDITPTR;
   ptr->lineno = LINENUMBER;
   ptr->log = LOGP;
   ptr->inform = INFORMP;
   ptr->logaudit = AUDITP;
   ptr->timeout = timeout;
   ptr->useshell = useshell;
   ptr->umask = UMASK;
   ptr->fork = FORK;
   ptr->preview = PREVIEW;
   ptr->noabspath = NOABSPATH;
   ptr->next = NULL;
   ptr->done = 'n';
   ptr->scope = strdup(CONTEXTID);

   //ExpandVarstring(DEFINECLASSBUFFER,ebuff,"");
   strcpy(ebuff,DEFINECLASSBUFFER);
   
   if ((ptr->defines = strdup(ebuff)) == NULL)
      {
      FatalError("Memory Allocation failed for AppendShellcommand() #3");
      }

   //ExpandVarstring(ELSECLASSBUFFER,ebuff,"");
   strcpy(ebuff,ELSECLASSBUFFER);
   
   if ((ptr->elsedef = strdup(ebuff)) == NULL)
      {
      FatalError("Memory Allocation failed for AppendShellcommand() #4");
      }
   
   AddInstallable(ptr->defines);
   AddInstallable(ptr->elsedef);
   VSCRIPTTOP = ptr;
   }

Delete2DList(tp);
}

/*******************************************************************/

void AppendSCLI(char *item,int timeout,char useshell,char *uidname,char *gidname)

{ struct TwoDimList *tp = NULL;
  struct ShellComm *ptr;
  struct passwd *pw;
  struct group *gw;
  char *sp, ebuff[CF_EXPANDSIZE];
  int uid = CF_NOUSER; 
  int gid = CF_NOUSER;
}

/********************************************************************/

void AppendDisable(char *path,char *type,short rotate,char comp,int size)

{ char *sp;
  struct Disable *ptr;
  struct TwoDimList *tp = NULL;
  char ebuff[CF_EXPANDSIZE];
 
Debug1("Installing item (%s) in the disable list\n",path);

/*
if (!IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing %s, no match\n",path);
   return;
   }
*/

Build2DListFromVarstring(&tp,path,LISTSEPARATOR,false);
Set2DList(tp);

for (sp = Get2DListEnt(tp); sp != NULL; sp = Get2DListEnt(tp))
   {
   if (strlen(type) > 0 && strcmp(type,"plain") != 0 && strcmp(type,"file") !=0 && strcmp(type,"link") !=0
       && strcmp(type,"links") !=0 )
      {
      yyerror("Invalid file type in Disable");
      }

   if ((ptr = (struct Disable *)malloc(sizeof(struct Disable))) == NULL)
      {
      FatalError("Memory Allocation failed for AppendDisable() #1");
      }
   
   if ((ptr->name = strdup(sp)) == NULL)
      {
      FatalError("Memory Allocation failed for AppendDisable() #2");
      }
   
   //ExpandVarstring(DEFINECLASSBUFFER,ebuff,"");
   strcpy(ebuff,DEFINECLASSBUFFER);

   if ((ptr->defines = strdup(ebuff)) == NULL)
      {
      FatalError("Memory Allocation failed for AppendDisable() #3");
      }

   //ExpandVarstring(ELSECLASSBUFFER,ebuff,"");
   strcpy(ebuff,ELSECLASSBUFFER);
   
   if ((ptr->elsedef = strdup(ebuff)) == NULL)
      {
      FatalError("Memory Allocation failed for AppendDisable() #3");
      } 
      
   if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for AppendDisable() #4");
      }
   
   if (strlen(type) == 0)
      {
      sprintf(ebuff,"all");
      }
   else
      {
      sprintf(ebuff,"%s",type);
      }
   
   if ((ptr->type = strdup(ebuff)) == NULL)
      {
      FatalError("Memory Allocation failed for AppendDisable() #4");
      }

   //ExpandVarstring(DESTINATION,ebuff,"");
   strcpy(ebuff,DESTINATION);

   if ((ptr->destination = strdup(ebuff)) == NULL)
      {
      FatalError("Memory Allocation failed for AppendDisable() #3");
      }

   if (VDISABLETOP == NULL)                 /* First element in the list */
      {
      VDISABLELIST = ptr;
      }
   else
      {
      VDISABLETOP->next = ptr;
      }

   if (strlen(LOCALREPOS) > 0)
      {
      //ExpandVarstring(LOCALREPOS,ebuff,"");
      strcpy(ebuff,LOCALREPOS);
      ptr->repository = strdup(ebuff);
      }
   else
      {
      ptr->repository = NULL;
      }

      ptr->ifelapsed = PIFELAPSED;
      ptr->expireafter = PEXPIREAFTER;

   ptr->audit = AUDITPTR;
   ptr->lineno = LINENUMBER;
   ptr->rotate = rotate;
   ptr->comp = comp;
   ptr->size = size;
   ptr->next = NULL;
   ptr->log = LOGP;
   ptr->inform = INFORMP;
   ptr->logaudit = AUDITP;
   ptr->done = 'n';
   ptr->scope = strdup(CONTEXTID);
   ptr->action = PROACTION;
   
   VDISABLETOP = ptr;
   InitializeAction();
   AddInstallable(ptr->defines);
   AddInstallable(ptr->elsedef);
   }
 
 Delete2DList(tp);  
}

/*******************************************************************/

void InstallMethod(char *function,char *file)

{ char *sp,*vp,work[CF_EXPANDSIZE],name[CF_BUFSIZE];
  struct Method *ptr;
  struct Item *bare_send_args = NULL, *sip;
  uid_t uid = CF_NOUSER;
  gid_t gid = CF_NOUSER;
  struct passwd *pw;
  struct group *gw;
  struct TwoDimList *tp = NULL;
   
Debug1("Install item (%s=%s) in the methods list iff %s?\n",function,file,CLASSBUFF);

if (strlen(file) == 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Missing action file from declaration of method %s",function);
   yyerror(OUTPUT);
   return;
   }
 
memset(name,0,CF_BUFSIZE);

if (!strstr(function,"("))
   {
   yyerror("Missing parenthesis or extra space");
   InitializeAction();
   return;
   }

/* First look at bare args to cache an arg fingerprint */ 
strcpy(work,function);

if (work[strlen(work)-1] != ')')
   {
   yyerror("Illegal use of space or nested parentheses");
   }
 
work [strlen(work)-1] = '\0';   /*chop last ) */
 
sscanf(function,"%[^(]",name); 

if (strlen(name) == 0)
   {
   yyerror("Empty method");
   return;
   }
 
for (sp = work; sp != NULL; sp++) /* Pick out the args*/
   {
   if (*sp == '(')
      {
      break;
      }
   }
 
sp++; 

if (strlen(sp) == 0)
   {
   yyerror("Missing argument (void?) to method");
   }
 
/* Now expand variables */
/* ExpandVarstring(function,work,""); */

strcpy(work,function);

Build2DListFromVarstring(&tp,work,LISTSEPARATOR,true); /* step in time for lists here */
Set2DList(tp);

for (vp = Get2DListEnt(tp); vp != NULL; vp = Get2DListEnt(tp))
   {
   Debug1("Installing item (%s=%s) in the methods list iff %s\n",function,file,CLASSBUFF);
   
   if ((ptr = (struct Method *)malloc(sizeof(struct Method))) == NULL)
      {
      FatalError("Memory Allocation failed for InstallMethod() #1");
      }
   
   if (VMETHODSTOP == NULL)
      {
      VMETHODS = ptr;
      }
   else
      {
      VMETHODSTOP->next = ptr;
      }
   
   if (vp[strlen(vp)-1] != ')')
      {
      yyerror("Illegal use of space or nested parentheses");
      }
   
   vp [strlen(vp)-1] = '\0';   /*chop last ) */
   
   sscanf(work,"%[^(]",name); 
   
   if (strlen(name) == 0)
      {
      yyerror("Empty method");
      return;
      }

   for (sp = vp; sp != NULL; sp++) /* Pick out the args*/
      {
      if (*sp == '(')
         {
         break;
         }
      }
   
   sp++; 
   
   if (strlen(sp) == 0)
      {
      yyerror("Missing argument (void?) to method");
      }

   Debug("Found args [%s]\n\n",sp);
   
   ptr->send_args = ListFromArgs(sp);
   ptr->send_classes = SplitStringAsItemList(METHODREPLYTO,','); 

   METHODREPLYTO[0] = '\0';
   
   if ((ptr->name = strdup(name)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallMethod() #2");
      }
   
   if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallMethod() #3");
      }
   
   if (strlen(file) == 0)
      {
      yyerror("Missing filename in method");
      return;
      }
   
   if (strcmp(file,"dispatch") == 0)
      {
      ptr->invitation = 'y';
      }
   else
      {
      ptr->invitation = 'n';
      }
   
   if (file[0] == '/' || file[0] == '.')
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Method name (%s) was absolute. Must be in trusted Modules directory (no path prefix)",file);
      yyerror(OUTPUT);
      return;
      }
   
   ptr->file = strdup(file);
   
   if (strlen(CFSERVER) > 0)
      {
      ptr->servers = SplitStringAsItemList(CFSERVER,',');
      }
   else
      {
      ptr->servers = SplitStringAsItemList("localhost",',');
      }
   
   bare_send_args = ListFromArgs(sp);
   
   /* Append server to make this unique */

   for (sip = ptr->servers; sip != NULL; sip = sip->next)
      {
      PrependItem(&bare_send_args,sip->name,NULL);
      }
   
   ChecksumList(bare_send_args,ptr->digest,'m');
   DeleteItemList(bare_send_args);   

   ptr->audit = AUDITPTR;
   ptr->lineno = LINENUMBER;
   ptr->bundle = NULL;
   ptr->return_vars = SplitStringAsItemList(METHODFILENAME,',');
   ptr->return_classes = SplitStringAsItemList(PARSEMETHODRETURNCLASSES,','); 
   ptr->scope = strdup(CONTEXTID);
   ptr->useshell = USESHELL;
   ptr->log = LOGP;
   ptr->inform = INFORMP;
   ptr->logaudit = AUDITP;
   
   if (*VUIDNAME == '*')
      {
      ptr->uid = CF_SAME_OWNER;      
      }
   else if (isdigit((int)*VUIDNAME))
      {
      sscanf(VUIDNAME,"%d",&uid);
      
      if (uid == CF_NOUSER)
         {
         ptr->uid = 678;
         }
      else
         {
         ptr->uid = uid;
         }
      }
   else if ((pw = getpwnam(VUIDNAME)) == NULL)
      {
      ptr->uid = 678;
      }
   else
      {
      ptr->uid = pw->pw_uid;
      }
   
   if (*VGIDNAME == '*')
      {
      ptr->gid = CF_SAME_GROUP;
      }
   else if (isdigit((int)*VGIDNAME))
      {
      sscanf(VGIDNAME,"%d",&gid);
      if (gid == CF_NOUSER)
         {
         ptr->gid = 678;
         }
      else
         {
         ptr->gid = gid;
         }
      }
   else if ((gw = getgrnam(VGIDNAME)) == NULL)
      {
      ptr->gid = 678;
      }
   else
      {
      ptr->gid = gw->gr_gid;
      }
   
      ptr->ifelapsed = PIFELAPSED;
      ptr->expireafter = PEXPIREAFTER;
   
   //ExpandVarstring(CHROOT,work,"");
   strcpy(work,CHROOT);
   
   if ((ptr->chroot = strdup(work)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallProcItem() #4b");
      }
   
   //ExpandVarstring(CHDIR,work,"");
   strcpy(work,CHDIR);
   
   if ((ptr->chdir = strdup(work)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallProcItem() #4c");
      }
   
   //ExpandVarstring(METHODFORCE,work,"");
   strcpy(work,METHODFORCE);
   
   if ((ptr->forcereplyto = strdup(work)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallProcItem() #4c");
      }
   
   ptr->next = NULL;
   VMETHODSTOP = ptr;
   }

InitializeAction();
}

/*******************************************************************/

void InstallTidyItem(char *path,char *wild,int rec,short age,char travlinks,int tidysize,char type,char ldirs,char tidydirs,char *classes)

{ struct TwoDimList *tp = NULL;
  char *sp;

if (strcmp(path,"/") != 0)
   {
   DeleteSlash(path);
   }

Build2DListFromVarstring(&tp,path,LISTSEPARATOR,false);
Set2DList(tp);

for (sp = Get2DListEnt(tp); sp != NULL; sp = Get2DListEnt(tp))
   {
   if (TidyPathExists(sp))
      {
      AddTidyItem(sp,wild,rec,age,travlinks,tidysize,type,ldirs,tidydirs,classes);
      }
   else
      {
      InstallTidyPath(sp,wild,rec,age,travlinks,tidysize,type,ldirs,tidydirs,classes);
      }
   }

Delete2DList(tp);
InitializeAction();
}

/*******************************************************************/

void InstallMakePath(char *path,mode_t plus,mode_t minus,char *uidnames,char *gidnames)

{ struct File *ptr;
 char buffer[CF_EXPANDSIZE],ebuff[CF_MAXVARSIZE]; 
  struct Item *list = NULL, *ip;
  struct TwoDimList *tp = NULL;
  char *sp;
  int lastnode = false;
  
Debug1("InstallMakePath (%s) (+%o)(-%o)(%s)(%s)\n",path,plus,minus,uidnames,gidnames);

/*
if (!IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing directory item, no match\n");
   return;
   }
*/

Build2DListFromVarstring(&tp,path,LISTSEPARATOR,false);

Set2DList(tp);

for (sp = Get2DListEnt(tp); sp != NULL; sp = Get2DListEnt(tp))    
   {
   if (strcmp(uidnames,"LastNode") == 0)
      {
      strncpy(ebuff,ReadLastNode(sp),CF_MAXVARSIZE-1);
      Verbose("File owner will be set to directory last node (%s)\n",ebuff);      
      }
   else
      {
      strncpy(ebuff,uidnames,CF_MAXVARSIZE-1);
      }


   if ((ptr = (struct File *)malloc(sizeof(struct File))) == NULL)
      {
      FatalError("Memory Allocation failed for InstallMakepath() #1");
      }
   
   if ((ptr->path = strdup(sp)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallMakepath() #2");
      }
   
   if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallMakepath() #3");
      }
   
   //ExpandVarstring(DEFINECLASSBUFFER,buffer,""); 
   strcpy(buffer,DEFINECLASSBUFFER);
   
   if ((ptr->defines = strdup(buffer)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallMakepath() #3a");
      }
   
   //ExpandVarstring(ELSECLASSBUFFER,buffer,""); 
   strcpy(buffer,ELSECLASSBUFFER);
   
   if ((ptr->elsedef = strdup(buffer)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallMakepath() #3a");
      }
   
   AddInstallable(ptr->defines);
   AddInstallable(ptr->elsedef);  
   
   if (VMAKEPATHTOP == NULL)                 /* First element in the list */
      {
      VMAKEPATH = ptr;
      }
   else
      {
      VMAKEPATHTOP->next = ptr;
      }
   
      ptr->ifelapsed = PIFELAPSED;
   
      ptr->expireafter = PEXPIREAFTER;


   ptr->audit = AUDITPTR;
   ptr->lineno = LINENUMBER;
   ptr->plus = plus;
   ptr->minus = minus;
   ptr->thismode = strdup(THISMODE);
   ptr->recurse = 0;
   ptr->action = fixdirs;
   ptr->uid = strdup(ebuff);
   ptr->gid = strdup(gidnames);
   ptr->inclusions = NULL;
   ptr->ignores = NULL;
   ptr->exclusions = NULL;
   ptr->acl_aliases= VACLBUILD; 
   ptr->log = LOGP;
   ptr->filters = NULL;
   ptr->inform = INFORMP;
   ptr->logaudit = AUDITP;
   ptr->plus_flags = PLUSFLAG;
   ptr->minus_flags = MINUSFLAG;
   ptr->done = 'n';
   ptr->scope = strdup(CONTEXTID); 
   ptr->next = NULL;
   VMAKEPATHTOP = ptr;
   }

InitializeAction();
}

/*******************************************************************/

void HandleTravLinks(char *value)

{
if (ACTION == tidy && strncmp(CURRENTOBJECT,"home",4) == 0)
   {
   yyerror("Can't use links= option with special variable home in tidy");
   yyerror("Use command line options instead.\n");
   }

if (PTRAVLINKS != '?')
   {
   Warning("redefinition of links= option");
   }

if ((strcmp(value,"stop") == 0) || (strcmp(value,"false") == 0))
   {
   PTRAVLINKS = (short) 'F';
   return;
   }

if ((strcmp(value,"traverse") == 0) || (strcmp(value,"follow") == 0) || (strcmp(value,"true") == 0))
   {
   PTRAVLINKS = (short) 'T';
   return;
   }

if ((strcmp(value,"tidy"))==0)
   {
   PTRAVLINKS = (short) 'K';
   return;
   }

yyerror("Illegal links= specifier");
}

/*******************************************************************/

void HandleTidySize(char *value)

{ int num = -1;
  char *sp, units = 'k';

for (sp = value; *sp != '\0'; sp++)
   {
   *sp = ToLower(*sp);
   }

if (strcmp(value,"empty") == 0)
   {
   TIDYSIZE = CF_EMPTYFILE;
   }
else
   {
   sscanf(value,"%d%c",&num,&units);

   if (num <= 0)
      {
      if (*value == '>')
         {
         sscanf(value+1,"%d%c",&num,&units);
         if (num <= 0)
            {
            yyerror("size value must be a decimal number with units m/b/k");
            }
         }
      else
         {
         yyerror("size value must be a decimal number with units m/b/k");
         }
      }
   
   switch (units)
      {
      case 'b': TIDYSIZE = num;
          break;
      case 'm': TIDYSIZE = num * 1024 * 1024;
          break;
      default:  TIDYSIZE = num * 1024;
      }
   }

}

/*******************************************************************/

void HandleUmask(char *value)

{ int num = -1;

Debug("HandleUmask(%s)",value);
 
sscanf(value,"%o",&num);

if (num < 0)
   {
   yyerror("umask value must be an octal number >= zero");
   }

UMASK = (mode_t) num;
}

/*******************************************************************/

void HandleDisableSize(char *value)

{ int i = -1;
  char *sp, units = 'b';

for (sp = value; *sp != '\0'; sp++)
   {
   *sp = ToLower(*sp);
   }

switch (*value)
   {
   case '>': DISCOMP = '>';
             value++;
             break;
   case '<': DISCOMP = '<';
             value++;
             break;
   default : DISCOMP = '=';
   }

sscanf(value,"%d%c",&i,&units);

if (i < 1)
   {
   yyerror("disable size attribute with silly value (must be > 0)");
   }

switch (units)
   {
   case 'k': DISABLESIZE = i * 1024;
             break;
   case 'm': DISABLESIZE = i * 1024 * 1024;
             break;
   default:  DISABLESIZE = i;
   }
}

/*******************************************************************/

void HandleCopySize(char *value)

{ int i = -1;
  char *sp, units = 'b';

for (sp = value; *sp != '\0'; sp++)
   {
   *sp = ToLower(*sp);
   }

switch (*value)
   {
   case '>': IMGCOMP = '>';
             value++;
             break;
   case '<': IMGCOMP = '<';
             value++;
             break;
   default : IMGCOMP = '=';
   }

sscanf(value,"%d%c",&i,&units);

if (i < 0)
   {
   yyerror("copy size attribute with silly value (must be a non-negative number)");
   }

switch (units)
   {
   case 'k': IMGSIZE = i * 1024;
             break;
   case 'm': IMGSIZE = i * 1024 * 1024;
             break;
   default:  IMGSIZE = i;
   }
}

/*******************************************************************/

void HandleRequiredSize(char *value)

{ int i = -1;
  char *sp, units = 'b';

for (sp = value; *sp != '\0'; sp++)
   {
   *sp = ToLower(*sp);
   }

switch (*value)
   {
   case '>': IMGCOMP = '>';
             value++;
             break;
   case '<': IMGCOMP = '<';
             value++;
             break;
   default : IMGCOMP = '=';
   }

sscanf(value,"%d%c",&i,&units);

if (i < 1)
   {
   yyerror("disk/required size attribute with silly value (must be > 0)");
   }

switch (units)
   {
   case 'b': IMGSIZE = i / 1024;
             break;
   case 'm': IMGSIZE = i * 1024;
             break;
   case '%': IMGSIZE = -i;       /* -ve number signals percentage */
             break;
   default:  IMGSIZE = i;
   }
}

/*******************************************************************/

void HandleTidyType(char *value)

{
if (strcmp(value,"a")== 0 || strcmp(value,"atime") == 0)
   {
   AGETYPE = 'a';
   return;
   }

if (strcmp(value,"m")== 0 || strcmp(value,"mtime") == 0)
   {
   AGETYPE = 'm';
   return;
   }

if (strcmp(value,"c")== 0 || strcmp(value,"ctime") == 0)
   {
   AGETYPE = 'c';
   return;
   }

yyerror("Illegal age search type, must be atime/ctime/mtime");
}

/*******************************************************************/

void HandleTidyLinkDirs(char *value)

{
if (strcmp(value,"keep")== 0)
   {
   LINKDIRS = 'k';
   return;
   }

if ((strcmp(value,"tidy")== 0) || (strcmp(value,"delete") == 0))
   {
   LINKDIRS = 'y';
   return;
   }

yyerror("Illegal linkdirs value, must be keep/delete/tidy");
}

/*******************************************************************/

void HandleTidyRmdirs(char *value)

{
if ((strcmp(value,"true") == 0)||(strcmp(value,"all") == 0))
   {
   TIDYDIRS = 'y';
   return;
   }

if ((strcmp(value,"false") == 0)||(strcmp(value,"none") == 0))
   {
   TIDYDIRS = 'n';
   return;
   }

if (strcmp(value,"sub") == 0)
   {
   TIDYDIRS = 's';
   return;
   }

yyerror("Illegal rmdirs value, must be true/false/sub");
}

/*******************************************************************/

void HandleTimeOut(char *value)

{ int num = -1;

sscanf(value,"%d",&num);

if (num <= 0)
   {
   yyerror("timeout value must be a decimal number > 0");
   }

VTIMEOUT = num;
}


/*******************************************************************/

void HandleUseShell(char *value)

{
 if (strcmp(value,"true") == 0)
   {
   USESHELL = 'y';
   return;
   }

if (strcmp(value,"false") == 0)
   {
   USESHELL = 'n';
   return;
   }

if (strcmp(value,"dumb") == 0)
   {
   USESHELL = 'd';
   return;
   }

yyerror("Illegal attribute for useshell= ");
}

/*******************************************************************/

void HandleChecksum(char *value)

{
CHECKSUM = ChecksumType(ToLowerStr(value));
}

/*******************************************************************/

void HandleTimeStamps(char *value)

{
if (strcmp(value,"preserve") == 0 || strcmp(value,"keep") == 0)
   {
   PRESERVETIMES = 'y';
   return;
   }

PRESERVETIMES = 'n';
}

/*******************************************************************/

int GetFileAction(char *action)

{ int i;

for (i = 0; FILEACTIONTEXT[i] != '\0'; i++)
   {
   if (strcmp(action,FILEACTIONTEXT[i]) == 0)
      {
      return i;
      }
   }

yyerror("Unknown action type");
return (int) warnall;
}


/*******************************************************************/

void InstallFileListItem(char *path,mode_t plus,mode_t minus,enum fileactions action,char *uidnames,char *gidnames,int recurse,char travlinks,char chksum)

{ struct File *ptr;
  char *spl,ebuff[CF_EXPANDSIZE];
  struct TwoDimList *tp = NULL;

Debug1("InstallFileaction (%s) (+%o)(-%o) (%s) (%d) (%c)\n",path,plus,minus,FILEACTIONTEXT[action],action,travlinks);

/*
if (!IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing file item, no match\n");
   return;
   }
*/

Build2DListFromVarstring(&tp,path,LISTSEPARATOR,false);
Set2DList(tp);

for (spl = Get2DListEnt(tp); spl != NULL; spl = Get2DListEnt(tp))
   {
   if ((ptr = (struct File *)malloc(sizeof(struct File))) == NULL)
      {
      FatalError("Memory Allocation failed for InstallFileListItem() #1");
      }

   if ((ptr->path = strdup(spl)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallFileListItem() #2");
      }

   if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallFileListItem() #3");
      }

   //ExpandVarstring(DEFINECLASSBUFFER,ebuff,""); 
   strcpy(ebuff,DEFINECLASSBUFFER);
   
   if ((ptr->defines = strdup(ebuff)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallFileListItem() #3");
      }

   //ExpandVarstring(ELSECLASSBUFFER,ebuff,""); 
   strcpy(ebuff,ELSECLASSBUFFER);
   
   if ((ptr->elsedef = strdup(ebuff)) == NULL)
      {
      FatalError("Memory Allocation failed for InstallFileListItem() #3");
      }
   
   AddInstallable(ptr->defines);
   AddInstallable(ptr->elsedef);   

   if (VFILETOP == NULL)                 /* First element in the list */
      {
      VFILE = ptr;
      }
   else
      {
      VFILETOP->next = ptr;
      }

      ptr->ifelapsed = PIFELAPSED;   
      ptr->expireafter = PEXPIREAFTER;


   ptr->audit = AUDITPTR;
   ptr->lineno = LINENUMBER;
   ptr->action = action;
   ptr->plus = plus;
   ptr->minus = minus;
   ptr->thismode = strdup(THISMODE);
   ptr->recurse = recurse;
   ptr->uid = strdup(uidnames);
   ptr->gid = strdup(gidnames);
   ptr->exclusions = VEXCLUDEPARSE;
   ptr->inclusions = VINCLUDEPARSE;
   ptr->filters = NULL;
   ptr->ignores = VIGNOREPARSE;
   ptr->travlinks = travlinks;
   ptr->acl_aliases  = VACLBUILD;
   ptr->filters = VFILTERBUILD;
   ptr->next = NULL;
   ptr->log = LOGP;
   ptr->xdev = XDEV;
   ptr->rxdirs = RXDIRS;
   ptr->inform = INFORMP;
   ptr->logaudit = AUDITP;
   ptr->checksum = chksum;
   ptr->plus_flags = PLUSFLAG;
   ptr->minus_flags = MINUSFLAG;
   ptr->done = 'n';
   ptr->scope = strdup(CONTEXTID);

   VFILETOP = ptr;
   Debug1("Installed file object %s\n",spl);
   }

Delete2DList(tp);
InitializeAction();
}


/*******************************************************************/

void InstallProcessItem(char *expr,char *restart,short matches,char comp,short signal,char action,char *classes,char useshell,char *uidname,char *gidname)

{ struct Process *ptr;
  char ebuff[CF_EXPANDSIZE];
  uid_t uid;
  gid_t gid;
  struct passwd *pw;
  struct group *gw;
  

Debug1("InstallProcessItem(%s,%s,%d,%d,%c)\n",expr,restart,matches,signal,action);

/*
if ( ! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing process item, no match\n");
   return;
   }
*/

if ((ptr = (struct Process *)malloc(sizeof(struct Process))) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #1");
   }

//ExpandVarstring(expr,ebuff,"");
strcpy(ebuff,expr);
    
if ((ptr->expr = strdup(ebuff)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #2");
   }

//ExpandVarstring(restart,ebuff,"");
strcpy(ebuff,restart);
   
if (strcmp(ebuff,"SetOptionString") == 0)
   {
   if ((strlen(ebuff) > 0) && ebuff[0] != '/')
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Restart command (%s) does not have an absolute pathname",ebuff);
      yyerror(OUTPUT);
      }
   }
 
if ((ptr->restart = strdup(ebuff)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #3");
   }

//ExpandVarstring(DEFINECLASSBUFFER,ebuff,""); 
strcpy(ebuff,DEFINECLASSBUFFER);
    
if ((ptr->defines = strdup(ebuff)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #4");
   }

//ExpandVarstring(ELSECLASSBUFFER,ebuff,""); 
strcpy(ebuff,ELSECLASSBUFFER);
   
if ((ptr->elsedef = strdup(ebuff)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #4a");
   }

//ExpandVarstring(CHROOT,ebuff,"");
strcpy(ebuff,CHROOT);
    
if ((ptr->chroot = strdup(ebuff)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #4b");
   }

//ExpandVarstring(CHDIR,ebuff,"");
strcpy(ebuff,CHDIR);
    
if ((ptr->chdir = strdup(ebuff)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #4c");
   }
 
if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallProcItem() #5");
   }

if (VPROCTOP == NULL)                 /* First element in the list */
   {
   VPROCLIST = ptr;
   }
else
   {
   VPROCTOP->next = ptr;
   }

   ptr->ifelapsed = PIFELAPSED;
   ptr->expireafter = PEXPIREAFTER;

ptr->audit = AUDITPTR;
ptr->lineno = LINENUMBER;
ptr->matches = matches;
ptr->comp = comp;
ptr->signal = signal;
ptr->action = action;
ptr->umask = UMASK;
ptr->useshell = useshell; 
ptr->next = NULL;
ptr->log = LOGP;
ptr->inform = INFORMP;
ptr->logaudit = AUDITP;
ptr->exclusions = VEXCLUDEPARSE;
ptr->inclusions = VINCLUDEPARSE;
ptr->filters = ptr->filters = VFILTERBUILD;
ptr->done = 'n';
ptr->scope = strdup(CONTEXTID);
 
ptr->uid = strdup(uidname);
ptr->gid = strdup(gidname);

VPROCTOP = ptr;
InitializeAction();
AddInstallable(ptr->defines);
AddInstallable(ptr->elsedef);  
}

/*******************************************************************/

void InstallPackagesItem(char *name,char *ver,enum cmpsense sense,enum pkgmgrs mgr,enum pkgactions action)

{ struct Package *ptr;
  char buffer[CF_EXPANDSIZE];

  /*
if ( ! IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing packages item, no match\n");
   return;
   }
  */
  
/* If the package manager is set to pkgmgr_none, then an invalid
 * manager was specified, so we don't need to do anything */

  
Debug("InstallPackagesItem(%s,%s,%s,%s,%s)\n",
        name,ver,CMPSENSETEXT[sense],PKGMGRTEXT[mgr],PKGACTIONTEXT[action]);

if ((ptr = (struct Package *)malloc(sizeof(struct Package))) == NULL)
   {
   FatalError("Memory Allocation failed for InstallPackageItem() #1");
   }

if ((ptr->name = strdup(name)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallPackageItem() #2");
   }

if ((ptr->ver = strdup(ver)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallPackageItem() #3");
   }

if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallPackageItem() #4");
   }

//ExpandVarstring(DEFINECLASSBUFFER,buffer,"");
strcpy(buffer,DEFINECLASSBUFFER);
   
if ((ptr->defines = strdup(buffer)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallPackageItem() #4a");
   }

//ExpandVarstring(ELSECLASSBUFFER,buffer,"");
strcpy(buffer,ELSECLASSBUFFER);
   
if ((ptr->elsedef = strdup(buffer)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallPackageItem() #4b");
   }

AddInstallable(ptr->defines);
AddInstallable(ptr->elsedef);

if (VPKGTOP == NULL)                 /* First element in the list */
   {
   VPKG = ptr;
   }
else
   {
   VPKGTOP->next = ptr;
   }

   ptr->ifelapsed = PIFELAPSED;
   ptr->expireafter = PEXPIREAFTER;
 
ptr->audit = AUDITPTR;
ptr->lineno = LINENUMBER;
ptr->log = LOGP;
ptr->inform = INFORMP;
ptr->logaudit = AUDITP;
ptr->cmp = sense;
ptr->pkgmgr = mgr;
ptr->action = action;
ptr->done = 'n';
ptr->scope = strdup(CONTEXTID);

ptr->next = NULL;
VPKGTOP = ptr;
InitializeAction();
}

/*******************************************************************/

int GetCmpSense(char *sense)

{ int i;

for (i = 0; CMPSENSETEXT[i] != '\0'; i++)
   {
   if (strcmp(sense,CMPSENSETEXT[i]) == 0)
      {
      return i;
      }
   }

yyerror("Unknown comparison sense");
return (int) cmpsense_eq;
}

/*******************************************************************/

int GetPkgMgr(char *pkgmgr)

{ int i;
for (i = 0; PKGMGRTEXT[i] != '\0'; i++)
   {
   if (strcmp(pkgmgr,PKGMGRTEXT[i]) == 0)
      {
      return i;
      }
   }

yyerror("Unknown package manager");
return (int) pkgmgr_none;
}

/*******************************************************************/

int GetPkgAction(char *pkgaction)

{ int i;
for (i = 0; PKGACTIONTEXT[i] != '\0'; i++)
   {
   if (strcmp(pkgaction,PKGACTIONTEXT[i]) == 0)
      {
      return i;
      }
   }

yyerror("Unknown package action");
return (int) pkgaction_none;
}

/*******************************************************************/

void InstallImageItem(char *cf_findertype,char *path,mode_t plus,mode_t minus,char *destination,char *action,char *uidnames,char *gidnames,int size,char comp,int rec,char type,char lntype,char *server)

{ struct Image *ptr;
  char *spl; 
  char buf1[CF_EXPANDSIZE], buf2[CF_EXPANDSIZE], buf3[CF_EXPANDSIZE], buf4[CF_EXPANDSIZE];
  struct TwoDimList *tp = NULL;
  struct Item *expserver = NULL, *ep;

/*
if (!IsInstallable(CLASSBUFF))
   {
   Debug1("Not installing copy item, no match (%s,%s)\n",path,CLASSBUFF);
   InitializeAction();
   return;
   }
*/
  
buf2[0] = '\0';
  
Debug1("InstallImageItem (%s) (+%o)(-%o) (%s), server=%s\n",path,plus,minus,destination,server);

if (strlen(action) == 0)   /* default action */
   {
   strcat(action,"fix");
   }

if (!(strcmp(action,"silent") == 0 || strcmp(action,"warn") == 0 || strcmp(action,"fix") == 0))
   {
   sprintf(VBUFF,"Illegal action in image/copy item: %s",action);
   yyerror(VBUFF);
   return;
   }

//ExpandVarstring(path,buf1,"");
strcpy(buf1,path);
//ExpandVarstring(server,buf3,"");
strcpy(buf3,server);
   
if (strlen(buf1) > 1)
   {
   DeleteSlash(buf1);
   }

expserver = SplitStringAsItemList(buf3,LISTSEPARATOR);
   
Build2DListFromVarstring(&tp,path,LISTSEPARATOR,false);  /* Must split on space in comm string */
Set2DList(tp);

for (spl = Get2DListEnt(tp); spl != NULL; spl = Get2DListEnt(tp))
   {
   for (ep = expserver; ep != NULL; ep=ep->next)
      {
      Debug("\nBegin pass with server = %s\n",ep->name);
      
      if ((ptr = (struct Image *)malloc(sizeof(struct Image))) == NULL)
         {
         FatalError("Memory Allocation failed for InstallImageItem() #1");
         }
      
      if ((ptr->classes = strdup(CLASSBUFF)) == NULL)
         {
         FatalError("Memory Allocation failed for InstallImageItem() #3");
         }
            
      if (!FORCENETCOPY && ((strcmp(ep->name,VFQNAME) == 0) || (strcmp(ep->name,VUQNAME) == 0) || (strcmp(ep->name,VSYSNAME.nodename) == 0)))
         {
         Debug("Swapping %s for localhost\n",server);
         free(ep->name);
         ep->name = strdup("localhost");
         }
            
      if (strlen(ep->name) > MAXHOSTNAMELEN)
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Server name (%s) is too long",ep->name);
         yyerror(OUTPUT);
         DeleteItemList(expserver);   
         return;
         }
      
      if (!IsItemIn(VSERVERLIST,ep->name))   /* cache list of servers */
         {
         AppendItem(&VSERVERLIST,ep->name,NULL);
         }
     
   
      Debug("Transformed server %s into %s\n",server,ep->name);
      
      if ((ptr->server = strdup(ep->name)) == NULL)
         {
         FatalError("Memory Allocation failed for InstallImageItem() #5");
         }
      
      if ((ptr->action = strdup(action)) == NULL)
         {
         FatalError("Memory Allocation failed for InstallImageItem() #6");
         }
      
      //ExpandVarstring(DEFINECLASSBUFFER,buf4,"");
      strcpy(buf4,DEFINECLASSBUFFER);
         
      if ((ptr->defines = strdup(buf4)) == NULL)
         {
         FatalError("Memory Allocation failed for InstallImageItem() #7");
         }
      
      //ExpandVarstring(ELSECLASSBUFFER,buf4,"");
      strcpy(buf4,ELSECLASSBUFFER);
         
      if ((ptr->elsedef = strdup(buf4)) == NULL)
         {
         FatalError("Memory Allocation failed for InstallImageItem() #7");
         }
      
      //ExpandVarstring(FAILOVERBUFFER,buf4,"");
      strcpy(buf4,FAILOVERBUFFER);
         
      if ((ptr->failover = strdup(buf4)) == NULL)
         {
         FatalError("Memory Allocation failed for InstallImageItem() #8");
         }
      
      if (strlen(destination) == 0)
         {
         strcpy(buf2,spl);
         }
      else
         {
         //ExpandVarstring(destination,buf2,"");
         strcpy(buf2,destination);
         }

      if (IsDefinedClass(CLASSBUFF))
         {
         if ((strcmp(spl,buf2) == 0) && (strcmp(ep->name,"localhost") == 0))
            {
            yyerror("Image loop: file/dir copies to itself or missing destination file");
            continue;
            }
         }

      if (strlen(buf2) > 1)
         {
         DeleteSlash(buf2);
         }
      
      if (!IsAbsoluteFileName(buf2))
         {
         if (strncmp(buf2,"home",4) == 0)
            {
            if (strlen(buf2) > 4 && buf2[4] != '/')
               {
               yyerror("illegal use of home or not absolute pathname");
               return;
               }
            }
         else if (*buf2 == '$')
            {
            /* unexpanded variable */
            }
         else
            {
            snprintf(OUTPUT,CF_BUFSIZE,"Image %s needs an absolute pathname",buf2);
            yyerror(OUTPUT);
            return;
            }
         }
      
      if ((ptr->destination = strdup(buf2)) == NULL)
         {
         FatalError("Memory Allocation failed for InstallImageItem() #4");
         }
      
      if ((ptr->path = strdup(spl)) == NULL)
         {
         FatalError("Memory Allocation failed for InstallImageItem() #2");
         }
      
      if (VIMAGETOP == NULL)
         {
         VIMAGE = ptr;
         }
      else
         {
         VIMAGETOP->next = ptr;
         }
      
      
      if ((ptr->cf_findertype = strdup(cf_findertype)) == NULL)
         {
         FatalError("Memory Allocation failed for cf_findertype ptr in InstallImageItem()");
         }
      
      ptr->plus = plus;
      ptr->minus = minus;
      ptr->thismode = strdup(THISMODE);
      ptr->uid = strdup(uidnames);
      ptr->gid = strdup(gidnames);
      ptr->force = FORCE;
      ptr->next = NULL;
      ptr->backup = IMAGEBACKUP;
      ptr->done = 'n';
      ptr->audit = AUDITPTR;
      ptr->lineno = LINENUMBER;
      ptr->scope = strdup(CONTEXTID);
      
      if (strlen(LOCALREPOS) > 0)
         {
         //ExpandVarstring(LOCALREPOS,buf2,"");
         strcpy(buf2,LOCALREPOS);
         ptr->repository = strdup(buf2);
         }
      else
         {
         ptr->repository = NULL;
         }
      
         ptr->ifelapsed = PIFELAPSED;
         ptr->expireafter = PEXPIREAFTER;
      ptr->recurse = rec;
      ptr->type = type;
      ptr->stealth = STEALTH;
      ptr->checkroot = CHKROOT;
      ptr->preservetimes = PRESERVETIMES;
      ptr->encrypt = ENCRYPT;
      ptr->verify = VERIFY;
      ptr->size = size;
      ptr->comp = comp;
      ptr->linktype = lntype;
      ptr->symlink = VCPLNPARSE;
      ptr->exclusions = VEXCLUDEPARSE;
      ptr->inclusions = VINCLUDEPARSE;
      ptr->filters = VFILTERBUILD;
      ptr->ignores = VIGNOREPARSE;
      ptr->cache = NULL;
      ptr->purge = PURGE;
      
      if (PURGE == 'y')
         {
         ptr->forcedirs = 'y';
         ptr->typecheck = 'n';
         }
      else
         {
         ptr->forcedirs = FORCEDIRS;
         ptr->typecheck = TYPECHECK;
         }

      if (ptr->purge == 'y' && strstr(ep->name,"localhost") != 0)
         {
         Verbose("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
         Verbose("!! Purge detected in local (non-cfservd) file copy to file %s\n",ptr->destination);
         Verbose("!! Do not risk purge if source %s is NFS mounted (see manual)\n",ptr->path);
         Verbose("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
         }

      ptr->log = LOGP;
      ptr->inform = INFORMP;
      ptr->logaudit = AUDITP;
      ptr->plus_flags = PLUSFLAG;
      ptr->minus_flags = MINUSFLAG;
      ptr->trustkey = TRUSTKEY;
      ptr->compat = COMPATIBILITY;
      ptr->forceipv4 = FORCEIPV4;
      ptr->xdev = XDEV;
      
      if (ptr->compat == 'y' && ptr->encrypt == 'y')
         {
         yyerror("You cannot mix version1 compatibility with encrypted transfers");
         return;
         }
      
      ptr->acl_aliases = VACLBUILD;   
      ptr->inode_cache = NULL;
      
      VIMAGETOP = ptr;
      
      AddInstallable(ptr->defines);
      AddInstallable(ptr->elsedef);
      }
   }

DeleteItemList(expserver);   
Delete2DList(tp);

InitializeAction();
}

/*******************************************************************/

void InstallAuthItem(char *path,char *attribute,struct Auth **list,struct Auth **listtop,char *classes)

/* This is the top level interface for installing access rules
   for the server. Picks out the structures by name. */


{ struct TwoDimList *tp = NULL;
  char attribexp[CF_EXPANDSIZE];
  char *sp1;
  struct Item *vlist = NULL,*ip;

Debug1("InstallAuthItem(%s,%s)\n",path,attribute);
  
vlist = SplitStringAsItemList(attribute,LISTSEPARATOR);
 
Build2DListFromVarstring(&tp,path,LISTSEPARATOR,false);
Set2DList(tp);

for (sp1 = Get2DListEnt(tp); sp1 != NULL; sp1 = Get2DListEnt(tp))
   {
   for (ip = vlist; ip != NULL; ip = ip->next)
      {
      //ExpandVarstring(ip->name,attribexp,"");
      strcpy(attribexp,ip->name);
      
      Debug1("InstallAuthItem(%s=%s,%s)\n",path,sp1,attribexp);
      
      if (AuthPathExists(sp1,*list))
         {
         AddAuthHostItem(sp1,attribexp,classes,list);
         }
      else
         {
         InstallAuthPath(sp1,attribexp,classes,list,listtop);
         }
      }
   }
 
 Delete2DList(tp);
 DeleteItemList(vlist); 
}

/*******************************************************************/

int GetCommAttribute(char *s)

{ int i;
  char comm[CF_MAXVARSIZE];

for (i = 0; s[i] != '\0'; i++)
   {
   s[i] = ToLower(s[i]);
   }

comm[0]='\0';

sscanf(s,"%[^=]",comm);

Debug1("GetCommAttribute(%s)\n",comm);

for (i = 0; COMMATTRIBUTES[i] != NULL; i++)
   {
   if (strncmp(COMMATTRIBUTES[i],comm,strlen(comm)) == 0)
      {
      Debug1("GetCommAttribute - got: %s\n",COMMATTRIBUTES[i]);
      return i;
      }
   }

return cfbad;
}

/*******************************************************************/

void HandleRecurse(char *value)

{ int n = -1;

if (strcmp(value,"inf") == 0)
   {
   VRECURSE = CF_INF_RECURSE;
   }
else
   {
   /* Bas
   if (strncmp(CURRENTOBJECT,"home",4) == 0)
      {
      Bas
      yyerror("Recursion is always infinite for home");
      return;
      }
   */

   sscanf(value,"%d",&n);

   if (n == -1)
      {
      yyerror("Illegal recursion specifier");
      }
   else
      {
      VRECURSE = n;
      }
   }
}

/*******************************************************************/

void HandleCopyType(char *value)

{
if (strcmp(value,"ctime") == 0)
   {
   Debug1("Set copy by ctime\n");
   COPYTYPE = 't';
   return;
   }
else if (strcmp(value,"mtime") == 0)
   {
   Debug1("Set copy by mtime\n");
   COPYTYPE = 'm';
   return;
   }
 else if (strcmp(value,"checksum")==0 || strcmp(value,"sum") == 0)
   {
   Debug1("Set copy by md5 checksum\n");
   COPYTYPE = 'c';
   return;
   }
else if (strcmp(value,"byte")==0 || strcmp(value,"binary") == 0)
   {
   Debug1("Set copy by byte comaprison\n");
   COPYTYPE = 'b';
   return;
   }
else if (strcmp(value,"any")==0 || strcmp(value,"binary") == 0)
   {
   Debug1("Set copy by any comaprison\n");
   COPYTYPE = 'a';
   return;   
   }
yyerror("Illegal copy type");
}

/*******************************************************************/

void HandleDisableFileType(char *value)

{
if (strlen(CURRENTITEM) != 0)
   {
   Warning("Redefinition of filetype in disable");
   }

if (strcmp(value,"link") == 0 || strcmp(value,"links") == 0)
   {
   strcpy(CURRENTITEM,"link");
   }
else if (strcmp(value,"plain") == 0 || strcmp(value,"file") == 0)
   {
   strcpy(CURRENTITEM,"file");
   }
else
   {
   yyerror("Disable filetype unknown");
   }
}

/*******************************************************************/

void HandleDisableRotate(char *value)

{ int num = 0;

if (strcmp(value,"empty") == 0 || strcmp(value,"truncate") == 0)
   {
   ROTATE = CF_TRUNCATE;
   }
else
   {
   sscanf(value,"%d",&num);

   if (num == 0)
      {
      yyerror("disable/rotate value must be a decimal number greater than zero or keyword empty");
      }

   if (! SILENT && num > 99)
      {
      Warning("rotate value looks silly");
      }

   ROTATE = (short) num;
   }
}

/*******************************************************************/

void HandleAge(char *days)

{ 
sscanf(days,"%d",&VAGE);
Debug1("HandleAge(%d)\n",VAGE);
}

/*******************************************************************/

void HandleProcessMatches(char *value)

{ int i = -1;

switch (*value)
   {
   case '>': PROCOMP = '>';
             value++;
             break;
   case '<': PROCOMP = '<';
             value++;
             break;
   default : PROCOMP = '=';
   }

sscanf(value,"%d",&i);

if (i < 0) 
   {
   yyerror("matches attribute with silly value (must be >= 0)");
   }

PROMATCHES = (short) i;
ACTIONPENDING = true; 
}

/*******************************************************************/

void HandleProcessSignal(char *value)

{ int i;
  char *sp;

for (i = 1; i < highest_signal; i++)
   {
   for (sp = value; *sp != '\0'; sp++)
      {
      *sp = ToUpper(*sp);
      }

   if (strcmp(SIGNALS[i],"no signal") == 0)
      {
      continue;
      }
   
   if (strcmp(SIGNALS[i]+3,value) == 0)  /* 3 to cut off "sig" */
      {
      PROSIGNAL = (short) i;
      return;
      }
   }

i = 0;

sscanf(value,"%d",&i);

if (i < 1 && i >= highest_signal)
   {
   yyerror("Unknown signal in attribute - try using a number");
   }

ACTIONPENDING = true; 
PROSIGNAL = (short) i;
}

/*******************************************************************/

void HandleNetmask(char *value)

{
 if (strlen(DESTINATION) == 0)
    {
    strcpy(DESTINATION,value);
    }
 else
    {
    yyerror("redefinition of netmask");
    }
}

/*******************************************************************/

void HandleIPAddress(char *value)

{
 if (strlen(LINKTO) == 0)
    {
    strcpy(LINKTO,value);
    }
 else
    {
    yyerror("redefinition of ip address");
    }
}

/*******************************************************************/

void HandleBroadcast(char *value)

{
if (strlen(CURRENTOBJECT) != 0)
   { 
   yyerror("redefinition of broadcast address");
   printf("Previous val = %s\n",CURRENTOBJECT);
   } 
 
if (strcmp("ones",value) == 0)
   {
   strcpy(CURRENTOBJECT,"one");
   return;
   }

if (strcmp("zeroes",value) == 0)
   {
   strcpy(CURRENTOBJECT,"zero");
   return;
   }

if (strcmp("zeros",value) == 0)
   {
   strcpy(CURRENTOBJECT,"zero");
   return;
   }

yyerror("Illegal broadcast specifier (ones/zeros)"); 
}

/*******************************************************************/

void AppendToActionSequence (char *action)

{ int j = 0;
  char *sp,cbuff[CF_BUFSIZE],actiontxt[CF_BUFSIZE];

Debug1("Installing item (%s) in the action sequence list\n",action);

AppendItem(&VACTIONSEQ,action,CLASSBUFF);

if (strstr(action,"module"))
   {
   USEENVIRON=true;
   }
 
cbuff[0]='\0';
actiontxt[0]='\0';
sp = action;

while (*sp != '\0')
   {
   ++j;
   sscanf(sp,"%[^. ]",cbuff);

   while ((*sp != '\0') && (*sp !='.'))
      {
      sp++;
      }
 
   if (*sp == '.')
      {
      sp++;
      }
 
   if (IsHardClass(cbuff))
      {
      char *tmp = malloc(strlen(action)+30);
      sprintf(tmp,"Error in action sequence: %s\n",action);
      yyerror(tmp);
      free(tmp);
      yyerror("You cannot add a reserved class!");
      return;
      }
 
   if (j == 1)
      {
      strcpy(actiontxt,cbuff);
      continue;
      }
   else if (!IsSpecialClass(cbuff))
      {
      AddInstallable(cbuff);
      }
   }
}

/*******************************************************************/

void AppendToAccessList (char *user)

{ char id[CF_MAXVARSIZE];
  struct passwd *pw;

Debug1("Adding to access list for %s\n",user);

if (isalpha((int)user[0]))
   {
   if ((pw = getpwnam(user)) == NULL)
      {
      yyerror("No such user in password database");
      return;
      }

   sprintf(id,"%d",pw->pw_uid);
   AppendItem(&VACCESSLIST,id,NULL);
   }
else
   {
   AppendItem(&VACCESSLIST,user,NULL);
   }
}

/*******************************************************************/

void HandleLinkAction(char *value)

{
if (strcmp(value,"silent") == 0)
   {
   LINKSILENT = true;
   return;
   }

yyerror("Invalid link action");
}

/*******************************************************************/

void HandleDeadLinks(char *value)

{
if (strcmp(value,"kill") == 0)
   {
   DEADLINKS = true;
   return;
   }

if (strcmp(value,"force") == 0)
   {
   FORCELINK = 'y';
   return;
   }
 
yyerror("Invalid deadlink option");
}

/*******************************************************************/

void HandleLinkType(char *value)

{
if (strcmp(value,"hard") == 0)
   {
   if (ACTION_IS_LINKCHILDREN)
      {
      yyerror("hard links inappropriate for multiple linkage");
      }

   if (ACTION == image)
      {
      yyerror("hard links inappropriate for copy operation");
      }

   LINKTYPE = 'h';
   return;
   }

if (strcmp(value,"symbolic") == 0 || strcmp(value,"sym") == 0)
   {
   LINKTYPE = 's';
   return;
   }

if (strcmp(value,"abs") == 0 || strcmp(value,"absolute") == 0)
   {
   LINKTYPE = 'a';
   return;
   }

if (strcmp(value,"rel") == 0 || strcmp(value,"relative") == 0)
   {
   LINKTYPE = 'r';
   return;
   }
 
if (strcmp(value,"none") == 0 || strcmp(value,"copy") == 0)
   {
   LINKTYPE = 'n';
   return;
   }

 snprintf(OUTPUT,CF_BUFSIZE*2,"Invalid link type %s\n",value);
yyerror(OUTPUT);
}

/*******************************************************************/

void HandleServer(char *value)

{
Debug("Server in copy set to : %s\n",value);
strcpy(CFSERVER,value);
}

/*******************************************************************/

void HandleDefine(char *value)

{ char *sp;
 
Debug("Define response classes: %s\n",value);

if (strlen(value) > CF_BUFSIZE)
   {
   yyerror("class list too long - can't handle it!");
   }

/*if (!IsInstallable(value))
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Undeclared installable define=%s (see AddInstallable)",value);
   yyerror(OUTPUT);
   }
*/
strcpy(DEFINECLASSBUFFER,value);

for (sp = value; *sp != '\0'; sp++)
   {
   if (*sp == ':' || *sp == ',' || *sp == '.')
      {
      continue;
      }
   
   if (! isalnum((int)*sp) && *sp != '_')
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Illegal class list in define=%s\n",value);
      yyerror(OUTPUT);
      }
   }
}

/*******************************************************************/

void HandleElseDefine(char *value)

{ char *sp;
 
Debug("Define elsedefault classes: %s\n",value);

if (strlen(value) > CF_BUFSIZE)
   {
   yyerror("class list too long - can't handle it!");
   }

strcpy(ELSECLASSBUFFER,value);

for (sp = value; *sp != '\0'; sp++)
   {
   if (*sp == ':' || *sp == ',' || *sp == '.')
      {
      continue;
      }
   
   if (! isalnum((int)*sp) && *sp != '_')
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Illegal class list in elsedefine=%s\n",value);
      yyerror(OUTPUT);
      }
   }
}

/*******************************************************************/

void HandleFailover(char *value)

{ char *sp;
 
Debug("Define failover classes: %s\n",value);

if (strlen(value) > CF_BUFSIZE)
   {
   yyerror("class list too long - can't handle it!");
   }

strcpy(FAILOVERBUFFER,value);

for (sp = value; *sp != '\0'; sp++)
   {
   if (*sp == ':' || *sp == ',' || *sp == '.')
      {
      continue;
      }
   
   if (! isalnum((int)*sp) && *sp != '_')
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Illegal class list in failover=%s\n",value);
      yyerror(OUTPUT);
      }
   }
}

/*******************************************************************/
/* Level 4                                                         */
/*******************************************************************/

struct UidList *MakeUidList(char *uidnames)

{ struct UidList *uidlist;
  struct Item *ip, *tmplist;
  char uidbuff[CF_BUFSIZE];
  char *sp;
  int offset;
  struct passwd *pw;
  char *machine, *user, *domain, buffer[CF_EXPANDSIZE], *usercopy=NULL;
  int uid;
  int tmp = -1;

uidlist = NULL;
buffer[0] = '\0';

//ExpandVarstring(uidnames,buffer,"");
strcpy(buffer,uidnames);
    
for (sp = buffer; *sp != '\0'; sp+=strlen(uidbuff))
   {
   if (*sp == ',')
      {
      sp++;
      }

   if (sscanf(sp,"%[^,]",uidbuff))
      {
      if (uidbuff[0] == '+')        /* NIS group - have to do this in a roundabout     */
         {                          /* way because calling getpwnam spoils getnetgrent */
         offset = 1;
         if (uidbuff[1] == '@')
            {
            offset++;
            }

         setnetgrent(uidbuff+offset);
         tmplist = NULL;

         while (getnetgrent(&machine,&user,&domain))
            {
            if (user != NULL)
               {
               AppendItem(&tmplist,user,NULL);
               }
            }
                   
         endnetgrent();

         for (ip = tmplist; ip != NULL; ip=ip->next)
            {
            if ((pw = getpwnam(ip->name)) == NULL)
               {
               if (!PARSING)
                  {
                  snprintf(OUTPUT,CF_BUFSIZE*2,"Unknown user [%s]\n",ip->name);
                  CfLog(cferror,OUTPUT,"");
                  }
               uid = CF_UNKNOWN_OWNER; /* signal user not found */
               usercopy = ip->name;
               }
            else
               {
               uid = pw->pw_uid;
               }
            AddSimpleUidItem(&uidlist,uid,usercopy); 
            }
         
         DeleteItemList(tmplist);
         continue;
         }
      
      if (isdigit((int)uidbuff[0]))
         {
         sscanf(uidbuff,"%d",&tmp);
         uid = (uid_t)tmp;
         }
      else
         {
         if (strcmp(uidbuff,"*") == 0)
            {
            uid = CF_SAME_OWNER;                     /* signals wildcard */
            }
         else if ((pw = getpwnam(uidbuff)) == NULL)
            {
            if (!PARSING)
               {
               snprintf(OUTPUT,CF_BUFSIZE,"Unknown user %s\n",uidbuff);
               CfLog(cferror,OUTPUT,"");
               }
            uid = CF_UNKNOWN_OWNER;  /* signal user not found */
            usercopy = uidbuff;
            }
         else
            {
            uid = pw->pw_uid;
            }
         }
      AddSimpleUidItem(&uidlist,uid,usercopy);
      }
   }
 
 if (uidlist == NULL)
   {
   AddSimpleUidItem(&uidlist,CF_SAME_OWNER,(char *) NULL);
   }

return (uidlist);
}

/*********************************************************************/

struct GidList *MakeGidList(char *gidnames)

{ struct GidList *gidlist;
  char gidbuff[CF_BUFSIZE],buffer[CF_EXPANDSIZE];
  char *sp, *groupcopy=NULL;
  struct group *gr;
  int gid;
  int tmp = -1;

gidlist = NULL;
buffer[0] = '\0';
 
//ExpandVarstring(gidnames,buffer,"");
strcpy(buffer,gidnames);
    
for (sp = buffer; *sp != '\0'; sp+=strlen(gidbuff))
   {
   if (*sp == ',')
      {
      sp++;
      }

   if (sscanf(sp,"%[^,]",gidbuff))
      {
      if (isdigit((int)gidbuff[0]))
         {
         sscanf(gidbuff,"%d",&tmp);
         gid = (gid_t)tmp;
         }
      else
         {
         if (strcmp(gidbuff,"*") == 0)
            {
            gid = CF_SAME_GROUP;                     /* signals wildcard */
            }
         else if ((gr = getgrnam(gidbuff)) == NULL)
            {
            if (!PARSING)
               {
               snprintf(OUTPUT,CF_BUFSIZE,"Unknown group %s\n",gidbuff);
               CfLog(cferror,OUTPUT,"");
               }
            
            gid = CF_UNKNOWN_GROUP;
            groupcopy = gidbuff;
            }
         else
            {
            gid = gr->gr_gid;
            }
         }
      
      AddSimpleGidItem(&gidlist,gid,groupcopy);
      }
   }

if (gidlist == NULL)
   {
   AddSimpleGidItem(&gidlist,CF_SAME_GROUP,NULL);
   }

return(gidlist);
}


/*******************************************************************/

void InstallTidyPath(char *path,char *wild,int rec,short age,char travlinks,int tidysize,char type,char ldirs,char tidydirs,char *classes)

{ struct Tidy *ptr;
  char *sp,ebuff[CF_EXPANDSIZE];
  int no_of_links = 0;

  /*
if (!IsInstallable(classes))
   {
   InitializeAction();
   Debug1("Not installing tidy path, no match\n");
   return;
   }
  */
  
VBUFF[0]='\0';                                /* Expand any variables */
//ExpandVarstring(path,ebuff,"");
strcpy(ebuff,path);
   
if (strlen(ebuff) != 1)
   {
   DeleteSlash(ebuff);
   }

if ((ptr = (struct Tidy *)malloc(sizeof(struct Tidy))) == NULL)
   {
   FatalError("Memory Allocation failed for InstallTidyItem() #1");
   }

if ((ptr->path = strdup(ebuff)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallTidyItem() #3");
   }

if (VTIDYTOP == NULL)                 /* First element in the list */
   {
   VTIDY = ptr;
   }
else
   {
   VTIDYTOP->next = ptr;
   }

if (rec != CF_INF_RECURSE && strncmp("home/",ptr->path,5) == 0) /* Is this a subdir of home wildcard? */
   {
   for (sp = ptr->path; *sp != '\0'; sp++)                     /* Need to make recursion relative to start */
      {                                                        /* of the search, not relative to home */
      if (*sp == '/')
         {
         no_of_links++;
         }
      }
   }

   ptr->ifelapsed = PIFELAPSED;

   ptr->expireafter = PEXPIREAFTER;
 
ptr->done = 'n';
ptr->scope = strdup(CONTEXTID); 
ptr->tidylist = NULL;
ptr->maxrecurse = rec + no_of_links;
ptr->next = NULL;
ptr->xdev = XDEV; 
ptr->exclusions = VEXCLUDEPARSE;
ptr->ignores = VIGNOREPARSE;

VEXCLUDEPARSE = NULL; 
VIGNOREPARSE = NULL;      
VTIDYTOP = ptr;

AddTidyItem(path,wild,rec+no_of_links,age,travlinks,tidysize,type,ldirs,tidydirs,classes);
}

/*********************************************************************/

void AddTidyItem(char *path,char *wild,int rec,short age,char travlinks,int tidysize,char type,char ldirs,short tidydirs,char *classes)

{ char varbuff[CF_EXPANDSIZE];
  struct Tidy *ptr;
  struct Item *ip;

Debug1("AddTidyItem(%s,pat=%s,size=%d)\n",path,wild,tidysize);

/*
if (!IsInstallable(CLASSBUFF))
   {
   InitializeAction();
   Debug1("Not installing TidyItem no match\n");
   return;
   }
*/

for (ptr = VTIDY; ptr != NULL; ptr=ptr->next)
   {
   //ExpandVarstring(path,varbuff,"");
   strcpy(varbuff,path);

   if (strcmp(ptr->path,varbuff) == 0)
      {
      PrependTidy(&(ptr->tidylist),wild,rec,age,travlinks,tidysize,type,ldirs,tidydirs,classes);
      
      for (ip = VEXCLUDEPARSE; ip != NULL; ip=ip->next)
         {
         AppendItem(&(ptr->exclusions),ip->name,ip->classes);
         }
      
      for (ip = VIGNOREPARSE; ip != NULL; ip=ip->next)
         {
         AppendItem(&(ptr->ignores),ip->name,ip->classes);
         }
      
      DeleteItemList(VEXCLUDEPARSE);
      DeleteItemList(VIGNOREPARSE);
      /* Must have the maximum recursion level here */
      
      if (rec == CF_INF_RECURSE || ((ptr->maxrecurse < rec) && (ptr->maxrecurse != CF_INF_RECURSE)))
         {
         ptr->maxrecurse = rec;
         }
      return;
      }
   }
}

/*********************************************************************/

int TidyPathExists(char *path)

{ struct Tidy *tp;
  char ebuff[CF_EXPANDSIZE]; 

//ExpandVarstring(path,ebuff,"");

  strcpy(ebuff,path);
     
for (tp = VTIDY; tp != NULL; tp=tp->next)
   {
   if (strcmp(tp->path,ebuff) == 0)
      {
      Debug1("TidyPathExists(%s)\n",ebuff);
      return true;
      }
   }

return false;
}


/*******************************************************************/
/* Level 5                                                         */
/*******************************************************************/

void AddSimpleUidItem(struct UidList **uidlist,int uid,char *uidname)

{ struct UidList *ulp, *u;
  char *copyuser;

if ((ulp = (struct UidList *)malloc(sizeof(struct UidList))) == NULL)
   {
   FatalError("cfengine: malloc() failed #1 in AddSimpleUidItem()");
   }

ulp->uid = uid;
 
if (uid == CF_UNKNOWN_OWNER)   /* unknown user */
   {
   if ((copyuser = strdup(uidname)) == NULL)
      {
      FatalError("cfengine: malloc() failed #2 in AddSimpleUidItem()");
      }
   
   ulp->uidname = copyuser;
   }
else
   {
   ulp->uidname = NULL;
   }

ulp->next = NULL;

if (*uidlist == NULL)
   {
   *uidlist = ulp;
   }
else
   {
   for (u = *uidlist; u->next != NULL; u = u->next)
      {
      }
   u->next = ulp;
   }
}

/*******************************************************************/

void AddSimpleGidItem(struct GidList **gidlist,int gid,char *gidname)

{ struct GidList *glp,*g;
  char *copygroup;

if ((glp = (struct GidList *)malloc(sizeof(struct GidList))) == NULL)
   {
   FatalError("cfengine: malloc() failed #1 in AddSimpleGidItem()");
   }
 
glp->gid = gid;
 
if (gid == CF_UNKNOWN_GROUP)   /* unknown group */
   {
   if ((copygroup = strdup(gidname)) == NULL)
      {
      FatalError("cfengine: malloc() failed #2 in AddSimpleGidItem()");
      }
   
   glp->gidname = copygroup;
   }
else
   {
   glp->gidname = NULL;
   }
 
glp->next = NULL;

if (*gidlist == NULL)
   {
   *gidlist = glp;
   }
else
   {
   for (g = *gidlist; g->next != NULL; g = g->next)
      {
      }
   g->next = glp;
   }
}


/***********************************************************************/

void InstallAuthPath(char *path,char *hostname,char *classes,struct Auth **list,struct Auth **listtop)

{ struct Auth *ptr;
  char ebuff[CF_EXPANDSIZE]; 

Debug1("InstallAuthPath(%s,%s) for %s\n",path,hostname,classes);

//ExpandVarstring(path,ebuff,"");
strcpy(ebuff,path);
   
if (strlen(ebuff) != 1)
   {
   DeleteSlash(ebuff);
   }

if ((ptr = (struct Auth *)malloc(sizeof(struct Auth))) == NULL)
   {
   FatalError("Memory Allocation failed for InstallAuthPath() #1");
   }

if ((ptr->path = strdup(ebuff)) == NULL)
   {
   FatalError("Memory Allocation failed for InstallAuthPath() #3");
   }

if (*listtop == NULL)                 /* First element in the list */
   {
   *list = ptr;
   }
else
   {
   (*listtop)->next = ptr;
   }

ptr->classes = strdup(CLASSBUFF);
ptr->accesslist = NULL;
ptr->maproot = NULL;
ptr->encrypt = false; 
ptr->next = NULL;
*listtop = ptr;

AddAuthHostItem(ptr->path,hostname,classes,list);
}

/***********************************************************************/

void AddAuthHostItem(char *path,char *attribute,char *classes,struct Auth **list)

{ char varbuff[CF_EXPANDSIZE];
  struct Auth *ptr;
  struct Item *ip, *split = NULL;

Debug1("AddAuthHostItem(%s,%s)\n",path,attribute);

split = SplitStringAsItemList(attribute,LISTSEPARATOR);

for (ptr = *list; ptr != NULL; ptr=ptr->next)
   {
   //ExpandVarstring(path,varbuff,"");
   strcpy(varbuff,path);
   
   if (strcmp(ptr->path,varbuff) == 0)
      {
      for (ip = split; ip != NULL; ip=ip->next)
         {     
         if (!HandleAdmitAttribute(ptr,ip->name))
            {
            PrependItem(&(ptr->accesslist),ip->name,classes);
            }
         }

      DeleteItemList(split);
      return;      
      }
   }

DeleteItemList(split);
}


/*********************************************************************/

int AuthPathExists(char *path,struct Auth *list)

{ struct Auth *ap;
  char ebuff[CF_EXPANDSIZE]; 

Debug1("AuthPathExists(%s)\n",path);

//ExpandVarstring(path,ebuff,"");
strcpy(ebuff,path);
   
if (ebuff[0] != '/')
   {
   yyerror("Missing absolute path to a directory");
   FatalError("Cannot continue");
   }

for (ap = list; ap != NULL; ap=ap->next)
   {
   if (strcmp(ap->path,ebuff) == 0)
      {
      return true;
      }
   }

return false;
}

/*********************************************************************/

int HandleAdmitAttribute(struct Auth *ptr,char *attribute)

{ char value[CF_MAXVARSIZE],buffer[CF_EXPANDSIZE],host[CF_MAXVARSIZE],ebuff[CF_EXPANDSIZE],*sp;

value[0] = '\0';

//ExpandVarstring(attribute,ebuff,NULL);
strcpy(ebuff,attribute);
   
sscanf(ebuff,"%*[^=]=%s",value);

if (value[0] == '\0')
   {
   return false;
   }

Debug1("HandleAdmitFileAttribute(%s)\n",value);

switch(GetCommAttribute(attribute))
   {
   case cfencryp:

       Debug("\nENCRYPTION tag %s\n",value);
       if ((strcmp(value,"on")==0) || (strcmp(value,"true")==0))
          {
          ptr->encrypt = true;
          return true;
          }
       break;
       
   case cfroot:

       Debug("\nROOTMAP tag %s\n",value);
       //ExpandVarstring(value,buffer,"");
       strcpy(buffer,value);
          
       for (sp = buffer; *sp != '\0'; sp+=strlen(host))
          {
          if (*sp == ',')
             {
             sp++;
             }
          
          host[0] = '\0';
          
          if (sscanf(sp,"%[^,\n]",host))
             {
             char copyhost[CF_BUFSIZE];
             
             strncpy(copyhost,host,CF_BUFSIZE-1);
             
             if (!strstr(copyhost,"."))
                {
                if (strlen(copyhost)+strlen(VDOMAIN) < CF_MAXVARSIZE-2)
                   {
                   strcat(copyhost,".");
                   strcat(copyhost,VDOMAIN);
                   }
                else
                   {
                   yyerror("Server name too long");
                   }
                }
             
             if (!IsItemIn(ptr->maproot,copyhost))
                {
                PrependItem(&(ptr->maproot),copyhost,NULL);
                }
             else
                {
                Debug("Not installing %s in rootmap\n",host);
                }
             }
          }
       return true;
       break;
   }
 
 yyerror("Illegal admit/deny attribute"); 
 return false;
}


/*********************************************************************/

void PrependTidy(struct TidyPattern **list,char *wild,int rec,short age,char travlinks,int tidysize,char type,char ldirs,char tidydirs,char *classes)

{ struct TidyPattern *tp;
  char *spe = NULL,*sp, buffer[CF_EXPANDSIZE];

if ((tp = (struct TidyPattern *)malloc(sizeof(struct TidyPattern))) == NULL)
   {
   perror("Can't allocate memory in PrependTidy()");
   FatalError("");
   }

if ((sp = strdup(wild)) == NULL)
   {
   FatalError("Memory Allocation failed for PrependTidy() #2");
   }

//ExpandVarstring(DEFINECLASSBUFFER,buffer,""); 
strcpy(buffer,DEFINECLASSBUFFER);
   
if ((tp->defines = strdup(buffer)) == NULL)
   {
   FatalError("Memory Allocation failed for PrependTidy() #2a");
   }

//ExpandVarstring(ELSECLASSBUFFER,buffer,"");  
strcpy(buffer,ELSECLASSBUFFER);
   
if ((tp->elsedef = strdup(buffer)) == NULL)
   {
   FatalError("Memory Allocation failed for PrependTidy() #2a");
   }
 
AddInstallable(tp->defines);
AddInstallable(tp->elsedef); 
 
if ((classes!= NULL) && (spe = malloc(strlen(classes)+2)) == NULL)
   {
   perror("Can't allocate memory in PrependItem()");
   FatalError("");
   }

if (travlinks == '?')
   {
   travlinks = 'F';  /* False is default */
   }

tp->audit = AUDITPTR;
tp->lineno = LINENUMBER;
tp->size = tidysize;
tp->recurse = rec;
tp->age = age;
tp->searchtype = type;
tp->travlinks = travlinks;
tp->filters = VFILTERBUILD;
tp->pattern = sp;
tp->next = *list;
tp->dirlinks = ldirs;
tp->log = LOGP;
tp->inform = INFORMP;
tp->logaudit = AUDITP;
tp->compress = COMPRESS;
tp->rmdirs = tidydirs;
tp->tidied = false;

*list = tp;

if (classes != NULL)
   {
   strcpy(spe,classes);
   tp->classes = spe;
   }
else
   {
   tp->classes = NULL;
   }
}


/* EOF */

/* cfengine for GNU
 
        Copyright (C) 2003
        Free Software Foundation, Inc.
 
   This file is part of GNU cfengine - written and maintained 
   by Mark Burgess, Dept of Computing and Engineering, Oslo College,
   Dept. of Theoretical physics, University of Oslo

   The contributions in this file have been made by volunteers and
   only checked for serious errors by Mark Burgess. The orginal code
   was written by Red Hat Linux and has since been modified by a number
   of others.
   
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

/****************************************************************************/

int BuildCommandLine  (char *resolvedcmd, char* rawcmd, struct Item *pkglist );
int RPMPackageCheck (char *package, char *version, enum cmpsense cmp);
int RPMPackageList (char *package, char *version, enum cmpsense cmp, struct Item **pkglist);
int DPKGPackageCheck (char *package, char *version, enum cmpsense cmp);
int DPKGPackageList (char *package, char *version, enum cmpsense cmp, struct Item **pkglist);
int SUNPackageCheck (char *package, char *version, enum cmpsense cmp);
int SUNPackageList (char *package, char *version, enum cmpsense cmp, struct Item **pkglist);
void ParseSUNVR(char * vr, int *major, int *minor, int *micro, char **revis);
int PortagePackageCheck (char *package, char *version, enum cmpsense cmp);
int PortagePackageList (char *package, char *version, enum cmpsense cmp, struct Item **pkglist);
int AIXPackageCheck (char *package, char *version, enum cmpsense cmp);
int AIXPackageList (char *package, char *version, enum cmpsense cmp, struct Item **pkglist);
void ParseAIXVR(char * vr, int *ver, int *release, int *maint, int *fix);
int FreeBSDPackageCheck (char *package, char *version, enum cmpsense cmp);
int FreeBSDPackageList (char *package, char *version, enum cmpsense cmp, struct Item **pkglist);
int rpmvercmp(const char *a, const char *b);
void ParseEVR(char * evr, const char **ep, const char **vp, const char **rp);
int xislower(int c);
int xisupper(int c);
int xisalpha(int c);
int xisalnum(int c);
int xisdigit(int c);

/****************************************************************************/

/* Make sure we are insensitive to the locale so everyone gets the
 * same experience from these. */

int xislower(int c)
{
 return (c >= 'a' && c <= 'z');
}

int xisupper(int c)
{
 return (c >= 'A' && c <= 'Z');
}

int xisalpha(int c)
{
return (xislower(c) || xisupper(c));
}

int xisdigit(int c)
{
 return (c >= '0' && c <= '9');
}

int xisalnum(int c)
{
 return (xisalpha(c) || xisdigit(c));
}


/***********************************************************************************/

/* This is moved here to allow do.c to be ignorant of the various package managers */

int PackageCheck(struct Package *ptr,char* package, enum pkgmgrs pkgmgr, char *version,enum cmpsense cmp)
{ int match=0;

switch(pkgmgr)
   {
   case pkgmgr_rpm:
      match = RPMPackageCheck(package, version, cmp);
      break;
   case pkgmgr_dpkg:
      match = DPKGPackageCheck(package, version, cmp);
      break;
   case pkgmgr_sun:
      match = SUNPackageCheck(package, version, cmp);
      break;
   case pkgmgr_aix:
      match = AIXPackageCheck(package, version, cmp);
      break;
   case pkgmgr_portage:
      match = PortagePackageCheck(package, version, cmp);
      break;
   case pkgmgr_freebsd:
      match = FreeBSDPackageCheck(package, version, cmp);
      break;
   default:
      /* UGH!  This should *never* happen.  GetPkgMgr() and
       * InstallPackagesItem() should have caught this before it
       * was ever installed!!!
       * */
       snprintf(OUTPUT,CF_BUFSIZE,"Internal error!  Tried to check package %s in an unknown database: %d.  This should never happen!\n", package, pkgmgr);
       CfLog(cferror,OUTPUT,"");
       break;
   }
return match;
}

/************************************************************************/

int PackageList(struct Package *ptr,char* package, enum pkgmgrs pkgmgr, char *version,enum cmpsense cmp, struct Item **pkglist)
{ int match=0;

switch(pkgmgr)
   {
   case pkgmgr_rpm:
      match = RPMPackageList(package, version, cmp, pkglist);
      break;
   case pkgmgr_dpkg:
      match = DPKGPackageList(package, version, cmp, pkglist);
      break;
   case pkgmgr_sun:
      match = SUNPackageList(package, version, cmp, pkglist);
      break;
   case pkgmgr_aix:
      match = AIXPackageList(package, version, cmp, pkglist);
      break;
   case pkgmgr_portage:
      match = PortagePackageList(package, version, cmp, pkglist);
      break;
   case pkgmgr_freebsd:
      match = FreeBSDPackageList(package, version, cmp, pkglist);
      break;
   default:
      /* This should *never* happen.  GetPkgMgr() and
       * InstallPackagesItem() should have caught this before it
       * was ever installed!!!
       * */
       snprintf(OUTPUT,CF_BUFSIZE,"Internal error!  Tried to check package %s in an unknown database: %d.  This should never happen!\n", package, pkgmgr);
       CfLog(cferror,OUTPUT,"");
       break;
   }
return match;
}

/*********************************************************************************/

int InstallPackage(struct Package *ptr,enum pkgmgrs pkgmgr, struct Item **pkglist)

{ char rawinstcmd[CF_BUFSIZE];
 /* Make the instcmd twice the normal buffer size since the package list
    limit is CF_BUFSIZE so this can obviously get larger! */
 char instcmd[CF_BUFSIZE*2];
 char line[CF_BUFSIZE];
 FILE *pp;
 int result = 0;

Debug("Entering InstallPackage.\n");

switch(pkgmgr)
   {
   case pkgmgr_rpm:

       if (!GetMacroValue(CONTEXTID,"RPMInstallCommand"))
          {
          CfLog(cferror,"RPMInstallCommand NOT Set.  Package Installation Not Possible!\n","");
          return 0;
          }
       strncpy(rawinstcmd, GetMacroValue(CONTEXTID,"RPMInstallCommand"),CF_BUFSIZE);
       break;
       
   case pkgmgr_dpkg:

       if (!GetMacroValue(CONTEXTID,"DPKGInstallCommand"))
          {
          CfLog(cferror,"DPKGInstallCommand NOT Set.  Package Installation Not Possible!\n","");
          return 0;
          }
       strncpy(rawinstcmd, GetMacroValue(CONTEXTID,"DPKGInstallCommand"),CF_BUFSIZE);
       break;
       
   case pkgmgr_sun:

       if (!GetMacroValue(CONTEXTID,"SUNInstallCommand"))
          {
          CfLog(cferror,"SUNInstallCommand NOT Set.  Package Installation Not Possible!\n","");
          return 0;
          }
       strncpy(rawinstcmd, GetMacroValue(CONTEXTID,"SUNInstallCommand"),CF_BUFSIZE);
       break;
       
   case pkgmgr_aix:
   if (!GetMacroValue(CONTEXTID,"AIXInstallCommand"))
          {
          CfLog(cferror,"AIXInstallCommand NOT Set.  Package Installation Not Possible!\n","");
          return 0;
          }
       strncpy(rawinstcmd, GetMacroValue(CONTEXTID,"AIXInstallCommand"),CF_BUFSIZE);

       break;

   case pkgmgr_portage:
       
       if (!GetMacroValue(CONTEXTID,"PortageInstallCommand"))
          {
          CfLog(cferror,"PortageInstallCommand NOT Set.  Package Installation Not Possible!\n","packages");
          return 0;
          }
       strncpy(rawinstcmd, GetMacroValue(CONTEXTID,"PortageInstallCommand"),CF_BUFSIZE);
       break;

   case pkgmgr_freebsd:
       
      if (!GetMacroValue(CONTEXTID,"FreeBSDInstallCommand"))
         {
         CfLog(cferror,"FreeBSDInstallCommand NOT Set.  Package Installation Not Possible!\n","");
         return 0;
         }
      strncpy(rawinstcmd, GetMacroValue(CONTEXTID,"FreeBSDInstallCommand"),CF_BUFSIZE);
      break;
       
   default:
       snprintf(OUTPUT,CF_BUFSIZE,"Unknown package manager %d\n",pkgmgr);
       CfLog(cferror,OUTPUT,"");
       break;
   }

if (BuildCommandLine(instcmd,rawinstcmd,*pkglist))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Installing package(s) using '%s'\n", instcmd);
   CfLog(cfinform,OUTPUT,"");
   
   if (DONTDO)
      {
      Verbose("--skipping because DONTDO is enabled.\n");
      result = 1;
      }
   else 
      {
      if ((pp = cfpopen(instcmd, "r")) == NULL)
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Couldn't exec package installer %s",instcmd);
         CfLog(cfinform,OUTPUT,"popen");
         AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_FAIL);
         return 0;
         }
      
      while (!feof(pp))
         {
         ReadLine(line,CF_BUFSIZE-1,pp);
         snprintf(OUTPUT,CF_BUFSIZE,"%s",line);
         CfLog(cfinform,OUTPUT,""); 
         }
      
      if (cfpclose(pp) != 0)
         {
         CfLog(cfinform,"Package install command was not successful.\n\n","popen");
         AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,"Package installer did not exit properly",CF_FAIL);
         result = 0;
         }
      else
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Packages installed: %s",instcmd);
         CfLog(cfinform,OUTPUT,"");
         AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_CHG);
         result = 1;
         }
      }
   }
else 
   {
   CfLog(cferror,"Unable to evaluate package manager command.\n","");
   AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,"Unable to evaluate package manager",CF_NOP);
   result = 0;
   }

return result;
}


/************************************************************************************/

int UpgradePackage(struct Package *ptr,char *name, enum pkgmgrs pkgmgr, char* version, enum cmpsense cmp)

{ struct Item *removelist = NULL;
  struct Item *addlist = NULL;
  int result = 0;
 
Verbose("Package upgrade for %s: %s\n", PKGMGRTEXT[pkgmgr], name );

if ((ptr->pkgmgr == pkgmgr_freebsd) || (ptr->pkgmgr == pkgmgr_sun))
   {
   /* Handle package managers with independant removes/installs */

   PackageList(ptr,name,pkgmgr,version,cmp,&removelist);

   if (RemovePackage(ptr,pkgmgr,&removelist))
      {
      AppendItem(&addlist, name, NULL);
      result = InstallPackage(ptr,pkgmgr,&addlist);
      }
   else
      {
      CfLog(cfinform,"Package cannot be upgraded because the old version was not removed.\n\n","");
      AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,"Package not upgraded - another version in the way",CF_FAIL);
      result = 0;
      }
   }
else
   {
   /* Handle package managers that do atomic upgrades */

   AppendItem(&addlist, name, NULL);
   result = InstallPackage(ptr,pkgmgr,&addlist);
   }

DeleteItemList(removelist);
DeleteItemList(addlist);
return result;
}

/************************************************************************************/

int RemovePackage(struct Package *ptr,enum pkgmgrs pkgmgr, struct Item **pkglist)
    
{ char rawdelcmd[CF_BUFSIZE];
  char delcmd[CF_BUFSIZE*2];
  char line[CF_BUFSIZE];
  FILE *pp;
  int result = 0;

if (pkglist == NULL)
   {
   CfLog(cferror,"RemovePackage: no packages to remove.\n",""); 
   return 0;
   }

switch(pkgmgr)
   {
   case pkgmgr_rpm:

      if (!GetMacroValue(CONTEXTID,"RPMRemoveCommand"))
         {
         CfLog(cferror,"RPMRemoveCommand NOT set, using default!\n","");
         strncpy(rawdelcmd, "/bin/rpm -e %s", CF_BUFSIZE - 1);
         }
      else
         {
         strncpy(rawdelcmd, GetMacroValue(CONTEXTID,"RPMRemoveCommand"), CF_BUFSIZE - 1);
         }
      break;

   case pkgmgr_sun:
      if (!GetMacroValue(CONTEXTID,"SunRemoveCommand"))
         {
         CfLog(cferror,"SunRemoveCommand NOT set, using default!\n","");
         strncpy(rawdelcmd, "/usr/sbin/pkgrm -n %s", CF_BUFSIZE - 1);
         }
      else
         {
         strncpy(rawdelcmd, GetMacroValue(CONTEXTID,"SunRemoveCommand"), CF_BUFSIZE - 1);
         }
      break;

   case pkgmgr_dpkg:

      if (!GetMacroValue(CONTEXTID,"DPKGRemoveCommand"))
         {
         CfLog(cferror,"DPKGRemoveCommand NOT Set.  Package Removal Not Possible!\n","");
         }
      else
         {
         strncpy(rawdelcmd, GetMacroValue(CONTEXTID,"DPKGRemoveCommand"), CF_BUFSIZE - 1);
         }
      break;

   case pkgmgr_freebsd:

      if (!GetMacroValue(CONTEXTID,"FreeBSDRemoveCommand"))
         {
         strncpy(rawdelcmd, "/usr/sbin/pkg_delete %s", CF_BUFSIZE - 1);
         }
      else
         {
         strncpy(rawdelcmd, GetMacroValue(CONTEXTID,"FreeBSDRemoveCommand"), CF_BUFSIZE - 1);
         }
      break;

   case pkgmgr_portage:

      if (!GetMacroValue(CONTEXTID,"PortageRemoveCommand"))
         {
         strncpy(rawdelcmd, "/usr/bin/emerge -C %s", CF_BUFSIZE - 1);
         }
      else
         {
         strncpy(rawdelcmd, GetMacroValue(CONTEXTID,"PortageRemoveCommand"), CF_BUFSIZE - 1);
         }
      break;

   default:
      CfLog(cferror,"RemovePackage: Package removal not yet implemented for this package manager.\n","p");
      AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,"No support for package removal",CF_NOP);
      break;
   }


if (BuildCommandLine( delcmd, rawdelcmd, *pkglist))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Removing package(s) using '%s'\n", delcmd);
   CfLog(cfinform,OUTPUT,"");

   if (DONTDO)
      {
      Verbose("--skipping because \"-n\" (dryrun) option is enabled.\n");
      result = 1;
      }
   else 
      {
      if ((pp = cfpopen(delcmd, "r")) == NULL)
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Could not execute package removal command %s\n",delcmd);
         CfLog(cferror,OUTPUT,"popen");
         AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_FAIL);
         return 0;
         }
      
      while (!feof(pp))
         {
         ReadLine(line,CF_BUFSIZE-1,pp);
         snprintf(OUTPUT,CF_BUFSIZE,"%s\n",line);
         CfLog(cfinform,OUTPUT,""); 
         }
      
      if (cfpclose(pp) != 0)
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Could not execute package removal command %s\n",delcmd);
         CfLog(cferror,OUTPUT,"popen");
         AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_FAIL);
         result = 0;
         }
      else
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Package removal succeeded (%s)\n",delcmd);
         CfLog(cfinform,OUTPUT,"popen");
         AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,OUTPUT,CF_CHG);
         result = 1;
         }
      }
   }
else 
   {
   CfLog(cferror,"Unable to evaluate package manager command.\n","");
   AuditLog(ptr->logaudit,ptr->audit,ptr->lineno,"Could not evaluate package manager",CF_FAIL);
   result = 0;
   }

return result;
}


/****************************************************************************/

int BuildCommandLine(char *resolvedcmd, char *rawcmd, struct Item *pkglist )   

{ struct Item *package;
  char *cmd_tail;
  FILE *pp;
  char *ptr, *cmd_ptr;
  int cmd_args = 0;
  int cmd_tail_len = 0;
  int original_len;
  int result;

for (ptr = rawcmd, cmd_args = 1; NULL != ptr; ptr = strchr(++ptr, ' '))
   {
   ++cmd_args;
   }

if (cmd_tail = strstr(rawcmd, "%s"))
   {
   *cmd_tail = '\0';
   cmd_tail += 2;
   cmd_tail_len = strlen(cmd_tail);
   --cmd_args;
   }
   
strncpy(resolvedcmd, rawcmd, CF_BUFSIZE*2);

Verbose("Package manager will be invoked as %s\n", resolvedcmd);

/* Iterator through package list until we reach the maximum number that cfpopen can take */

original_len = strlen(resolvedcmd);
cmd_ptr = &resolvedcmd[original_len];

for (package = pkglist; package != NULL; package = package->next)
   {
   Debug("BuildCommandLine(): Processing package %s at location %d.\n", package->name, &package );

   if (cmd_args == CF_MAXSHELLARGS)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Skipping package %s (reached CF_MAXSHELLARGS)\n", package->name);
      CfLog(cfinform,OUTPUT,"");
      }
   else if (cmd_ptr - resolvedcmd + strlen(package->name) + 2 > CF_BUFSIZE*2)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Skipping package %s (max buffer size)\n", package->name);
      CfLog(cfinform,OUTPUT,"");
      }
   else
      {
      Verbose("BuildCommandLine(): Adding package '%s' to the list.\n", package->name);
      
      strncpy(cmd_ptr, package->name, &resolvedcmd[CF_BUFSIZE*2] - cmd_ptr);
      cmd_ptr += strlen(package->name);
      *cmd_ptr++ = ' ';
      ++cmd_args;
      }
   }

/* If the revised command line is the same, we have nothing to do. */

if (cmd_ptr == &resolvedcmd[original_len])
   {
   result = 0;
   Verbose("No packages to install.\n");
   }
else
   {      
   /* Have a full command line, so remove trailing space */
   *--cmd_ptr = '\0';
   
   /* Append the tail if necessary. */
   if (cmd_tail_len > 0)
      {
      strncpy(cmd_ptr, cmd_tail, &resolvedcmd[CF_BUFSIZE*2] - cmd_ptr);
      }
   result = 1;
   }
return result;
}
      
/*********************************************************************/
/* RPM */
/* returns: 1 - found a match
 *          0 - found no match
 */
/*********************************************************************/

int RPMPackageCheck(char *package,char *version,enum cmpsense cmp)

{ FILE *pp;
  struct Item *evrlist = NULL;
  struct Item *evr;
  int epochA = 0; /* What is installed.  Assume 0 if we don't get one. */
  int epochB = 0; /* What we are looking for.  Assume 0 if we don't get one. */
  const char *eA = NULL; /* What is installed */
  const char *eB = NULL; /* What we are looking for */
  const char *vA = NULL; /* What is installed */
  const char *vB = NULL; /* What we are looking for */
  const char *rA = NULL; /* What is installed */
  const char *rB = NULL; /* What we are looking for */
  enum cmpsense result;
  int match = 0;

   char tver1[CF_BUFSIZE];
   char tver2[CF_BUFSIZE];

if (GetMacroValue(CONTEXTID,"RPMcommand"))
   {
   snprintf(VBUFF,CF_BUFSIZE,"%s -q --queryformat \"%%{EPOCH}:%%{VERSION}-%%{RELEASE}\\n\" %s",GetMacroValue(CONTEXTID,"RPMcommand"),package);
   }
else
   {
   snprintf(VBUFF,CF_BUFSIZE,"/bin/rpm -q --queryformat \"%%{EPOCH}:%%{VERSION}-%%{RELEASE}\\n\" %s", package);
   }

if ((pp = cfpopen(VBUFF, "r")) == NULL)
   {
   Verbose("Could not execute the RPM command.  Assuming package not installed.\n");
   return 0;
   }

while(!feof(pp))
   {
   *VBUFF = '\0';
   
   ReadLine(VBUFF,CF_BUFSIZE,pp);
   
   if (*VBUFF != '\0')
      {
      AppendItem(&evrlist,VBUFF,"");
      }
   }

/* Non-zero exit status means that we could not find the package, so
 * Zero the list and bail. */

if (cfpclose(pp) != 0)
   {
   DeleteItemList(evrlist);
   evrlist = NULL;
   }

if (evrlist == NULL)
   {
   Verbose("RPM Package %s not installed.\n", package);
   return 0;
   }

Verbose("Requesting %s %s %s\n", package, CMPSENSETEXT[cmp],(version[0] ? version : "ANY"));

/* If no version was specified, just return 1, because if we got this far
 * some package by that name exists. */

if (!version[0])
   {
   DeleteItemList(evrlist);
   return 1;
   }

/* Parse the EVR we are looking for once at the start */
memset( tver1, 0, CF_BUFSIZE );
strncpy( tver1, version, CF_BUFSIZE - 1 );
ParseEVR( tver1, &eB, &vB, &rB);

/* The rule here will be: if any package in the list matches, then the
 * first one to match wins, and we bail out. */

for (evr = evrlist; evr != NULL; evr=evr->next)
   {
   char *evrstart;
   evrstart = evr->name;
   result = cmpsense_eq;
   
   /* RPM returns the string "(none)" for the epoch if there is none
    * instead of the number 0.  This will cause ParseEVR() to misinterpret
    * it as part of the version component, since epochs must be numeric.
    * If we get "(none)" at the start of the EVR string, we must be careful
    * to replace it with a 0 and reset evrstart to where the 0 is.  Ugh.
    */
   
   if (!strncmp(evrstart, "(none)", strlen("(none)")))
      {
      /* We have no EVR in the installed package.  Fudge it. */
      evrstart = strchr(evrstart, ':') - 1;
      *evrstart = '0';
      }
   
   Verbose("RPMCheckPackage(): Trying installed version %s\n", evrstart);
   memset( tver2, 0, CF_BUFSIZE );
   strncpy( tver2, evrstart, CF_BUFSIZE - 1 );
   ParseEVR( tver2, &eA, &vA, &rA);

   
   epochA = atol(eA);   /* The above code makes sure we always have this. */
   
   if (eB && *eB) /* the B side is what the user entered.  Better check. */
      {
      epochB = atol(eB);
      }
   
   /* First try the epoch. */
   
   if (epochA > epochB)
      {
      result = cmpsense_gt;
      }
   
   if (epochA < epochB)
      {
      result = cmpsense_lt;
      }
   
   /* If that did not decide it, try version.  We must *always* have
    * a version string.  That's just the way it is.*/
   
   if (result == cmpsense_eq)
      {
      switch (rpmvercmp(vA, vB))
         {
         case 1:    result = cmpsense_gt;
             break;
         case -1:   result = cmpsense_lt;
             break;
         }
      }
   
   /* if we wind up here, everything rides on the release if both have it.
    * RPM always stores a release internally in the database, so the A side
    * will have it.  It's just a matter of whether or not the user cares
    * about it at this point. */
   
   if ((result == cmpsense_eq) && (rB && *rB))
      {
      switch (rpmvercmp(rA, rB))
         {
         case 1:  result = cmpsense_gt;
             break;
         case -1: result = cmpsense_lt;
             break;
         }
      }
   
   Verbose("Comparison result: %s\n",CMPSENSETEXT[result]);
   
   switch(cmp)
      {
      case cmpsense_gt:
          match = (result == cmpsense_gt);
          break;
      case cmpsense_ge:
          match = (result == cmpsense_gt || result == cmpsense_eq);
          break;
      case cmpsense_lt:
          match = (result == cmpsense_lt);
          break;
      case cmpsense_le:
          match = (result == cmpsense_lt || result == cmpsense_eq);
          break;
      case cmpsense_eq:
          match = (result == cmpsense_eq);
          break;
      case cmpsense_ne:
          match = (result != cmpsense_eq);
          break;
      }
   
   /* If we find a match, just return it now, and don't bother checking
    * anything else RPM returned, if it returns multiple packages */
   
   if (match)
      {
      DeleteItemList(evrlist);
      return 1;
      }
   }

/* If we manage to make it out of the loop, we did not find a match. */

DeleteItemList(evrlist);
return 0;
}

/*********************************************************************/

int RPMPackageList (char *package, char *version, enum cmpsense cmp, struct Item **pkglist)

 { FILE *pp;
   struct Item *evrlist = NULL;
   struct Item *evr;
   int epochA = 0; /* What is installed.  Assume 0 if we don't get one. */
   int epochB = 0; /* What we are looking for.  Assume 0 if we don't get one. */
   const char *eA = NULL; /* What is installed */
   const char *eB = NULL; /* What we are looking for */
   const char *vA = NULL; /* What is installed */
   const char *vB = NULL; /* What we are looking for */
   const char *rA = NULL; /* What is installed */
   const char *rB = NULL; /* What we are looking for */
   enum cmpsense result;
   int match = 0;
   int matchcnt = 0;
   int cmdresult = 0;

   char *tmpp;
   char line[CF_BUFSIZE];
   char tver1[CF_BUFSIZE];
   char tver2[CF_BUFSIZE];
   char tpack[CF_BUFSIZE];

  Verbose ("RPMPackageList(): Requested version %s %s of %s \n",
           CMPSENSETEXT[cmp],(version[0] ? version : "ANY"), package );

/* find out whats installed */
 if (GetMacroValue(CONTEXTID,"RPMcommand"))
    {
    snprintf(VBUFF,CF_BUFSIZE,"%s -q --queryformat \"%%{EPOCH}:%%{VERSION}-%%{RELEASE}\\n\" %s",GetMacroValue(CONTEXTID,"RPMcommand"),package);
    }
 else
    {
    snprintf(VBUFF,CF_BUFSIZE,"/bin/rpm -q --queryformat \"%%{EPOCH}:%%{VERSION}-%%{RELEASE}\\n\" %s", package);
    }

 if ((pp = cfpopen(VBUFF, "r")) == NULL)
    {
    Verbose("Could not execute the RPM command.  Assuming package not installed.\n");
    return -1;
    }

 while(!feof(pp))
    {
    *line = '\0';

    ReadLine( line, CF_BUFSIZE, pp );
    Debug("PackageList: read line %s\n", line);

    if (*line != '\0')
       {
       AppendItem( &evrlist, line, "" );
       }
    }


 cmdresult = cfpclose(pp);

 /* Non-zero exit status means that we could not find the package, so
  * Zero the list and bail. */

 if ( cmdresult != 0 )
    {
    DeleteItemList(evrlist);
    evrlist = NULL;
    }

 if (evrlist == NULL)
    {
/*  this return is okay, we would returned 0 if we had parsed the whole list
 *  and found no matches
 */
    Verbose("RPM Package %s not installed.\n", package);
    return 0;
    }

 /* Parse the EVR we are looking for once at the start */
 memset( tver1, 0, CF_BUFSIZE );
 strncpy( tver1, version, CF_BUFSIZE - 1 );
 ParseEVR( tver1, &eB, &vB, &rB);

 /*
  * The evrlist list contains the lines of output from the
  * rpm -q --queryformat "%{EPOCH}:%%{VERSION}-%%{RELEASE}\n"
  * command.  These will be version of requested package installed on
  * on the system.  In the loop, use the value of result and the ParseEVR
  * function to determine if the installed package fits the selection
  * criteria specified in the cfengine config file
  */

 for (evr = evrlist; evr != NULL; evr=evr->next)
    {
    char *evrstart;
    evrstart = evr->name;
    result = cmpsense_eq;

    /*
     * RPM returns the string "(none)" for the epoch if there is none
     * instead of the number 0.  This will cause ParseEVR() to misinterpret
     * it as part of the version component, since epochs must be numeric.
     * If we get "(none)" at the start of the EVR string, we must be careful
     * to replace it with a 0 and reset evrstart to where the 0 is.  Ugh.
     */

    if (!strncmp(evrstart, "(none)", strlen("(none)")))
       {
       /* We have no Epoch in the installed package, force it to "0". */
       evrstart = strchr(evrstart, ':') - 1;
       *evrstart = '0';
       }

   Verbose("RPMPackageList():  "
           "Checking installed version %s against requested version %s\n",
           evrstart, version );
    memset( tver2, 0, CF_BUFSIZE );
    strncpy( tver2, evrstart, CF_BUFSIZE - 1 );
    ParseEVR( tver2, &eA, &vA, &rA);

    epochA = atol(eA);   /* The above code makes sure we always have this. */

    if (eB && *eB) /* the B side is what the user entered.  Better check. */
       {
       epochB = atol(eB);
       }

    /* First try the epoch. */

    if (epochA > epochB)
       {
       result = cmpsense_gt;
       }

    if (epochA < epochB)
       {
       result = cmpsense_lt;
       }

    /* If that did not decide it, try version.  We must *always* have
     * a version string.  That's just the way it is.*/

    if (result == cmpsense_eq)
       {
       switch (rpmvercmp(vA, vB))
          {
          case 1:    result = cmpsense_gt;
              break;
          case -1:   result = cmpsense_lt;
              break;
          }
       }

    /* if we wind up here, everything rides on the release if both have it.
     * RPM always stores a release internally in the database, so the A side
     * will have it.  It's just a matter of whether or not the user cares
     * about it at this point. */

    if ((result == cmpsense_eq) && (rB && *rB))
       {
       switch (rpmvercmp(rA, rB))
          {
          case 1:  result = cmpsense_gt;
              break;
          case -1: result = cmpsense_lt;
              break;
          }
       }

    Verbose("Comparison result: %s\n",CMPSENSETEXT[result]);

    switch(cmp)
       {
       case cmpsense_gt:
           match = (result == cmpsense_gt);
           break;
       case cmpsense_ge:
           match = (result == cmpsense_gt || result == cmpsense_eq);
           break;
       case cmpsense_lt:
           match = (result == cmpsense_lt);
           break;
       case cmpsense_le:
           match = (result == cmpsense_lt || result == cmpsense_eq);
           break;
       case cmpsense_eq:
           match = (result == cmpsense_eq);
           break;
       case cmpsense_ne:
           match = (result != cmpsense_eq);
           break;
       }

    /*
     * If we find a match, build a string with the package name, version and
     * release using the approprieate delimiters and put it on the list
     */
    if (match)
       {
       matchcnt++;

 /*    point to the colon after the epoch and make it a dash for
       creating the pacakge name with the version and revsion */
       tmpp = strchr( evrstart, ':' );
       tmpp[0] = '-';

 /*    concatenate the package name and version-revision */
       memset( tpack, 0, CF_BUFSIZE );
       strncpy( tpack, package, CF_BUFSIZE - 1 );
       strncat( tpack, tmpp, CF_BUFSIZE - 1 - strlen(tpack) );

 /*    append the string with pkgname, version and revision to pkglist */
       AppendItem( pkglist, tpack, NULL );
       }
    }

  Debug("RPMPackageList():  "
        "Found %d installed version(s) of %s that met match criteria\n",
         matchcnt, package);

 /* Once we manage to make it out of the loop,
  * if pkglist is NULL, we did not find a match.
  */
 if ( *pkglist == NULL )
    {
     return 0;
    }
 else
    {
     return 1;
    }
 }

/*********************************************************************/
/* Debian                                                            */
/*********************************************************************/

int DPKGPackageCheck(char *package,char *version,enum cmpsense cmp)

{ FILE *pp;
 struct Item *evrlist = NULL;
 struct Item *evr;
 char *evrstart;
 enum cmpsense result;
 int match = 0;
 char tmpBUFF[CF_BUFSIZE];
 
Verbose ("Package: %s\n",package);

/* check that the package exists in the package database */
snprintf (VBUFF, CF_BUFSIZE, "LC_ALL=C /usr/bin/apt-cache policy %s 2>&1 | " \
          "grep -v \"W: Unable to locate package \"", package);

if ((pp = cfpopen_sh(VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute APT-command (apt-cache).\n");
   return 0;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   }

if (cfpclose (pp) != 0)
   {
   Verbose ("The package %s did not exist in the package database.\n",package);
   return 0;
   }

/* check what version is installed on the system (if any) */
snprintf (VBUFF, CF_BUFSIZE, "LC_ALL=C /usr/bin/apt-cache policy %s", package);

/*
 * HvB: cfopen to cfopen_sh, fix bug for packages without version number
*/

if ((pp = cfpopen_sh(VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute APT-command (apt-cache policy).\n");
   return 0;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   if (*VBUFF != '\0')
      {
      if (sscanf (VBUFF, "  Installed: %s", tmpBUFF) > 0)
         {
         AppendItem (&evrlist, tmpBUFF, "");
         }
      }
   }

if (cfpclose (pp) != 0)
   {
   Verbose ("Something impossible happened... ('grep' exited abnormally).\n");
   DeleteItemList (evrlist);
   return 0;
   }

/* Assume that there is only one package in the list */
evr = evrlist;

if (evr == NULL)
   {
   /* We did not find a match, and returns */
   DeleteItemList (evrlist);
   return 0;
   }

evrstart = evr->name;

/* if version value is "(null)", the packages was not installed
   -> the package has no version and dpkg --compare-versions will
   treat "" as "no version" */

if (strncmp (evrstart, "(none)", strlen ("(none)")) == 0)
   {
   sprintf (evrstart, "\"\"");
   
   /* RB 34.02.06:
    *   Set compare result to nothing when (not)installed version is none
    *   because else we might return True for package check
    *   if checking without version/cmp statement.
    *
    *   Or else it will cause us to assume package is installed
    *   while it is actually not.
    */
   
   result = cmpsense_none;
   }
else
   {
   result = cmpsense_eq;
   }

if (strncmp (version, "(none)", strlen ("(none)")) == 0)
   {
   sprintf (version, "\"\"");
   }

/* the evrstart shall be a version number which we will
   compare to version using '/usr/bin/dpkg --compare-versions' */

/* check if installed version is gt version */

snprintf (VBUFF, CF_BUFSIZE, "/usr/bin/dpkg --compare-versions %s gt %s", evrstart, version);

if ((pp = cfpopen (VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute DPKG-command.\n");
   return 0;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   }

/* if dpkg --compare-versions exits with zero result the condition
   was satisfied, else not satisfied */

if (cfpclose (pp) == 0)
   {
   result = cmpsense_gt;
   }    

/* check if installed version is lt version */
snprintf (VBUFF, CF_BUFSIZE, "/usr/bin/dpkg --compare-versions %s lt %s", evrstart, version);

if ((pp = cfpopen (VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute DPKG-command.\n");
   return 0;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   }

/* if dpkg --compare-versions exits with zero result the condition
   was satisfied, else not satisfied */

if (cfpclose (pp) == 0)
   {
   result = cmpsense_lt;
   }    

Verbose ("Comparison result: %s\n", CMPSENSETEXT[result]);

switch (cmp)
   {
   case cmpsense_gt:
       match = (result == cmpsense_gt);
       break;
   case cmpsense_ge:
       match = (result == cmpsense_gt || result == cmpsense_eq);
       break;
   case cmpsense_lt:
       match = (result == cmpsense_lt);
       break;
   case cmpsense_le:
       match = (result == cmpsense_lt || result == cmpsense_eq);
       break;
   case cmpsense_eq:
       match = (result == cmpsense_eq);
       break;
   case cmpsense_ne:
       match = (result != cmpsense_eq);
       break;
   }

if (match)
   {
   DeleteItemList (evrlist);
   return 1;
   }

/* if we manage to make it here, we did not find a match */
DeleteItemList (evrlist);
return 0;
}

/*********************************************************************/

int DPKGPackageList (char *package, char *version, enum cmpsense cmp, struct Item **pkglist)
{
 /* Rather than re-checking packages, assume the package name is
  * installed since the DPKGPackageCheck was positive.  This is
  * possible since Cfengine+dpkg doesn't support granular version
  * install/removes */

AppendItem(pkglist,package,"");
return 1;
}

/*********************************************************************/
/* Sun - pkginfo/pkgadd/pkgrm                                        */
/*********************************************************************/

/* 
 * On solaris a package that can be installed is a file or a
 * directory on disk. An installed package isn't, it's just a name.
 * So in these functions below, the dirname of the package argument is
 * stripped so that we can handle requests about installed packages.
 * The last '-' is the delimiter between the package name and the version.
 */

int SUNPackageCheck(char *package,char *version,enum cmpsense cmp)

{ FILE *pp;
  char tmpBUFF[CF_BUFSIZE];
  struct Item *evrlist = NULL;
  struct Item *evr;
  char *evrstart;
  enum cmpsense result;
  int match = 0;
  char tmppkg[CF_BUFSIZE];
  char *pkgname;
  char *pkgversion;
  int majorA = 0;
  int majorB = 0;
  int minorA = 0;
  int minorB = 0;
  int microA = 0;
  int microB = 0;
  char *revisA = NULL;
  char *revisB = NULL;

/* The parsing below modifies one of our arguments, so copy it first */
strncpy(tmppkg, package, CF_BUFSIZE-1);
tmppkg[CF_BUFSIZE-1] = 0;

if ((pkgname = strrchr(tmppkg, '/')) != NULL)
   {
   *pkgname = '\0';
   pkgname++;
   }
else
   {
   pkgname = tmppkg;
   }

if ((pkgversion = strrchr(pkgname, '-')) != NULL)
   {
   *pkgversion = '\0';
   }

Verbose ("Package: %s\n",pkgname);

/* check that the package exists in the package database */

snprintf (VBUFF, CF_BUFSIZE, "/usr/bin/pkginfo -i -q %s", pkgname);

if ((pp = cfpopen (VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute pkginfo -i -q.\n");
   return 0;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   }

if (cfpclose (pp) != 0)
   {
   Verbose ("The package %s did not exist in the package database.\n",pkgname);
   return 0;
   }

/* If no version was specified, we're just checking if the package
 * is present, not for a particular number, and we can skip the
 * version number fetch and check.
 */

if (!*version)
   {
   return 1;
   }

/* check what version is installed on the system (if any) */

snprintf (VBUFF, CF_BUFSIZE, "/usr/bin/pkginfo -i -l %s", pkgname);

if ((pp = cfpopen (VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute pkginfo -i -l.\n");
   return 0;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   if (*VBUFF != '\0')
      {
      if (sscanf (VBUFF, "   VERSION:  %s", tmpBUFF) > 0)
         {
         AppendItem (&evrlist, tmpBUFF, "");
         }
      }
   }

if (cfpclose (pp) != 0)
   {
   Verbose ("pkginfo -i -l exited abnormally.\n");
   DeleteItemList (evrlist);
   return 0;
   }

/* Parse the Sun Version we are looking for once at the start */

ParseSUNVR(version, &majorB, &minorB, &microB, &revisB);

/* The rule here will be: if any package in the list matches, then the
 * first one to match wins, and we bail out. */

for (evr = evrlist; evr != NULL; evr=evr->next)
   {
   char *evrstart;
   evrstart = evr->name;
   
   /* Start out assuming the comparison will be equal. */
   result = cmpsense_eq;
   
   ParseSUNVR(evrstart, &majorA, &minorA, &microA, &revisA);
   
   /* Compare the major versions. */
   if (majorA > majorB)
      {
      result = cmpsense_gt;
      }
   if (majorA < majorB)
      {
      result = cmpsense_lt;
      }
   
   /* If the major versions are the same, check the minor versions. */
   if (result == cmpsense_eq)
      {
      if(minorA > minorB)
         {
         result = cmpsense_gt;
         }
      
      if(minorA < minorB)
         {
         result = cmpsense_lt;
         }
      
      /* If the minor versions match, compare the micro versions. */
      
      if (result == cmpsense_eq)
         {
         if (microA > microB)
            {
            result = cmpsense_gt;
            }
         if (microA < microB)
            {
            result = cmpsense_lt;
            }
         }

      /* If the micro versions match, compare the revisions. */

      if (result == cmpsense_eq)
         {
         if (strncasecmp(revisA ? revisA : "", revisB ? revisB : "", CF_BUFSIZE) > 0)
            {
            result = cmpsense_gt;
            }
         if (strncasecmp(revisA ? revisA : "", revisB ? revisB : "", CF_BUFSIZE) < 0)
            {
            result = cmpsense_lt;
            }
         }
      }
   
   switch(cmp)
      {
      case cmpsense_gt:
          match = (result == cmpsense_gt);
          break;
      case cmpsense_ge:
          match = (result == cmpsense_gt || result == cmpsense_eq);
          break;
      case cmpsense_lt:
          match = (result == cmpsense_lt);
          break;
      case cmpsense_le:
          match = (result == cmpsense_lt || result == cmpsense_eq);
          break;
      case cmpsense_eq:
          match = (result == cmpsense_eq);
          break;
      case cmpsense_ne:
          match = (result != cmpsense_eq);
          break;
      }
   
   if (match)
      {
      DeleteItemList(evrlist);
      return 1;
      }
   }

/* If we made it out of the loop, there were no matches. */
DeleteItemList(evrlist);
return 0;
}

/*********************************************************************/

int SUNPackageList (char *package, char *version, enum cmpsense cmp, struct Item **pkglist)

{ FILE *pp;
  struct Item *evrlist = NULL;
  struct Item *evr;
  int match = 0, result;
  char line[CF_BUFSIZE];
  char tmppkg[CF_BUFSIZE];
  char *pkgname;
  char *pkgversion;
  char tmpBUFF[CF_BUFSIZE];
  int majorA = 0;
  int majorB = 0;
  int minorA = 0;
  int minorB = 0;
  int microA = 0;
  int microB = 0;
  char *revisA = NULL;
  char *revisB = NULL;

/* The parsing below modifies one of our arguments, so copy it first */
strncpy(tmppkg, package, CF_BUFSIZE-1);
tmppkg[CF_BUFSIZE-1] = 0;

if ((pkgname = strrchr(tmppkg, '/')) != NULL)
{
   *pkgname = '\0';
   pkgname++;
   }
else
   {
   pkgname = tmppkg;
   }

if ((pkgversion = strrchr(pkgname, '-')) != NULL)
   {
   *pkgversion = '\0';
   }

Debug("SUNPackageList(): Requested version %s %s of %s\n", CMPSENSETEXT[cmp],(version[0] ? version : "ANY"), pkgname);

/* If no version was specified, we're just checking if the package
 * is present, not for a particular number, so >0 will match.
 */
if (!*version)
   {
   cmp = cmpsense_gt;
   version[0] = '0';
   version[1] = 0;
   }

/* check what version is installed on the system (if any) */

snprintf (VBUFF, CF_BUFSIZE, "/usr/bin/pkginfo -i -l %s", pkgname);
Verbose("SUNPackageList(): Running /usr/bin/pkginfo -i -l %s\n", pkgname);

if ((pp = cfpopen (VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute pkginfo -i -l.\n");
   return 0;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   if (*VBUFF != '\0')
      {
      if (sscanf (VBUFF, "   VERSION:  %s", tmpBUFF) > 0)
         {
         Debug("SUNPackageList: found package version %s\n", tmpBUFF);
         AppendItem(&evrlist,tmpBUFF,"");
         }
      }
   }
if (cfpclose (pp) != 0)
   {
   Verbose ("pkginfo -i -l exited abnormally.\n");
   DeleteItemList (evrlist);
   return 0;
   }

/* Parse the Sun Version we are looking for once at the start */

ParseSUNVR(version, &majorB, &minorB, &microB, &revisB);

/* The rule here will be: if any package in the list matches, then the
 * first one to match wins, and we bail out. */

for (evr = evrlist; evr != NULL; evr=evr->next)
   {
   char *evrstart;
   evrstart = evr->name;

   /* Start out assuming the comparison will be equal. */
   result = cmpsense_eq;

   ParseSUNVR(evrstart, &majorA, &minorA, &microA, &revisA);

   /* Compare the major versions. */
   if (majorA > majorB)
      {
      result = cmpsense_gt;
      }
   if (majorA < majorB)
      {
      result = cmpsense_lt;
      }

   /* If the major versions are the same, check the minor versions. */
   if (result == cmpsense_eq)
      {
      if(minorA > minorB)
         {
         result = cmpsense_gt;
         }

      if(minorA < minorB)
         {
         result = cmpsense_lt;
         }

      /* If the minor versions match, compare the micro versions. */

      if (result == cmpsense_eq)
         {
         if (microA > microB)
            {
            result = cmpsense_gt;
            }
         if (microA < microB)
            {
            result = cmpsense_lt;
            }
         }
      }

      /* If the micro versions match, compare the revisions. */

      if (result == cmpsense_eq)
         {
         if (strncasecmp(revisA ? revisA : "", revisB ? revisB : "", CF_BUFSIZE) > 0)
            {
            result = cmpsense_gt;
            }
         if (strncasecmp(revisA ? revisA : "", revisB ? revisB : "", CF_BUFSIZE) < 0)
            {
            result = cmpsense_lt;
            }
         }

   switch(cmp)
      {
      case cmpsense_gt:
          match = (result == cmpsense_gt);
          break;
      case cmpsense_ge:
          match = (result == cmpsense_gt || result == cmpsense_eq);
          break;
      case cmpsense_lt:
          match = (result == cmpsense_lt);
          break;
      case cmpsense_le:
          match = (result == cmpsense_lt || result == cmpsense_eq);
          break;
      case cmpsense_eq:
          match = (result == cmpsense_eq);
          break;
      case cmpsense_ne:
          match = (result != cmpsense_eq);
          break;
      }

   if (match)
      {
      AppendItem(pkglist, pkgname, NULL);
      DeleteItemList(evrlist);
      return 1;
      }
   }

/* If we made it out of the loop, there were no matches. */
DeleteItemList(evrlist);
return 0;
}

/*********************************************************************/
/* Sun's manual pages say that the version number is a major, minor,
 * and optional micro version number.  This code checks for that.
 * It will not handle other arbitrary and strange values people might
 * put in like "2.6d.12a" or "1.11 beta" or "pre-release 7"
 * Note that Sun uses REV=<timestamp> revision numbers appended to the
 * version string. These are handled correctly.
 *********************************************************************/

void ParseSUNVR (char *vr, int *major, int *minor, int *micro, char **revis)

{ char *tmpcpy = strdup(vr);
  char *startMinor = NULL;
  char *startMicro = NULL;
  char *startRev   = NULL;
  char *p = NULL;

*major = 0;
*minor = 0;
*micro = 0;
*revis = NULL;

/* Break the copy in to major, minor, and micro. */
for(p = tmpcpy; *p; p++)
   {
   if (*p == '.')
      {
      *p = '\0';
      if (startMinor == NULL)
         {
         startMinor = p+1;
         }
      else if (startMicro == NULL)
         {
         startMicro = p+1;
         }
      else if (startRev == NULL)
         {
         startRev = p+1;
         break; /* stop parsing, because the revision contains '.' chars */
         }
      }
   }
  
*major = atoi(tmpcpy);

if (startMinor)
   {
   *minor = atoi(startMinor);
   }

if (startMicro)
   {
   *micro = atoi(startMicro);
   }

if (startRev)
   {
   *revis = startRev;
   }

free(tmpcpy);  
}

/*********************************************************************/
/* AIX - lslpp/installp                                              */
/*********************************************************************/

int AIXPackageCheck(char *package,char *version,enum cmpsense cmp)

{ FILE *pp;
  struct Item *evrlist = NULL;
  struct Item *evr;
  char *evrstart;
  enum cmpsense result;
  int match = 0;
  char tmpBUFF[CF_BUFSIZE];
  char *vs, *ve;
  int verA = 0;
  int verB = 0;
  int releaseA = 0;
  int releaseB = 0;
  int maintA = 0;
  int maintB = 0;
  int fixA = 0;
  int fixB = 0;

Verbose ("Package: %s\n",package);

/* check that the package exists in the package database */
/* we're using -l here so that RPM pkgs aren't included.  These should
be treated seperatly */

snprintf(VBUFF, CF_BUFSIZE, "/usr/bin/lslpp -qcl %s", package);

if ((pp = cfpopen (VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute lslpp -qcl.\n");
   return 0;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   }

if (cfpclose (pp) != 0)
   {
   Verbose ("The package %s did not exist in the package database.\n",package);
   return 0;
   }

/* If no version was specified, we're just checking if the package
 * is present, not for a particular number, and we can skip the
 * version number fetch and check.
 */

if (!*version)
   {
   return 1;
   }

/* check what version is installed on the system (if any) */
/* now that we know its installed and not an rpm we can use -L to consolidate
   the Usr and ROOT portions for the version check */

snprintf (VBUFF, CF_BUFSIZE, "/usr/bin/lslpp -qcL %s", package);

if ((pp = cfpopen (VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute lslpp -qcL.\n");
   return 0;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (VBUFF, CF_BUFSIZE, pp);
   if (*VBUFF != '\0')
      {
      /* find third field of : delimited string */
      if (sscanf (VBUFF, "%*[^:]:%*[^:]:%[^:]:%*s",tmpBUFF) > 0)
         {
         AppendItem (&evrlist, tmpBUFF, "");
         }
      }
   }

if (cfpclose (pp) != 0)
   {   
   Verbose ("lslpp -qcL exited abnormally.\n");
   DeleteItemList (evrlist);
   return 0;
   }

/* Parse the AIX Version we are looking for once at the start */

ParseAIXVR(version, &verB, &releaseB, &maintB, &fixB);

/* The rule here will be: if any package in the list matches, then the
 * first one to match wins, and we bail out. */

for (evr = evrlist; evr != NULL; evr=evr->next)
   {
   char *evrstart;
   evrstart = evr->name;
   result = cmpsense_eq;
   
   ParseAIXVR(evrstart, &verA, &releaseA, &maintA, &fixA);
   
   if (verA > verB)
      {
      result = cmpsense_gt;
      }
   if(verA < verB)
      {
      result = cmpsense_lt;
      }
   
   if (result == cmpsense_eq)
      {
      if (releaseA > releaseB)
         {
         result = cmpsense_gt;
         }
      if (releaseA < releaseB)
         {
         result = cmpsense_lt;
         }
      
      if (result == cmpsense_eq)
         {
         if (maintA > maintB)
            {
            result = cmpsense_gt;
            }
         if (maintA < maintB)
            {
            result = cmpsense_lt;
            }
         
         /* If the maint versions match, compare the fix level */
         
         if (result == cmpsense_eq)
            {
            if (fixA > fixB)
               {
               result = cmpsense_gt;
               }
            if (fixA < fixB)
               {
               result = cmpsense_lt;
               }
            }
         }
      }
   
   switch(cmp)
      {
      case cmpsense_gt:
          match = (result == cmpsense_gt);
          break;
      case cmpsense_ge:
          match = (result == cmpsense_gt || result == cmpsense_eq);
          break;
      case cmpsense_lt:
          match = (result == cmpsense_lt);
          break;
      case cmpsense_le:
          match = (result == cmpsense_lt || result == cmpsense_eq);
          break;
      case cmpsense_eq:
          match = (result == cmpsense_eq);
          break;
      case cmpsense_ne:
          match = (result != cmpsense_eq);
          break;
      } 
   
   if (match)
      {
      DeleteItemList(evrlist);
      return 1;
      }
   }

/* If we made it out of the loop, there were no matches. */
DeleteItemList(evrlist);
return 0;
}

/*********************************************************************/

int AIXPackageList(char *package, char *version, enum cmpsense cmp, struct Item **pkglist)
{
return 0; /* not implemented yet */
}

/*********************************************************************/
/* AIX docs describe the version as:
 * Version.Release.Maintenance/Modification.Fix (V.R.M.F).  
 * any non digits [0-9] in these fields will be ignored and only the 
 * numeric digits will be extracted.  standalone non-digits will be 
 * treated as 0 for the entire field.  V.R.M.F shouldn't contain any 
 * non numeric data (this is enforced by IBM tools like mkinstallp)
/*********************************************************************/

void ParseAIXVR(char * vr, int *ver, int *release, int *maint, int *fix)

{ int v,r,m,f;

*ver = 0;
*release = 0;
*maint = 0;
*fix = 0;

if (sscanf(vr,"%d.%*s",&v) > 0)
   {
   *ver = v;
   }

if (sscanf(vr,"%*[^.].%d.%*s",&r) > 0)
   {
   *release = r;
   }

if (sscanf(vr,"%*[^.].%*[^.].%d.%*s",&m) > 0)
   {
   *maint = m;
   }
if (sscanf(vr,"%*[^.].%*[^.].%*[^.].%d%*s",&f) > 0)
   {
   *fix = f;
   }
}

/*********************************************************************/
/* Gentoo Portage                                                    */
/*********************************************************************/

int PortagePackageCheck(char *package,char *version,enum cmpsense cmp)

{ FILE *pp;
  struct Item *ebuildlist = NULL;
  struct Item *ebuild = NULL;
  int match = 0;
  char *result = NULL;
  char pkgname[CF_BUFSIZE] = {0};
  char *nameptr = NULL;

/* Create a working copy of the name */
strncpy(pkgname, package, CF_BUFSIZE - 1);

/* Test if complete package atom was given */
if (pkgname[0] == '=' || pkgname[0] == '<' || pkgname[0] == '>')
   {
   /* Strip version */
   nameptr = strchr(pkgname, '/');
   if (nameptr == NULL)
      {
      /* Package does not include category, which is fine */
      nameptr = pkgname;
      }
   while (!xisdigit(nameptr[1]))
      {
      nameptr = strchr(++nameptr, '-');
      if (nameptr == NULL)
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Unable to parse version from %s!\n",pkgname);
         CfLog(cferror,OUTPUT,"");
         return -1;
         }
      }
   nameptr[0] = '\0';

   /* Strip comparison operator (or rather, seek past) */
   nameptr = pkgname;
   while (!xisalpha(nameptr[0]))
      {
      ++nameptr;
      }
   }
else
   {
   nameptr = pkgname;
   }

/* Search for installed versions of package */
snprintf(VBUFF,CF_BUFSIZE,"/usr/bin/qlist -IevC %s",nameptr);

if ((pp = cfpopen(VBUFF, "r")) == NULL)
   {
   CfLog(cferror,"Could not execute the qlist command. Is portage-utils installed?\n","");
   return -1;
   }

while(!feof(pp))
   {
   *VBUFF = '\0';
   
   ReadLine(VBUFF,CF_BUFSIZE,pp);
   
   if (*VBUFF != '\0')
      {
      AppendItem(&ebuildlist,VBUFF,"");
      }
   }

cfpclose(pp);

if (ebuildlist == NULL)
   {
   Verbose("PortagePackageCheck(): Package %s not installed.\n", nameptr);
   return 0;
   }

Verbose("PortagePackageCheck(): Requested %s %s %s\n", nameptr, CMPSENSETEXT[cmp],(version[0] ? version : "ANY"));

/* If no version was specified, return successful (found something) */
if (!version[0])
   {
   DeleteItemList(ebuildlist);
   return 1;
   }

/* Iterate through all installed versions until match is found */
for (ebuild = ebuildlist; ebuild != NULL; ebuild=ebuild->next)
   {
   Verbose("PortagePackageCheck(): Trying installed version %s\n", ebuild->name);

   /* Run comparison tool to do the grunt work */
   snprintf(VBUFF,CF_BUFSIZE,"/usr/bin/qatom -cC %s %s-%s", ebuild->name, nameptr, version);

   if ((pp = cfpopen(VBUFF, "r")) == NULL)
      {
      CfLog(cferror,"Could not execute the qatom command. Is portage-utils installed?\n","");
      continue;
      }
  
   if (feof(pp))
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Internal error!  No output from %s.",VBUFF);
      CfLog(cferror,OUTPUT,"");
      continue;
      }

   /* Format of output is `package < package' */
   *VBUFF = '\0';
   ReadLine(VBUFF,CF_BUFSIZE,pp);
   Verbose("PortagePackageCheck(): Result %s\n",VBUFF);
   cfpclose(pp);

   /* Find first space, give up otherwise */
   result = strchr(VBUFF, ' ');
   if (result == NULL) continue;

   /* Relocate to right of space (the comparison symbol) */
   ++result;

   switch(cmp)
      {
      case cmpsense_gt:
         match = (*result == *CMPSENSEOPERAND[cmpsense_gt]);
         break;
      case cmpsense_ge:
         match = (*result == *CMPSENSEOPERAND[cmpsense_gt] || *result == *CMPSENSEOPERAND[cmpsense_eq]);
         break;
      case cmpsense_lt:
         match = (*result == *CMPSENSEOPERAND[cmpsense_lt]);
         break;
      case cmpsense_le:
         match = (*result == *CMPSENSEOPERAND[cmpsense_lt] || *result == *CMPSENSEOPERAND[cmpsense_eq]);
         break;
      case cmpsense_eq:
         match = (*result == *CMPSENSEOPERAND[cmpsense_eq]);
         break;
      case cmpsense_ne:
         match = (*result != *CMPSENSEOPERAND[cmpsense_eq]);
         break;
      }

   /* Return successful on finding a match */
   if (match)
      {
      DeleteItemList(ebuildlist);
      return 1;
      }
   }

/* No match found, return false */
DeleteItemList(ebuildlist);
return 0;
}

/*********************************************************************/

int PortagePackageList(char *package, char *version, enum cmpsense cmp, struct Item **pkglist)
{ FILE *pp;
  struct Item *ebuildlist = NULL;
  struct Item *ebuild = NULL;
  int match = 0;
  int nummatches = 0;
  char *result = NULL;
  char pkgname[CF_BUFSIZE] = {0};
  char pkgatom[CF_BUFSIZE] = {0};
  char *nameptr = NULL;

/* Create a working copy of the name */
strncpy(pkgname, package, CF_BUFSIZE - 1);

/* Test if complete package atom was given */
if (pkgname[0] == '=' || pkgname[0] == '<' || pkgname[0] == '>')
   {
   /* Strip version */
   nameptr = strchr(pkgname, '/');
   if (nameptr == NULL)
      {
      /* Package does not include category, which is fine */
      nameptr = pkgname;
      }
   while (!xisdigit(nameptr[1]))
      {
      nameptr = strchr(++nameptr, '-');
      if (nameptr == NULL)
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Unable to parse version from %s!\n",pkgname);
         CfLog(cferror,OUTPUT,"");
         return -1;
         }
      }
   nameptr[0] = '\0';

   /* Strip comparison operator (or rather, seek past) */
   nameptr = pkgname;
   while (!xisalpha(nameptr[0]))
      {
      ++nameptr;
      }
   }
else
   {
   nameptr = pkgname;
   }

/* Search for installed versions of package */
snprintf(VBUFF,CF_BUFSIZE,"/usr/bin/qlist -IevC %s",nameptr);

if ((pp = cfpopen(VBUFF, "r")) == NULL)
   {
   CfLog(cferror,"Could not execute the qlist command. Is portage-utils installed?\n","");
   return -1;
   }

while(!feof(pp))
   {
   *VBUFF = '\0';

   ReadLine(VBUFF,CF_BUFSIZE,pp);

   if (*VBUFF != '\0')
      {
      AppendItem(&ebuildlist,VBUFF,"");
      }
   }

cfpclose(pp);

if (ebuildlist == NULL)
   {
   Verbose("PortagePackageList(): Package %s not installed.\n", nameptr);
   return 0;
   }

Verbose("PortagePackageList(): Requested %s %s %s\n", nameptr, CMPSENSETEXT[cmp],(version[0] ? version : "ANY"));

/* Iterate through all installed versions and register matches */
for (ebuild = ebuildlist; ebuild != NULL; ebuild=ebuild->next)
   {
   Verbose("PortagePackageList(): Trying installed version %s\n", ebuild->name);

   /* If no version was specified, register ebuild */
   if (!version[0])
      {
      strncpy(pkgatom, "=", CF_BUFSIZE - 1);
      strncat(pkgatom, ebuild->name, CF_BUFSIZE - 2);
      snprintf(OUTPUT,CF_BUFSIZE,"Package atom matches: %s\n",pkgatom);
      AppendItem(pkglist,pkgatom,"");
      ++nummatches;
      continue;
      }

   /* Run comparison tool to do the grunt work */
   snprintf(VBUFF,CF_BUFSIZE,"/usr/bin/qatom -cC %s %s-%s", ebuild->name, nameptr, version);

   if ((pp = cfpopen(VBUFF, "r")) == NULL)
      {
      CfLog(cferror,"Could not execute the qatom command. Is portage-utils installed?\n","");
      continue;
      }

   if (feof(pp))
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Internal error!  No output from %s.",VBUFF);
      CfLog(cferror,OUTPUT,"");
      continue;
      }

   /* Format of output is `package < package' */
   *VBUFF = '\0';
   ReadLine(VBUFF,CF_BUFSIZE,pp);
   Verbose("PortagePackageList(): Result %s\n",VBUFF);
   cfpclose(pp);

   /* Find first space, give up otherwise */
   result = strchr(VBUFF, ' ');
   if (result == NULL) continue;

   /* Relocate to right of space (the comparison symbol) */
   ++result;

   switch(cmp)
      {
      case cmpsense_gt:
         match = (*result == *CMPSENSEOPERAND[cmpsense_gt]);
         break;
      case cmpsense_ge:
         match = (*result == *CMPSENSEOPERAND[cmpsense_gt] || *result == *CMPSENSEOPERAND[cmpsense_eq]);
         break;
      case cmpsense_lt:
         match = (*result == *CMPSENSEOPERAND[cmpsense_lt]);
         break;
      case cmpsense_le:
         match = (*result == *CMPSENSEOPERAND[cmpsense_lt] || *result == *CMPSENSEOPERAND[cmpsense_eq]);
         break;
      case cmpsense_eq:
         match = (*result == *CMPSENSEOPERAND[cmpsense_eq]);
         break;
      case cmpsense_ne:
         match = (*result != *CMPSENSEOPERAND[cmpsense_eq]);
         break;
      }

   /* Register ebuild on finding a match */
   if (match)
      {
      strncpy(pkgatom, "=", CF_BUFSIZE - 1);
      strncat(pkgatom, ebuild->name, CF_BUFSIZE - 2);
      snprintf(OUTPUT,CF_BUFSIZE,"Package atom matches: %s\n",pkgatom);
      AppendItem(pkglist,pkgatom,"");
      ++nummatches;
      continue;
      }
   }

DeleteItemList(ebuildlist);

/* Return whether matches found */
return nummatches > 0 ? 1 : 0;

}

/*********************************************************************/
/* FreeBSD - pkg_info/pkg_add/pkg_delete                             */
/*********************************************************************/

int FreeBSDPackageCheck(char *package,char *version,enum cmpsense cmp)

{ FILE *pp;
  int match = 0;
  int result;
  char line[CF_BUFSIZE];
  char pkgname[CF_BUFSIZE];
  char *pkgversion;

/* The package to compare must contain a version number
 * The version starts after the last '-' in the pkgname
 */

strncpy(pkgname, package, CF_BUFSIZE - 1);
pkgversion = strrchr(pkgname, '-');
if (pkgversion) 
   {
   /* insert a null to remove version for comparison */
   *pkgversion = '\0';
   }

Debug("FreeBSDPackageCheck(): Requested version %s %s of %s\n", CMPSENSETEXT[cmp],(version[0] ? version : "ANY"), pkgname);

if (!*version)
   {
   cmp = cmpsense_gt;
   version[0] = '0';
   version[1] = 0;
   }

/* check what version is installed on the system (if any) */
Verbose("FreeBSDPackageCheck(): Running /usr/sbin/pkg_info -qE '%s%s%s'\n", pkgname, CMPSENSEOPERAND[cmp], version);
snprintf (VBUFF, CF_BUFSIZE, "/usr/sbin/pkg_info -qE '%s%s%s'", pkgname, CMPSENSEOPERAND[cmp], version);

if ((pp = cfpopen (VBUFF, "r")) == NULL)
   {
   CfLog(cferror,"FATAL: Could not execute pkg_info.\n","popen");
   return -1;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (line, CF_BUFSIZE - 1, pp);
   snprintf(OUTPUT,CF_BUFSIZE,"%s\n",line);
   }

result =  cfpclose( pp );
switch( result )
   {
   case 0: 
       Verbose ("FreeBSDPackageCheck(): %s %s %s is installed on this system.\n", pkgname, CMPSENSETEXT[cmp],(version[0] ? version : "ANY") );
       match = 1;
       break;
       
   case 1: 
       Verbose ("FreeBSDPackageCheck(): %s %s %s is not installed on this system.\n", pkgname, CMPSENSETEXT[cmp],(version[0] ? version : "ANY") );
       match = 0;
       break;
       
   case 256: 
       Verbose ("FreeBSDPackageCheck(): %s %s %s is not installed on this system.\n", pkgname, CMPSENSETEXT[cmp],(version[0] ? version : "ANY") );
       match = 0;
       break;
       
   default:
       Verbose ("FreeBSDPackageCheck(): error running package query: %d\n", result );
       match = -1;
       break;
   }

return match;
}

/***********************************************************************/

int FreeBSDPackageList(char *package, char *version, enum cmpsense cmp, struct Item **pkglist)
    
{ FILE *pp;
  int match = 0, result;
  char line[CF_BUFSIZE];
  char pkgname[CF_BUFSIZE];
  char *pkgversion;

/* The package name is derived by stripping off the version number */
strncpy(pkgname, package, CF_BUFSIZE - 1);
pkgversion = strrchr(pkgname, '-');
if( pkgversion )
   {
   *pkgversion = '\0';
   }

Debug("FreeBSDPackageCheck(): Requested version %s %s of %s\n", CMPSENSETEXT[cmp],(version[0] ? version : "ANY"), pkgname);

/* If no version was specified, we're just checking if the package
 * is present, not for a particular number, so >0 will match.
 */
if (!*version)
   {
   cmp = cmpsense_gt;
   version[0] = '0';
   version[1] = 0;
   }

Verbose("FreeBSDPackageList(): Running /usr/sbin/pkg_info -E '%s%s%s'\n", pkgname, CMPSENSEOPERAND[cmp], version);
snprintf (VBUFF, CF_BUFSIZE, "/usr/sbin/pkg_info -E '%s%s%s'", pkgname, CMPSENSEOPERAND[cmp], version);

if ((pp = cfpopen (VBUFF, "r")) == NULL)
   {
   Verbose ("Could not execute pkg_info.\n");
   return -1;
   }

while (!feof (pp))
   {
   *VBUFF = '\0';
   ReadLine (line, CF_BUFSIZE - 1, pp);
   Debug("PackageList: read line %s\n",line);
   
   if( strlen(line) > 1 )
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Package to remove: %s\n",line);
      AppendItem(pkglist,line,"");
      }
   }

result =  cfpclose( pp );

switch( result )
   {
   case 0: 
       Verbose ("FreeBSDPackageList(): %s %s %s is installed on this system.\n", pkgname, CMPSENSETEXT[cmp],(version[0] ? version : "ANY") );
       match=1;
       break;
       
   case 1: 
       Verbose ("FreeBSDPackageList(): %s %s %s is not installed on this system.\n", pkgname, CMPSENSETEXT[cmp],(version[0] ? version : "ANY") );
       match=0;
       break;
       
   case 256: 
       Verbose ("FreeBSDPackageList(): %s %s %s is not installed on this system.\n", pkgname, CMPSENSETEXT[cmp],(version[0] ? version : "ANY") );
       match=0;
       break;
       
   default:
       Verbose ("FreeBSDPackageList(): error running package query: %d\n", result );
       match=-1;
       break;
   }

return match;
}

/*********************************************************************/
/* RPM Version string comparison logic
 *
 * ParseEVR and rpmvercmp are taken directly from the rpm 4.1 sources.
 * ParseEVR is taken from the parseEVR routine in lib/rpmds.c and rpmvercmp
 * is taken from llib/rpmvercmp.c
 */

/* compare alpha and numeric segments of two versions */
/* return 1: a is newer than b */
/*        0: a and b are the same version */
/*       -1: b is newer than a */
/*********************************************************************/

int rpmvercmp(const char * a, const char * b)

{ char oldch1, oldch2;
  char * str1, * str2;
  char * one, * two;
  char *s1,*s2;
  int rc;
  int isnum;
 
/* easy comparison to see if versions are identical */
if (!strcmp(a, b))
   {
   return 0;
   }

one = str1 = s1 = strdup(a);
two = str2 = s2 = strdup(b);

/* loop through each version segment of str1 and str2 and compare them */

while (*one && *two)
   {
   while (*one && !xisalnum(*one)) one++;
   while (*two && !xisalnum(*two)) two++;
   
   str1 = one;
   str2 = two;
   
   /* grab first completely alpha or completely numeric segment */
   /* leave one and two pointing to the start of the alpha or numeric */
   /* segment and walk str1 and str2 to end of segment */

   if (xisdigit(*str1))
       {
       while (*str1 && xisdigit(*str1)) str1++;
       while (*str2 && xisdigit(*str2)) str2++;
       isnum = 1;
       }
    else
       {
       while (*str1 && xisalpha(*str1)) str1++;
       while (*str2 && xisalpha(*str2)) str2++;
       isnum = 0;
       }
    
    /* save character at the end of the alpha or numeric segment */
    /* so that they can be restored after the comparison */
    
    oldch1 = *str1;
    *str1 = '\0';
    oldch2 = *str2;
    *str2 = '\0';
    
    /* take care of the case where the two version segments are */
    /* different types: one numeric, the other alpha (i.e. empty) */
    
    if (one == str1)
       {
       free(s1);
       free(s2);
       return -1; /* arbitrary */
       }
    
    if (two == str2)
       {
       free(s1);
       free(s2);
       return -1;
       }
    
    if (isnum)
       {
       /* this used to be done by converting the digit segments */
       /* to ints using atoi() - it's changed because long  */
       /* digit segments can overflow an int - this should fix that. */
       
       /* throw away any leading zeros - it's a number, right? */
       while (*one == '0') one++;
       while (*two == '0') two++;
       
       /* whichever number has more digits wins */
       if (strlen(one) > strlen(two))
          {
          free(s1);
          free(s2);
          return 1;
          }
       
       if (strlen(two) > strlen(one))
          {
          free(s1);
          free(s2);
          return -1;
          }
       }
    
    /* strcmp will return which one is greater - even if the two */
    /* segments are alpha or if they are numeric.  don't return  */
    /* if they are equal because there might be more segments to */
    /* compare */
    
    rc = strcmp(one, two);
    if (rc)
       {
       free(s1);
       free(s2);

       if (rc > 0)
          {
          return 1;
          }
       
       if (rc < 0)
          {
          return -1;
          }
       }
    
    /* restore character that was replaced by null above */
    
    *str1 = oldch1;
    one = str1;
    *str2 = oldch2;
    two = str2;
    }
 
 /* this catches the case where all numeric and alpha segments have */
 /* compared identically but the segment sepparating characters were */
 /* different */
 
 if ((!*one) && (!*two))
    {
    free(s1);
    free(s2);
    return 0;
    }
 
 /* whichever version still has characters left over wins */
 if (!*one)
    {
    free(s1);
    free(s2);
    return -1;
    }
 else
    {
    free(s1);
    free(s2);
    return 1;
    }
}

/*************************************************************************/

/**
 * Split EVR into epoch, version, and release components.
 * @param evr  [epoch:]version[-release] string
 * @retval *ep  pointer to epoch
 * @retval *vp  pointer to version
 * @retval *rp  pointer to release
 */

void ParseEVR(char * evr,const char ** ep,const char ** vp,const char ** rp)

{ const char *epoch;
  const char *version;  /* assume only version is present */
  const char *release;
  char *s, *se;
 
s = evr;

while (*s && xisdigit(*s))
   {
   s++; /* s points to epoch terminator */
   }

se = strrchr(s, '-');  /* se points to version terminator */

if (*s == ':')
   {
   epoch = evr;
   *s++ = '\0';
   version = s;
   if (*epoch == '\0') epoch = "0";
   }
else
   {
   epoch = NULL; /* XXX disable epoch compare if missing */
   version = evr;
   }

if (se)
   {
   *se++ = '\0';
   release = se;
   }
else
   {
   release = NULL;
   }

if (ep) *ep = epoch;
if (vp) *vp = version;
if (rp) *rp = release;
}

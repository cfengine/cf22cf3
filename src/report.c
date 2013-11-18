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
/* File: report.c                                                            */
/*                                                                           */
/*****************************************************************************/

#define INET

#include "cf.defs.h"
#include "cf.extern.h"

#define CF_SPACER  printf("\n.   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .\n\n")

/*******************************************************************/

void ListActionSequence()

{ struct Item *ptr;

printf("\nAction sequence = (");

for (ptr=VACTIONSEQ; ptr!=NULL; ptr=ptr->next)
   {
   printf("%s ",ptr->name);
   }

printf(")\n");
}

/*******************************************************************/

void ListDefinedClasses()

{ struct Item *ip;
  struct LoL *lp;

  printf("classes:\n\n");
  
for (lp = WORKLIST; lp != NULL; lp=lp->next)
   {
   printf("  \"%s\" or => { ",CanonifyName(lp->name));

   for (ip = lp->list; ip != NULL; ip=ip->next)
      {
      printf(" \"%s\", ",CanonifyName(ip->name));
      }
   
   printf(" };\n");
   }

}

/*********************************************************************/

void ListDefinedHomePatterns(char *classes)

{ struct Item *ptr;

printf ("\nDefined wildcards to match home directories = ( ");

for (ptr = VHOMEPATLIST; ptr != NULL; ptr=ptr->next)
   {
   if (!ShowClass(classes,ptr->classes))
      {
      continue;
      }
   
   printf("%s ",ptr->name);
   }

printf (")\n");
}

/*********************************************************************/

void ListDefinedBinservers(char *classes)

{ struct Item *ptr;

printf ("\nDefined Binservers = ( ");

for (ptr = VBINSERVERS; ptr != NULL; ptr=ptr->next)
   {
   if (!ShowClass(classes,ptr->classes))
      {
      continue;
      }

   printf("%s ",ptr->name);
   
   if (ptr->classes)
      {
      printf("(pred::%s), ",ptr->classes);
      }
   }

printf (")\n");
}


/*******************************************************************/

void ListDefinedStrategies(char *classes)

{ struct Strategy *ptr;
  struct Item *ip;
 
if (VSTRATEGYLIST)
   {
   printf("classes:\n\n");
   for (ptr = VSTRATEGYLIST; ptr != NULL; ptr=ptr->next)
      {

      printf("\"%s\" dist => { ",ptr->name);
      if (ptr->strategies)
         {
         for (ip = ptr->strategies; ip !=NULL; ip=ip->next)
            {
            printf(" \"%s\", ",ip->classes);
            }
         }

      printf(" };\n");
      }
   }
}

/*******************************************************************/

void ListDefinedVariables()

{ struct cfObject *cp = NULL;

printf("vars:\n\n");
 
for (cp = VOBJ; cp != NULL; cp=cp->next)
   {
   if (strcmp(cp->scope,"global") != 0)
      {
      printf("\n# scope: bundle agent %s\n\n",cp->scope);
      PrintHashTable((char **)cp->hashtable);
      }
   }
}

/*******************************************************************/

void ListACLs()

{ struct CFACL *ptr;
  struct CFACE *ep;

printf("\nDEFINED ACCESS CONTROL BODIES\n\n");

for (ptr = VACLLIST; ptr != NULL; ptr=ptr->next)
   {
   printf("%s (type=%d,method=%c)\n",ptr->acl_alias,ptr->type,ptr->method);
   
   for (ep = ptr->aces; ep != NULL; ep=ep->next)
      {
      if (ep->name != NULL)
         {
         printf(" Type = %s, obj=%s, mode=%s (classes=%s)\n",ep->acltype,ep->name,ep->mode,ep->classes);
         }
      }
   CF_SPACER;
   }

}

/*********************************************************************/
/* Promises                                                          */
/*********************************************************************/

void ListDefinedInterfaces(char *classes)

{ struct Interface *ifp;

printf ("\nDEFINED INTERFACE PROMISES\n\n");
 
for (ifp = VIFLIST; ifp !=NULL; ifp=ifp->next)
   {
   if (!ShowClass(classes,ifp->classes))
      {
      continue;
      }

   InterfacePromise(ifp);
   CF_SPACER;
   }
}

/*********************************************************************/

void InterfacePromise(struct Interface *ifp)

{
printf("Interface %s promise if context is [%s]\n",ifp->ifdev,ifp->classes);
printf("  Constraint Body:\n");
printf("     Address ipv4=%s\n",ifp->ipaddress);
printf("     netmask=%s and broadcast=%s\n",ifp->netmask,ifp->broadcast);
}

/*********************************************************************/

void ListDefinedLinks(char *classes)

{ struct Link *ptr;

 if (VLINK)
    {
    printf ("\n##########################################################\n");
    }

for (ptr = VLINK; ptr != NULL; ptr=ptr->next)
   {
   LinkPromise(ptr,"be a link");
   }
}

/*********************************************************************/

void ListDefinedLinkchs(char *classes)

{ struct Link *ptr;
  struct Item *ip;

  if (VCHLINK)
     {
     printf ("\n##########################################################\n");
     }

for (ptr = VCHLINK; ptr != NULL; ptr=ptr->next)
   {
   LinkPromise(ptr,"link its children");
   }
}

/*********************************************************************/

void LinkPromise(struct Link *ptr, char *type)

{ struct Item *ip;
 
printf(" %s::\n\n",ptr->classes);
printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->to);

KMBody();

if (strstr(type,"children"))
   {
   printf("       link_from => linkchildren(\"%s\"),",ptr->from);
   }
else
   {
   printf("       link_from => ln_s(\"%s\"),",ptr->from);
   }

  // classes

if (strlen(ptr->defines) && strlen(ptr->elsedef))
   {
   printf("       classes => cf2_if_else(\"%s\",\"%s\"),\n",ptr->defines,ptr->elsedef);
   }
else if (strlen(ptr->defines))
   {
   printf("       classes => if_repaired(\"%s\"),\n",ptr->defines);
   }
else if (strlen(ptr->elsedef))
   {
   printf("       classes => if_notkept(\"%s\"),\n",ptr->elsedef);
   }

// action

if (ptr->ifelapsed != -1 || ptr->expireafter != -1)
   {
   printf("       action => if_elapsed(\"%d\");\n\n",ptr->ifelapsed);
   }
else
   {
   printf("       action => if_elapsed(\"%d\");\n\n",60);
   }
}

/*********************************************************************/

void PromiseItem(struct Item *ptr)
    
{
printf(" Item: \"%s\" is promised",ptr->name);
printf(" if context matches [%s]\n",ptr->classes);
printf("   ifelapsed %d, expireafter %d\n",ptr->ifelapsed,ptr->expireafter);
}

/*********************************************************************/

void ListDefinedResolvers(char *classes)

{ struct Item *ptr;

printf ("\nDEFINED RESOLVER CONFIGURATION PROMISES\n\n");

for (ptr = VRESOLVE; ptr != NULL; ptr=ptr->next)
   {
   if (!ShowClass(classes,ptr->classes))
      {
      continue;
      }

   PromiseItem(ptr);
   CF_SPACER;
   }
}

/*********************************************************************/

void ListDefinedAlerts(char *classes)

{ struct Item *ptr;
  int start = true;

if (VALERTS)
   {
   printf ("reports:\n\n");

   for (ptr = VALERTS; ptr != NULL; ptr=ptr->next)
      {
      if (strstr(ptr->name,"ShowState("))
         {
         *(ptr->name+strlen(ptr->name)-1) = '\0';
         printf("\n    showstate => { \"%s\" }",ptr->name+strlen("ShowState("));
         }
      else
         {
         if (!start)
            {
            printf (";\n\n");
            }
         
         if (ptr->classes && strcmp(ptr->classes,"any"))
            {
            printf(" %s::\n\n",ptr->classes);
            }
         else
            {
            printf(" %s::\n\n","any");
            }
                  
         printf("  \"%s\"",ptr->name);
         }

      start = false;
      }   
   }

printf (";\n\n");
}

/*********************************************************************/

void ListDefinedHomeservers(char *classes)

{ struct Item *ptr;

printf ("\nUse home servers = ( ");

for (ptr = VHOMESERVERS; ptr != NULL; ptr=ptr->next)
   {
   if (!ShowClass(classes,ptr->classes))
      {
      continue;
      }

   PromiseItem(ptr);
   CF_SPACER;
   }

printf(" )\n");
}

/*********************************************************************/

void ListDefinedImports()

{ struct Item *ptr;
  struct Audit *ap;
 
printf ("\nUSE IMPORTS\n\n");

for (ptr = VIMPORT; ptr != NULL; ptr=ptr->next)
   {
   PromiseItem(ptr);
   CF_SPACER;
   }

/* Report these inputs at audit */

for (ap = VAUDIT; ap != NULL; ap=ap->next)
   {
   if (ap->version)
      {
      printf("File %s %30s - version %s edited %s\n",ChecksumPrint('m',ap->digest),ap->filename, ap->version,ap->date);
      }
   else
      {
      printf("File %s %30s - (no version string) edited %s\n",ChecksumPrint('m',ap->digest),ap->filename,ap->date);
      }
   }
}

/*********************************************************************/

void ListDefinedIgnore(char *classes)

{ struct Item *ptr;

printf ("\n");

if (VIGNORE != NULL)
   {
   printf("  \"ignore_list\" slist => {\n");
   
   for (ptr = VIGNORE; ptr != NULL; ptr=ptr->next)
      {
      if (ptr->classes && strcmp(ptr->classes,"any"))
         {
         printf("                         \"%s\", # ifcontext %s\n",ptr->name,ptr->classes);
         }
      else
         {
         printf("                         \"%s\",\n",ptr->name);
         }
      }
   
   printf("                          };\n\n");

   printf("classes:\n");
   
   printf("   \"have_ignores\" expression => \"any\";\n");
   }
else
   {
   }
}

/*********************************************************************/

void ListDefinedMethods(char *classes)

{ struct Method *ptr;

 if (VMETHODS)
    {
    printf ("\n##########################################################\n");
    }
 
for (ptr = VMETHODS; ptr != NULL; ptr=ptr->next)
   {
   if (!ShowClass(classes,ptr->classes))
      {
      continue;
      }

   PromiseMethod(ptr);
   CF_SPACER;
   }
}

/*********************************************************************/

void PromiseMethod(struct Method *ptr)

{ struct Item *ip;
 int i,amserver;

amserver = (IsItemIn(ptr->servers,IPString2Hostname(VFQNAME)) ||
            IsItemIn(ptr->servers,IPString2UQHostname(VUQNAME)) ||
            IsItemIn(ptr->servers,VIPADDRESS));


printf("%s::\n\n     \"any\" usebundle => %s",ptr->classes,CanonifyName(ptr->file));

if (ptr->send_args)
   {
   printf("(");
   
   for (ip = ptr->send_args; ip != NULL; ip=ip->next)
      {
      printf("   Provide argument %d: %s\n",i++,ip->name);
      }

   printf("),\n");   
   }
else
   {
   printf(",\n");
   }

// action

if (ptr->ifelapsed != -1 || ptr->expireafter != -1)
   {
   printf("       action => if_elapsed(\"%d\");\n\n",ptr->ifelapsed);
   }
else
   {
   printf("       action => if_elapsed(\"%d\");\n\n",60);
   }
}

/*********************************************************************/

void ListDefinedScripts(char *classes)

{ struct ShellComm *ptr;

if (VSCRIPT)
   {
   printf ("\n##########################################################\n");
   }
 
printf("\ncommands:\n\n");

for (ptr = VSCRIPT; ptr != NULL; ptr=ptr->next)
   {
   PromiseShellCommand(ptr);
   }
}

/*********************************************************************/

void PromiseShellCommand(struct ShellComm *ptr)

{ int uid = false,root = false, dir = false;
 
printf(" %s::\n\n",ptr->classes);
printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->name);

  // classes

if (strlen(ptr->defines) && strlen(ptr->elsedef))
   {
   printf("       classes => cf2_if_else(\"%s\",\"%s\"),\n",ptr->defines,ptr->elsedef);
   }
else if (strlen(ptr->defines))
   {
   printf("       classes => if_repaired(\"%s\"),\n",ptr->defines);
   }
else if (strlen(ptr->elsedef))
   {
   printf("       classes => if_notkept(\"%s\"),\n",ptr->elsedef);
   }

KMBody();

if (strcmp(ptr->uid,"*") != 0)
   {
   uid = true;
   }

if (strlen(ptr->chdir) > 0)
   {
   dir = true;
   }

if (strlen(ptr->chroot) > 0)
   {
   root = true;
   }

if (uid && root && dir)
   {
   printf("       contain => jail(\"%s\",\"%s\",\"%s\"),\n",ptr->uid,ptr->chroot,ptr->chdir);
   }
else if (uid && root)
   {
   printf("       contain => jail(\"%s\",\"%s\",\"%s\"),\n",ptr->uid,ptr->chroot,"/");
   }
else if (uid && ptr->useshell == 'y')
   {
   printf("       contain => setuid_sh(\"%s\"),\n",ptr->uid);
   }
else  if (uid && ptr->useshell == 'n')
   {
   printf("       contain => setuid(\"%s\"),\n",ptr->uid);
   }

if (ptr->umask != 077 || ptr->fork == 'y' || ptr->timeout)
   {
   printf("      contain => custombody,\n\n");
   printf("      # body contain custombody\n      # {\n");
   
   if (ptr->umask)
      {
      printf("      # umask => \"%o\";\n",ptr->umask);
      }
   
   if (ptr->timeout)
      {
      printf("      # exec_timeout => \"%d\";\n",ptr->timeout);
      }
   
   if (ptr->fork == 'y')
      {
      printf("      # background => \"true\";\n",ptr->fork);
      }

   printf("      # }\n\n");
   }


// action

if (ptr->ifelapsed != -1 || ptr->expireafter != -1)
   {
   printf("       action => if_elapsed(\"%d\");\n\n",ptr->ifelapsed);
   }
else
   {
   printf("       action => if_elapsed(\"%d\");\n\n",60);
   }
}

/*********************************************************************/

void ListDefinedImages(char *classes)

{ struct Image *ptr;
  struct Item *svp;

if (VSERVERLIST)
   {
   printf ("\n##########################################################\n");
   }
  
for (svp = VSERVERLIST; svp != NULL; svp=svp->next) /* order servers */
   {
   for (ptr = VIMAGE; ptr != NULL; ptr=ptr->next)
      {
      if (strcmp(svp->name,ptr->server) != 0)  /* group together similar hosts so */
         {                                     /* can can do multiple transactions */
         continue;                             /* on one connection */
         } 
      
      PromiseFileCopy(ptr);
      }
   }
}

/*********************************************************************/

void PromiseFileCopy(struct Image *ptr)

{ struct Item *iip;
  struct Item *ip;
  char ignores[CF_BUFSIZE],excludes[CF_BUFSIZE],includes[CF_BUFSIZE];
  int defs = false;

ignores[0] = '\0';
includes[0] = '\0';
excludes[0] = '\0';
  
if (ptr->ignores)
   {
   printf("vars:\n\n");

   snprintf(ignores,CF_BUFSIZE,"ignore_%s",CanonifyName(ptr->destination));

   printf("   \"%s\" slist => { ",ignores);

   for (ip = ptr->ignores; ip != NULL; ip = ip->next)
      {
      printf(" \"%s\",",FixWildcards(ip->name));
      }

   printf("};\n");
   defs = true;
   }

if (defs)
   {
   printf("\n\nfiles:\n");
   }

printf(" %s::\n\n",ptr->classes);
printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->destination);

KMBody();

if ((strcmp(ptr->uid,"*") == 0) && (strcmp(ptr->gid,"*") == 0))
   {
   printf("       perms => m(\"%s\"),\n",ptr->thismode);
   }
else if (strcmp(ptr->uid,"*") == 0)
   {
   printf("       perms => mg(\"%s\",\"%s\"),\n",ptr->thismode,ptr->gid);
   }
else if (strcmp(ptr->uid,"*") == 0)
   {
   printf("       perms => mo(\"%s\",\"%s\"),\n",ptr->thismode,ptr->uid);
   }
else
   {
   printf("       perms => mog(\"%s\",\"%s\",\"%s\"),\n",ptr->thismode,ptr->uid,ptr->gid);
   }


// depth_search

if (ptr->recurse == CF_INF_RECURSE)
   {
   if (ignores[0] != '\0')
      {
      printf("       depth_search => recurse_ignore(\"inf\",\"@(%s)\"),\n",ignores);
      }
   else
      {
      printf("       depth_search => recurse(\"inf\"),\n");
      }
   }
else if (ptr->recurse > 0)
   {
   if (ignores[0] != '\0')
      {
      printf("       depth_search => recurse_ignore(\"%d\",\"@(%s)\"),\n",ptr->recurse,ignores);
      }
   else
      {
      printf("       depth_search => recurse(\"%d\"),\n",ptr->recurse);
      }
   }

// select_files

if (ptr->exclusions)
   {
   snprintf(excludes,CF_BUFSIZE,"excludes_%s",CanonifyName(ptr->destination));

   printf("       file_select => %s,\n\n",excludes);
   printf("   # INSTALL THIS\n");
   printf("   # body file_select %s\n",excludes);
   printf("   # {\n");
   printf("   # leaf_name => {");

   for (ip = ptr->exclusions; ip != NULL; ip = ip->next)       
      {
      printf(" \"%s\",",FixWildcards(ip->name));
      }
   
   printf(" };\n");
   printf("   # file_result => \"leaf_name\";\n");
   printf("   # }\n\n");
   }

if (ptr->inclusions)
   {
   snprintf(includes,CF_BUFSIZE,"includes_%s",CanonifyName(ptr->destination));

   if (ptr->exclusions)
      {
      printf("   # You'll need to integrate this with the above exclude somehow\n");
      }
   else
      {
      printf("       file_select => %s,\n\n",includes);
      printf("   # INSTALL THIS\n");
      }
   
   printf("   # body file_select %s\n",includes);
   printf("   # {\n");
   printf("   # leaf_name => {");

   for (ip = ptr->inclusions; ip != NULL; ip = ip->next)       
      {
      printf(" \"%s\",",FixWildcards(ip->name));
      }
   
   printf(" };\n");
   printf("   # files_result => \"leaf_name\";\n");
   printf("   # }\n\n");
   }


for (ip = ptr->filters; ip != NULL; ip=ip->next)
   {
   printf("     # filter %s needs integrating \n",ip->name);
   }

  // classes

if (strlen(ptr->defines) && strlen(ptr->elsedef))
   {
   printf("       classes => cf2_if_else(\"%s\",\"%s\"),\n",ptr->defines,ptr->elsedef);
   }
else if (strlen(ptr->defines))
   {
   printf("       classes => if_repaired(\"%s\"),\n",ptr->defines);
   }
else if (strlen(ptr->elsedef))
   {
   printf("       classes => if_notkept(\"%s\"),\n",ptr->elsedef);
   }


// copy_from

if (ptr->repository)
   {
   printf("       repository => \"%s\",\n",ptr->repository);
   }

if (ptr->backup == 'n')
   {
   if (ptr->encrypt == 'y')
      {
      printf("       copy_from => no_backup_scp(\"%s\",\"%s\"),",ptr->path,ptr->server);
      }
   else if (ptr->server)
      {
      printf("       copy_from => no_backup_rcp(\"%s\",\"%s\"),",ptr->path,ptr->server);
      }
   else
      {
      printf("       copy_from => no_backup_cp(\"%s\"),",ptr->path);
      }
   }
else
   {
   if (ptr->encrypt == 'y')
      {
      printf("       copy_from => secure_cp(\"%s\",\"%s\"),\n",ptr->path,ptr->server);
      }
   else if (ptr->server)
      {
      printf("       copy_from => remote_cp(\"%s\",\"%s\"),\n",ptr->path,ptr->server);
      }
   else
      {
      printf("       copy_from => local_cp(\"%s\"),\n",ptr->path);
      }
   }

printf("\n     # More accurate translation requires custom coding..\n");
printf("     # body copy_from custom_body\n");
printf("     # {\n");
printf("     # servers => { \"%s\"},\n",ptr->path);

if (ptr->trustkey == 'y')
   {
   printf("     # trustkey => \"true\";\n");
   }
else if (ptr->server)
   {
   printf("     # trustkey => \"false\";\n");
   }

switch (ptr->type)
   {
   case 'c':
       printf("     # compare => \"digest\";\n");
       break;

   case 't':
       printf("     # compare => \"ctime\";\n");
       break;
       
   default:
       printf("     # compare => \"mtime\";\n");
       break;
   }

if (ptr->purge == 'y')
   {
   printf("     # purge => \"true\";\n");
   }

if (ptr->size != CF_NOSIZE)
   {
   switch (ptr->comp)
      {
      case '<':
          printf("     # copy_size => irange(\"0\",\"%d\");\n",ptr->size);
          break;
          
      case '=':
          printf("     # copy_size => irange(\"%d\",\"%d\");\n",ptr->size);
          break;
          
      default:
          printf("     # copy_size => irange(\"%d\",\"inf\");\n",ptr->size);
          break;
      }
   }

if (ptr->failover && strlen(ptr->failover) > 0)
   {
   printf("      # failover signal \"%s\"\n",ptr->failover);
   }

if (ptr->stealth == 'y')
   {
   printf("     # stealth => \"true\";\n");
   }

if (ptr->checkroot == 'y')
   {
   printf("     # check_root => \"true\",\n");
   }

printf("     # }\n\n");

// move_obstructions

if (ptr->forcedirs == 'y')
   {
   printf("       move_obstructions => \"true\",\n");
   }

// action

if (ptr->ifelapsed != -1 || ptr->expireafter != -1)
   {
   printf("       action => if_elapsed(\"%d\");\n\n",ptr->ifelapsed);
   }
else
   {
   printf("       action => if_elapsed(\"60\");\n\n");
   }

}

/*********************************************************************/

void ListDefinedTidy(char *nothing)

{ struct Tidy *ptr;
  struct TidyPattern *tp;

if (VTIDY)
   {
   printf ("\n##########################################################\n");
   }
  
for (ptr = VTIDY; ptr != NULL; ptr=ptr->next)
   {
   int something = false;
   for (tp = ptr->tidylist; tp != NULL; tp=tp->next)
      {
      something = true;
      break;
      }
   
   if (!something)
      {
      continue;
      }
   
   PromiseTidy(ptr,nothing);
   }
}

/*********************************************************************/

void PromiseTidy(struct Tidy *ptr, char *nothing)

{ struct TidyPattern *tp;
 struct Item *ip,*alldefines = NULL,*allclasses = NULL;
  char ignores[CF_BUFSIZE],excludes[CF_BUFSIZE],includes[CF_BUFSIZE],pattern[CF_BUFSIZE];
  int defs = false;
  int maxdepth = 0;
  int minage = 0;
  int listlen = 0;
  int classes = 0;
  int defines = 0;
  int thissize = 0;
  
ignores[0] = '\0';
includes[0] = '\0';
excludes[0] = '\0';
  
if (ptr->ignores)
   {
   printf("vars:\n\n");

   snprintf(ignores,CF_BUFSIZE,"tidyignore_%s",CanonifyName(ptr->path));

   printf("   \"%s\" slist => { ",ignores);

   for (ip = ptr->ignores; ip != NULL; ip = ip->next)
      {
      printf(" \"%s\",",FixWildcards(ip->name));
      }

   printf("};\n");
   defs = true;
   }


// See how far recursion goes, etc
  
for (tp = ptr->tidylist; tp != NULL; tp=tp->next)
   {
   if (strcmp(tp->classes,"any") != 0)
      {
      classes++;
      if (!IsItemIn(allclasses,tp->classes))
         {
         PrependItem(&allclasses,tp->classes,NULL);
         }
      }

   if (strlen(tp->defines) > 0)
      {
      defines++;
      PrependItem(&alldefines,tp->defines,NULL);
      }

   if (tp->recurse > maxdepth)
      {
      maxdepth = tp->recurse;
      }

   if (tp->age < minage)
      {
      minage = tp->age;
      printf(" # WARNING: tidy expression has multiple ages for patterns (%s)\n",tp->pattern);
      printf(" #          might need manual intervention\n");
      }

   if (tp->travlinks != 'F')
      {
      printf("     # Traverse links=%c goes in depth_search for \"%s\"\n",tp->travlinks,tp->pattern);
      }

   for (ip = tp->filters; ip != NULL; ip=ip->next)
      {
      printf("     # filter %s needs integrating \n",ip->name);
      }

   listlen++;
   };

// Start

if (defs)
   {
   printf("\n\nfiles:  # tidy conversion\n");
   }

if (listlen == 1)
   {
   printf(" %s::\n\n",ptr->tidylist->classes);
      
   printf("   \"%s/%s\"   # tidy         # -> { \"optional_promisee_list\" },\n",ptr->path,FixWildcards(ptr->tidylist->pattern));

   KMBody();
   }
else
   {
   printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->path);

   snprintf(pattern,CF_BUFSIZE,"tidypatterns_%s",CanonifyName(ptr->path));

   KMBody();

   printf("       file_select => %s,\n\n",pattern);
   printf("   # INSTALL THIS\n");
   printf("   # body file_select %s\n",pattern);
   printf("   # {\n");
   
   for (ip = allclasses; ip != NULL; ip=ip->next)
      {
      printf("   # %s::\n",ip->name);
      printf("   #  leaf_name => { ",pattern);

      for (tp = ptr->tidylist; tp != NULL; tp=tp->next)
         {
         if (strcmp(ip->name,tp->classes) == 0)
            {
            printf("\"%s\",",FixWildcards(tp->pattern));
            }

         if (tp->size > thissize)
            {
            thissize = tp->size;
            }
         }

      printf(" };\n");
      
      if (thissize)
         {
         printf("   # search_size => irange(\"0\",\"%d\");\n",thissize);
         }
      }
   
   printf("   # any::\n   # file_result => \"leaf_name\";\n");
   printf("   # }\n\n");
   }

// age

printf("       delete => tidy,\n",minage);
printf("       file_select => days_old(\"%d\"),\n",minage);

// depth_search

if (maxdepth == CF_INFINITY)
   {
   if (ignores[0] != '\0')
      {
      printf("       depth_search => recurse_ignore(\"inf\",\"@(%s)\"),\n",ignores);
      }
   else
      {
      printf("       depth_search => recurse(\"inf\"),\n");
      }
   }
else if (maxdepth > 0)
   {
   if (ignores[0] != '\0')
      {
      printf("       depth_search => recurse_ignore(\"%d\",\"@(%s)\"),\n",maxdepth,ignores);
      }
   else
      {
      printf("       depth_search => recurse(\"%d\"),\n",maxdepth);
      }
   }



// select_files

if (ptr->exclusions)
   {
   snprintf(excludes,CF_BUFSIZE,"tidyexcludes_%s",CanonifyName(ptr->path));

   printf("       file_select => %s,\n\n",excludes);
   printf("   # INSTALL THIS\n");
   printf("   # body file_select %s\n",excludes);
   printf("   # {\n");
   printf("   # leaf_name => {");

   for (ip = ptr->exclusions; ip != NULL; ip = ip->next)       
      {
      printf(" \"%s\",",FixWildcards(ip->name));
      }
   
   printf(" };\n");
   printf("   # file_result => \"leaf_name\";\n");
   printf("   # }\n\n");
   }


  // classes

if (alldefines)
   {
   printf("       classes => if_repaired(\"");

   for (ip = alldefines; ip != NULL; ip=ip->next)
      {
      printf("%s,",ip->name);
      }

   printf("\"),\n");
   }


// action

if (ptr->ifelapsed != -1 || ptr->expireafter != -1)
   {
   printf("       action => if_elapsed(\"%d\");\n\n",ptr->ifelapsed);
   }
else
   {
   printf("       action => if_elapsed(\"%d\");\n\n",60);
   }

DeleteItemList(alldefines);
DeleteItemList(allclasses);
}

/*********************************************************************/

void ListDefinedMountables(char *classes)

{ struct Mountables *ptr;

printf ("\nPROMISED MOUNTABLES\n\n");

for (ptr = VMOUNTABLES; ptr != NULL; ptr=ptr->next)
   {
   if (!ShowClass(classes,ptr->classes))
      {
      continue;
      }

   PromiseMountable(ptr);
   CF_SPACER;
   }
}

/*********************************************************************/

void PromiseMountable(struct Mountables *ptr)

{
if (ptr == NULL)
   {
   return;
   }

if (ptr->classes == NULL)
   {
   printf("  Promise to use filesystem %s if context matches [any]\n",ptr->filesystem);
   }
else
   {
   printf("  Promise to use filesystem %s if context matches [%s]\n",ptr->filesystem,ptr->classes);
   }

if (ptr->readonly)
   {
   printf("  Using option ro\n");
   }
else
   {
   printf("  Using option rw\n");
   }

if (ptr->mountopts != NULL)
   {
   printf("  Using options %s\n", ptr->mountopts);
   }

printf("Rule from %s at/before line %d\n",ptr->audit->filename,ptr->lineno);
}

/*********************************************************************/

void ListMiscMounts(char *classes)

{ struct MiscMount *ptr;

if (VMISCMOUNT)
   {
   printf("\n#############################################################\n\n");
   }
 
printf("storage:\n\n");

for (ptr = VMISCMOUNT; ptr != NULL; ptr=ptr->next)
   {
   PromiseMiscMount(ptr);
   }
}

/*********************************************************************/

void PromiseMiscMount(struct MiscMount *ptr)

{ char server[CF_BUFSIZE],from[CF_BUFSIZE];

server[0] = '\0';
from[0] = '\0';

sscanf(ptr->from,"%[^:]:%s",server,from);

printf(" %s::\n\n",ptr->classes);
printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->onto);

KMBody();

if (ptr->options && strlen(ptr->options) > 0)
   {
   printf("       mount => nfs_p(\"%s\",\"%s\",\"%s\"),\n",server,from,ptr->options);
   }
else
   {
   printf("       mount => nfs(\"%s\",\"%s\"),\n",server,from);
   }

// action

if (ptr->ifelapsed != -1 || ptr->expireafter != -1)
   {
   printf("       action => if_elapsed(\"%d\");\n\n",ptr->ifelapsed);
   }
else
   {
   printf("       action => if_elapsed(\"%d\");\n\n",60);
   }


}

/*********************************************************************/

void ListDefinedRequired(char *classes)

{ struct Disk *ptr;

if (VREQUIRED)
   {
   printf ("\n##########################################################\n");
   }

printf("\nstorage:\n\n");

for (ptr = VREQUIRED; ptr != NULL; ptr=ptr->next)
   {
   DiskPromises(ptr);
   }
}

/*********************************************************************/

void DiskPromises(struct Disk *ptr)

{
printf(" %s::\n\n",ptr->classes);
printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->name);

KMBody();

if (ptr->force == 'n')
   {
   if ((ptr->freespace != -1))
      {
      if ((ptr->freespace < 0))
         {
         printf("       volume => min_free_space(\"%d\%\"),\n",ptr->freespace);
         }
      else
         {
         printf("       volume => min_free_space(\"%d\"),\n",ptr->freespace);
         }
      }
   }
else
   {
   if ((ptr->freespace != -1))
      {
      if ((ptr->freespace < 0))
         {
         printf("       volume => free_space(\"%d\%\"),\n",ptr->freespace);
         }
      else
         {
         printf("       volume => free_space(\"%d\"),\n",ptr->freespace);
         }
         
      printf("\n     # body volume frees_pace(free)\n");
      printf("     # {\n");
      printf("     # check_foreign  => \"true\";\n");
      printf("     # freespace      => \"$(free)\";\n");
      printf("     # }\n\n");
      }
   }
    
// classes

if (strlen(ptr->define) && strlen(ptr->elsedef))
   {
   printf("       classes => cf2_if_else(\"%s\",\"%s\"),\n",ptr->define,ptr->elsedef);
   }
else if (strlen(ptr->define))
   {
   printf("       classes => if_repaired(\"%s\"),\n",ptr->define);
   }
else if (strlen(ptr->elsedef))
   {
   printf("       classes => if_notkept(\"%s\"),\n",ptr->elsedef);
   }

// action

if (ptr->ifelapsed != -1 || ptr->expireafter != -1)
   {
   printf("       action => if_elapsed(\"%d\");\n\n",ptr->ifelapsed);
   }
else
   {
   printf("       action => if_elapsed(\"%d\");\n\n",60);
   }

}

/*********************************************************************/

void ListDefinedDisable(char *classes)

{ struct Disable *ptr;

if (VDISABLELIST)
   {
   printf ("\n##########################################################\n");
   }
 
for (ptr = VDISABLELIST; ptr != NULL; ptr=ptr->next)
   {
   PromiseDisable(ptr);
   }
}

/*********************************************************************/

void PromiseDisable(struct Disable *ptr)

{
printf(" %s::\n\n",ptr->classes);
printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->name);

KMBody();

if (strlen(ptr->destination) > 0)
   {
   printf("       rename => to(\"%s\"),",ptr->destination);
   }
else if (ptr->rotate != 0)
   {
   switch (ptr->rotate)
      {
      case CF_TRUNCATE:
          printf("       rename => rotate(\"0\"),\n");
          break;
      default:
          printf("       rename => rotate(\"%d\"),\n",ptr->rotate);
          break;
      }
   }
else
   {
   printf("       rename => disable,\n");
   }


// Selection

if (ptr->size > 0)
   {
   switch (ptr->comp)
      {
      case '<':
          printf("       file_select => size_range(\"0\",\"%d\"),\n",ptr->size);
          break;
          
      case '=':
          printf("       file_select => size_range(\"%d\",\"%d\"),\n",ptr->size,ptr->size);
          break;
          
      default:
          printf("       file_select => size_range(\"%d\",\"inf\"),\n",ptr->size);
          break;
      }
   }

// classes

if (strlen(ptr->defines) && strlen(ptr->elsedef))
   {
   printf("       classes => cf2_if_else(\"%s\",\"%s\"),\n",ptr->defines,ptr->elsedef);
   }
else if (strlen(ptr->defines))
   {
   printf("       classes => if_repaired(\"%s\"),\n",ptr->defines);
   }
else if (strlen(ptr->elsedef))
   {
   printf("       classes => if_notkept(\"%s\"),\n",ptr->elsedef);
   }

// action

if (ptr->ifelapsed != -1 || ptr->expireafter != -1)
   {
   printf("       action => if_elapsed(\"%d\");\n\n",ptr->ifelapsed);
   }
else
   {
   printf("       action => if_elapsed(\"%d\");\n\n",60);
   }
}

/*********************************************************************/

void ListDefinedMakePaths(char *classes)

{ struct File *ptr;

 if (VMAKEPATH)
    {
    printf ("\n##########################################################\n");
    }
 
for (ptr = VMAKEPATH; ptr != NULL; ptr=ptr->next)
   {
   if (!ShowClass(classes,ptr->classes))
      {
      continue;
      }

   PromiseFiles(ptr);
   }
}

/*********************************************************************/

void PromiseDirectories(struct File *ptr)

{ struct UidList *up;
  struct GidList *gp;
  struct Item *ip;
}

/*********************************************************************/

void ListFiles(char *classes)

{ struct File *ptr;

 if (VFILE)
    {
    printf ("\n##########################################################\n");
    }
 
for (ptr = VFILE; ptr != NULL; ptr=ptr->next)
   {
   PromiseFiles(ptr);
   printf("\n");
   }
}

/*********************************************************************/

void PromiseFiles(struct File *ptr)

{ struct Item *ip;
  char ignores[CF_BUFSIZE],excludes[CF_BUFSIZE],includes[CF_BUFSIZE];
  int defs = false;

ignores[0] = '\0';
includes[0] = '\0';
excludes[0] = '\0';
  
if (ptr->ignores)
   {
   printf("vars:\n\n");

   snprintf(ignores,CF_BUFSIZE,"ignore_%s",CanonifyName(ptr->path));

   printf("   \"%s\" slist => { ",ignores);

   for (ip = ptr->ignores; ip != NULL; ip = ip->next)
      {
      printf(" \"%s\",",FixWildcards(ip->name));
      }

   printf("};\n");
   defs = true;
   }

if (defs)
   {
   printf("\n\nfiles:\n");
   }

printf(" %s::\n\n",ptr->classes);
printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->path);

KMBody();

if (strlen(ptr->thismode) > 0)
   {
   if ((strcmp(ptr->uid,"*") == 0) && (strcmp(ptr->gid,"*") == 0))
      {
      printf("       perms => m(\"%s\")\n",ptr->thismode);
      }
   else if (strcmp(ptr->uid,"*") == 0)
      {
      printf("       perms => mg(\"%s\",\"%s\")\n",ptr->thismode,ptr->gid);
      }
   else if (strcmp(ptr->uid,"*") == 0)
      {
      printf("       perms => mo(\"%s\",\"%s\")\n",ptr->thismode,ptr->uid);
      }
   else
      {
      printf("       perms => mog(\"%s\",\"%s\",\"%s\")\n",ptr->thismode,ptr->uid,ptr->gid);
      }
   }
else
   {
   if ((strcmp(ptr->gid,"*") == 0) && (strcmp(ptr->uid,"*") != 0))
      {
      printf("       perms => owner(\"%s\")\n",ptr->uid);
      }
   else if (strcmp(ptr->gid,"*") != 0 && strcmp(ptr->uid,"*") != 0)
      {
      printf("       perms => og(\"%s\",\"%s\")\n",ptr->uid,ptr->gid);
      }
   }

if (ptr->ifelapsed != -1 || ptr->expireafter != -1)
   {
   printf("       action => if_elapsed(\"%d\"), # expireafter %d\n");
   }

// depth_search

if (ptr->recurse == CF_INF_RECURSE)
   {
   if (ignores[0] != '\0')
      {
      printf("       depth_search => recurse_ignore(\"inf\",\"@(%s)\"),\n",ignores);
      }
   else
      {
      printf("       depth_search => recurse(\"inf\"),\n");
      }
   }
else if (ptr->recurse > 0)
   {
   if (ignores[0] != '\0')
      {
      printf("       depth_search => recurse_ignore(\"%d\",\"@(%s)\"),\n",ptr->recurse,ignores);
      }
   else
      {
      printf("       depth_search => recurse(\"%d\"),\n",ptr->recurse);
      }
   }

if (ptr->travlinks != 'n')
   {
   printf("     # Traverse links=%c goes in depth_search\n",ptr->travlinks);
   }


// select_files

if (ptr->exclusions)
   {
   snprintf(excludes,CF_BUFSIZE,"excludes_%s",CanonifyName(ptr->path));

   printf("       file_select => %s,\n\n",excludes);
   printf("   # INSTALL THIS\n");
   printf("   # body file_select %s\n",excludes);
   printf("   # {\n");
   printf("   # leaf_name => {");

   for (ip = ptr->exclusions; ip != NULL; ip = ip->next)       
      {
      printf(" \"%s\",",FixWildcards(ip->name));
      }
   
   printf(" };\n");
   printf("   # file_result => \"leaf_name\";\n");
   printf("   # }\n\n");
   }

if (ptr->inclusions)
   {
   snprintf(includes,CF_BUFSIZE,"includes_%s",CanonifyName(ptr->path));

   if (ptr->exclusions)
      {
      printf("   # You'll need to integrate this with the above exclude somehow\n");
      }
   else
      {
      printf("       file_select => %s,\n\n",includes);
      printf("   # INSTALL THIS\n");
      }
   
   printf("   # body file_select %s\n",includes);
   printf("   # {\n");
   printf("   # leaf_name => {");

   for (ip = ptr->inclusions; ip != NULL; ip = ip->next)       
      {
      printf(" \"%s\",",FixWildcards(ip->name));
      }
   
   printf(" };\n");
   printf("   # files_result => \"leaf_name\";\n");
   printf("   # }\n\n");
   }


for (ip = ptr->filters; ip != NULL; ip=ip->next)
   {
   printf("     # filter %s needs integrating \n",ip->name);
   }

// changes

if (ptr->checksum != 'n')
   {
   printf("       changes => detect_content,\n",ptr->recurse);
   }


  // classes

if (strlen(ptr->defines) > 0 && strlen(ptr->elsedef) > 0)
   {
   printf("       classes => cf2_if_else(\"%s\",\"%s\"),\n",ptr->defines,ptr->elsedef);
   }
else if (strlen(ptr->defines) > 0)
   {
   printf("       classes => if_repaired(\"%s\"),\n",ptr->defines);
   }
else if (strlen(ptr->elsedef) > 0)
   {
   printf("       classes => if_notkept(\"%s\"),\n",ptr->elsedef);
   }

if (strstr(FILEACTIONTEXT[ptr->action],"warn"))
   {
   printf("       action => warn_only;\n");
   }
else
   {
   printf("       action => if_elapsed(\"60\");\n");
   }

}

/*******************************************************************/

void ListUnmounts(char *classes)

{ struct UnMount *ptr;

for (ptr=VUNMOUNT; ptr!=NULL; ptr=ptr->next)
   {
   PromiseUnmount(ptr);
   }
}

/*******************************************************************/

void PromiseUnmount(struct UnMount *ptr)
    
{
printf(" %s::\n\n",ptr->classes);
printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->name);

KMBody();

printf("       mount => unmount");

// action

if (ptr->ifelapsed != -1 || ptr->expireafter != -1)
   {
   printf("       action => if_elapsed(\"%d\");\n\n",ptr->ifelapsed);
   }
else
   {
   printf("       action => if_elapsed(\"%d\");\n\n",60);
   }

}

/*******************************************************************/

void ListProcesses(char *classes)

{ struct Process *ptr;

 if (VPROCLIST)
    {
    printf ("\n##########################################################\n");
    }
 
printf("\nprocesses:\n\n");

for (ptr = VPROCLIST; ptr != NULL; ptr=ptr->next)
   {
   PromiseProcess(ptr);
   }
}

/*******************************************************************/

void PromiseProcess(struct Process *ptr)
    
{ struct Item *ip; 
  char ignores[CF_BUFSIZE],excludes[CF_BUFSIZE],includes[CF_BUFSIZE],restart[CF_BUFSIZE];
  int defs = false;
  int uid = false, root = false, dir = false;


if (strstr(ptr->expr,"SetOptionString"))
   {
   return;
   }

if (strlen(ptr->expr) == 0)
   {
   return;
   }

ignores[0] = '\0';
includes[0] = '\0';
excludes[0] = '\0';

printf(" %s::\n\n",ptr->classes);
printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->expr);

KMBody();

if (ptr->matches != CF_NOSIZE)
   {
   switch (ptr->comp)
      {
      case '<':
          printf("       process_count => check_range(\"prefix\",\"0\",\"%d\")",ptr->matches);
          break;
          
      case '=':
          printf("       process_count => check_range(\"prefix\",\"%d\",\"%d\")",ptr->matches,ptr->matches);
          break;
          
      default:
          printf("       process_count => check_range(\"prefix\",\"%d\",\"inf\")",ptr->matches);
          break;
      }
   }


// select_process

if (ptr->exclusions)
   {
   snprintf(excludes,CF_BUFSIZE,"excludes_%s",CanonifyName(ptr->expr));

   printf("       process_select => %s,\n\n",excludes);
   printf("   # INSTALL THIS\n");
   printf("   # body process_select %s\n",excludes);
   printf("   # {\n");
   printf("   # command => {");

   for (ip = ptr->exclusions; ip != NULL; ip = ip->next)       
      {
      printf(" \"%s\",",FixWildcards(ip->name));
      }
   
   printf(" };\n");
   printf("   # process_result => \"command\";\n");
   printf("   # }\n\n");
   }

if (ptr->inclusions)
   {
   snprintf(includes,CF_BUFSIZE,"includes_%s",CanonifyName(ptr->expr));

   if (ptr->exclusions)
      {
      printf("   # You'll need to integrate this with the above exclude somehow\n");
      }
   else
      {
      printf("       process_select => %s,\n\n",includes);
      printf("   # INSTALL THIS\n");
      }
   
   printf("   # body process_select %s\n",includes);
   printf("   # {\n");
   printf("   # command => {");

   for (ip = ptr->inclusions; ip != NULL; ip = ip->next)       
      {
      printf(" \"%s\",",FixWildcards(ip->name));
      }
   
   printf(" };\n");
   printf("   # process_result => \"command\";\n");
   printf("   # }\n\n");
   }

for (ip = ptr->filters; ip != NULL; ip=ip->next)
   {
   printf("     # filter %s needs integrating \n",ip->name);
   }

if (ptr->signal > 0)
   {
   printf("       signals => { \"%s\" },\n",ToLowerStr(SIGNALS[ptr->signal]+3));
   }

// restart

if (ptr->restart && strlen(ptr->restart) != 0)
   {
   printf("       restart_class => \"restart_%s\",\n",CanonifyName(ptr->restart));
   }

// action

if (ptr->ifelapsed != -1 || ptr->expireafter != -1)
   {
   printf("       action => if_elapsed(\"%d\");\n\n",ptr->ifelapsed);
   }
else
   {
   printf("       action => if_elapsed(\"%d\");\n\n",60);
   }



// Restart
   
if (ptr->restart && strlen(ptr->restart) != 0)
   {
   printf("\ncommands:\n\n");   

   printf("restart_%s:: \n",CanonifyName(ptr->restart));   

   printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->restart);

  // classes

   if (strlen(ptr->defines) && strlen(ptr->elsedef))
      {
      printf("       classes => cf2_if_else(\"%s\",\"%s\"),\n",ptr->defines,ptr->elsedef);
      }
   else if (strlen(ptr->defines))
      {
      printf("       classes => if_repaired(\"%s\"),\n",ptr->defines);
      }
   else if (strlen(ptr->elsedef))
      {
      printf("       classes => if_notkept(\"%s\"),\n",ptr->elsedef);
      }
   
   KMBody();
   
   if (strcmp(ptr->uid,"*") != 0)
      {
      uid = true;
      }
   
   if (strlen(ptr->chdir) > 0)
      {
      dir = true;
      }
   
   if (strlen(ptr->chroot) > 0)
      {
      root = true;
      }
   
   if (uid && root && dir)
      {
      printf("       contain => jail(\"%s\",\"%s\",\"%s\"),\n",ptr->uid,ptr->chroot,ptr->chdir);
      }
   else if (uid && root)
      {
      printf("       contain => jail(\"%s\",\"%s\",\"%s\"),\n",ptr->uid,ptr->chroot,"/");
      }
   else if (uid && ptr->useshell == 'y')
      {
      printf("       contain => setuid_sh(\"%s\"),\n",ptr->uid);
      }
   else  if (uid && ptr->useshell == 'n')
      {
      printf("       contain => setuid(\"%s\"),\n",ptr->uid);
      }
   
   if (ptr->ifelapsed != -1 || ptr->expireafter != -1)
      {
      printf("       action => if_elapsed(\"%d\");\n\n",ptr->ifelapsed);
      }
   else
      {
      printf("       action => if_elapsed(\"%d\");\n\n",60);
      }
   
   
   printf("processes:\n\n");
   }

}

/*******************************************************************/

void ListFileEdits(char *classes)

{ struct Edit *ptr;
  struct Edlist *ep;

if (VEDITLIST)
   {
   printf ("\n##########################################################\n\n");
   }
  
for (ptr=VEDITLIST; ptr != NULL; ptr=ptr->next)
   {
   int something = false;
   for (ep = ptr->actions; ep != NULL; ep=ep->next)
      {
      something = true;
      }

   PromiseFileEdits(ptr,classes);
   }
}

/*******************************************************************/

void PromiseFileEdits(struct Edit *ptr,char *classes)

{ struct Edlist *ep;
  struct Item *ip;
  struct Item *insert = NULL, *delete = NULL, *replace = NULL, *column = NULL, *misc = NULL;
  int create = false, justappend = true, justcomment = true, simpleclasses = true;
  int count = 0;
  char class[CF_BUFSIZE],buf[CF_BUFSIZE],replacestr[CF_BUFSIZE],*defines = NULL,*elsedef = NULL;

class[0] = '\0';
replacestr[0] = '\0';
  
// Edit bundle ********************************************

// First scan and turn into lists.

for (ep = ptr->actions; ep != NULL; ep=ep->next)
   {
   if (strlen(class) > 0)
      {
      if (strcmp(class,ep->classes) != 0)
         {
         simpleclasses = false;
         }
      }
   else if (strlen(ep->classes) > 0)
      {
      strcpy(class,ep->classes);
      }
   
   if (strcmp(VEDITNAMES[ep->code],"AutoCreate") == 0)
      {
      create = true;
      continue;
      }

   count++;
   
   if (strcmp(VEDITNAMES[ep->code],"DeleteLinesMatching") == 0)
      {
      snprintf(buf,CF_BUFSIZE-1," \"%s\" ",ep->data);
      AppendItem(&delete,buf,ep->classes);
      justappend = false;
      justcomment = false;
      continue;
      }

   if (strcmp(VEDITNAMES[ep->code],"DeleteLinesContaining") == 0)
      {
      snprintf(buf,CF_BUFSIZE-1," \".*%s.*\" ",ep->data);
      AppendItem(&delete,buf,ep->classes);
      justappend = false;
      justcomment = false;
      continue;
      }

   if (strcmp(VEDITNAMES[ep->code],"DeleteLinesStarting") == 0)
      {
      snprintf(buf,CF_BUFSIZE-1," \"^%s.*\" ",ep->data);
      AppendItem(&delete,buf,ep->classes);
      justappend = false;
      justcomment = false;
      continue;
      }

   if (strcmp(VEDITNAMES[ep->code],"AppendIfNoSuchLine") == 0)
      {
      snprintf(buf,CF_BUFSIZE-1," \"%s\"",ep->data);
      AppendItem(&insert,buf,ep->classes);
      justcomment = false;
      continue;
      }

   if (strcmp(VEDITNAMES[ep->code],"ReplaceAll") == 0)
      {
      strcpy(replacestr,ep->data);
      justcomment = false;
      justappend = false;
      continue;
      }

   if (strcmp(VEDITNAMES[ep->code],"With") == 0)
      {
      snprintf(buf,CF_BUFSIZE-1," \"%s\" replace_with => value(\"%s\")",replacestr,ep->data);
      AppendItem(&replace,buf,ep->classes);
      justcomment = false;
      continue;
      }

   
   if (strcmp(VEDITNAMES[ep->code],"HashCommentLinesMatching") == 0)
      {
      snprintf(buf,CF_BUFSIZE-1," \"%s\" replace_with => comment(\"#\")",ep->data);
      AppendItem(&replace,buf,ep->classes);
      justappend = false;
      continue;
      }

   if (strcmp(VEDITNAMES[ep->code],"HashCommentLinesContaining") == 0)
      {
      snprintf(buf,CF_BUFSIZE-1," \".*%s.*\" replace_with => comment(\"#\")",ep->data);
      AppendItem(&replace,buf,ep->classes);
      justappend = false;
      justcomment = false;
      continue;
      }

   if (strcmp(VEDITNAMES[ep->code],"HashCommentLinesStarting") == 0)
      {
      snprintf(buf,CF_BUFSIZE-1," \"^%s.*\" replace_with => comment(\"#\")",ep->data);
      AppendItem(&replace,buf,ep->classes);
      justappend = false;
      justcomment = false;
      continue;
      }
   
   if (strcmp(VEDITNAMES[ep->code],"DefineClasses") == 0)
      {
      defines = ep->data;
      continue;
      }

   if (strcmp(VEDITNAMES[ep->code],"ElseDefineClasses") == 0)
      {
      elsedef = ep->data;
      continue;
      }

   if (strcmp(VEDITNAMES[ep->code],"InsertFile") == 0)
      {
      snprintf(buf,CF_BUFSIZE-1," \"%s\" \n#           insert_type => \"file\"",VEDITNAMES[ep->code],ep->classes);
      AppendItem(&insert,buf,ep->classes);
      justcomment = false;
      justappend = false;
      continue;
      }

   if (ep->data == NULL)
      {
      snprintf(buf,CF_BUFSIZE-1,"# fix-me  [%s]\n",VEDITNAMES[ep->code]);
      }
   else
      {
      snprintf(buf,CF_BUFSIZE-1,"# fix-me  [%s] -> \"%s\"\n",VEDITNAMES[ep->code],ep->data);
      }

   justcomment = false;
   justappend = false;
   AppendItem(&misc,buf,ep->classes);
   }

if (strlen(class) > 0)
   {
   strcpy(class,"any");
   }

// Now print format output

if (justappend && simpleclasses && count == 1)
   {
   printf(" %s::\n",class);
   
   printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->fname);
   KMBody();
   printf("       edit_line => append_if_no_lines(%s),\n",insert->name);
   }
else if (justcomment && simpleclasses && count == 1)
   {
   printf(" %s::\n",class);
   printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->fname);
   KMBody();
   printf("       edit_line => comment_lines(\"%s\"),\n",replace->name);
   }
else if (justappend && simpleclasses)
   {
   printf("vars:\n\n");

   printf(" \"myedit_%s\" slist => { ",CanonifyName(ptr->fname));

   for (ip = insert; ip != NULL; ip=ip->next)
      {
      printf("%s,",ip->name);
      }
   
   printf(" };\n\n");
   printf("files:\n\n %s::\n",class);
   printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->fname);
   KMBody();
   printf("       edit_line => append_if_no_lines(@(myedit_%s)),\n",CanonifyName(ptr->fname));
   }
else if (justcomment && simpleclasses)
   {
   printf("vars:\n\n");
   printf(" \"myedit_%s\" slist => { ",CanonifyName(ptr->fname));

   for (ip = replace; ip != NULL; ip=ip->next)
      {
      printf("%s,",ip->name);
      }
   
   printf(" };\n\n");
   
   printf("files:\n\n %s::\n",class);
   printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->fname);
   KMBody();
   printf("       edit_line => comment_lines(@(myedit_%s)),\n",CanonifyName(ptr->fname));
   }
else
   {
   printf("\n# bundle edit_line myedit_%s,\n",CanonifyName(ptr->fname));
   printf("# {\n");

   if (delete)
      {
      printf("# delete_lines:\n#\n");
      }
   
   for (ip = delete; ip != NULL; ip=ip->next)
      {
      printf("#   %s::\n#     %s;\n",ip->classes,ip->name);
      }
   
//
   
   if (insert)
      {
      printf("# insert_lines:\n#\n");
      }
   
   for (ip = insert; ip != NULL; ip=ip->next)
      {
      printf("#   %s::\n#     %s;\n",ip->classes,ip->name);
      }
   
//
   
   if (replace)
      {
      printf("# replace_patterns:\n#\n");
      }
   
   for (ip = replace; ip != NULL; ip=ip->next)
      {
      printf("#   %s::\n#     %s;\n",ip->classes,ip->name);
      }
   
//
   
   if (misc)
      {
      for (ip = misc; ip != NULL; ip=ip->next)
         {
         printf("# ## %s",ip->name);
         }
      }
   
   printf("# }\n\n");

   printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->fname);
   KMBody();
   printf("       edit_line => myedit_%s,\n",CanonifyName(ptr->fname));
   }

// promise *********************************************************
  

// Misc

if (ptr->repository)
   {
   printf("       repository => \"%s\",\n",ptr->repository);
   }

if (create)
   {
   printf("       create => \"true\",\n");
   }

  // classes

if (defines && elsedef)
   {
   printf("       classes => cf2_if_else(\"%s\",\"%s\"),\n",defines,elsedef);
   }
else if (defines)
   {
   printf("       classes => if_repaired(\"%s\"),\n",defines);
   }
else if (elsedef)
   {
   printf("       classes => if_notkept(\"%s\"),\n",elsedef);
   }

// action

if (ptr->ifelapsed != -1 || ptr->expireafter != -1)
   {
   printf("       action => if_elapsed(\"%d\");\n\n",ptr->ifelapsed);
   }
else
   {
   printf("       action => if_elapsed(\"%d\");\n\n",60);
   }


}

/*******************************************************************/

void ListFilters(char *classes)

{ struct Filter *ptr;
  int i;

printf("\nDEFINED FILTERS\n");

for (ptr=VFILTERLIST; ptr != NULL; ptr=ptr->next)
   {
   if (!ShowClass(classes,ptr->classes))
      {
      continue;
      }

   printf("Filter name %s :\n",ptr->alias);

   if (ptr->defines)
      {
      printf(" Defines: %s\n",ptr->defines);
      }

   if (ptr->elsedef)
      {
      printf(" ElseDefines: %s\n",ptr->elsedef);
      }
   
   for (i = 0; i < NoFilter; i++)
      {
      if (ptr->criteria[i] != NULL)
         {
         printf(" (%s) [%s]\n",VFILTERNAMES[i],ptr->criteria[i]);
         }
      }

   CF_SPACER;
   }
}


/*******************************************************************/

void ListDefinedPackages(char *classes)

{ struct Package ref, *ptr = NULL;
  int allsame = true;
 
if (VPKG == NULL)
   {
   return;
   }

ref.action = VPKG->action;
ref.cmp = VPKG->cmp;
ref.classes = VPKG->classes;

for (ptr = VPKG; ptr != NULL; ptr = ptr->next)
   {
   if (ref.action != ptr->action || ref.cmp != ptr->cmp || strcmp(ref.classes,ptr->classes) != 0)
      {
      allsame = false;
      }
   }
 
printf ("\n##########################################################\n");

if (allsame)
   {
   printf("\nvars:\n\n");
   
   for (ptr = VPKG; ptr != NULL; ptr = ptr->next)
      {
      printf("   \"pack[%s]\" string => \"%s\";\n",ptr->name,ptr->ver);
      }

   printf("   \"packages\" slist => getindices(\"v\");\n");

   printf("\npackages:\n\n");
      
   if (VPKG->ver)
      {
      printf("       package_version => \"$(pack[$(packages)])\",\n");
      printf("       package_select => \"%s\",\n",CMPSENSEOPERAND[VPKG->cmp]);
      }
   
   if (PKGMGRTXT[VPKG->pkgmgr])
      {
      printf("       package_method => \"%s\",\n",PKGMGRTXT[VPKG->pkgmgr]);
      }
   else
      {
      printf("       package_method => \"don\'t know\",\n");
      }
   
   // classes

   if (strlen(VPKG->defines) && strlen(VPKG->elsedef))
      {
      printf("       classes => cf2_if_else(\"%s\",\"%s\"),\n",VPKG->defines,VPKG->elsedef);
      }
   else if (strlen(VPKG->defines))
      {
      printf("       classes => if_repaired(\"%s\"),\n",VPKG->defines);
      }
   else if (strlen(VPKG->elsedef))
      {
      printf("       classes => if_notkept(\"%s\"),\n",VPKG->elsedef);
      }
   
// action
   
   if (VPKG->ifelapsed != -1 || VPKG->expireafter != -1)
      {
      printf("       action => if_elapsed(\"%d\");\n\n",VPKG->ifelapsed);
      }
   else
      {
      printf("       action => if_elapsed(\"%d\");\n\n",60);
      }
   }
else
   {
   printf("\npackages:\n\n");
   
   for (ptr = VPKG; ptr != NULL; ptr = ptr->next)
      {
      PromisePackages(ptr);
      }
   }
}

/*******************************************************************/

void PromisePackages(struct Package *ptr)

{ char name[CF_EXPANDSIZE];


printf(" %s::\n\n",ptr->classes);
printf("   \"%s\"            # -> { \"optional_promisee_list\" },\n",ptr->name);

KMBody();

printf("       package_policy => \"%s\", \n",PKGACTIONTEXT2[ptr->action]);

if (ptr->ver)
   {
   printf("       package_version => \"%s\",\n", ptr->ver);
   printf("       package_select => \"%s\",\n",CMPSENSEOPERAND[ptr->cmp]);
   }

if (PKGMGRTXT[ptr->pkgmgr])
   {
   printf("       package_method => \"%s\",\n",PKGMGRTXT[ptr->pkgmgr]);
   }
else
   {
   printf("       package_method => \"don\'t know\",\n");
   }

  // classes

if (strlen(ptr->defines) && strlen(ptr->elsedef))
   {
   printf("       classes => cf2_if_else(\"%s\",\"%s\"),\n",ptr->defines,ptr->elsedef);
   }
else if (strlen(ptr->defines))
   {
   printf("       classes => if_repaired(\"%s\"),\n",ptr->defines);
   }
else if (strlen(ptr->elsedef))
   {
   printf("       classes => if_notkept(\"%s\"),\n",ptr->elsedef);
   }

// action

if (ptr->ifelapsed != -1 || ptr->expireafter != -1)
   {
   printf("       action => if_elapsed(\"%d\");\n\n",ptr->ifelapsed);
   }
else
   {
   printf("       action => if_elapsed(\"%d\");\n\n",60);
   }
}


/*********************************************************************/

void KMBody()
{
printf("\n");
printf("       comment => \"...\",\n");
printf("     # handle => \"...\",\n");
printf("     # depends_on => { \"...\" ...},\n\n");
}

/*********************************************************************/

void ListServerAccess()

{ struct Auth *ptr,*ptr2;
 struct Item *ip;
 
printf("\naccess:\n");

for (ptr = VADMIT; ptr != NULL; ptr=ptr->next)
   {
   printf("\n%s::\n\n",ptr->classes);
   printf("  \"%s\"\n",ptr->path);

   if (ptr->encrypt)
      {
      printf("    ifencrypted => \"true\"\n");
      }

   KMBody();

   if (ptr && ptr->maproot)
      {
      printf("    maproot => { ");
      for (ip = ptr->maproot; ip !=NULL; ip=ip->next)
         {
         printf("\"%s\",",FixWildcards(ip->name));
         }

      printf("}\n");
      }

   for (ptr2 = VDENY; ptr2 != NULL; ptr2=ptr2->next)
      {
      if (ptr && ptr2 && strcmp(ptr->path,ptr2->path) == 0)
         {
         if (ptr2->accesslist)
            {
            printf("    deny => { ");
            for (ip = ptr->accesslist; ip != NULL; ip=ip->next)
               {
               printf("\"%s\",",FixWildcards(ip->name));
               }
            
            printf("},\n");
            }
         }
      }
   
   printf("    admit => { ");
   for (ip = ptr->accesslist; ip != NULL; ip=ip->next)
      {
      printf("\"%s\",",FixWildcards(ip->name));
      }
   
   printf("};\n");
   }
}

/*********************************************************************/

void ServerControl(struct Item *server)

{ struct Item *ip;

printf("body server control\n{\n");

for (ip = server; ip != NULL; ip=ip->next)
   {
   printf("%s",ip->name);
   }

if (NONATTACKERLIST)
   {
   printf("allowconnects => { ");
   
   for (ip = NONATTACKERLIST; ip != NULL; ip=ip->next)
      {
      printf(" \"%s\", ",ip->name);
      }

   printf("};\n");
   }

if (ATTACKERLIST)
   {
   printf("denyconnects => { ");
   
   for (ip = ATTACKERLIST; ip != NULL; ip=ip->next)
      {
      printf(" \"%s\", ",ip->name);
      }
   
   printf("};\n");
   }

if (MULTICONNLIST)
   {
   printf("allowallconnects => { ");
   
   for (ip = MULTICONNLIST; ip != NULL; ip=ip->next)
      {
      printf(" \"%s\", ",ip->name);
      }

   printf("};\n");
   }

if (TRUSTKEYLIST)
   {
   printf("trustkeysfrom => { ");
   
   for (ip = TRUSTKEYLIST; ip != NULL; ip=ip->next)
      {
      printf(" \"%s\", ",ip->name);
      }

   printf("};\n");
   }

if (SKIPVERIFY)
   {
   printf("skipverify => { ");
   
   for (ip = SKIPVERIFY; ip != NULL; ip=ip->next)
      {
      printf(" \"%s\", ",ip->name);
      }

   printf("};\n");
   }

if (DHCPLIST)
   {
   printf("dynamicaddresses => { ");
   
   for (ip = DHCPLIST; ip != NULL; ip=ip->next)
      {
      printf(" \"%s\", ",ip->name);
      }

   printf("};\n");
   }

printf("}\n\n");
}

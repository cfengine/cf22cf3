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


/*********************************************************************/
/*                                                                   */
/*  TOOLKIT: the "item" object library for cfengine                  */
/*                                                                   */
/*********************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"


/*********************************************************************/
/* TOOLKIT : Item list                                               */
/*********************************************************************/

int ListLen(struct Item *list)

{ int count = 0;
  struct Item *ip;

Debug("Check ListLen\n");
  
for (ip = list; ip != NULL; ip=ip->next)
   {
   count++;
   }

return count; 
}

/*********************************************************************/

struct Item *String2List(char *string)
    
{ struct Item *liststart = NULL;

AppendItem(&liststart,string,"");

return liststart;
}

/*********************************************************************/

int ByteSizeList(struct Item *list)

{ int count = 0;
  struct Item *ip;
 
for (ip = list; ip != NULL; ip=ip->next)
   {
   count+=strlen(ip->name);
   }

return count; 
}

/*********************************************************************/

int IsItemIn(struct Item *list,char *item)

{ struct Item *ptr; 

if ((item == NULL) || (strlen(item) == 0))
   {
   return true;
   }
 
for (ptr = list; ptr != NULL; ptr=ptr->next)
   {
   if (strcmp(ptr->name,item) == 0)
      {
      return(true);
      }
   }
 
return(false);
}

/*********************************************************************/

int GetItemListCounter(struct Item *list,char *item)

{ struct Item *ptr; 

if ((item == NULL) || (strlen(item) == 0))
   {
   return -1;
   }
 
for (ptr = list; ptr != NULL; ptr=ptr->next)
   {
   if (strcmp(ptr->name,item) == 0)
      {
      return ptr->counter;
      }
   }
 
 return -1;
}

/*********************************************************************/

void IncrementItemListCounter(struct Item *list,char *item)

{ struct Item *ptr; 

if ((item == NULL) || (strlen(item) == 0))
   {
   return;
   }
 
for (ptr = list; ptr != NULL; ptr=ptr->next)
   {
   if (strcmp(ptr->name,item) == 0)
      {
      ptr->counter++;
      return;
      }
   }
}

/*********************************************************************/

void SetItemListCounter(struct Item *list,char *item,int value)

{ struct Item *ptr; 

if ((item == NULL) || (strlen(item) == 0))
   {
   return;
   }
 
for (ptr = list; ptr != NULL; ptr=ptr->next)
   {
   if (strcmp(ptr->name,item) == 0)
      {
      ptr->counter = value;
      return;
      }
   }
}

/*********************************************************************/

int IsFuzzyItemIn(struct Item *list,char *item)

 /* This is for matching ranges of IP addresses, like CIDR e.g.

 Range1 = ( 128.39.89.250/24 )
 Range2 = ( 128.39.89.100-101 )
 
 */

{ struct Item *ptr; 

Debug("\nFuzzyItemIn(LIST,%s)\n",item);
 
if ((item == NULL) || (strlen(item) == 0))
   {
   return true;
   }
 
for (ptr = list; ptr != NULL; ptr=ptr->next)
   {
   Debug(" Try FuzzySetMatch(%s,%s)\n",ptr->name,item);
   
   if (FuzzySetMatch(ptr->name,item) == 0)
      {
      return(true);
      }
   }
 
return(false);
}

/*********************************************************************/

void CopyList (struct Item **dest, struct Item *source)

/* Copy or concat lists */
    
{ struct Item *ip;

if (*dest != NULL)
   {
   FatalError("CopyList - list not initialized");
   }
 
if (source == NULL)
   {
   return;
   }
 
for (ip = source; ip != NULL; ip = ip ->next)
   {
   AppendItem(dest,ip->name,ip->classes);
   }
}

/*********************************************************************/

struct Item *ConcatLists (struct Item *list1,struct Item *list2)

/* Notes: * Refrain from freeing list2 after using ConcatLists
          * list1 must have at least one element in it */

{ struct Item *endOfList1;

if (list1 == NULL)
   {
   FatalError("ConcatLists: first argument must have at least one element");
   }

for (endOfList1=list1; endOfList1->next!=NULL; endOfList1=endOfList1->next)
   {
   }

endOfList1->next = list2;
return list1;
}


/*********************************************************************/

void PrependLoL (struct LoL **liststart,char *name)

{ struct LoL *ip;

for (ip = *liststart; ip != NULL; ip=ip->next)
   {
   if (strcmp(ip->name,name) == 0)
      {
      // Already defined, idemp
      return;
      }
   }

if ((ip = (struct LoL *)malloc(sizeof(struct LoL))) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

ip->name = strdup(name);
ip->list = NULL;
ip->next = *liststart;
*liststart = ip;
}

/*********************************************************************/

void AddToLoL (struct LoL **liststart,char *name,char *itemstring)

{ struct LoL *ip;
  char *sp,*spe = NULL;
  
for (ip = *liststart; ip != NULL; ip=ip->next)
   {
   if (strcmp(ip->name,name) == 0)
      {
      PrependItem(&(ip->list),itemstring,NULL);
      return;
      }
   }
}

/*********************************************************************/

void PrependItem (struct Item **liststart,char *itemstring,char *classes)

{ struct Item *ip;
  char *sp,*spe = NULL;

if (!PARSING && (ACTION == editfiles))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Prepending [%s]\n",itemstring);
   CfLog(cfinform,OUTPUT,"");
   }
else
   {
   EditVerbose("Prepending [%s]\n",itemstring);
   }
 
if ((ip = (struct Item *)malloc(sizeof(struct Item))) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

if ((sp = malloc(strlen(itemstring)+2)) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

if ((classes != NULL) && (spe = malloc(strlen(classes)+2)) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

strcpy(sp,itemstring);
ip->name = sp;
ip->next = *liststart;
ip->counter = 0;
*liststart = ip;

if (classes != NULL)
   {
   strcpy(spe,classes);
   ip->classes = spe;
   }
else
   {
   ip->classes = NULL;
   }

NUMBEROFEDITS++;
}

/*********************************************************************/

void AppendItems (struct Item **liststart,char *itemstring,char *classes)

{ char *sp;
  char currentitem[CF_MAXVARSIZE],local[CF_MAXVARSIZE];
 
if ((itemstring == NULL) || strlen(itemstring) == 0)
   {
   return;
   }
 
memset(local,0,CF_MAXVARSIZE);
strncpy(local,itemstring,CF_MAXVARSIZE-1);
 
 /* split and iteratate across the list with space and comma sep */

for (sp = local ; *sp != '\0'; sp++)
   {
   /* find our separating character */
   memset(currentitem,0,CF_MAXVARSIZE);
   sscanf(sp,"%250[^ ,\n\t]",currentitem);
   
   if (strlen(currentitem) == 0)
      {
      continue;
      }
   /* add it to whatever list we're operating on */
   Debug("Appending %s\n",currentitem);
   AppendItem(liststart,currentitem,classes);
   
   /* and move up to the next list */
   sp += strlen(currentitem);
   }
}

/*********************************************************************/

void AppendItem (struct Item **liststart,char *itemstring,char *classes)

{ struct Item *ip, *lp;
  char *sp,*spe = NULL;

if (!PARSING && (ACTION == editfiles))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Appending [%s]\n",itemstring);
   CfLog(cfinform,OUTPUT,"");
   }
else
   {
   EditVerbose("Appending [%s]\n",itemstring);
   }

if ((ip = (struct Item *)malloc(sizeof(struct Item))) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

if ((sp = malloc(strlen(itemstring)+CF_EXTRASPC)) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

if (*liststart == NULL)
   {
   *liststart = ip;
   }
else
   {
   for (lp = *liststart; lp->next != NULL; lp=lp->next)
      {
      }

   lp->next = ip;
   }

if ((classes != NULL) && (spe = malloc(strlen(classes)+2)) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

strcpy(sp,itemstring);
ip->name = sp;
ip->next = NULL;
ip->counter = 0;
 
if (classes != NULL)
   {
   strcpy(spe,classes);
   ip->classes = spe;
   }
else
   {
   ip->classes = NULL;
   }

NUMBEROFEDITS++;
}


/*********************************************************************/

void InstallItem (struct Item **liststart,char *itemstring,char *classes,int ifelapsed,int expireafter)

{ struct Item *ip, *lp;
  char *sp,*spe = NULL;

if (!PARSING && (ACTION == editfiles))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Appending [%s]\n",itemstring);
   CfLog(cfinform,OUTPUT,"");
   }
else
   {
   EditVerbose("Appending [%s]\n",itemstring);
   }


if ((ip = (struct Item *)malloc(sizeof(struct Item))) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

if ((sp = malloc(strlen(itemstring)+CF_EXTRASPC)) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

if (*liststart == NULL)
   {
   *liststart = ip;
   }
else
   {
   for (lp = *liststart; lp->next != NULL; lp=lp->next)
      {
      }

   lp->next = ip;
   }

if ((classes!= NULL) && (spe = malloc(strlen(classes)+2)) == NULL)
   {
   CfLog(cferror,"","malloc");
   FatalError("");
   }

strcpy(sp,itemstring);

if (PIFELAPSED != -1)
   {
   ip->ifelapsed = PIFELAPSED;
   }
else
   {
   ip->ifelapsed = ifelapsed;
   }

if (PEXPIREAFTER != -1)
   {
   ip->expireafter = PEXPIREAFTER;
   }
else
   {
   ip->expireafter = expireafter;
   }
 
ip->name = sp;
ip->next = NULL;

if (classes != NULL)
   {
   strcpy(spe,classes);
   ip->classes = spe;
   }
else
   {
   ip->classes = NULL;
   }

ip->logaudit = AUDITP;
ip->audit = AUDITPTR;
ip->lineno = LINENUMBER;

NUMBEROFEDITS++;
}

/*********************************************************************/

void DeleteItemList(struct Item *item)                /* delete starting from item */
 
{
if (item != NULL)
   {
   DeleteItemList(item->next);
   item->next = NULL;

   if (item->name != NULL)
      {
      Debug("Unappending %s\n",item->name);
      free (item->name);
      }

   if (item->classes != NULL)
      {
      free (item->classes);
      }

   free((char *)item);
   }
}

/*********************************************************************/

void DeleteLoLList(struct LoL *item)
 
{
if (item != NULL)
   {
   DeleteLoLList(item->next);
   item->next = NULL;

   if (item->name != NULL)
      {
      Debug("Unappending %s\n",item->name);
      DeleteItemList(item->list);
      free (item->name);
      }

   free((char *)item);
   }
}

/*********************************************************************/

void DeleteItem(struct Item **liststart,struct Item *item)
 
{ struct Item *ip, *sp;

if (item != NULL)
   {
   EditVerbose("Delete Item: %s\n",item->name);

   if (item->name != NULL)
      {
      free(item->name);
      }

   if (item->classes != NULL)
      {
      free(item->classes);
      }

   sp = item->next;

   if (item == *liststart)
      {
      *liststart = sp;
      }
   else
      {
      for (ip = *liststart; ip->next != item; ip=ip->next)
         {
         }

      ip->next = sp;
      }

   free((char *)item);

   NUMBEROFEDITS++;
   }
}

/*********************************************************************/

void DebugListItemList(struct Item *liststart)

{ struct Item *ptr;

for (ptr = liststart; ptr != NULL; ptr=ptr->next)
   {
   printf("CFDEBUG: [%s]\n",ptr->name);
   }
}

/*********************************************************************/

int ItemListsEqual(struct Item *list1,struct Item *list2)

{ struct Item *ip1, *ip2;

ip1 = list1;
ip2 = list2;

while (true)
   {
   if ((ip1 == NULL) && (ip2 == NULL))
      {
      return true;
      }

   if ((ip1 == NULL) || (ip2 == NULL))
      {
      return false;
      }
   
   if (strcmp(ip1->name,ip2->name) != 0)
      {
      return false;
      }

   ip1 = ip1->next;
   ip2 = ip2->next;
   }
}

/*********************************************************************/
/* Fuzzy Match                                                       */
/*********************************************************************/

int FuzzyMatchParse(char *s)

{ char *sp;
  short isCIDR = false, isrange = false, isv6 = false, isv4 = false, isADDR = false; 
  char address[CF_ADDRSIZE];
  int mask,count = 0;

Debug("Check ParsingIPRange(%s)\n",s);

for (sp = s; *sp != '\0'; sp++)  /* Is this an address or hostname */
   {
   if (!isxdigit((int)*sp))
      {
      isADDR = false;
      break;
      }
   
   if (*sp == ':')              /* Catches any ipv6 address */
      {
      isADDR = true;
      break;
      }

   if (isdigit((int)*sp))      /* catch non-ipv4 address - no more than 3 digits */
      {
      count++;
      if (count > 3)
         {
         isADDR = false;
         break;
         }
      }
   else
      {
      count = 0;
      }
   }
 
if (! isADDR)
   {
   return true;
   }
 
if (strstr(s,"/") != 0)
   {
   isCIDR = true;
   }

if (strstr(s,"-") != 0)
   {
   isrange = true;
   }

if (strstr(s,".") != 0)
   {
   isv4 = true;
   }

if (strstr(s,":") != 0)
   {
   isv6 = true;
   }

if (isv4 && isv6)
   {
   yyerror("Mixture of IPv6 and IPv4 addresses");
   return false;
   }

if (isCIDR && isrange)
   {
   yyerror("Cannot mix CIDR notation with xx-yy range notation");
   return false;
   }

if (isv4 && isCIDR)
   {
   if (strlen(s) > 4+3*4+1+2)  /* xxx.yyy.zzz.mmm/cc */
      {
      yyerror("IPv4 address looks too long");
      return false;
      }
   
   address[0] = '\0';
   mask = 0;
   sscanf(s,"%16[^/]/%d",address,&mask);

   if (mask < 8)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Mask value %d in %s is less than 8",mask,s);
      yyerror(OUTPUT);
      return false;
      }

   if (mask > 30)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Mask value %d in %s is silly (> 30)",mask,s);
      yyerror(OUTPUT);
      return false;
      }
   }


if (isv4 && isrange)
   {
   long i, from = -1, to = -1;
   char *sp1,buffer1[CF_MAX_IP_LEN];
   
   sp1 = s;
   
   for (i = 0; i < 4; i++)
      {
      buffer1[0] = '\0';
      sscanf(sp1,"%[^.]",buffer1);
      sp1 += strlen(buffer1)+1;
      
      if (strstr(buffer1,"-"))
         {
         sscanf(buffer1,"%ld-%ld",&from,&to);
         
         if (from < 0 || to < 0)
            {
            yyerror("Error in IP range - looks like address, or bad hostname");
            return false;
            }
         
         if (to < from)
            {
            yyerror("Bad IP range");
            return false;
            }
         
         }
      }
   }
 
 if (isv6 && isCIDR)
    {
    char address[CF_ADDRSIZE];
    int mask,blocks;
    
    if (strlen(s) < 20)
       {
       yyerror("IPv6 address looks too short");
       return false;
       }
    
    if (strlen(s) > 42)
       {
       yyerror("IPv6 address looks too long");
       return false;
       }
    
    address[0] = '\0';
    mask = 0;
    sscanf(s,"%40[^/]/%d",address,&mask);
    blocks = mask/8;
    
    if (mask % 8 != 0)
       {
       CfLog(cferror,"Cannot handle ipv6 masks which are not 8 bit multiples (fix me)","");
       return false;
       }
    
    if (mask > 15)
       {
       yyerror("IPv6 CIDR mask is too large");
       return false;
       }
    }

return true; 
}

/*********************************************************************/

int FuzzySetMatch(char *s1,char *s2)

/* Match two IP strings - with : or . in hex or decimal
   s1 is the test string, and s2 is the reference e.g.
   FuzzySetMatch("128.39.74.10/23","128.39.75.56") == 0 */

{ short isCIDR = false, isrange = false, isv6 = false, isv4 = false;
  char address[CF_ADDRSIZE];
  int mask;
  unsigned long a1,a2;

if (strstr(s1,"/") != 0)
   {
   isCIDR = true;
   }

if (strstr(s1,"-") != 0)
   {
   isrange = true;
   }

if (strstr(s1,".") != 0)
   {
   isv4 = true;
   }

if (strstr(s1,":") != 0)
   {
   isv6 = true;
   }

if (strstr(s2,".") != 0)
   {
   isv4 = true;
   }

if (strstr(s2,":") != 0)
   {
   isv6 = true;
   }

if (isv4 && isv6)
   {
   /* This is just wrong */
   return -1;
   }

if (isCIDR && isrange)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Cannot mix CIDR notation with xxx-yyy range notation: %s",s1);
   CfLog(cferror,OUTPUT,"");
   return -1;
   }

if (!(isv6 || isv4))
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Not a valid address range - or not a fully qualified name: %s",s1);
   CfLog(cferror,OUTPUT,"");
   return -1;
   }

if (!(isrange||isCIDR)) 
   {
   return strncmp(s1,s2,strlen(s1)); /* do partial string match */
   }

 
if (isv4)
   {
   struct sockaddr_in addr1,addr2;
   int shift;

   memset(&addr1,0,sizeof(struct sockaddr_in));
   memset(&addr2,0,sizeof(struct sockaddr_in));
   
   if (isCIDR)
      {
      address[0] = '\0';
      mask = 0;
      sscanf(s1,"%16[^/]/%d",address,&mask);
      shift = 32 - mask;
      
      memcpy(&addr1,(struct sockaddr_in *) sockaddr_pton(AF_INET,address),sizeof(struct sockaddr_in));
      memcpy(&addr2,(struct sockaddr_in *) sockaddr_pton(AF_INET,s2),sizeof(struct sockaddr_in));

      a1 = htonl(addr1.sin_addr.s_addr);
      a2 = htonl(addr2.sin_addr.s_addr);
      
      a1 = a1 >> shift;
      a2 = a2 >> shift;
      
      if (a1 == a2)
         {
         return 0;
         }
      else
         {
         return -1;
         }
      }
   else
      {
      long i, from = -1, to = -1, cmp = -1;
      char *sp1,*sp2,buffer1[CF_MAX_IP_LEN],buffer2[CF_MAX_IP_LEN];
      
      sp1 = s1;
      sp2 = s2;
      
      for (i = 0; i < 4; i++)
         {
         if (sscanf(sp1,"%[^.]",buffer1) <= 0)
            {
            break;
            }
         sp1 += strlen(buffer1)+1;
         sscanf(sp2,"%[^.]",buffer2);
         sp2 += strlen(buffer2)+1;
         
         if (strstr(buffer1,"-"))
            {
            sscanf(buffer1,"%ld-%ld",&from,&to);
            sscanf(buffer2,"%ld",&cmp);
            
            if (from < 0 || to < 0)
               {
               Debug("Couldn't read range\n");
               return -1;
               }
            
            if ((from > cmp) || (cmp > to))
               {
               Debug("Out of range %d > %d > %d (range %s)\n",from,cmp,to,buffer2);
               return -1;
               }
            }
         else
            {
            sscanf(buffer1,"%ld",&from);
            sscanf(buffer2,"%ld",&cmp);
            
            if (from != cmp)
               {
               Debug("Unequal\n");
               return -1;
               }
            }
         
         Debug("Matched octet %s with %s\n",buffer1,buffer2);
         }
      
      Debug("Matched IP range\n");
      return 0;
      }
   }

#if defined(HAVE_GETADDRINFO)
if (isv6)
   {
   struct sockaddr_in6 addr1,addr2;
   int blocks, i;

   memset(&addr1,0,sizeof(struct sockaddr_in6));
   memset(&addr2,0,sizeof(struct sockaddr_in6));
   
   if (isCIDR)
      {
      address[0] = '\0';
      mask = 0;
      sscanf(s1,"%40[^/]/%d",address,&mask);
      blocks = mask/8;

      if (mask % 8 != 0)
         {
         CfLog(cferror,"Cannot handle ipv6 masks which are not 8 bit multiples (fix me)","");
         return -1;
         }
      
      memcpy(&addr1,(struct sockaddr_in6 *) sockaddr_pton(AF_INET6,address),sizeof(struct sockaddr_in6));
      memcpy(&addr2,(struct sockaddr_in6 *) sockaddr_pton(AF_INET6,s2),sizeof(struct sockaddr_in6));
      
      for (i = 0; i < blocks; i++) /* blocks < 16 */
         {
         if (addr1.sin6_addr.s6_addr[i] != addr2.sin6_addr.s6_addr[i])
            {
            return -1;
            }
         }
      return 0;
      }
   else
      {
      long i, from = -1, to = -1, cmp = -1;
      char *sp1,*sp2,buffer1[CF_MAX_IP_LEN],buffer2[CF_MAX_IP_LEN];

      sp1 = s1;
      sp2 = s2;
      
      for (i = 0; i < 8; i++)
         {
         sscanf(sp1,"%[^:]",buffer1);
         sp1 += strlen(buffer1)+1;
         sscanf(sp2,"%[^:]",buffer2);
         sp2 += strlen(buffer2)+1;
         
         if (strstr(buffer1,"-"))
            {
            sscanf(buffer1,"%lx-%lx",&from,&to);
            sscanf(buffer2,"%lx",&cmp);
            
            if (from < 0 || to < 0)
               {
               return -1;
               }
            
            if ((from >= cmp) || (cmp > to))
               {
               printf("%x < %x < %x\n",from,cmp,to);
               return -1;
               }
            }
         else
            {
            sscanf(buffer1,"%ld",&from);
            sscanf(buffer2,"%ld",&cmp);
            
            if (from != cmp)
               {
               return -1;
               }
            }
         }
      
      return 0;
      }
   }
#endif 

return -1; 
}

/*********************************************************************/
/* Host Match                                                        */
/* written by steve rader <rader@hep.wisc.edu>                       */
/*********************************************************************/

int FuzzyHostParse(char *arg1,char *arg2)

/*
 * do something like...
 *   if ( $s !~ /^.*?\,\d+\-\d+$/ ) { return false; }
 *   return true;
 */ 

{ struct Item *args;
  long start = -1, end = -1, where = -1;
  int n;

n = sscanf(arg2,"%ld-%ld%n",&start,&end,&where);

if ( n != 2 )
   {
   /* all other syntax errors */    
   yyerror("HostRange() syntax error: second arg should have X-Y format where X and Y are decimal numbers");
   return false;
   } 

return true; 
}

/*********************************************************************/

int FuzzyHostMatch(char *arg0, char* arg1, char *refhost)

/*
 * do something like...
 *   if ( $refhost !~ /(\d+)$/ ) {
 *     return 1; # failure: refhost doesn't end in numeral
 *   }
 *   $hostnum = $1;
 *   if ( $hostnum < $args[1] || $hostnum > $args[2] ) {
 *     return 1; # failure: refhost hostnum not in range
 *   }
 *   $refhost =~ s/^(.*)\d.*$/$1/;
 *   if ( $refhost ne $args[0] ) {
 *     return 1; # failure: hostname doesn't match basename
 *   }
 *   return 0; # success
 */

{ struct Item *args;
  char *sp, refbase[CF_MAXVARSIZE];
  long cmp = -1, start = -1, end = -1;
  char buf1[CF_BUFSIZE], buf2[CF_BUFSIZE];

strncpy(refbase,refhost,CF_MAXVARSIZE-1);
sp = refbase + strlen(refbase) - 1;
while ( isdigit((int)*sp) ) { sp--; }
sp++;
sscanf(sp,"%ld",&cmp);
*sp = '\0';
Debug("SRDEBUG FuzzyHostMatch: split refhost=%s into refbase=%s and cmp=%d\n",refhost,refbase,cmp);

if (cmp < 0)
   {
   return 1;
   }

if (strlen(refbase) == 0)
   {
   return 1;
   }

sscanf(arg1,"%ld-%ld",&start,&end);

if ( cmp < start || cmp > end )
   {
   Debug("SRDEBUG Failed on numberical range (%d < %d < %d)\n",end,cmp,start);
   return 1;
   }

strncpy(buf1,ToLowerStr(refbase),CF_BUFSIZE-1);
strncpy(buf2,ToLowerStr(arg0),CF_BUFSIZE-1);

if (strcmp(buf1,buf2) != 0)
   {
   Debug("SRDEBUG Failed on name (%s != %s)\n",buf1,buf2);
   return 1;
   }

return 0;
}

/*********************************************************************/
/* String Handling                                                   */
/*********************************************************************/

struct Item *SplitStringAsItemList(char *string,char sep)

 /* Splits a string containing a separator like : 
    into a linked list of separate items, */

{ struct Item *liststart = NULL;
  char format[9], *sp;
  char node[CF_MAXVARSIZE];
  
Debug("SplitStringAsItemList(%s,%c)\n",string,sep);

sprintf(format,"%%255[^%c]",sep);   /* set format string to search */

for (sp = string; *sp != '\0'; sp++)
   {
   memset(node,0,CF_MAXVARSIZE);
   sscanf(sp,format,node);

   if (strlen(node) == 0)
      {
      continue;
      }
   
   sp += strlen(node)-1;

   AppendItem(&liststart,node,NULL);

   if (*sp == '\0')
      {
      break;
      }
   }

return liststart;
}

/*******************************************************************/

/* Borrowed this algorithm from merge-sort implementation */

struct Item *SortItemListNames(struct Item *list) /* Alphabetical */

{ struct Item *p, *q, *e, *tail, *oldhead;
  int insize, nmerges, psize, qsize, i;

if (list == NULL)
   { 
   return NULL;
   }
 
 insize = 1;
 
 while (true)
    {
    p = list;
    oldhead = list;                /* only used for circular linkage */
    list = NULL;
    tail = NULL;
    
    nmerges = 0;  /* count number of merges we do in this pass */
    
    while (p)
       {
       nmerges++;  /* there exists a merge to be done */
       /* step `insize' places along from p */
       q = p;
       psize = 0;
       
       for (i = 0; i < insize; i++)
          {
          psize++;
          q = q->next;

          if (!q)
              {
              break;
              }
          }
       
       /* if q hasn't fallen off end, we have two lists to merge */
       qsize = insize;
       
       /* now we have two lists; merge them */
       while (psize > 0 || (qsize > 0 && q))
          {          
          /* decide whether next element of merge comes from p or q */
          if (psize == 0)
             {
             /* p is empty; e must come from q. */
             e = q; q = q->next; qsize--;
             }
          else if (qsize == 0 || !q)
             {
             /* q is empty; e must come from p. */
             e = p; p = p->next; psize--;
             }
          else if (strcmp(p->name, q->name) <= 0)
             {
             /* First element of p is lower (or same);
              * e must come from p. */
             e = p; p = p->next; psize--;
             }
          else
             {
             /* First element of q is lower; e must come from q. */
             e = q; q = q->next; qsize--;
             }
          
          /* add the next element to the merged list */
          if (tail)
             {
             tail->next = e;
             }
          else
             {
             list = e;
             }
          tail = e;
          }
       
       /* now p has stepped `insize' places along, and q has too */
       p = q;
       }
    tail->next = NULL;
    
    /* If we have done only one merge, we're finished. */

    if (nmerges <= 1)   /* allow for nmerges==0, the empty list case */
       {
       return list;
       }

    /* Otherwise repeat, merging lists twice the size */
    insize *= 2;
    }
}

/*******************************************************************/

/* Borrowed this algorithm from merge-sort implementation */

struct Item *SortItemListCounters(struct Item *list) /* Biggest first */

{ struct Item *p, *q, *e, *tail, *oldhead;
  int insize, nmerges, psize, qsize, i;

if (list == NULL)
   { 
   return NULL;
   }
 
 insize = 1;
 
 while (true)
    {
    p = list;
    oldhead = list;                /* only used for circular linkage */
    list = NULL;
    tail = NULL;
    
    nmerges = 0;  /* count number of merges we do in this pass */
    
    while (p)
       {
       nmerges++;  /* there exists a merge to be done */
       /* step `insize' places along from p */
       q = p;
       psize = 0;
       
       for (i = 0; i < insize; i++)
          {
          psize++;
          q = q->next;

          if (!q)
              {
              break;
              }
          }
       
       /* if q hasn't fallen off end, we have two lists to merge */
       qsize = insize;
       
       /* now we have two lists; merge them */
       while (psize > 0 || (qsize > 0 && q))
          {          
          /* decide whether next element of merge comes from p or q */
          if (psize == 0)
             {
             /* p is empty; e must come from q. */
             e = q; q = q->next; qsize--;
             }
          else if (qsize == 0 || !q)
             {
             /* q is empty; e must come from p. */
             e = p; p = p->next; psize--;
             }
          else if (p->counter > q->counter)
             {
             /* First element of p is higher (or same);
              * e must come from p. */
             e = p; p = p->next; psize--;
             }
          else
             {
             /* First element of q is lower; e must come from q. */
             e = q; q = q->next; qsize--;
             }
          
          /* add the next element to the merged list */
          if (tail)
             {
             tail->next = e;
             }
          else
             {
             list = e;
             }
          tail = e;
          }
       
       /* now p has stepped `insize' places along, and q has too */
       p = q;
       }
    tail->next = NULL;
    
    /* If we have done only one merge, we're finished. */

    if (nmerges <= 1)   /* allow for nmerges==0, the empty list case */
       {
       return list;
       }

    /* Otherwise repeat, merging lists twice the size */
    insize *= 2;
    }
}

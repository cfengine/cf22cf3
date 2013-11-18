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
/* File: ipname.c                                                            */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*****************************************************************************/

void StrCfenginePort()

{ struct servent *server;

if ((server = getservbyname(CFENGINE_SERVICE,"tcp")) == NULL)
   {
   CfLog(cflogonly,"Couldn't get cfengine service, using default","getservbyname");
   snprintf(STR_CFENGINEPORT,15,"5308");
   }
else
   {
   snprintf(STR_CFENGINEPORT,15,"%d",ntohs(server->s_port));
   }

Debug("Setting cfengine old port to %s\n",STR_CFENGINEPORT);
}

/*****************************************************************************/

char *Hostname2IPString(char *hostname)

{ static char ipbuffer[CF_SMALLBUF];
  int err;

#if defined(HAVE_GETADDRINFO)

 struct addrinfo query, *response, *ap;

 memset(&query,0,sizeof(struct addrinfo));   
 query.ai_family = AF_UNSPEC;
 query.ai_socktype = SOCK_STREAM;

 memset(ipbuffer,0,CF_SMALLBUF-1);
 
if ((err = getaddrinfo(hostname,NULL,&query,&response)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Unable to lookup hostname (%s) or cfengine service: %s",hostname,gai_strerror(err));
   CfLog(cferror,OUTPUT,"");
   return hostname;
   }
 
for (ap = response; ap != NULL; ap = ap->ai_next)
   {
   strncpy(ipbuffer,sockaddr_ntop(ap->ai_addr),64);
   Debug("Found address (%s) for host %s\n",ipbuffer,hostname);

   if (strlen(ipbuffer) == 0)
      {
      snprintf(ipbuffer,CF_SMALLBUF-1,"Empty IP result for %s",hostname);
      }
   freeaddrinfo(response);   
   return ipbuffer;
   }
#else
 struct hostent *hp;
 struct sockaddr_in cin;
 memset(&cin,0,sizeof(cin));

 memset(ipbuffer,0,CF_SMALLBUF-1);

if ((hp = gethostbyname(hostname)) != NULL)
   {
   cin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
   strncpy(ipbuffer,inet_ntoa(cin.sin_addr),CF_SMALLBUF-1);
   Verbose("Found address (%s) for host %s\n",ipbuffer,hostname);
   return ipbuffer;
   }
#endif

snprintf(ipbuffer,CF_SMALLBUF-1,"Unknown IP %s",hostname);
return ipbuffer;
}


/*****************************************************************************/

char *IPString2Hostname(char *ipaddress)

{ static char hostbuffer[MAXHOSTNAMELEN];
  int err;

#if defined(HAVE_GETADDRINFO)

  struct addrinfo query, *response, *ap;

memset(&query,0,sizeof(query));
memset(&response,0,sizeof(response));

query.ai_flags = AI_CANONNAME;

memset(hostbuffer,0,MAXHOSTNAMELEN);

if ((err = getaddrinfo(ipaddress,NULL,&query,&response)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Unable to lookup IP address (%s): %s",ipaddress,gai_strerror(err));
   CfLog(cflogonly,OUTPUT,"");
   snprintf(hostbuffer,MAXHOSTNAMELEN-1,"(Non registered IP)"); 
   return hostbuffer;
   }

for (ap = response; ap != NULL; ap = ap->ai_next)
   {   
   if ((err = getnameinfo(ap->ai_addr,ap->ai_addrlen,hostbuffer,MAXHOSTNAMELEN,0,0,0)) != 0)
      {
      snprintf(hostbuffer,MAXHOSTNAMELEN-1,"(Non registered IP)");
      freeaddrinfo(response);
      return hostbuffer;
      }
   
   Debug("Found address (%s) for host %s\n",hostbuffer,ipaddress);
   freeaddrinfo(response);
   return hostbuffer;
   }

 snprintf(hostbuffer,MAXHOSTNAMELEN-1,"(Non registered IP)");
 
#else

struct hostent *hp;
struct sockaddr_in myaddr;
struct in_addr iaddr;
  
memset(hostbuffer,0,MAXHOSTNAMELEN);

if ((iaddr.s_addr = inet_addr(ipaddress)) != -1)
   {
   hp = gethostbyaddr((void *)&iaddr,sizeof(struct sockaddr_in),AF_INET);
  
   if ((hp == NULL) || (hp->h_name == NULL))
      {
      strcpy(hostbuffer,"(Non registered IP)");
      return hostbuffer;
      }

   strncpy(hostbuffer,hp->h_name,MAXHOSTNAMELEN-1);
   }
else
   {
   strcpy(hostbuffer,"(non registered IP)");
   }

#endif

return hostbuffer;
}

/*****************************************************************************/

char *IPString2UQHostname(char *ipaddress)

/* Return an unqualified hostname */
    
{ static char hostbuffer[MAXHOSTNAMELEN];
  char *sp;

strcpy(hostbuffer,IPString2Hostname(ipaddress));

for (sp = hostbuffer; *sp != '\0'; sp++)
   {
   if (*sp == '.')
      {
      *sp = '\0';
      break;
      }
   }

return hostbuffer;
}

/* 

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
/* File Image copying                                              */
/*                                                                 */
/* client part for remote copying                                  */
/*                                                                 */
/*******************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*********************************************************************/

int BadProtoReply(char *buf)

{
 Debug("Protoreply: (%s)\n",buf);
 return (strncmp(buf,"BAD:",4) == 0);
}

/*********************************************************************/

int OKProtoReply(char *buf)

{
return(strncmp(buf,"OK:",3) == 0);
}

/*********************************************************************/

int FailedProtoReply(char *buf)

{
return(strncmp(buf,CF_FAILEDSTR,strlen(CF_FAILEDSTR)) == 0);
}

/*********************************************************************/

int IdentifyForVerification(int sd,char *localip,int family)

{ char sendbuff[CF_BUFSIZE],dnsname[CF_BUFSIZE];
  struct in_addr *iaddr;
  struct hostent *hp;
  int len,err;
  struct passwd *user_ptr;
  char *uname;
#if defined(HAVE_GETADDRINFO)
  char myaddr[256]; /* Compilation trick for systems that don't know ipv6 */
#else
  struct sockaddr_in myaddr;
#endif
  
memset(sendbuff,0,CF_BUFSIZE);
memset(dnsname,0,CF_BUFSIZE);

if (!SKIPIDENTIFY && (strcmp(VDOMAIN,CF_START_DOMAIN) == 0))
   {
   CfLog(cferror,"Undefined domain name","");
   return false;
   }

if (!SKIPIDENTIFY)
   {
/* First we need to find out the IP address and DNS name of the socket
   we are sending from. This is not necessarily the same as VFQNAME if
   the machine has a different uname from its IP name (!) This can
   happen on stupidly set up machines or on hosts with multiple
   interfaces, with different names on each interface ... */ 
   
   switch (family)
      {
      case AF_INET: len = sizeof(struct sockaddr_in);
          break;
#if defined(HAVE_GETADDRINFO)
      case AF_INET6: len = sizeof(struct sockaddr_in6);
          break;
#endif
      default:
          CfLog(cferror,"Software error in IdentifyForVerification","");
      }
   
   if (getsockname(sd,(struct sockaddr *)&myaddr,&len) == -1)
      {
      CfLog(cferror,"Couldn't get socket address\n","getsockname");
      return false;
      }
   
   snprintf(localip,CF_MAX_IP_LEN-1,"%s",sockaddr_ntop((struct sockaddr *)&myaddr)); 
   
   Debug("Identifying this agent as %s i.e. %s, with signature %d\n",localip,VFQNAME,CFSIGNATURE);
   
#if defined(HAVE_GETADDRINFO)
   
   if ((err=getnameinfo((struct sockaddr *)&myaddr,len,dnsname,CF_MAXVARSIZE,NULL,0,0)) != 0)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"Couldn't look up address v6 for %s: %s\n",dnsname,gai_strerror(err));
      CfLog(cferror,OUTPUT,"");
      return false;
      }
   
#else 
   
   iaddr = &(myaddr.sin_addr); 

   hp = gethostbyaddr((void *)iaddr,sizeof(myaddr.sin_addr),family);
   
   if ((hp == NULL) || (hp->h_name == NULL))
      {
      CfLog(cferror,"Couldn't lookup IP address\n","gethostbyaddr");
      return false;
      }
   
   strncpy(dnsname,hp->h_name,CF_MAXVARSIZE);
   
   if ((strstr(hp->h_name,".") == 0) && (strlen(VDOMAIN) > 0))
      {
      strcat(dnsname,".");
      strcat(dnsname,VDOMAIN);
      } 
#endif 
   }
else
   {
   strcpy(localip,VIPADDRESS);
   
   if (strlen(VFQNAME) > 0)
      {
      Verbose("SkipIdent was requested, so we are trusting and annoucning the identity as (%s) for this host\n",VFQNAME);
      strcat(dnsname,VFQNAME);
      }
   else
      {
      strcat(dnsname,"skipident");
      }
   }

user_ptr = getpwuid(getuid());
uname = user_ptr ? user_ptr->pw_name : "UNKNOWN";

/* Some resolvers will not return FQNAME and missing PTR will give numerical result */

if ((strlen(VDOMAIN) > 0) && !IsIPV6Address(dnsname) && !strchr(dnsname,'.'))
   {
   Debug("Appending domain %s to %s\n",VDOMAIN,dnsname);
   strcat(dnsname,".");
   strncat(dnsname,VDOMAIN,CF_MAXVARSIZE/2);
   }  

if (strncmp(dnsname,localip,strlen(localip)) == 0)
   {
   /* Seems to be a bug in some resolvers that adds garbage, when it just returns the input */
   strcpy(dnsname,localip);
   }

if (strlen(dnsname) == 0)
   {
   strcpy(dnsname,localip);
   }

snprintf(sendbuff,CF_BUFSIZE-1,"CAUTH %s %s %s %d",localip,dnsname,uname,CFSIGNATURE);

Debug("SENT:::%s\n",sendbuff);
 
SendTransaction(sd,sendbuff,0,CF_DONE);
return true;
}

/*********************************************************************/

int KeyAuthentication(struct Image *ip)

{ char sendbuffer[CF_EXPANDSIZE],in[CF_BUFSIZE],*out,*decrypted_cchall;
 BIGNUM *nonce_challenge, *bn = NULL;
 unsigned long err;
 unsigned char digest[EVP_MAX_MD_SIZE];
 int encrypted_len,nonce_len = 0,len;
 char cant_trust_server, keyname[CF_BUFSIZE];
 RSA *server_pubkey = NULL;

if (COMPATIBILITY_MODE)
   {
   return true;
   }

if (PUBKEY == NULL || PRIVKEY == NULL) 
   {
   CfLog(cferror,"No public/private key pair found\n","");
   return false;
   }


/* Generate a random challenge to authenticate the server */
 
nonce_challenge = BN_new();
BN_rand(nonce_challenge,CF_NONCELEN,0,0);

nonce_len = BN_bn2mpi(nonce_challenge,in);
ChecksumString(in,nonce_len,digest,'m');

/* We assume that the server bound to the remote socket is the official one i.e. = root's */

if (OptionIs(CONTEXTID,"HostnameKeys",true))
   {
   snprintf(keyname,CF_BUFSIZE,"root-%s",ip->server); 
   Debug("KeyAuthentication(with hostname key %s)\n",keyname);
   }
else
   {
   snprintf(keyname,CF_BUFSIZE,"root-%s",CONN->remoteip); 
   Debug("KeyAuthentication(with IP keyname %s)\n",keyname);
   }

if (server_pubkey = HavePublicKey(keyname))
   {
   cant_trust_server = 'y';
   /* encrypted_len = BN_num_bytes(server_pubkey->n);*/   
   encrypted_len = RSA_size(server_pubkey);
   }
else 
   {
   cant_trust_server = 'n';                      /* have to trust server, since we can't verify id */
   encrypted_len = nonce_len;
   }

snprintf(sendbuffer,CF_BUFSIZE,"SAUTH %c %d %d",cant_trust_server,encrypted_len,nonce_len);
 
if ((out = malloc(encrypted_len)) == NULL)
   {
   FatalError("memory failure");
   }

if (server_pubkey != NULL)
   {
   if (RSA_public_encrypt(nonce_len,in,out,server_pubkey,RSA_PKCS1_PADDING) <= 0)
      {
      err = ERR_get_error();
      snprintf(OUTPUT,CF_BUFSIZE,"Public encryption failed = %s\n",ERR_reason_error_string(err));
      CfLog(cferror,OUTPUT,"");
      free(out);
      return false;
      }
   
   memcpy(sendbuffer+CF_RSA_PROTO_OFFSET,out,encrypted_len); 
   }
else
   {
   memcpy(sendbuffer+CF_RSA_PROTO_OFFSET,in,nonce_len); 
   }

/* proposition C1 - Send challenge / nonce */
 
SendTransaction(CONN->sd,sendbuffer,CF_RSA_PROTO_OFFSET+encrypted_len,CF_DONE);

BN_free(bn);
BN_free(nonce_challenge);
free(out);

if (DEBUG||D2)
   {
   RSA_print_fp(stdout,PUBKEY,0);
   }

/*Send the public key - we don't know if server has it */ 
/* proposition C2 */

memset(sendbuffer,0,CF_EXPANDSIZE); 
len = BN_bn2mpi(PUBKEY->n,sendbuffer); 
SendTransaction(CONN->sd,sendbuffer,len,CF_DONE); /* No need to encrypt the public key ... */

/* proposition C3 */ 
memset(sendbuffer,0,CF_EXPANDSIZE);   
len = BN_bn2mpi(PUBKEY->e,sendbuffer); 
SendTransaction(CONN->sd,sendbuffer,len,CF_DONE);

/* check reply about public key - server can break connection here */

/* proposition S1 */  
memset(in,0,CF_BUFSIZE);  

if (ReceiveTransaction(CONN->sd,in,NULL) == -1)
   {
   CfLog(cferror,"Protocol transaction broken off",NULL);
   return false;
   }

if (BadProtoReply(in))
   {
   CfLog(cferror,in,"");
   return false;
   }

/* Get challenge response - should be md5 of challenge */

/* proposition S2 */   
memset(in,0,CF_BUFSIZE);  

if (ReceiveTransaction(CONN->sd,in,NULL) == -1)
   {
   CfLog(cferror,"Protocol transaction broken off",NULL);
   return false;   
   }

if (!ChecksumsMatch(digest,in,'m')) 
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Challenge response from server %s/%s was incorrect!",ip->server,CONN->remoteip);
   CfLog(cferror,OUTPUT,"");
   return false;
   }
else
   {
   char server[CF_EXPANDSIZE];
   ExpandVarstring(ip->server,server,NULL);
   
   if (cant_trust_server == 'y')  /* challenge reply was correct */ 
      {
      Verbose("\n...............................................................\n");
      snprintf(OUTPUT,CF_BUFSIZE,"Strong authentication of server=%s connection confirmed\n",server);
      CfLog(cfverbose,OUTPUT,"");
      }
   else
      {
      if (ip->trustkey == 'y')
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Trusting server identity and willing to accept key from %s=%s",server,CONN->remoteip);
         CfLog(cferror,OUTPUT,"");
         }
      else
         {
         snprintf(OUTPUT,CF_BUFSIZE,"Not authorized to trust the server=%s's public key (trustkey=false)\n",server);
         CfLog(cferror,OUTPUT,"");
         return false;
         }
      }
   }

/* Receive counter challenge from server */ 

Debug("Receive counter challenge from server\n");  
/* proposition S3 */   
memset(in,0,CF_BUFSIZE);  
encrypted_len = ReceiveTransaction(CONN->sd,in,NULL);

if (encrypted_len < 0)
   {
   CfLog(cferror,"Protocol transaction sent illegal cipher length",NULL);
   return false;      
   }

if ((decrypted_cchall = malloc(encrypted_len)) == NULL)
   {
   FatalError("memory failure");
   }
 
if (RSA_private_decrypt(encrypted_len,in,decrypted_cchall,PRIVKEY,RSA_PKCS1_PADDING) <= 0)
   {
   err = ERR_get_error();
   snprintf(OUTPUT,CF_BUFSIZE,"Private decrypt failed = %s, abandoning\n",ERR_reason_error_string(err));
   CfLog(cferror,OUTPUT,"");
   return false;
   }

/* proposition C4 */   
ChecksumString(decrypted_cchall,nonce_len,digest,'m');
Debug("Replying to counter challenge with md5\n"); 
SendTransaction(CONN->sd,digest,16,CF_DONE);
free(decrypted_cchall); 

/* If we don't have the server's public key, it will be sent */


if (server_pubkey == NULL)
   {
   RSA *newkey = RSA_new();

   Debug("Collecting public key from server!\n"); 

   /* proposition S4 - conditional */  
   if ((len = ReceiveTransaction(CONN->sd,in,NULL)) <= 0)
      {
      CfLog(cferror,"Protocol error in RSA authentation from IP %s\n",ip->server);
      return false;
      }
   
   if ((newkey->n = BN_mpi2bn(in,len,NULL)) == NULL)
      {
      err = ERR_get_error();
      snprintf(OUTPUT,CF_BUFSIZE,"Private decrypt failed = %s\n",ERR_reason_error_string(err));
      CfLog(cferror,OUTPUT,"");
      RSA_free(newkey);
      return false;
      }

   /* proposition S5 - conditional */  
   if ((len=ReceiveTransaction(CONN->sd,in,NULL)) == 0)
      {
      CfLog(cfinform,"Protocol error in RSA authentation from IP %s\n",ip->server);
      RSA_free(newkey);
      return false;
      }
   
   if ((newkey->e = BN_mpi2bn(in,len,NULL)) == NULL)
      {
      err = ERR_get_error();
      snprintf(OUTPUT,CF_BUFSIZE,"Private decrypt failed = %s\n",ERR_reason_error_string(err));
      CfLog(cferror,OUTPUT,"");
      RSA_free(newkey);
      return false;
      }

   SavePublicKey(keyname,newkey);
   server_pubkey = RSAPublicKey_dup(newkey);
   RSA_free(newkey);
   }
 
/* proposition C5 */

GenerateRandomSessionKey();

DebugBinOut(CONN->session_key,CF_BLOWFISHSIZE);

if (CONN->session_key == NULL)
   {
   CfLog(cferror,"A random session key could not be established","");
   return false;
   }
else
   {
   Debug("Generated session key\n");
   DebugBinOut(CONN->session_key,CF_BLOWFISHSIZE);
   }

/* blowfishmpisize = BN_bn2mpi((BIGNUM *)CONN->session_key,in); */

DebugBinOut(CONN->session_key,CF_BLOWFISHSIZE);

encrypted_len = RSA_size(server_pubkey);

Debug("Encrypt %d to %d\n",CF_BLOWFISHSIZE,encrypted_len);

if ((out = malloc(encrypted_len)) == NULL)
   {
   FatalError("memory failure");
   }

if (RSA_public_encrypt(CF_BLOWFISHSIZE,CONN->session_key,out,server_pubkey,RSA_PKCS1_PADDING) <= 0)
   {
   err = ERR_get_error();
   snprintf(OUTPUT,CF_BUFSIZE,"Public encryption failed = %s\n",ERR_reason_error_string(err));
   CfLog(cferror,OUTPUT,"");
   free(out);
   return false;
   }

Debug("Encryption succeeded\n");

SendTransaction(CONN->sd,out,encrypted_len,CF_DONE);
DebugBinOut(out,encrypted_len);

if (server_pubkey != NULL)
   {
   RSA_free(server_pubkey);
   }

free(out);
return true; 
}

/*********************************************************************/

void CheckRemoteVersion()

{ char sendbuffer[CF_BUFSIZE];
  char recvbuffer[CF_BUFSIZE];
  int tosend;

Debug("CheckRemoteVersion\n");  
snprintf(sendbuffer,CF_BUFSIZE,"VERSION");
tosend = strlen(sendbuffer);

if (SendTransaction(CONN->sd,sendbuffer,tosend,CF_DONE) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Transmission failed while checking version");
   CfLog(cfinform,OUTPUT,"send");
   return;
   }

if (ReceiveTransaction(CONN->sd,recvbuffer,NULL) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Reply failed while checking version");
   CfLog(cfinform,OUTPUT,"send");
   CONN->protoversion = 0;
   return;
   }

if (BadProtoReply(recvbuffer))
   {
   CONN->protoversion = 0;
   return;
   }
else
   {
   CONN->protoversion = 1;
   return;
   }
}

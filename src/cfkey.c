/* 

        Copyright (C) 1999
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
/* File: cfkey.c                                                             */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

char CFLOCK[CF_BUFSIZE];
char USERKEYFILE[CF_BUFSIZE];

void Initialize(void);
int RecursiveTidySpecialArea(char *name, struct Tidy *tp, int maxrecurse, struct stat *sb);
void Syntax(void);
void CheckOpts(int argc, char **argv);

/*******************************************************************/
/* Command line options                                            */
/*******************************************************************/

  /* GNU STUFF FOR LATER #include "getopt.h" */
 
 struct option OPTIONS[5] =
      {
      { "help",no_argument,0,'h' },
      { "file",required_argument,0,'f' },
      { "version",no_argument,0,'V' },
      { "quiet",no_argument,0,'w' },
      { NULL,0,0,0 }
      };


int main(int argc, char** argv)

{ unsigned long err;
  RSA *pair;
  FILE *fp;
  struct stat statbuf;
  int fd;
  static char *passphrase = "Cfengine passphrase";
  EVP_CIPHER *cipher = EVP_des_ede3_cbc();

CheckOpts(argc, argv);

Initialize();
           
if (stat(CFPRIVKEYFILE,&statbuf) != -1)
   {
   printf("A key file already exists at %s.\n",CFPRIVKEYFILE);
   return 1;
   }

if (stat(CFPUBKEYFILE,&statbuf) != -1)
   {
   printf("A key file already exists at %s.\n",CFPUBKEYFILE);
   return 1;
   }

 if(VERBOSE) 
   {
   printf("Making a key pair for cfengine, please wait, this could take a minute...\n"); 
   }

pair = RSA_generate_key(2048,35,NULL,NULL);

if (pair == NULL)
   {
   err = ERR_get_error();
   printf("Error = %s\n",ERR_reason_error_string(err));
   return 1;
   }

if (VERBOSE)
   {
   RSA_print_fp(stdout,pair,0);
   }
 
fd = open(CFPRIVKEYFILE,O_WRONLY | O_CREAT | O_TRUNC, 0600);

if (fd < 0)
   {
   printf("Ppen %s failed: %s.",CFPRIVKEYFILE,strerror(errno));
   return 1;
   }
 
if ((fp = fdopen(fd, "w")) == NULL )
   {
   printf("fdopen %s failed: %s.",CFPRIVKEYFILE, strerror(errno));
   close(fd);
   return 1;
   }

if (VERBOSE)
   {
printf("Writing private key to %s\n",CFPRIVKEYFILE);
   }
 
if (!PEM_write_RSAPrivateKey(fp,pair,cipher,passphrase,strlen(passphrase),NULL,NULL))
    {
    err = ERR_get_error();
    printf("Error = %s\n",ERR_reason_error_string(err));
    return 1;
    }
 
fclose(fp);
 
fd = open(CFPUBKEYFILE,O_WRONLY | O_CREAT | O_TRUNC, 0600);

if (fd < 0)
   {
   printf("open %s failed: %s.",CFPUBKEYFILE,strerror(errno));
   return 1;
   }
 
if ((fp = fdopen(fd, "w")) == NULL )
   {
   printf("fdopen %s failed: %s.",CFPUBKEYFILE, strerror(errno));
   close(fd);
   return 1;
   }

if (VERBOSE)
   {
   printf("Writing public key to %s\n",CFPUBKEYFILE);
   }
 
if(!PEM_write_RSAPublicKey(fp,pair))
   {
   err = ERR_get_error();
   printf("Error = %s\n",ERR_reason_error_string(err));
   return 1;
   }

fclose(fp);

 
snprintf(VBUFF,CF_BUFSIZE,"%s/randseed",VLOGDIR);
RAND_write_file(VBUFF);
chmod(VBUFF,0644); 
return 0;
}

/*******************************************************************/
/* Level 1                                                         */
/*******************************************************************/

void Initialize()

{
umask(077);
 /* XXX Initialize workdir for non privileged users */

strcpy(CFWORKDIR,WORKDIR);

#ifndef NT
if (geteuid() != 0)
   {
   char *homedir;
   if ((homedir = getenv("HOME")) != NULL)
      {
      strcpy(CFWORKDIR,homedir);
      strcat(CFWORKDIR,"/.cfagent");
      }
   }
#endif
 
strcpy(VLOCKDIR,CFWORKDIR);
strcpy(VLOGDIR,CFWORKDIR); 

OpenSSL_add_all_algorithms();
ERR_load_crypto_strings();

CheckWorkDirectories();
if (USERKEYFILE[0] != '\0')
  {
    strncpy(CFPRIVKEYFILE, USERKEYFILE, CF_BUFSIZE-6);
    strcat(CFPRIVKEYFILE, ".priv");
    strncpy(CFPUBKEYFILE, USERKEYFILE, CF_BUFSIZE-5);
    strcat(CFPUBKEYFILE, ".pub");
  }
RandomSeed();
}

/*******************************************************************/

void Syntax()

{ int i;

printf("GNU cfengine: A system configuration engine (cfkey)\n%s\n%s\n",VERSION,COPYRIGHT);
printf("\n");
printf("Options:\n\n");

for (i=0; OPTIONS[i].name != NULL; i++)
   {
   printf("--%-20s    (-%c)\n",OPTIONS[i].name,(char)OPTIONS[i].val);
   }

printf("\nBug reports to bug-cfengine@cfengine.org\n");
printf("General help to help-cfengine@cfengine.org\n");
printf("Info & fixes at http://www.cfengine.org\n");
}

/*****************************************************************************/

void CheckOpts(int argc,char **argv)

{ extern char *optarg;
  struct Item *actionList;
  char *tmp, *suffix;
  int optindex = 0;
  int c;

  while ((c=getopt_long(argc,argv,"f:hvVw",OPTIONS,&optindex)) != EOF)
  {
  switch ((char) c)
      {
      case 'f':
          strncpy(USERKEYFILE, optarg, CF_BUFSIZE-1);
          /* search for .priv or .pub suffix and wipe it out */
          suffix = strrchr(USERKEYFILE, '.');
          if (NULL != suffix 
              && ((0 == strcmp(suffix, ".priv")) 
                  || (0 == strcmp(suffix, ".pub"))))
            *suffix = '\0';
          break;
          
      case 'V': printf("GNU cfengine %s\n%s\n",VERSION,COPYRIGHT);
          printf("This program is covered by the GNU Public License and may be\n");
          printf("copied free of charge.  No warranty is implied.\n\n");
          exit(0);
          
      case 'h': Syntax();
          exit(0);

      case 'w': VERBOSE = false;
           break;
          
      default:  Syntax();
          exit(1);
         
      }
  }
}


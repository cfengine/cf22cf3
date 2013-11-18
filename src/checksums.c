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
/* Checksums                                                       */
/*                                                                 */
/*******************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*******************************************************************/

int CompareCheckSums(char *file1,char *file2,struct Image *ip,struct stat *sstat,struct stat *dstat)

{ static unsigned char digest1[EVP_MAX_MD_SIZE+1], digest2[EVP_MAX_MD_SIZE+1];
  int i;

Debug("CompareCheckSums(%s,%s)\n",file1,file2);

if (sstat->st_size != dstat->st_size)
   {
   Debug("File sizes differ, no need to compute checksum\n");
   return true;
   }
  
Debug2("Compare checksums on %s:%s & %s\n",ip->server,file1,file2);

if (strcmp(ip->server,"localhost") == 0)
   {
   ChecksumFile(file1,digest1,'m');
   ChecksumFile(file2,digest2,'m');

   for (i = 0; i < EVP_MAX_MD_SIZE; i++)
      {
      if (digest1[i] != digest2[i])
         {
         Verbose("Checksum mismatch...\n");
         return true;
         }
      }

   Debug("Files were identical\n");
   return false;  /* only if files are identical */
   }
else
   {
   return CompareMD5Net(file1,file2,ip); /* client.c */
   }
}

/*******************************************************************/

int CompareBinarySums(char *file1,char *file2,struct Image *ip,struct stat *sstat,struct stat *dstat)

     /* See the md5 algorithms in pub-lib/md5.c */
     /* file 1 is source                        */

 { int fd1, fd2,bytes1,bytes2;
   char buff1[BUFSIZ],buff2[BUFSIZ];

Debug("CompareBinarySums(%s,%s)\n",file1,file2);

if (sstat->st_size != dstat->st_size)
   {
   Debug("File sizes differ, no need to compute checksum\n");
   return true;
   }
  
Debug2("Compare binary sums on %s:%s & %s\n",ip->server,file1,file2);

if (strcmp(ip->server,"localhost") == 0)
   {
   fd1 = open(file1, O_RDONLY|O_BINARY, 0400);
   fd2 = open(file2, O_RDONLY|O_BINARY, 0400);
  
   do
      {
      bytes1 = read(fd1, buff1, BUFSIZ);
      bytes2 = read(fd2, buff2, BUFSIZ);

      if ((bytes1 != bytes2) || (memcmp(buff1, buff2, bytes1) != 0))
         {
         Verbose("Binary Comparison mismatch...\n");
         close(fd2);
         close(fd1);
         return true;
         }
      }
   while (bytes1 > 0);
   
   close(fd2);
   close(fd1);
   
   return false;  /* only if files are identical */
   }
 else
    {
    Debug("Using network md5 checksum instead\n");
    return CompareMD5Net(file1,file2,ip); /* client.c */
    }
 }

/*******************************************************************/

void ChecksumFile(char *filename,unsigned char digest[EVP_MAX_MD_SIZE+1],char type)

{ FILE *file;
  EVP_MD_CTX context;
  int len, md_len;
  unsigned char buffer[1024];
  const EVP_MD *md = NULL;

Debug2("ChecksumFile(%c,%s)\n",type,filename);

if ((file = fopen (filename, "rb")) == NULL)
   {
   printf ("%s can't be opened\n", filename);
   }
else
   {
   md = EVP_get_digestbyname(ChecksumName(type));
   
   EVP_DigestInit(&context,md);

   while (len = fread(buffer,1,1024,file))
      {
      EVP_DigestUpdate(&context,buffer,len);
      }

   EVP_DigestFinal(&context,digest,&md_len);
   
   /* Digest length stored in md_len */
   fclose (file);
   }
}

/*******************************************************************/

void ChecksumList(struct Item *list,unsigned char digest[EVP_MAX_MD_SIZE+1],char type)

{ struct Item *ip;
  EVP_MD_CTX context;
  int md_len;
  const EVP_MD *md = NULL;

Debug2("ChecksumList(%c)\n",type);

memset(digest,0,EVP_MAX_MD_SIZE+1);

md = EVP_get_digestbyname(ChecksumName(type));

EVP_DigestInit(&context,md);

for (ip = list; ip != NULL; ip=ip->next) 
   {
   Debug(" digesting %s\n",ip->name);
   EVP_DigestUpdate(&context,ip->name,strlen(ip->name));
   }

EVP_DigestFinal(&context,digest,&md_len);
}

/*******************************************************************/

void ChecksumString(char *buffer,int len,unsigned char digest[EVP_MAX_MD_SIZE+1],char type)

{ EVP_MD_CTX context;
  const EVP_MD *md = NULL;
  int md_len;

Debug2("ChecksumString(%c)\n",type);

md = EVP_get_digestbyname(ChecksumName(type));

EVP_DigestInit(&context,md); 
EVP_DigestUpdate(&context,(unsigned char*)buffer,len);
EVP_DigestFinal(&context,digest,&md_len);
}

/*******************************************************************/

int ChecksumsMatch(unsigned char digest1[EVP_MAX_MD_SIZE+1],unsigned char digest2[EVP_MAX_MD_SIZE+1],char type)


{ int i,size = EVP_MAX_MD_SIZE;
 
size = ChecksumSize(type);

for (i = 0; i < size; i++)
   {
   if (digest1[i] != digest2[i])
      {
      return false;
      }
   }

return true;
}

/*********************************************************************/

char *ChecksumPrint(char type,unsigned char digest[EVP_MAX_MD_SIZE+1])

{ unsigned int i;
  static char buffer[EVP_MAX_MD_SIZE*4];
  int len = 16;

switch(type)
   {
   case 's': sprintf(buffer,"SHA=  ");
       len = 20;
       break;
   case 'm': sprintf(buffer,"MD5=  ");
       len = 16;
       break;
   }
  
for (i = 0; i < len; i++)
   {
   sprintf((char *)(buffer+4+2*i),"%02x", digest[i]);
   }

return buffer; 
}    

/*********************************************************************/

char ChecksumType(char *typestr)

{ int i;

for (i = 0; CF_DIGEST_TYPES[i][0] != NULL; i++)
   {
   if (strcmp(typestr,CF_DIGEST_TYPES[i][0]) == 0)
      {
      return *CF_DIGEST_TYPES[i][1];
      }
   }

if (PARSING)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Illegal checksum/digest type: %s",typestr);
   yyerror(OUTPUT);
   FatalError("Should be md5,sha,sha1,sha224,sha256,sha348,sha512");
   }

return 'x';
}

/*********************************************************************/

char *ChecksumName(char type)

{ int i;

for (i = 0; CF_DIGEST_TYPES[i][0] != NULL; i++)
   {
   if (type == *CF_DIGEST_TYPES[i][1])
      {
      return CF_DIGEST_TYPES[i][0];
      }
   }

return NULL;
}

/*********************************************************************/

int ChecksumSize(char type)

{ int i,size = 0;
 
for (i = 0; CF_DIGEST_TYPES[i][0] != NULL; i++)
   {
   if (type == *CF_DIGEST_TYPES[i][1])
      {
      return CF_DIGEST_SIZES[i];
      }
   }

return size;
}

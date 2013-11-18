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
/* File copying low level                                          */
/*                                                                 */
/*******************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*********************************************************************/

void CheckForHoles(struct stat *sstat,struct Image *ip)

/* Need a transparent way of getting this into CopyReg() */
/* Use a public member in struct Image                   */

{
#ifndef IRIX
if (sstat->st_size > sstat->st_blocks * DEV_BSIZE)
#else
# ifdef HAVE_ST_BLOCKS
if (sstat->st_size > sstat->st_blocks * DEV_BSIZE)
# else
if (sstat->st_size > ST_NBLOCKS((*sstat)) * DEV_BSIZE)
# endif
#endif
   {
   ip->makeholes = 1;   /* must have a hole to get checksum right */
   }

ip->makeholes = 0;
}

/*********************************************************************/

int CopyRegDisk(char *source,char *new,struct Image *ip)

{ int sd, dd, buf_size;
  char *buf, *cp;
  int n_read, *intp;
  long n_read_total = 0;
  int last_write_made_hole = 0;
  
if ((sd = open(source,O_RDONLY|O_BINARY)) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Can't copy %s!\n",source);
   CfLog(cfinform,OUTPUT,"open");
   unlink(new);
   return false;
   }

unlink(new);  /* To avoid link attacks */
 
if ((dd = open(new,O_WRONLY|O_CREAT|O_TRUNC|O_EXCL|O_BINARY, 0600)) == -1)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Copy %s:%s security - failed attempt to exploit a race? (Not copied)\n",ip->server,new);
   CfLog(cfinform,OUTPUT,"open");
   close(sd);
   unlink(new);
   return false;
   }

buf_size = ST_BLKSIZE(dstat);
buf = (char *) malloc(buf_size + sizeof(int));

while (true)
   {
   if ((n_read = read (sd, buf, buf_size)) == -1)
      {
      if (errno == EINTR) 
         {
         continue;
         }

      close(sd);
      close(dd);
      free(buf);
      return false;
      }

   if (n_read == 0)
      {
      break;
      }

   n_read_total += n_read;

   intp = 0;

   if (ip->makeholes)
      {
      buf[n_read] = 1;                    /* Sentinel to stop loop.  */

      /* Find first non-zero *word*, or the word with the sentinel.  */

      intp = (int *) buf;

      while (*intp++ == 0)
         {
         }

      /* Find the first non-zero *byte*, or the sentinel.  */

      cp = (char *) (intp - 1);

      while (*cp++ == 0)
         {
         }

      /* If we found the sentinel, the whole input block was zero,
         and we can make a hole.  */

      if (cp > buf + n_read)
         {
         /* Make a hole.  */
         if (lseek (dd, (off_t) n_read, SEEK_CUR) < 0L)
            {
            snprintf(OUTPUT,CF_BUFSIZE,"Copy failed (no space?) while doing %s to %s\n",source,new);
            CfLog(cferror,OUTPUT,"lseek");
            free(buf);
            unlink(new);
            close(dd);
            close(sd);
            return false;
            }
         last_write_made_hole = 1;
         }
      else
         {
         /* Clear to indicate that a normal write is needed. */
         intp = 0;
         }
      }
   
   if (intp == 0)
      {
      if (cf_full_write (dd, buf, n_read) < 0)
         {
         snprintf(OUTPUT,CF_BUFSIZE*2,"Copy failed (no space?) while doing %s to %s\n",source,new);
         CfLog(cferror,OUTPUT,"");
         close(sd);
         close(dd);
         free(buf);
         unlink(new);
         return false;
         }
      last_write_made_hole = 0;
      }
   }
 
  /* If the file ends with a `hole', something needs to be written at
     the end.  Otherwise the kernel would truncate the file at the end
     of the last write operation.  */

  if (last_write_made_hole)
    {
    /* Write a null character and truncate it again.  */
    
    if (cf_full_write (dd, "", 1) < 0 || ftruncate (dd, n_read_total) < 0)
       {
       CfLog(cferror,"cfengine: full_write or ftruncate error in CopyReg\n","write");
       free(buf);
       unlink(new);
       close(sd);
       close(dd);
       return false;
       }
    }

close(sd);
close(dd);

free(buf);
return true;
}

/*********************************************************************/

int EmbeddedWrite(char *new,int dd,char *buf,struct Image *ip,int towrite,int *last_write_made_hole,int n_read)

{ int *intp;
 char *cp;
 
 intp = 0;
 
 if (ip->makeholes)
    {
    buf[n_read] = 1;                    /* Sentinel to stop loop.  */
    
    /* Find first non-zero *word*, or the word with the sentinel.  */
    intp = (int *) buf;
    
    while (*intp++ == 0)
       {
       }
    
    /* Find the first non-zero *byte*, or the sentinel.  */
    
    cp = (char *) (intp - 1);
    
    while (*cp++ == 0)
       {
       }
    
    /* If we found the sentinel, the whole input block was zero,
       and we can make a hole.  */
    
    if (cp > buf + n_read)
       {
       /* Make a hole.  */
       if (lseek (dd,(off_t)n_read,SEEK_CUR) < 0L)
          {
          snprintf(OUTPUT,CF_BUFSIZE,"lseek in EmbeddedWrite, dest=%s\n", new);
          CfLog(cferror,OUTPUT,"lseek");
          return false;
          }
       *last_write_made_hole = 1;
       }
    else
       {
       /* Clear to indicate that a normal write is needed. */
       intp = 0;
       }
    }
 
 if (intp == 0)
    {
    if (cf_full_write (dd,buf,towrite) < 0)
       {
       snprintf(OUTPUT,CF_BUFSIZE*2,"Local disk write(%.256s) failed\n",new);
       CfLog(cferror,OUTPUT,"write");
       CONN->error = true;
       return false;
       }

    *last_write_made_hole = 0;
    }

return true;
}

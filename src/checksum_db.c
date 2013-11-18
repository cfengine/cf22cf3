
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
 
/*****************************************************************************/
/*                                                                           */
/* File: checksum_db.c                                                       */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/* Alter this code at your peril. Berkeley DB is *very* sensitive to errors. */

/***************************************************************/

int ChecksumChanged(char *filename,unsigned char digest[EVP_MAX_MD_SIZE+1],int warnlevel,int refresh,char type)

    /* Returns false if filename never seen before, and adds a checksum
       to the database. Returns true if checksums do not match and also
       updates database to the new value */

{ struct stat stat1, stat2;
  int i,needupdate = false, size = 21;
  unsigned char dbdigest[EVP_MAX_MD_SIZE+1],dbattr[EVP_MAX_MD_SIZE+1];
  unsigned char current_digest[EVP_MAX_MD_SIZE+1],attr_digest[EVP_MAX_MD_SIZE+1];
  DBT *key,*value;
  DB *dbp;
  DB_ENV *dbenv = NULL;
  FILE *fp;

Debug("ChecksumChanged: key %s (type=%c) with data %s\n",filename,type,ChecksumPrint(type,digest));

size = ChecksumSize(type);

memset(current_digest,0,EVP_MAX_MD_SIZE+1);
memset(attr_digest,0,EVP_MAX_MD_SIZE+1);

ChecksumFile(filename,current_digest,type);

if (CHECKSUMDB == NULL)
   {
   if (ISCFENGINE)
      {
      FatalError("Checksum database is undefined (shouldn't happen - misc.c)");
      }
   else
      {
      Debug("Direct comparison (no db)\n");
      
      for (i = 0; i < size; i++)
         {
         if (digest[i] != current_digest[i])
            {
            return true;
            }
         }
      return false;
      }
   }
 
if (refresh) /* this section should not be used anymore */
   {
   /* Check whether database is current wrt local file - simple cheap test */
   
   if (stat(filename,&stat1) == -1)
      {
      snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't stat %s\n",filename);
      CfLog(cferror,OUTPUT,"stat");
      return false;
      }
    
   if (stat(CHECKSUMDB,&stat2) != -1)
      {
      if (stat1.st_mtime > stat2.st_mtime)
         {
         Debug("Checksum database is older than %s...refresh needed\n",filename);
         needupdate = true;
         }
      else
         {
         Debug("Checksum up to date..\n");
         }
      }
   else
      {
      needupdate = true;
      }      
   }

if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open checksum database %s\n",CHECKSUMDB);
   CfLog(cferror,OUTPUT,"db_open");
   return false;
   }

#ifdef CF_OLD_DB
if ((errno = (dbp->open)(dbp,CHECKSUMDB,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = (dbp->open)(dbp,NULL,CHECKSUMDB,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open checksum database %s\n",CHECKSUMDB);
   CfLog(cferror,OUTPUT,"db_open");
   dbp->close(dbp,0);
   return false;
   }

if (needupdate) /* This section should not be needed any more */
   {
   DeleteChecksum(dbp,type,filename);    
   WriteChecksum(dbp,type,filename,current_digest,attr_digest);
   }

if (ReadChecksum(dbp,type,filename,dbdigest,dbattr))
   {
   /* Ignoring attr for now - future development */
   
   for (i = 0; i < size; i++)
      {
      if (current_digest[i] != dbdigest[i])
         {
         Debug("Found checksum for %s in database but it didn't match\n",filename);
         
         if (EXCLAIM)
            {
            CfLog(warnlevel,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!","");
            }
         
         snprintf(OUTPUT,CF_BUFSIZE*2,"SECURITY ALERT: Checksum (%s) for %s changed!",ChecksumName(type),filename);
         CfLog(warnlevel,OUTPUT,"");
         
         if (EXCLAIM)
            {
            CfLog(warnlevel,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!","");
            }
         
         if (CHECKSUMUPDATES)
            {
            Verbose("Updating checksum for %s to %s\n",filename,ChecksumPrint(type,current_digest));
            
            DeleteChecksum(dbp,type,filename);
            WriteChecksum(dbp,type,filename,current_digest,attr_digest);
            }
         
         dbp->close(dbp,0);
         return true;                        /* Checksum updated but was changed */
         }
      }
   
   Debug("Found checksum for %s in database and it matched\n",filename);
   dbp->close(dbp,0);
   return false;
   }
else
   {
   /* Key was not found, so install it */
   
   if (ISCFENGINE)
      {
      snprintf(OUTPUT,CF_BUFSIZE,"File %s was not in %s database - new file found",filename,ChecksumName(type));
      CfLog(cfsilent,OUTPUT,"");
      }
   
   Debug("Storing checksum for %s in database %s\n",filename,ChecksumPrint(type,current_digest));
   WriteChecksum(dbp,type,filename,current_digest,attr_digest);
   
   dbp->close(dbp,0);
   
   if (ISCFENGINE)
      {
      return false;      /* No need to warn when first installed */
      }
   else
      {
      return true;
      }
   }
}

/***************************************************************/

void ChecksumPurge()

/* Go through the database and purge records about non-existent files */

{ DBT key,value;
  DB *dbp;
  DBC *dbcp;
  DB_ENV *dbenv = NULL;
  int ret;
  struct stat statbuf;
  
if ((errno = db_create(&dbp,dbenv,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open checksum database %s\n",CHECKSUMDB);
   CfLog(cferror,OUTPUT,"db_open");
   return;
   }

#ifdef CF_OLD_DB
if ((errno = (dbp->open)(dbp,CHECKSUMDB,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#else
if ((errno = (dbp->open)(dbp,NULL,CHECKSUMDB,NULL,DB_BTREE,DB_CREATE,0644)) != 0)
#endif
   {
   snprintf(OUTPUT,CF_BUFSIZE*2,"Couldn't open checksum database %s\n",CHECKSUMDB);
   CfLog(cferror,OUTPUT,"db_open");
   dbp->close(dbp,0);
   return;
   }

/* Acquire a cursor for the database. */

if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
   {
   CfLog(cferror,"Error reading from checksum database","");
   dbp->err(dbp, ret, "DB->cursor");
   return;
   }

 /* Walk through the database and print out the key/data pairs. */

memset(&key,0,sizeof(key));
memset(&value,0,sizeof(value));

while (dbcp->c_get(dbcp, &key, &value, DB_NEXT) == 0)
   {
   char *obj = (char *)key.data + CF_CHKSUMKEYOFFSET;
   
   if (stat(obj,&statbuf) == -1)
      {
      Verbose("Purging file %s from checksum db - no longer exists\n",obj);
      snprintf(OUTPUT,CF_BUFSIZE*2,"SECURITY INFO: %s checksum for %s purged - file no longer exists!",key.data,obj);
      CfLog(cferror,OUTPUT,"");
      
      if ((errno = dbp->del(dbp,NULL,&key,0)) != 0)
         {
         CfLog(cferror,"","db_store");
         }
      }
   
   memset(&key,0,sizeof(key));
   memset(&value,0,sizeof(value));
   }

dbcp->c_close(dbcp);
dbp->close(dbp,0);
}

/*****************************************************************************/

int ReadChecksum(DB *dbp,char type,char *name,unsigned char digest[EVP_MAX_MD_SIZE+1], unsigned char *attr)

{ DBT *key,value;
  struct Checksum_Value chk_val;
  
key = NewChecksumKey(type,name);

memset(&value,0,sizeof(value));

if ((errno = dbp->get(dbp,NULL,key,&value,0)) == 0)
   {
   memset(digest,0,EVP_MAX_MD_SIZE+1);
   memset(&chk_val,0,sizeof(chk_val));
   
   memcpy(&chk_val,value.data,sizeof(chk_val));
   memcpy(digest,chk_val.mess_digest,EVP_MAX_MD_SIZE+1);
   
   Debug("READ %c %s %s\n",type,name,ChecksumPrint(type,digest));
   DeleteChecksumKey(key);
   return true;
   }
else
   {
   Debug("Checksum read failed: %s",db_strerror(errno));
   DeleteChecksumKey(key);
   return false;
   }
}

/*****************************************************************************/

int WriteChecksum(DB *dbp,char type,char *name,unsigned char digest[EVP_MAX_MD_SIZE+1], unsigned char *attr)

{ DBT *key,*value;
 
key = NewChecksumKey(type,name); 
value = NewChecksumValue(digest,attr);

Debug("DATA = %s\n",ChecksumPrint(type,value->data));

if ((errno = dbp->put(dbp,NULL,key,value,0)) != 0)
   {
   snprintf(OUTPUT,CF_BUFSIZE,"Checksum write failed: %s",db_strerror(errno));
   CfLog(cferror,OUTPUT,"db->put");
   
   DeleteChecksumKey(key);
   DeleteChecksumValue(value);
   return false;
   }
else
   {
   DeleteChecksumKey(key);
   DeleteChecksumValue(value);
   return true;
   }
}

/*****************************************************************************/

void DeleteChecksum(DB *dbp,char type,char *name)

{ DBT *key;

key = NewChecksumKey(type,name);

if ((errno = dbp->del(dbp,NULL,key,0)) != 0)
   {
   CfLog(cferror,"","db_store");
   }

DeleteChecksumKey(key);
Debug("DELETED %s\n",name);
}


/*****************************************************************************/
/* level 1                                                                   */
/*****************************************************************************/

DBT *NewChecksumKey(char type,char *name)

{ char *chk_key;
  DBT *key;

if ((chk_key = malloc(strlen(name)+CF_MAXDIGESTNAMELEN+2)) == NULL)
   {
   FatalError("NewChecksumKey malloc error");
   }

if ((key = (DBT *)malloc(sizeof(DBT))) == NULL)
   {
   FatalError("DBT  malloc error");
   }

memset(key,0,sizeof(DBT));
memset(chk_key,0,strlen(name)+CF_MAXDIGESTNAMELEN+2);

strcpy(chk_key,ChecksumName(type)); /* safe */

/* Berkeley DB needs this packed */

strncpy(chk_key+CF_CHKSUMKEYOFFSET,name,strlen(name));

Debug("KEY => %s,%s\n",chk_key,chk_key+CF_CHKSUMKEYOFFSET);
key->data = chk_key;
key->size = strlen(name)+CF_MAXDIGESTNAMELEN+2;

return key;
}

/*****************************************************************************/

void DeleteChecksumKey(DBT *key)

{
free((char *)key->data);
free((char *)key);
}

/*****************************************************************************/

DBT *NewChecksumValue(unsigned char digest[EVP_MAX_MD_SIZE+1],unsigned char attr[EVP_MAX_MD_SIZE+1])
{ struct Checksum_Value *chk_val;
  DBT *value;
  char *x;

if ((chk_val = (struct Checksum_Value *)malloc(sizeof(struct Checksum_Value))) == NULL)
   {
   FatalError("NewChecksumKey malloc error");
   }

if ((value = (DBT *) malloc(sizeof(DBT))) == NULL)
   {
   FatalError("DBT Value malloc error");
   }

memset(value,0,sizeof(DBT)); 

memset(chk_val,0,sizeof(struct Checksum_Value));
memcpy(chk_val->mess_digest,digest,EVP_MAX_MD_SIZE+1);
memcpy(chk_val->attr_digest,attr,EVP_MAX_MD_SIZE+1);

value->data = (void *) chk_val;
value->size = sizeof(*chk_val);

return value;
}

/*****************************************************************************/

void DeleteChecksumValue(DBT *value)

{ struct Checksum_Value *chk_val;

chk_val = (struct Checksum_Value *) value->data;

free((char *)chk_val);
free((char *)value);
}

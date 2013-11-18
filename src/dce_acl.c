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
/* File: dce_acl.c                                                           */
/* Author: Allan L. Bazinet                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

#ifdef HAVE_DCE_DACLIF_H

#include <pthread.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dce/rpc.h>
#include <dce/aclbase.h>
#include <dce/dce_msg.h>
#include <dce/daclif.h>
#include <dce/secidmap.h>
#include <dce/binding.h>
#include <dcedfs/aclint.h>

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

#define MGR_TYPES_SIZE_AVAIL (2)

#define ArraySize(x) ((sizeof(x))/(sizeof(x[0])))
#define SetBit(f,b) (f |= b)
#define UnSetBit(f,b) (f &= ~(b))

/*****************************************************************************/
/* Local Types                                                               */
/*****************************************************************************/

struct rgyData {
  boolean32            isId;
  uuid_t               uuid;
  sec_rgy_name_t       name;
  sec_acl_entry_type_t type;
};

struct aclData {
  sec_rgy_name_t   name;
  sec_acl_handle_t handle;
  uuid_t           mgrTypes[MGR_TYPES_SIZE_AVAIL];
  unsigned32       mgrTypesNum;
  unsigned32       mgrTypesSizeUsed;
  sec_acl_list_t   list;
  sec_acl_p_t      new;
  sec_acl_p_t      old;
};

/*****************************************************************************/
/* Local Prototypes                                                          */
/*****************************************************************************/

static void
setupACL(int,
  struct aclData *,
  error_status_t *);

static void
resetACL(struct aclData *);

static void
logError(const char *,
  enum cfoutputlevel,
  error_status_t);

static void
getRgyData(const char *,
    const char *,
    struct rgyData *,
    error_status_t *);

static boolean32
isMatchingACLEntry(sec_acl_entry_t * aclEntry,
     struct rgyData  * rgyData,
     error_status_t  * status);

static void
nameToUUID(sec_rgy_name_t,
    sec_acl_entry_type_t,
    uuid_p_t,
    error_status_t *);

static sec_acl_permset_t
permPrintableToBitmask(char);

static sec_acl_permset_t
permSetPrintableToBinary(const char *,
    sec_acl_permset_t,
    error_status_t *);

static sec_acl_entry_type_t
aclTypePrintableToBinary(const char *,
    boolean32,
    error_status_t *);

static boolean32
equalACLEntry(sec_acl_entry_t * lhs,
       sec_acl_entry_t * rhs,
       error_status_t  * status);

static boolean32
equalACLEntries(sec_acl_t      * lhs,
  sec_acl_t      * rhs,
  error_status_t * status);

static void
copyACLEntries(sec_acl_t const * source,
        sec_acl_t       * target);

/*****************************************************************************/
/* Exposed Routines                                                          */
/*****************************************************************************/

/*
** CheckDFSACE()
**
** This is called by CheckACLs, and should return true on success,
** false on failure.
*/

int
CheckDFSACE(struct CFACE     * aces,
     char               method,
     char             * filename,
     enum fileactions   action)
{
  struct CFACE   * ep;
  struct aclData   aclData;
  struct rgyData   rgyData;
  int              aclIndex;
  boolean32        entryMatch;
  error_status_t   aclStatus;
  error_status_t   fooStatus;

  /* Setup aclData structure by copying requested filename to name member */

  strncpy((char *)&(aclData.name[0]),
   (const char *)filename,
   sizeof(aclData.name));

  sec_acl_bind(aclData.name,
        FALSE,
        &aclData.handle,
        &aclStatus);

  if (aclStatus == error_status_ok) {

    sec_acl_get_manager_types(aclData.handle,
         sec_acl_type_object,
         ArraySize(aclData.mgrTypes),
         &aclData.mgrTypesSizeUsed,
         &aclData.mgrTypesNum,
         aclData.mgrTypes,
         &aclStatus);

    if (aclStatus == error_status_ok) {

      if (aclData.mgrTypesSizeUsed < aclData.mgrTypesNum) {
 CfLog(cfsilent, "Warning: some manager types missed\n", "");
      }

      sec_acl_lookup(aclData.handle,
       aclData.mgrTypes,
       sec_acl_type_object,
       &aclData.list,
       &aclStatus);

      if (aclStatus == error_status_ok) {

 setupACL(MAXDFSACL, &aclData, &aclStatus);

 if (aclStatus == error_status_ok) {

   /* If this is an append, copy all existing acls to the new
   ** ACL list allocated by setupACL()
   */

   if (method == 'a') copyACLEntries(aclData.old,
         aclData.new);

   /* Walk CFACE list until end-of-list or fatal error */

   for (ep = aces;
        ep && (aclStatus == error_status_ok);
        ep = ep->next) {

     if ((ep->name == NULL) || (IsExcluded(ep->classes))) continue;
    
     Verbose("%s: Mode =%s, name=%s, type=%s\n",
      VPREFIX,
      ep->mode,
      ep->name,
      ep->acltype);

     /* Translate external ACL representation to internal */

     getRgyData(ep->name, ep->acltype, &rgyData, &aclStatus);

     if (aclStatus == error_status_ok) {

       /* Zip through the ACL, looking for a match */

       entryMatch = FALSE;
       for (aclIndex = 0;
     aclIndex < aclData.new->num_entries;
     aclIndex++) {

  entryMatch = isMatchingACLEntry(&(aclData.new->
        sec_acl_entries[aclIndex]),
      &rgyData,
      &aclStatus);

  if (aclStatus == error_status_ok) {

    if (entryMatch) {
        
      /*
      ** "default" means remove the named user from the ACL,
      ** so that default permissions apply.  If this is a
      ** "default", then decrease number of entries in the ACL;
      ** if this wasn't the topmost entry, then move the topmost
      ** to this slot.
      **
      ** Otherwise, set the permissions set on this entry
      ** using the mode provided.
      */
  
      if (strcmp(ep->mode, "default") == 0) {
        aclData.new->num_entries--;
        if (aclData.new->num_entries != aclIndex) {
   aclData.new->sec_acl_entries[aclIndex] =
     aclData.new->sec_acl_entries[aclData.new->
            num_entries];
        }
      } else {
        aclData.new->sec_acl_entries[aclIndex].perms =
   permSetPrintableToBinary(ep->mode,
       aclData.new->
       sec_acl_entries[aclIndex].
       perms,
       &aclStatus);
      }

      /* ACL entry has been matched; stop looking. */

      break;
    }
  }
       }

       /* At this point, we've either experienced a fatal error or
       ** have searched the ACL list for the entry, successfully or
       ** unsuccessfully.  If we are still alive and yet haven't
       ** found a matching entry, then either this is a default entry,
       ** in which case we do nothing (since there's nothing to remove),
       ** or a new entry, in which case we set it up.
       */

       if (aclStatus == error_status_ok) {

  if ((entryMatch == FALSE) &&
      (strcmp(ep->mode, "default") != 0)) {

    aclIndex = aclData.new->num_entries;
    aclData.new->num_entries = aclIndex + 1;
    if (rgyData.isId) {
      aclData.new->sec_acl_entries[aclIndex].
        entry_info.tagged_union.id.uuid = rgyData.uuid;
    }
    aclData.new->sec_acl_entries[aclIndex].
      entry_info.entry_type = rgyData.type;
    aclData.new->sec_acl_entries[aclIndex].
      perms = permSetPrintableToBinary(ep->mode, 0, &aclStatus);
  }
       }
     }
   }

   /* We've now either successfully run the list of CFACE elements
   ** and have a new ACL, or we've been killed by a fatal error.
   ** If we are alive, and the new ACL is different from the
   ** preexisting one, then we will attempt to update; otherwise,
   ** we are a no-op.
   */

   if (aclStatus == error_status_ok) {

     entryMatch = equalACLEntries(aclData.old,
      aclData.new,
      &aclStatus);

     if (aclStatus == error_status_ok) {
       
       if (entryMatch) {
  Verbose("%s: No update necessary\n", VPREFIX);
  aclStatus = sec_acl_duplicate_entry;
       } else {
  if ((action == warnall)   ||
      (action == warnplain) ||
      (action == warndirs)) {
    snprintf(OUTPUT,CF_BUFSIZE,"File %s needs ACL update\n", filename);
    CfLog(cfinform, OUTPUT, "");
    aclStatus = sec_acl_not_authorized;
  } else {
    sec_acl_replace(aclData.handle,
      aclData.mgrTypes,
      sec_acl_type_object,
      &(aclData.list),
      &aclStatus);

    if (aclStatus == error_status_ok) {
      snprintf(OUTPUT,CF_BUFSIZE,"ACL for file %s updated\n", filename);
      CfLog(cfinform, OUTPUT, "");
    } else {
      logError("sec_acl_replace", cferror, aclStatus);
    }
  }
       }
     }
   }

   /* Reset aclData structure to initial state */

   resetACL(&aclData);
 }

 /*
 ** Free ACL storage that the system provided via sec_acl_lookup().
 */

 for (aclIndex = 0;
      aclIndex < aclData.list.num_acls;
      aclIndex++) {
   sec_acl_release(aclData.handle,
     aclData.list.sec_acls[aclIndex],
     &fooStatus);
   if (fooStatus != error_status_ok) {
     logError("sec_acl_release", cferror, fooStatus);
   }
 }

      } else {

 logError("sec_acl_lookup", cferror, aclStatus);
      }

    } else {

      logError("sec_acl_get_manager_types", cferror, aclStatus);
    }

    sec_acl_release_handle(&aclData.handle, &fooStatus);
    if (fooStatus != error_status_ok) {
      logError("sec_acl_release_handle", cferror, fooStatus);
    }

  } else {

    logError("sec_acl_bind", cferror, aclStatus);
  }

  return ((aclStatus == error_status_ok) ? true : false);
}

/*****************************************************************************/
/* Private Routines                                                          */
/*****************************************************************************/

/*
** getRgyData()
**
** Attempt to populate provided rgyData structure using provided name
** and acl information.  Status returned via status parameter.
**
*/

static void
getRgyData(const char     * aclName,
    const char     * aclType,
    struct rgyData * rgyData,
    error_status_t * status)
{
  /*
  ** asterisk in "name" field means that the user or group id
  ** is implicitly specified as the owner of the file, or that
  ** no id is applicable at all (as is the case for "other").
  */
  
  if ((strlen(aclName) == 0) || (strcmp(aclName, "*") == 0)) {
    rgyData->isId = FALSE;
  } else {
    rgyData->isId = TRUE;
    strncpy((char *)&(rgyData->name[0]), aclName, sizeof(rgyData->name));
  }

  rgyData->type = aclTypePrintableToBinary(aclType, rgyData->isId, status);

  if (*status == error_status_ok) {

    if (rgyData->isId) nameToUUID(rgyData->name,
      rgyData->type,
      &(rgyData->uuid),
      status);
  }
}

/*
** isMatchingACLEntry()
**
** Compare provided ACL entry to information contained in provided rgyData
** structure, returning boolean value representing matched/not matched
** status.  Status returned via status parameter.
*/

static boolean32
isMatchingACLEntry(sec_acl_entry_t * aclEntry,
     struct rgyData  * rgyData,
     error_status_t  * status)
{
  boolean32 isMatching;

  if (aclEntry->entry_info.entry_type == rgyData->type) {
    if (rgyData->isId) {
      isMatching = uuid_equal(&(aclEntry->entry_info.tagged_union.id.uuid),
         &(rgyData->uuid),
         status);
    } else {
      isMatching = TRUE;
    }
  } else {
    isMatching = FALSE;
  }

  return(isMatching);
}

/*
** aclTypePrintableToBinary()
**
** If type information is found to be valid, return binary type.
** Return status via provided status parameter.
**
** Notes: Since sec_acl_entry_type_t is an enumerated type (see
**        /usr/include/dce/aclbase.h), we can't simply return 0 on
**        failure, as that is equivalent to sec_acl_e_type_user_obj.
*/

static sec_acl_entry_type_t
aclTypePrintableToBinary(const char     * tString,
    boolean32        nUsed,
    error_status_t * status)
{
  static const struct Type {
    const char           * tString;
    sec_acl_entry_type_t   tType;
    sec_acl_entry_type_t   nType;
  } TypeTable[] = {
    {"any",             sec_acl_e_type_any_other,       0                    },
    {"foreign_group",   sec_acl_e_type_foreign_group,   0                    },
    {"foreign_other",   sec_acl_e_type_foreign_other,   0                    },
    {"foreign_user",    sec_acl_e_type_foreign_user,    0                    },
    {"group",           sec_acl_e_type_group_obj,       sec_acl_e_type_group },
    {"mask",            sec_acl_e_type_mask_obj,        0                    },
    {"other",           sec_acl_e_type_other_obj,       0                    },
    {"unauthenticated", sec_acl_e_type_unauthenticated, 0                    },
    {"user",            sec_acl_e_type_user_obj,        sec_acl_e_type_user  }
  };

  struct Type const * bot = &TypeTable[0];
  struct Type const * top = &TypeTable[ArraySize(TypeTable)];
  struct Type const * mid;
  int                 cmp;
    
  while (bot < top) {
    mid = bot + ((top - bot) >> 1);
    cmp = strcmp(tString, mid->tString);
    if (cmp < 0)
      top = mid;
    else if (cmp > 0)
      bot = mid + 1;
    else {
      *status = error_status_ok;
      if ((mid->nType) && (nUsed)) return mid->nType;
      else                         return mid->tType;
    }
  }

  snprintf(OUTPUT,CF_BUFSIZE,"Invalid DCE/DFS acl type '%s'\n", tString);
  CfLog(cferror, OUTPUT, "");
  *status = sec_acl_invalid_acl_type;
  return(0);
}

/*
** permSetPrintableToBinary()
**
** Convert provided permissions set to sec_acl_permset_t binary
** representation and return.  Status returned via status
** paramter.
*/

static sec_acl_permset_t
permSetPrintableToBinary(const char        * perms,
    sec_acl_permset_t   oldPermSet,
    error_status_t    * status)
{
  error_status_t      localStatus = error_status_ok;
  int                 isAdd;
  const char        * permsPtr;
  char                curPerm;
  sec_acl_permset_t   bitmask;
  sec_acl_permset_t   newPermSet;

  if ((strcmp(perms, "noaccess")) == 0) {
    newPermSet = 0;
  } else {
    newPermSet = oldPermSet;
    isAdd      = true;
    permsPtr   = perms;
    while (curPerm = *permsPtr++) {
      if ((curPerm == '+') || (curPerm == ',')) {
 isAdd = true;
      }
      else if (curPerm == '-') {
 isAdd = false;
      }
      else if (curPerm == '=') {
 isAdd      = true;
 newPermSet = 0;
      } else {
 if (bitmask = permPrintableToBitmask(curPerm)) {
   if (isAdd) SetBit(newPermSet, bitmask);
   else     UnSetBit(newPermSet, bitmask);
 } else {
   snprintf(OUTPUT,CF_BUFSIZE,
    "Invalid mode '%c' in DCE/DFS acl: %s\n",
    curPerm,
    perms);
   CfLog(cferror, OUTPUT, "");
   localStatus = sec_acl_invalid_dfs_acl;
   break;
 }
      }
    }
  }

  *status = localStatus;

  return newPermSet;
}

/*
** permPrintableToBitmask()
**
** Convert the provided DCE/DFS printable permission to the corresponding
** binary representation.  Returns nonzero binary representation on
** success, zero on failure.
*/

static sec_acl_permset_t
permPrintableToBitmask(char printable)
{
  static const struct Convert {
    char              printable;
    sec_acl_permset_t bitmask;
  } ConvertTable[] = {
    { 'c', sec_acl_perm_control },
    { 'd', sec_acl_perm_delete  },
    { 'i', sec_acl_perm_insert  },
    { 'r', sec_acl_perm_read    },
    { 'w', sec_acl_perm_write   },
    { 'x', sec_acl_perm_execute }
  };

  struct Convert const * bot = &ConvertTable[0];
  struct Convert const * top = &ConvertTable[ArraySize(ConvertTable)];
  struct Convert const * mid;
  int                    cmp;
    
  while (bot < top) {
    mid = bot + ((top - bot) >> 1);
    cmp = printable - mid->printable;
    if (cmp < 0)
      top = mid;
    else if (cmp > 0)
      bot = mid + 1;
    else
      return mid->bitmask;
  }

  return(0);
}

/*
** logError()
**
** Convert provided DCE status code to printable representation, if
** possible, and output error message at specified level.
*/

static void
logError(const char         * routine,
  enum cfoutputlevel   level,
  error_status_t       status)
{
  error_status_t    msgStatus;
  unsigned_char_p_t msgText;

  if (msgText = dce_msg_get_msg(status, &msgStatus)) {
    CfLog(level, msgText, routine);
    free(msgText);
  } else {
    snprintf(OUTPUT,CF_BUFSIZE,
     "<unable to retrieve message for status code %ld; "
     "retrieval code %ld>\n",
     status,
     msgStatus);
    CfLog(level, OUTPUT, routine);
  }
}

/*
** nameToUUID()
**
** Convert provided name to uuid as known by the registry.  Status
** returned via status parameter.
*/

static void
nameToUUID(sec_rgy_name_t         name,
    sec_acl_entry_type_t   type,
    uuid_p_t               uuid,
    error_status_t       * status)
{
  sec_rgy_handle_t rgySite;
  error_status_t   rgyStatus;
  error_status_t   fooStatus;
  
  sec_rgy_site_open((unsigned_char_p_t)"/.:",
      &rgySite,
      &rgyStatus);

  if (rgyStatus == error_status_ok) {

    switch (type) {
    case sec_acl_e_type_user:
      {
 sec_id_parse_name(rgySite,
     name,
     NULL,
     NULL,
     NULL,
     uuid,
     &rgyStatus);
 if (rgyStatus != error_status_ok) {
   logError("sec_rgy_parse_name", cferror, rgyStatus);
 }
      }
      break;
    case sec_acl_e_type_group:
      {
 sec_id_parse_group(rgySite,
      name,
      NULL,
      NULL,
      NULL,
      uuid,
      &rgyStatus);
 if (rgyStatus != error_status_ok) {
   logError("sec_rgy_parse_group", cferror, rgyStatus);
 }
      }
      break;
    case sec_acl_e_type_foreign_user:
      {
 CfLog(cferror,
       "acl type sec_acl_e_type_foreign_user not supported yet.\n",
       "");
 rgyStatus = sec_acl_not_implemented;
      }
      break;
    case sec_acl_e_type_foreign_group:
      {
 CfLog(cferror,
       "acl type sec_acl_e_type_foreign_group not supported yet\n",
       "");
 rgyStatus = sec_acl_not_implemented;
      }
      break;
    case sec_acl_e_type_foreign_other:
      {
 CfLog(cferror,
       "acl type sec_acl_e_type_foreign_other not supported yet.\n",
       "");
 rgyStatus = sec_acl_not_implemented;
      }
      break;
    default:
      {
 CfLog(cferror,
       "Unknown acl type in nameToUUID!\n",
       "");
 rgyStatus = sec_acl_invalid_acl_type;
      }
      break;
    }

    sec_rgy_site_close(rgySite, &fooStatus);
    if (fooStatus != error_status_ok) {
      logError("sec_rgy_site_close", cferror, fooStatus);
    }

  } else {

    logError("sec_rgy_site_open", cferror, rgyStatus);
  }

  *status = rgyStatus;
}

/*
** setupACL()
**
** dceACLData structure setup routine.  Allocate new ACL structure of the
** requested size at aclData->new, saves old value of aclData->list.sec_acls[0]
** at aclData->old for subsequent usage by resetACL(), sets
** aclData->list.sec_acls[0] to the new ACL structure allocated for
** replacement usage.
**
** Status returned via provided status parameter.
*/

static void
setupACL(int              aclNumEntries,
  struct aclData * aclData,
  error_status_t * status)
{
  if (
      (aclData->new = (sec_acl_t *)calloc(1, sizeof(sec_acl_t))) &&
      (aclData->new->sec_acl_entries
       = (sec_acl_entry_t *)calloc(aclNumEntries,
       sizeof(sec_acl_entry_t)))
      ) {

    aclData->old = aclData->list.sec_acls[0];
    
    aclData->new->default_realm        = aclData->old->default_realm;
    aclData->new->sec_acl_manager_type = aclData->old->sec_acl_manager_type;
    aclData->new->num_entries          = 0;
    
    aclData->list.sec_acls[0] = aclData->new;
    
    *status = error_status_ok;
    
  } else {

    CfLog(cferror, "Unable to allocate ACL storage\n", "");
    *status = sec_acl_cant_allocate_memory;
  }
}

/*
** resetACL()
**
** Return dceACLData structure to initial value, i.e., return the original
** primary ACL to the appropriate location and free the modified new
** ACL that was allocated by setupACL().
**
** Notes: This routine should only be called after a successful call to
**        setupACL().
*/

static void
resetACL(struct aclData * aclData)
{
  aclData->list.sec_acls[0] = aclData->old;
  free(aclData->new->sec_acl_entries);
  free(aclData->new);
}

/*
** equalACLEntry()
**
** Compare two sec_acl_entry_ts, returning TRUE if they match in
** permissions, type, and uuid, FALSE otherwise.
*/

static boolean32
equalACLEntry(sec_acl_entry_t * lhs,
       sec_acl_entry_t * rhs,
       error_status_t  * status)
{
  return ((lhs->perms == rhs->perms) &&
   (lhs->entry_info.entry_type == rhs->entry_info.entry_type) &&
   (uuid_equal(&(lhs->entry_info.tagged_union.id.uuid),
        &(rhs->entry_info.tagged_union.id.uuid),
        status)));
}

/*
** equalACLEntries()
**
** Compare to sec_acl_ts, returning TRUE if they match in number and
** composition of ACL entries, FALSE otherwise.
**
** Notes: The provided sec_acl_ts may have equivalent entries, but it
**        is somewhat unlikely that they are in equivalent order; thus
**        an exhaustive search must be performed to determine equality.
*/

static boolean32
equalACLEntries(sec_acl_t      * lhs,
  sec_acl_t      * rhs,
  error_status_t * status)
{
  boolean32  equalEntry;
  unsigned32 lhsIdx;
  unsigned32 rhsIdx;

  /* Obviously, if the number of entries in the ACLS aren't equal, then
  ** they aren't equal ACLs.
  */

  if (lhs->num_entries != rhs->num_entries) return(FALSE);

  /*
  ** ACL entries have the same number of entries, which is promising.
  ** search the rhs entries for items exactly matching all lhs entries.
  ** If we don't find an equivalent rhs entry for a given lhs entry,
  ** then the ACLs aren't equal; return immediately in this case.
  */

  for (lhsIdx = 0; lhsIdx < lhs->num_entries; lhsIdx++) {
    equalEntry = FALSE;
    for (rhsIdx = 0; rhsIdx < rhs->num_entries; rhsIdx++) {
      if (equalACLEntry(&(lhs->sec_acl_entries[lhsIdx]),
   &(rhs->sec_acl_entries[rhsIdx]),
   status)) {
 equalEntry = TRUE;
 break;
      }
    }
    if (equalEntry == FALSE) return(FALSE);
  }

  /* If we've made it this far, then either the ACLs are equal
  ** in all respects, or some error occurred, as indicated by
  ** the status parameter.  Let the caller deal with it...
  */

  return(TRUE);
}

/*
** copyACLEntries()
**
** Copy source sec_acl_t entries to target sec_acl_t.
**
** Notes: Target must contain adequate storage space.
*/

static void
copyACLEntries(sec_acl_t const * source,
        sec_acl_t       * target)
{
  int numEntries = source->num_entries;
  int curEntry; 

  target->num_entries = numEntries;

  for (curEntry = 0; curEntry < numEntries; curEntry++) {
    target->sec_acl_entries[curEntry].entry_info.entry_type =
      source->sec_acl_entries[curEntry].entry_info.entry_type;
    target->sec_acl_entries[curEntry].perms =
      source->sec_acl_entries[curEntry].perms;
    target->sec_acl_entries[curEntry].entry_info.tagged_union =
      source->sec_acl_entries[curEntry].entry_info.tagged_union;
  }
}

/*****************************************************************************/

#endif /* HAVE_DCE_DACLIF_H */


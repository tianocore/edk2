/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  StringParse.c  

Abstract:

  Routines for parsing HII string packs

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Tiano.h"
#include "EfiUtilityMsgs.h"
#include "EfiInternalFormRepresentation.h"
#include "Hii.h"
#include "StringParse.h"
#include "HiiPack.h"

typedef struct _STRING_PACK_RECORD {
  struct _STRING_PACK_RECORD  *Next;
  int                         Handle;
  EFI_GUID                    PackageGuid;
  EFI_GUID                    FormsetGuid;
  EFI_HII_STRING_PACK         *StringPack;
  int                         StringPackSize;
  int                         NumStringPacks;
} STRING_PACK_RECORD;

static STRING_PACK_RECORD *mStringPacks = NULL;

STATUS
StringGetPack (
  int                 Handle,           // matches handle passed in with StringParsePack()
  EFI_HII_STRING_PACK **StringPack,     // returned pointer to string pack
  int                 *StringPackSize,  // sizeof buffer pointed to by StringPack
  int                 *NumStringPacks,  // in the array pointed to by StringPack
  EFI_GUID            *FormsetGuid,
  EFI_GUID            *PackageGuid
  )
/*++

Routine Description:

  Get a string pack given to us previously
  
Arguments:
  Handle            - handle of string pack to get
  StringPack        - outgoing pointer to string pack on the given handle
  StringPackSize    - outgoing size of string pack pointed to by StringPack
  NumStringPacks    - outgoing number of string packs in StringPack[] array
  FormsetGuid       - outgoing GUID passed in with the string pack when it was parsed
  PackageGuid       - outgoing GUID passed in with the string pack when it was parsed

Returns:

  STATUS_SUCCESS  - string pack with matching handle was found
  STATUS_ERROR    - otherwise
  
--*/
{
  STRING_PACK_RECORD  *Rec;

  for (Rec = mStringPacks; Rec != NULL; Rec = Rec->Next) {
    if (Rec->Handle == Handle) {
      *StringPack     = Rec->StringPack;
      *StringPackSize = Rec->StringPackSize;
      *NumStringPacks = Rec->NumStringPacks;
      return STATUS_SUCCESS;
    }
  }

  return STATUS_ERROR;
}

STATUS
StringParsePack (
  int                   Handle,
  EFI_HII_STRING_PACK   *StringPack,
  EFI_GUID              *FormsetGuid,
  EFI_GUID              *PackageGuid
  )
/*++

Routine Description:

  Parse a string pack, saving the information for later retrieval by the caller
  
Arguments:
  Handle            - handle of string pack
  StringPack        - pointer to string pack array to parse
  FormsetGuid       - GUID of the string pack
  PackageGuid       - package GUID from the HII data table from which this string pack orginated

Returns:

  STATUS_SUCCESS  - Stringpack processed successfully
  STATUS_ERROR    - otherwise
  
--*/
{
  STRING_PACK_RECORD  *Rec;

  STRING_PACK_RECORD  *TempRec;
  int                 PackSize;
  EFI_HII_STRING_PACK *TempPack;
  //
  // Allocate a new string pack record
  //
  Rec = (STRING_PACK_RECORD *) malloc (sizeof (STRING_PACK_RECORD));
  if (Rec == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }

  memset (Rec, 0, sizeof (STRING_PACK_RECORD));
  Rec->Handle = Handle;
  if (PackageGuid != NULL) {
    memcpy (&Rec->PackageGuid, PackageGuid, sizeof (EFI_GUID));
  }

  if (FormsetGuid != NULL) {
    memcpy (&Rec->FormsetGuid, FormsetGuid, sizeof (EFI_GUID));
  }
  //
  // Walk the string packs to find the terminator
  //
  TempPack  = StringPack;
  PackSize  = 0;
  while (TempPack->Header.Length > 0) {
    if (TempPack->Header.Type != EFI_HII_STRING) {
      Error (NULL, 0, 0, "found a non-string pack in the string pack array", NULL);
      free (Rec);
      return STATUS_ERROR;
    }

    PackSize += TempPack->Header.Length;
    Rec->NumStringPacks++;
    TempPack = (EFI_HII_STRING_PACK *) ((char *) TempPack + TempPack->Header.Length);
  }
  //
  // Add space for the terminator
  //
  PackSize += sizeof (EFI_HII_STRING_PACK);
  Rec->StringPackSize = PackSize;
  //
  // Make a copy of the incoming string pack
  //
  Rec->StringPack = (EFI_HII_STRING_PACK *) malloc (PackSize);
  if (Rec->StringPack == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    free (Rec);
    return STATUS_ERROR;
  }

  memcpy ((void *) Rec->StringPack, StringPack, PackSize);
  //
  // Add this record to our list
  //
  if (mStringPacks == NULL) {
    mStringPacks = Rec;
  } else {
    for (TempRec = mStringPacks; TempRec->Next != NULL; TempRec = TempRec->Next)
      ;
    TempRec->Next = Rec;
  }
  free (Rec->StringPack);
  free (Rec);
  return STATUS_SUCCESS;
}

STATUS
StringInit (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
{
  StringEnd ();
  return STATUS_SUCCESS;
}

STATUS
StringEnd (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
{
  STRING_PACK_RECORD  *Next;
  //
  // Free up all the memory we've allocated
  //
  while (mStringPacks != NULL) {
    if (mStringPacks->StringPack != NULL) {
      free (mStringPacks->StringPack);
    }

    Next = mStringPacks->Next;
    free (mStringPacks);
    mStringPacks = Next;
  }

  return STATUS_SUCCESS;
}

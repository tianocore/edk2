/**@file

  This file contains the keyboard processing code to the HII database.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiDatabase.h"

EFI_GUID *
GetGuidOfFirstFormset (
  CONST EFI_HII_FORM_PACKAGE * FormPackage
) 
{
  UINT8             *StartOfNextPackage;
  EFI_IFR_OP_HEADER *OpCodeData;

  StartOfNextPackage = (UINT8 *) FormPackage + FormPackage->Header.Length;
  OpCodeData = (EFI_IFR_OP_HEADER *) (FormPackage + 1);

  while ((UINT8 *) OpCodeData < StartOfNextPackage) {
    if (OpCodeData->OpCode == EFI_IFR_FORM_SET_OP) {
      return &(((EFI_IFR_FORM_SET *) OpCodeData)->Guid);
    }
    OpCodeData = (EFI_IFR_OP_HEADER *) ((UINT8 *) OpCodeData + OpCodeData->Length);
  }

  ASSERT (FALSE);

  return NULL;
}

EFI_HII_HANDLE
FrameworkHiiHandleToUefiHiiHandle (
  IN CONST EFI_HII_THUNK_PRIVATE_DATA *Private,
  IN FRAMEWORK_EFI_HII_HANDLE          FrameworkHiiHandle
  )
{
  LIST_ENTRY                                *ListEntry;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY  *HandleMapEntry;

  ASSERT (FrameworkHiiHandle != (FRAMEWORK_EFI_HII_HANDLE) 0);
  ASSERT (Private != NULL);

  for (ListEntry = Private->HiiThunkHandleMappingDBListHead.ForwardLink;
       ListEntry != &Private->HiiThunkHandleMappingDBListHead;
       ListEntry = ListEntry->ForwardLink
       ) {

    HandleMapEntry = HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_FROM_LISTENTRY (ListEntry);

    if (FrameworkHiiHandle == HandleMapEntry->FrameworkHiiHandle) {
      return HandleMapEntry->UefiHiiHandle;
    }
  }
  
  return (EFI_HII_HANDLE) NULL;
}


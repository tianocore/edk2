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
      return AllocateCopyPool (sizeof(EFI_GUID), &(((EFI_IFR_FORM_SET *) OpCodeData)->Guid));
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
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY  *HandleMapEntry;

  ASSERT (FrameworkHiiHandle != (FRAMEWORK_EFI_HII_HANDLE) 0);
  ASSERT (Private != NULL);

  HandleMapEntry = FrameworkHiiHandleToMapDatabaseEntry (Private, FrameworkHiiHandle);

  if (HandleMapEntry != NULL) {
    return HandleMapEntry->UefiHiiHandle;
  }
  
  return (EFI_HII_HANDLE) NULL;
}


HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY *
FrameworkHiiHandleToMapDatabaseEntry (
  IN CONST EFI_HII_THUNK_PRIVATE_DATA *Private,
  IN FRAMEWORK_EFI_HII_HANDLE          FrameworkHiiHandle
  )
{
  LIST_ENTRY                 *ListEntry;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY *HandleMapEntry;

  for (ListEntry = Private->HiiThunkHandleMappingDBListHead.ForwardLink;
       ListEntry != &Private->HiiThunkHandleMappingDBListHead;
       ListEntry = ListEntry->ForwardLink
       ) {
    HandleMapEntry = HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_FROM_LISTENTRY (ListEntry);

    if (FrameworkHiiHandle == HandleMapEntry->FrameworkHiiHandle) {
      return HandleMapEntry;
    }
  }

  return (HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY *) NULL;
}

HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY *
UefiHiiHandleToMapDatabaseEntry (
  IN CONST EFI_HII_THUNK_PRIVATE_DATA *Private,
  IN EFI_HII_HANDLE                   UefiHiiHandle
  )
{
  LIST_ENTRY                 *ListEntry;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY *HandleMapEntry;

  for (ListEntry = Private->HiiThunkHandleMappingDBListHead.ForwardLink;
       ListEntry != &Private->HiiThunkHandleMappingDBListHead;
       ListEntry = ListEntry->ForwardLink
       ) {
    HandleMapEntry = HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_FROM_LISTENTRY (ListEntry);

    if (UefiHiiHandle == HandleMapEntry->UefiHiiHandle) {
      return HandleMapEntry;
    }
  }

  return (HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY *) NULL;
}

EFI_HII_HANDLE *
TagGuidToUefiIfrHiiHandle (
  IN CONST EFI_HII_THUNK_PRIVATE_DATA *Private,
  IN CONST EFI_GUID                   *Guid
  )
{
  LIST_ENTRY                 *ListEntry;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY *HandleMapEntry;

  for (ListEntry = Private->HiiThunkHandleMappingDBListHead.ForwardLink;
       ListEntry != &Private->HiiThunkHandleMappingDBListHead;
       ListEntry = ListEntry->ForwardLink
       ) {
    HandleMapEntry = HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_FROM_LISTENTRY (ListEntry);

    if (CompareGuid (Guid, &HandleMapEntry->TagGuid) && HandleMapEntry->DoesPackageListImportStringPackages) {
      return HandleMapEntry->UefiHiiHandle;
    }
  }

  return (EFI_HII_HANDLE *) NULL;
  
}

BOOLEAN
IsFrameworkHiiDatabaseHandleDepleted (
  IN CONST EFI_HII_THUNK_PRIVATE_DATA *Private
  )
{
  return (BOOLEAN) (Private->StaticHiiHandle == (UINTN) Private->StaticPureUefiHiiHandle);
}

EFI_STATUS

AssignHiiHandle (
  IN OUT EFI_HII_THUNK_PRIVATE_DATA *Private,
  OUT    FRAMEWORK_EFI_HII_HANDLE   *Handle
  )
{
  ASSERT (Handle != NULL);

  *Handle = Private->StaticHiiHandle;
  Private->StaticHiiHandle += 1;

  if (IsFrameworkHiiDatabaseHandleDepleted (Private)) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AssignPureUefiHiiHandle (
  IN OUT EFI_HII_THUNK_PRIVATE_DATA *Private,
    OUT    FRAMEWORK_EFI_HII_HANDLE   *Handle
  )
{
  ASSERT (Handle != NULL);

  *Handle = Private->StaticPureUefiHiiHandle;
  Private->StaticPureUefiHiiHandle -= 1;

  if (IsFrameworkHiiDatabaseHandleDepleted (Private)) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}


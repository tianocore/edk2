/** @file
  Shared variable store parsing and HOB store access helpers.

Copyright (c) 2006 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VariableCommon.h"

#include <Pi/PiMultiPhase.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>

/**

  Gets the pointer to the first variable header in given variable store area.

  @param[in] VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the first variable header.

**/
VARIABLE_HEADER *
GetStartPointer (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  )
{
  //
  // The start of variable store.
  //
  return (VARIABLE_HEADER *)HEADER_ALIGN (VarStoreHeader + 1);
}

/**

  Gets the pointer to the end of the variable storage area.

  This function gets pointer to the end of the variable storage
  area, according to the input variable store header.

  @param[in] VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the end of the variable storage area.

**/
VARIABLE_HEADER *
GetEndPointer (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  )
{
  //
  // The end of variable store
  //
  return (VARIABLE_HEADER *)HEADER_ALIGN ((UINTN)VarStoreHeader + VarStoreHeader->Size);
}

/**
  This code gets the size of variable header.

  @param AuthFlag   Authenticated variable flag.

  @return Size of variable header in bytes in type UINTN.

**/
UINTN
GetVariableHeaderSize (
  IN  BOOLEAN  AuthFlag
  )
{
  UINTN  Value;

  if (AuthFlag) {
    Value = sizeof (AUTHENTICATED_VARIABLE_HEADER);
  } else {
    Value = sizeof (VARIABLE_HEADER);
  }

  return Value;
}

/**
  This code gets the size of name of variable.

  @param  Variable  Pointer to the Variable Header.
  @param  AuthFlag  Authenticated variable flag.

  @return Size of variable in bytes in type UINTN.

**/
UINTN
NameSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFlag
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (AuthFlag) {
    if ((AuthVariable->State == (UINT8)(-1)) ||
        (AuthVariable->DataSize == (UINT32)(-1)) ||
        (AuthVariable->NameSize == (UINT32)(-1)) ||
        (AuthVariable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)AuthVariable->NameSize;
  } else {
    if ((Variable->State == (UINT8)(-1)) ||
        (Variable->DataSize == (UINT32)(-1)) ||
        (Variable->NameSize == (UINT32)(-1)) ||
        (Variable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)Variable->NameSize;
  }
}

/**
  This code gets the size of data of variable.

  @param  Variable  Pointer to the Variable Header.
  @param  AuthFlag  Authenticated variable flag.

  @return Size of variable in bytes in type UINTN.

**/
UINTN
DataSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFlag
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (AuthFlag) {
    if ((AuthVariable->State == (UINT8)(-1)) ||
        (AuthVariable->DataSize == (UINT32)(-1)) ||
        (AuthVariable->NameSize == (UINT32)(-1)) ||
        (AuthVariable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)AuthVariable->DataSize;
  } else {
    if ((Variable->State == (UINT8)(-1)) ||
        (Variable->DataSize == (UINT32)(-1)) ||
        (Variable->NameSize == (UINT32)(-1)) ||
        (Variable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)Variable->DataSize;
  }
}

/**
  This code gets the pointer to the variable name.

  @param   Variable  Pointer to the Variable Header.
  @param   AuthFlag  Authenticated variable flag.

  @return  A CHAR16* pointer to Variable Name.

**/
CHAR16 *
GetVariableNamePtr (
  IN VARIABLE_HEADER  *Variable,
  IN BOOLEAN          AuthFlag
  )
{
  return (CHAR16 *)((UINTN)Variable + GetVariableHeaderSize (AuthFlag));
}

/**
  This code gets the pointer to the variable data.

  @param   Variable         Pointer to the Variable Header.
  @param   VariableHeader   Pointer to the Variable Header that has consecutive content.
  @param   AuthFlag         Authenticated variable flag.

  @return  A UINT8* pointer to Variable Data.

**/
UINT8 *
GetVariableDataPtr (
  IN  VARIABLE_HEADER  *Variable,
  IN  VARIABLE_HEADER  *VariableHeader,
  IN  BOOLEAN          AuthFlag
  )
{
  UINTN  Value;

  //
  // Be careful about pad size for alignment
  //
  Value  =  (UINTN)GetVariableNamePtr (Variable, AuthFlag);
  Value += NameSizeOfVariable (VariableHeader, AuthFlag);
  Value += GET_PAD_SIZE (NameSizeOfVariable (VariableHeader, AuthFlag));

  return (UINT8 *)Value;
}

/**
  This code gets the pointer to the next variable header.

  @param  StoreInfo         Pointer to variable store info structure.
  @param  Variable          Pointer to the Variable Header.
  @param  VariableHeader    Pointer to the Variable Header that has consecutive content.

  @return  A VARIABLE_HEADER* pointer to next variable header.

**/
VARIABLE_HEADER *
GetNextVariablePtr (
  IN  VARIABLE_STORE_INFO  *StoreInfo,
  IN  VARIABLE_HEADER      *Variable,
  IN  VARIABLE_HEADER      *VariableHeader
  )
{
  EFI_PHYSICAL_ADDRESS  TargetAddress;
  EFI_PHYSICAL_ADDRESS  SpareAddress;
  UINTN                 Value;

  Value  =  (UINTN)GetVariableDataPtr (Variable, VariableHeader, StoreInfo->AuthFlag);
  Value += DataSizeOfVariable (VariableHeader, StoreInfo->AuthFlag);
  Value += GET_PAD_SIZE (DataSizeOfVariable (VariableHeader, StoreInfo->AuthFlag));
  //
  // Be careful about pad size for alignment
  //
  Value = HEADER_ALIGN (Value);

  if (StoreInfo->FtwLastWriteData != NULL) {
    TargetAddress = StoreInfo->FtwLastWriteData->TargetAddress;
    SpareAddress  = StoreInfo->FtwLastWriteData->SpareAddress;
    if (((UINTN)Variable < (UINTN)TargetAddress) && (Value >= (UINTN)TargetAddress)) {
      //
      // Next variable is in spare block.
      //
      Value = (UINTN)SpareAddress + (Value - (UINTN)TargetAddress);
    }
  }

  return (VARIABLE_HEADER *)Value;
}

/**
  Get HOB variable store.

  @param[out] StoreInfo             Return the store info.
  @param[out] VariableStoreHeader   Return variable store header.

**/
VOID
GetHobVariableStore (
  OUT VARIABLE_STORE_INFO    *StoreInfo,
  OUT VARIABLE_STORE_HEADER  **VariableStoreHeader
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  //
  // Make sure there is no more than one Variable HOB.
  //
  DEBUG_CODE_BEGIN ();
  GuidHob = GetFirstGuidHob (&gEfiAuthenticatedVariableGuid);
  if (GuidHob != NULL) {
    if ((GetNextGuidHob (&gEfiAuthenticatedVariableGuid, GET_NEXT_HOB (GuidHob)) != NULL)) {
      DEBUG ((DEBUG_ERROR, "ERROR: Found two Auth Variable HOBs\n"));
      ASSERT (FALSE);
    } else if (GetFirstGuidHob (&gEfiVariableGuid) != NULL) {
      DEBUG ((DEBUG_ERROR, "ERROR: Found one Auth + one Normal Variable HOBs\n"));
      ASSERT (FALSE);
    }
  } else {
    GuidHob = GetFirstGuidHob (&gEfiVariableGuid);
    if (GuidHob != NULL) {
      if ((GetNextGuidHob (&gEfiVariableGuid, GET_NEXT_HOB (GuidHob)) != NULL)) {
        DEBUG ((DEBUG_ERROR, "ERROR: Found two Normal Variable HOBs\n"));
        ASSERT (FALSE);
      }
    }
  }

  DEBUG_CODE_END ();

  GuidHob = GetFirstGuidHob (&gEfiAuthenticatedVariableGuid);
  if (GuidHob != NULL) {
    *VariableStoreHeader = (VARIABLE_STORE_HEADER *)GET_GUID_HOB_DATA (GuidHob);
    StoreInfo->AuthFlag  = TRUE;
  } else {
    GuidHob = GetFirstGuidHob (&gEfiVariableGuid);
    if (GuidHob != NULL) {
      *VariableStoreHeader = (VARIABLE_STORE_HEADER *)GET_GUID_HOB_DATA (GuidHob);
      StoreInfo->AuthFlag  = FALSE;
    }
  }
}

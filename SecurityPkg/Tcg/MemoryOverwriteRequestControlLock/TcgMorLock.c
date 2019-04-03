/** @file
  TCG MOR (Memory Overwrite Request) Lock Control Driver.

  This driver initilize MemoryOverwriteRequestControlLock variable.
  This module will add Variable Hook and allow MemoryOverwriteRequestControlLock variable set only once.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Guid/MemoryOverwriteControl.h>
#include <IndustryStandard/MemoryOverwriteRequestControlLock.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include "TcgMorLock.h"

typedef struct {
  CHAR16                                 *VariableName;
  EFI_GUID                               *VendorGuid;
} VARIABLE_TYPE;

VARIABLE_TYPE  mMorVariableType[] = {
  {MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,      &gEfiMemoryOverwriteControlDataGuid},
  {MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,  &gEfiMemoryOverwriteRequestControlLockGuid},
};

/**
  Returns if this is MOR related variable.

  @param  VariableName the name of the vendor's variable, it's a Null-Terminated Unicode String
  @param  VendorGuid   Unify identifier for vendor.

  @retval  TRUE            The variable is MOR related.
  @retval  FALSE           The variable is NOT MOR related.
**/
BOOLEAN
IsAnyMorVariable (
  IN CHAR16                                 *VariableName,
  IN EFI_GUID                               *VendorGuid
  )
{
  UINTN   Index;

  for (Index = 0; Index < sizeof(mMorVariableType)/sizeof(mMorVariableType[0]); Index++) {
    if ((StrCmp (VariableName, mMorVariableType[Index].VariableName) == 0) &&
        (CompareGuid (VendorGuid, mMorVariableType[Index].VendorGuid))) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
  Returns if this is MOR lock variable.

  @param  VariableName the name of the vendor's variable, it's a Null-Terminated Unicode String
  @param  VendorGuid   Unify identifier for vendor.

  @retval  TRUE            The variable is MOR lock variable.
  @retval  FALSE           The variable is NOT MOR lock variable.
**/
BOOLEAN
IsMorLockVariable (
  IN CHAR16                                 *VariableName,
  IN EFI_GUID                               *VendorGuid
  )
{
  if ((StrCmp (VariableName, MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME) == 0) &&
      (CompareGuid (VendorGuid, &gEfiMemoryOverwriteRequestControlLockGuid))) {
    return TRUE;
  }
  return FALSE;
}

/**
  This service is a checker handler for the UEFI Runtime Service SetVariable()

  @param  VariableName the name of the vendor's variable, as a
                       Null-Terminated Unicode String
  @param  VendorGuid   Unify identifier for vendor.
  @param  Attributes   Point to memory location to return the attributes of variable. If the point
                       is NULL, the parameter would be ignored.
  @param  DataSize     The size in bytes of Data-Buffer.
  @param  Data         Point to the content of the variable.

  @retval  EFI_SUCCESS            The firmware has successfully stored the variable and its data as
                                  defined by the Attributes.
  @retval  EFI_INVALID_PARAMETER  An invalid combination of attribute bits was supplied, or the
                                  DataSize exceeds the maximum allowed.
  @retval  EFI_INVALID_PARAMETER  VariableName is an empty Unicode string.
  @retval  EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval  EFI_DEVICE_ERROR       The variable could not be saved due to a hardware failure.
  @retval  EFI_WRITE_PROTECTED    The variable in question is read-only.
  @retval  EFI_WRITE_PROTECTED    The variable in question cannot be deleted.
  @retval  EFI_SECURITY_VIOLATION The variable could not be written due to EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS
                                  set but the AuthInfo does NOT pass the validation check carried
                                  out by the firmware.
  @retval  EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.

**/
EFI_STATUS
EFIAPI
SetVariableCheckHandlerMor (
  IN CHAR16     *VariableName,
  IN EFI_GUID   *VendorGuid,
  IN UINT32     Attributes,
  IN UINTN      DataSize,
  IN VOID       *Data
  )
{
  UINTN       MorLockDataSize;
  BOOLEAN     MorLock;
  EFI_STATUS  Status;

  //
  // do not handle non-MOR variable
  //
  if (!IsAnyMorVariable (VariableName, VendorGuid)) {
    return EFI_SUCCESS;
  }

  MorLockDataSize = sizeof(MorLock);
  Status = InternalGetVariable (
             MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
             &gEfiMemoryOverwriteRequestControlLockGuid,
             NULL,
             &MorLockDataSize,
             &MorLock
             );
  if (!EFI_ERROR (Status) && MorLock) {
    //
    // If lock, deny access
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // Delete not OK
  //
  if ((DataSize != sizeof(UINT8)) || (Data == NULL) || (Attributes == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // check format
  //
  if (IsMorLockVariable(VariableName, VendorGuid)) {
    //
    // set to any other value not OK
    //
    if ((*(UINT8 *)Data != 1) && (*(UINT8 *)Data != 0)) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Or grant access
  //
  return EFI_SUCCESS;
}

/**
  Entry Point for MOR Lock Control driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  A Pointer to the EFI System Table.

  @retval EFI_SUCEESS
  @return Others          Some error occurs.
**/
EFI_STATUS
EFIAPI
MorLockDriverInit (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT8       Data;

  Data = 0;
  Status = InternalSetVariable (
             MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
             &gEfiMemoryOverwriteRequestControlLockGuid,
             EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
             1,
             &Data
             );
  return Status;
}

/** @file
  TCG MOR (Memory Overwrite Request) Lock Control support (DXE version).

  This module clears MemoryOverwriteRequestControlLock variable to indicate
  MOR lock control unsupported.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Guid/MemoryOverwriteControl.h>
#include <IndustryStandard/MemoryOverwriteRequestControlLock.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include "Variable.h"

extern EDKII_VARIABLE_LOCK_PROTOCOL     mVariableLock;

/**
  This service is an MOR/MorLock checker handler for the SetVariable().

  @param  VariableName the name of the vendor's variable, as a
                       Null-Terminated Unicode String
  @param  VendorGuid   Unify identifier for vendor.
  @param  Attributes   Point to memory location to return the attributes of variable. If the point
                       is NULL, the parameter would be ignored.
  @param  DataSize     The size in bytes of Data-Buffer.
  @param  Data         Point to the content of the variable.

  @retval  EFI_SUCCESS            The MOR/MorLock check pass, and Variable driver can store the variable data.
  @retval  EFI_INVALID_PARAMETER  The MOR/MorLock data or data size or attributes is not allowed for MOR variable.
  @retval  EFI_ACCESS_DENIED      The MOR/MorLock is locked.
  @retval  EFI_ALREADY_STARTED    The MorLock variable is handled inside this function.
                                  Variable driver can just return EFI_SUCCESS.
**/
EFI_STATUS
SetVariableCheckHandlerMor (
  IN CHAR16     *VariableName,
  IN EFI_GUID   *VendorGuid,
  IN UINT32     Attributes,
  IN UINTN      DataSize,
  IN VOID       *Data
  )
{
  //
  // Just let it pass. No need provide protection for DXE version.
  //
  return EFI_SUCCESS;
}

/**
  Initialization for MOR Lock Control.

  @retval EFI_SUCEESS     MorLock initialization success.
  @return Others          Some error occurs.
**/
EFI_STATUS
MorLockInit (
  VOID
  )
{
  //
  // Always clear variable to report unsupported to OS.
  // The reason is that the DXE version is not proper to provide *protection*.
  // BIOS should use SMM version variable driver to provide such capability.
  //
  VariableServiceSetVariable (
    MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME,
    &gEfiMemoryOverwriteRequestControlLockGuid,
    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
    0,
    NULL
    );

  //
  // Need set this variable to be read-only to prevent other module set it.
  //
  VariableLockRequestToLock (&mVariableLock, MEMORY_OVERWRITE_REQUEST_CONTROL_LOCK_NAME, &gEfiMemoryOverwriteRequestControlLockGuid);
  return EFI_SUCCESS;
}

/** @file
  LockBox SMM/MM driver.

  Caution: This module requires additional review when modified.
  This driver will have external input - communicate buffer in SMM mode.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  SmmLockBoxHandler(), SmmLockBoxRestore(), SmmLockBoxUpdate(), SmmLockBoxSave()
  will receive untrusted input and do basic validation.

Copyright (c) 2010 - 2024, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiSmm.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/LockBoxLib.h>

#include <Protocol/SmmReadyToLock.h>
#include <Protocol/SmmCommunication.h>
#include <Protocol/LockBox.h>
#include <Guid/SmmLockBox.h>

/**
  This function is an abstraction layer for implementation specific Mm buffer validation routine.

  @param Buffer  The buffer start address to be checked.
  @param Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid per processor architecture and not overlap with SMRAM.
  @retval FALSE This buffer is not valid per processor architecture or overlap with SMRAM.
**/
BOOLEAN
IsBufferOutsideMmValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  );

/**
  Dispatch function for SMM lock box save.

  Caution: This function may receive untrusted input.
  Restore buffer and length are external input, so this function will validate
  it is in SMRAM.

  @param LockBoxParameterSave  parameter of lock box save
**/
VOID
SmmLockBoxSave (
  IN EFI_SMM_LOCK_BOX_PARAMETER_SAVE  *LockBoxParameterSave
  );

/**
  Dispatch function for SMM lock box set attributes.

  @param LockBoxParameterSetAttributes  parameter of lock box set attributes
**/
VOID
SmmLockBoxSetAttributes (
  IN EFI_SMM_LOCK_BOX_PARAMETER_SET_ATTRIBUTES  *LockBoxParameterSetAttributes
  );

/**
  Dispatch function for SMM lock box update.

  Caution: This function may receive untrusted input.
  Restore buffer and length are external input, so this function will validate
  it is in SMRAM.

  @param LockBoxParameterUpdate  parameter of lock box update
**/
VOID
SmmLockBoxUpdate (
  IN EFI_SMM_LOCK_BOX_PARAMETER_UPDATE  *LockBoxParameterUpdate
  );

/**
  Dispatch function for SMM lock box restore.

  Caution: This function may receive untrusted input.
  Restore buffer and length are external input, so this function will validate
  it is in SMRAM.

  @param LockBoxParameterRestore  parameter of lock box restore
**/
VOID
SmmLockBoxRestore (
  IN EFI_SMM_LOCK_BOX_PARAMETER_RESTORE  *LockBoxParameterRestore
  );

/**
  Dispatch function for SMM lock box restore all in place.

  @param LockBoxParameterRestoreAllInPlace  parameter of lock box restore all in place
**/
VOID
SmmLockBoxRestoreAllInPlace (
  IN EFI_SMM_LOCK_BOX_PARAMETER_RESTORE_ALL_IN_PLACE  *LockBoxParameterRestoreAllInPlace
  );

/**
  Dispatch function for a Software SMI handler.

  Caution: This function may receive untrusted input.
  Communicate buffer and buffer size are external input, so this function will do basic validation.

  @param DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param Context         Points to an optional handler context which was specified when the
                         handler was registered.
  @param CommBuffer      A pointer to a collection of data in memory that will
                         be conveyed from a non-SMM environment into an SMM environment.
  @param CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS Command is handled successfully.

**/
EFI_STATUS
EFIAPI
SmmLockBoxHandler (
  IN EFI_HANDLE  DispatchHandle,
  IN CONST VOID  *Context         OPTIONAL,
  IN OUT VOID    *CommBuffer      OPTIONAL,
  IN OUT UINTN   *CommBufferSize  OPTIONAL
  );

/**
  Smm Ready To Lock event notification handler.

  It sets a flag indicating that SMRAM has been locked.

  @param[in] Protocol   Points to the protocol's unique identifier.
  @param[in] Interface  Points to the interface instance.
  @param[in] Handle     The handle on which the interface was installed.

  @retval EFI_SUCCESS   Notification handler runs successfully.
 **/
EFI_STATUS
EFIAPI
SmmReadyToLockEventNotify (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  );

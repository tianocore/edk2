/** @file

  Implementation functions and structures for var check services.
  This file provides functions and structures to register and handle variable checks
  in the Standalone MM environment, specifically for HII variables.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmServicesTableLib.h>
#include <Protocol/MmCommunication.h>
#include <Library/VarCheckLib.h>

#include "VarCheckHii.h"
#include "VarCheckHiiLibCommon.h"

//
// In the standalone setup, mVarCheckHiiBin is used for sending, while mVarCheckHiiBinMmReceived is used for receiving,
// while in the traditional setup, mVarCheckHiiBin is used for both sending and receiving.
//
VAR_CHECK_HII_VARIABLE_HEADER  *mMmReceivedVarCheckHiiBin         = NULL;
UINTN                          mMmReceivedVarCheckHiiBinSize      = 0;
EFI_GUID                       gVarCheckReceivedHiiBinHandlerGuid = VAR_CHECK_RECEIVED_HII_BIN_HANDLER_GUID;

/**
  Registers a handler for HII variable checks in MM environment.
  This function is intended to be called to register a handler for checking variables
  in the Standalone MM environment. It allocates memory for the variable
  check data and copies the data from the communication buffer.

  @param[in] DispatchHandle    The handle of the dispatch function.
  @param[in] Context           Optional context for the handler, not used in this implementation.
  @param CommBuffer        The buffer of data being passed in.
  @param CommBufferSize    The size of the data being passed in.
  @retval EFI_SUCCESS           Registration and memory allocation were successful.
  @retval EFI_INVALID_PARAMETER The CommBuffer or CommBufferSize is NULL.
  @retval EFI_ACCESS_DENIED     The buffer size is invalid or the buffer is in an invalid location.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation for the variable check data failed.

**/
EFI_STATUS
EFIAPI
VarCheckHiiLibReceiveHiiBinHandler (
  IN EFI_HANDLE  DispatchHandle,
  IN CONST VOID  *Context        OPTIONAL,
  IN OUT VOID    *CommBuffer     OPTIONAL,
  IN OUT UINTN   *CommBufferSize OPTIONAL
  )
{
  EFI_STATUS  Status;

  //
  // If input is invalid, stop processing this SMI
  //
  if ((CommBuffer == NULL) || (CommBufferSize == NULL)) {
    return EFI_SUCCESS;
  }

  mMmReceivedVarCheckHiiBinSize = *CommBufferSize;

  if (mMmReceivedVarCheckHiiBinSize < sizeof (VAR_CHECK_HII_VARIABLE_HEADER)) {
    DEBUG ((DEBUG_ERROR, "%a: MM Communication buffer size is invalid for this handler!\n", __func__));
    return EFI_ACCESS_DENIED;
  }

  mMmReceivedVarCheckHiiBin = AllocateZeroPool (mMmReceivedVarCheckHiiBinSize);
  if (mMmReceivedVarCheckHiiBin == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory for mVarCheckHiiBinMm\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (mMmReceivedVarCheckHiiBin, CommBuffer, mMmReceivedVarCheckHiiBinSize);
  if (DispatchHandle != NULL) {
    Status = gMmst->MmiHandlerUnRegister (DispatchHandle);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to unregister handler - %r!\n", __func__, Status));
  } else {
    DEBUG ((DEBUG_INFO, "%a: Handler unregistered successfully.\n", __func__));
  }

  return EFI_SUCCESS;
}

/**

  Sets the variable check handler for HII.
  This function registers a handler that will be invoked for variable checks
  in the HII environment. It allows for custom validation logic to be implemented
  for setting HII variables.
  @param[in] VariableName               Name of Variable to set.
  @param[in] VendorGuid                 Variable vendor GUID.
  @param[in] Attributes                 Attribute value of the variable.
  @param[in] DataSize                   Size of Data to set.
  @param[in] Data                       Data pointer.

**/
EFI_STATUS
EFIAPI
SetVariableCheckHandlerHii (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  return CheckHiiVariableCommon (mMmReceivedVarCheckHiiBin, mMmReceivedVarCheckHiiBinSize, VariableName, VendorGuid, Attributes, DataSize, Data);
}

/**
  Constructor function for variable check library in Standalone MM.
  This function registers a handler for variable checks and sets up the environment
  for variable checking in the Standalone MM environment.
  @param[in] ImageHandle       The firmware allocated handle for the EFI image.
  @param[in] SystemTable       A pointer to the EFI system table.
  @retval EFI_SUCCESS          The constructor executed successfully.
  @retval Others               An error occurred during execution.

**/
EFI_STATUS
EFIAPI
VarCheckHiiLibConstructorStandaloneMm (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  DispatchHandle;

  DEBUG ((DEBUG_INFO, "%a: starts.\n", __func__));
  //
  // Register a handler to recieve the HII variable checking data.
  //
  Status = gMmst->MmiHandlerRegister (VarCheckHiiLibReceiveHiiBinHandler, &gVarCheckReceivedHiiBinHandlerGuid, &DispatchHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to register handler - %r!\n", __func__, Status));

    return Status;
  }

  VarCheckLibRegisterAddressPointer ((VOID **)&mMmReceivedVarCheckHiiBin);
  VarCheckLibRegisterSetVariableCheckHandler (SetVariableCheckHandlerHii);
  DEBUG ((DEBUG_INFO, "%a: ends.\n", __func__));
  return EFI_SUCCESS;
}

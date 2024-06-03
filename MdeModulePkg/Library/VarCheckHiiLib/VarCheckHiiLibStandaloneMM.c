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

extern EFI_GUID                gEfiVariableCheckHiiCommunicationGuid;
VAR_CHECK_HII_VARIABLE_HEADER  *mVarCheckHiiBinMmRecieved    = NULL;
UINTN                          mVarCheckHiiBinMmSizeRecieved = 0;

/**
  Registers a handler for variable checks in MM environment.
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
VarCheckLibRegisterMmHandler (
  IN EFI_HANDLE  DispatchHandle,
  IN CONST VOID  *Context        OPTIONAL,
  IN OUT VOID    *CommBuffer     OPTIONAL,
  IN OUT UINTN   *CommBufferSize OPTIONAL
  )
{
  //
  // If input is invalid, stop processing this SMI
  //

  if ((CommBuffer == NULL) || (CommBufferSize == NULL)) {
    return EFI_SUCCESS;
  }

  mVarCheckHiiBinMmSizeRecieved = *CommBufferSize;

  if (mVarCheckHiiBinMmSizeRecieved < sizeof (VAR_CHECK_HII_VARIABLE_HEADER)) {
    DEBUG ((DEBUG_ERROR, "VarCheckLibRegisterMmHandler - MM Communication buffer size is invalid for this handler!\n"));
    return EFI_ACCESS_DENIED;
  }

  mVarCheckHiiBinMmRecieved = AllocateZeroPool (mVarCheckHiiBinMmSizeRecieved);
  if (mVarCheckHiiBinMmRecieved == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate memory for mVarCheckHiiBinMm2\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (mVarCheckHiiBinMmRecieved, CommBuffer, mVarCheckHiiBinMmSizeRecieved);
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
SetVariableCheckHandlerHiiMm (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  return CheckHiiVariableCommon (mVarCheckHiiBinMmRecieved, mVarCheckHiiBinMmSizeRecieved, VariableName, VendorGuid, Attributes, DataSize, Data);
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
VarCheckLibVarCheckLibConstructorStandaloneMM (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  DispatchHandle;

  // Register a handler to communicate the SmmCpuPerf data between MM and Non-MM

  Status = gMmst->MmiHandlerRegister (VarCheckLibRegisterMmHandler, &gEfiVariableCheckHiiCommunicationGuid, &DispatchHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "VarCheckLibRegisterMmHandler - Failed to register handler - %r!\n", Status));

    return Status;
  }

  VarCheckLibRegisterAddressPointer ((VOID **)&mVarCheckHiiBinMmRecieved);
  VarCheckLibRegisterSetVariableCheckHandler (SetVariableCheckHandlerHiiMm);

  return EFI_SUCCESS;
}

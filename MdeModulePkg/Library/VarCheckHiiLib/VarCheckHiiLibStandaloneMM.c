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
extern EFI_GUID  gEfiVariableCheckHiiCommunicationGuid;

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

EFI_STATUS
EFIAPI
CheckHiiVariableCommon (
  IN VAR_CHECK_HII_VARIABLE_HEADER  *HiiVariableBin,
  IN UINTN                          HiiVariableBinSize,
  IN CHAR16                         *VariableName,
  IN EFI_GUID                       *VendorGuid,
  IN UINT32                         Attributes,
  IN UINTN                          DataSize,
  IN VOID                           *Data
  )
{
  VAR_CHECK_HII_VARIABLE_HEADER  *HiiVariable;
  VAR_CHECK_HII_QUESTION_HEADER  *HiiQuestion;

  if (HiiVariableBin == NULL) {
    return EFI_SUCCESS;
  }

  if ((((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) && (DataSize == 0)) || (Attributes == 0)) {
    //
    // Do not check delete variable.
    //
  }

  //
  // For Hii Variable header align.
  //
  HiiVariable = (VAR_CHECK_HII_VARIABLE_HEADER *)HEADER_ALIGN (HiiVariableBin);
  while ((UINTN)HiiVariable < ((UINTN)HiiVariableBin + HiiVariableBinSize)) {
    if ((StrCmp ((CHAR16 *)(HiiVariable + 1), VariableName) == 0) &&
        (CompareGuid (&HiiVariable->Guid, VendorGuid)))
    {
      //
      // Found the Hii Variable that could be used to do check.
      //
      DEBUG ((DEBUG_INFO, "VarCheckHiiVariable - %s:%g with Attributes = 0x%08x Size = 0x%x\n", VariableName, VendorGuid, Attributes, DataSize));
      if (HiiVariable->Attributes != Attributes) {
        DEBUG ((DEBUG_INFO, "VarCheckHiiVariable fail for Attributes - 0x%08x\n", HiiVariable->Attributes));
        return EFI_SECURITY_VIOLATION;
      }

      if (DataSize == 0) {
        DEBUG ((DEBUG_INFO, "VarCheckHiiVariable - CHECK PASS with DataSize == 0 !\n"));
        return EFI_SUCCESS;
      }

      if (HiiVariable->Size != DataSize) {
        DEBUG ((DEBUG_INFO, "VarCheckHiiVariable fail for Size - 0x%x\n", HiiVariable->Size));
        return EFI_SECURITY_VIOLATION;
      }

      //
      // Do the check.
      // For Hii Question header align.
      //
      HiiQuestion = (VAR_CHECK_HII_QUESTION_HEADER *)HEADER_ALIGN (((UINTN)HiiVariable + HiiVariable->HeaderLength));
      while ((UINTN)HiiQuestion < ((UINTN)HiiVariable + HiiVariable->Length)) {
        if (!VarCheckHiiQuestion (HiiQuestion, Data, DataSize)) {
          return EFI_SECURITY_VIOLATION;
        }

        //
        // For Hii Question header align.
        //
        HiiQuestion = (VAR_CHECK_HII_QUESTION_HEADER *)HEADER_ALIGN (((UINTN)HiiQuestion + HiiQuestion->Length));
      }

      DEBUG ((DEBUG_INFO, "VarCheckHiiVariable - ALL CHECK PASS!\n"));
      return EFI_SUCCESS;
    }

    //
    // For Hii Variable header align.
    //
    HiiVariable = (VAR_CHECK_HII_VARIABLE_HEADER *)HEADER_ALIGN (((UINTN)HiiVariable + HiiVariable->Length));
  }

  // Not found, so pass.
  return EFI_SUCCESS;
}

/**
  Sets the variable check handler for HII.
  This function registers a handler that will be invoked for variable checks
  in the HII environment. It allows for custom validation logic to be implemented
  for setting HII variables.
  @retval EFI_SUCCESS               The SetVariable check result was success.
  @retval EFI_SECURITY_VIOLATION    Check fail.
**/
EFI_STATUS
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

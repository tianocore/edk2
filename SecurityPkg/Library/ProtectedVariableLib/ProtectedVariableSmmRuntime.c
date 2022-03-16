/** @file
  Implemention of ProtectedVariableLib for BootService/Runtime use cases.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Uefi.h>

#include "Library/MemoryAllocationLib.h"
#include "Library/UefiBootServicesTableLib.h"
#include "Library/UefiRuntimeLib.h"
#include "ProtectedVariableInternal.h"

EFI_EVENT                       mVaChangeEvent = NULL;

PROTECTED_VARIABLE_CONTEXT_IN   mRtVariableContextIn = {
  PROTECTED_VARIABLE_CONTEXT_IN_STRUCT_VERSION,
  sizeof (PROTECTED_VARIABLE_CONTEXT_IN),
  0,
  FromRuntimeModule,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

PROTECTED_VARIABLE_GLOBAL       mRtProtectedVariableGlobal = {
  PROTECTED_VARIABLE_CONTEXT_OUT_STRUCT_VERSION,
  sizeof (PROTECTED_VARIABLE_GLOBAL),
  {0},
  {0},
  0,
  0,
  0,
  0,
  0,
  0,
  {0, 0, 0},
  0,
  0,
  {0, 0, 0, 0, 0, 0}
};

/**

  Get global data structure used to process protected variable.

  @param[out]   Global      Pointer to global configuration data.

  @retval EFI_SUCCESS         Get requested structure successfully.

**/
EFI_STATUS
GetProtectedVariableGlobal (
  OUT PROTECTED_VARIABLE_GLOBAL       **Global OPTIONAL
  )
{
  if (Global != NULL) {
    mRtProtectedVariableGlobal.ContextIn = GET_ADRS (&mRtVariableContextIn);
    *Global = &mRtProtectedVariableGlobal;
  }

  return EFI_SUCCESS;
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param[in]  Event        Event whose notification function is being invoked.
  @param[in]  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
VirtualAddressChangeEvent (
  IN EFI_EVENT                              Event,
  IN VOID                                   *Context
  )
{

  if (mRtVariableContextIn.FindVariableSmm != NULL) {
    EfiConvertPointer (0x0, (VOID **)&mRtVariableContextIn.FindVariableSmm);
  }
  if (mRtVariableContextIn.GetVariableInfo != NULL) {
    EfiConvertPointer (0x0, (VOID **)&mRtVariableContextIn.GetVariableInfo);
  }
  if (mRtVariableContextIn.GetNextVariableInfo != NULL) {
    EfiConvertPointer (0x0, (VOID **)&mRtVariableContextIn.GetNextVariableInfo);
  }
  if (mRtVariableContextIn.UpdateVariableStore != NULL) {
    EfiConvertPointer (0x0, (VOID **)&mRtVariableContextIn.UpdateVariableStore);
  }
  EfiConvertPointer (0x0, (VOID **)&mRtVariableContextIn);
  if (mRtProtectedVariableGlobal.VariableCache != 0) {
    EfiConvertPointer (0x0, (VOID **)&mRtProtectedVariableGlobal.VariableCache);
  }
  EfiConvertPointer (0x0, (VOID **)&mRtProtectedVariableGlobal);
}

/**

  Initialization for protected variable services.

  If this initialization failed upon any error, the whole variable services
  should not be used.  A system reset might be needed to re-construct NV
  variable storage to be the default state.

  @param[in]  ContextIn   Pointer to variable service context needed by
                          protected variable.

  @retval EFI_SUCCESS               Protected variable services are ready.
  @retval EFI_INVALID_PARAMETER     If ContextIn == NULL or something missing or
                                    mismatching in the content in ContextIn.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibInitialize (
  IN  PROTECTED_VARIABLE_CONTEXT_IN   *ContextIn
  )
{

  if (ContextIn == NULL
      || ContextIn->StructVersion != PROTECTED_VARIABLE_CONTEXT_IN_STRUCT_VERSION
      || ContextIn->FindVariableSmm == NULL
      || ContextIn->GetVariableInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&mRtVariableContextIn, ContextIn, sizeof (mRtVariableContextIn));

  //
  // Register the event to convert the pointer for runtime.
  //
  gBS->CreateEventEx (
         EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         VirtualAddressChangeEvent,
         NULL,
         &gEfiEventVirtualAddressChangeGuid,
         &mVaChangeEvent
         );

  return EFI_SUCCESS;
}

/**

  Prepare for variable update.

  Not supported in DXE phase.

  @retval EFI_UNSUPPORTED         Updating variable is not supported.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibWriteInit (
  VOID
  )
{

  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/**

  Update a variable with protection provided by this library.

  Not supported in DXE phase.

  @param[in,out]  CurrVariable        Variable to be updated. It's NULL if
                                      adding a new variable.
  @param[in,out]  CurrVariableInDel   In-delete-transition copy of updating variable.
  @param[in]      NewVariable         Buffer of new variable data.
  @param[out]     NewVariable         Buffer of "MetaDataHmacVar" and new
                                      variable (encrypted).
  @param[in]      NewVariableSize     Size of NewVariable.
  @param[out]     NewVariableSize     Size of (encrypted) NewVariable and
                                      "MetaDataHmacVar".

  @retval EFI_UNSUPPORTED             Not support updating variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibUpdate (
  IN  OUT VARIABLE_HEADER             *CurrVariable,
  IN      VARIABLE_HEADER             *CurrVariableInDel,
  IN  OUT VARIABLE_HEADER             *NewVariable,
  IN  OUT UINTN                       *NewVariableSize
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/**

  Finalize a variable updating after it's written to NV variable storage
  successfully.

  (Not supported for BootService/Runtime use cases.)

  @param[in]      NewVariable       Buffer of new variables and MetaDataHmacVar.
  @param[in]      VariableSize      Size of buffer pointed by NewVariable.
  @param[in]      StoreIndex        StoreIndex to NV variable storage from where the new
                                    variable and MetaDataHmacVar have been written.

  @retval EFI_UNSUPPORTED           Not supported for BootService/Runtime use cases.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibWriteFinal (
  IN  VARIABLE_HEADER         *NewVariable,
  IN  UINTN                   VariableSize,
  IN  UINT64                  StoreIndex
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

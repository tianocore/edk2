/** @file
  Implemention of ProtectedVariableLib for SMM variable services.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Uefi.h>

#include "Library/UefiBootServicesTableLib.h"
#include "Library/MemoryAllocationLib.h"

#include "ProtectedVariableInternal.h"

PROTECTED_VARIABLE_CONTEXT_IN  mVariableContextIn = {
  PROTECTED_VARIABLE_CONTEXT_IN_STRUCT_VERSION,
  sizeof (PROTECTED_VARIABLE_CONTEXT_IN),
  0,
  FromSmmModule,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

PROTECTED_VARIABLE_GLOBAL  mProtectedVariableGlobal = {
  PROTECTED_VARIABLE_CONTEXT_OUT_STRUCT_VERSION,
  sizeof (PROTECTED_VARIABLE_GLOBAL),
  { 0 },
  { 0 },
  0,
  0,
  0,
  0,
  0,
  0,
  { 0,                                          0,  0 },
  0,
  0,
  { 0,                                          0,  0, 0, 0, 0}
};

/**
  Fix incorrect state of MetaDataHmacVariable before any variable update.

  @param[in]   Event    The event that occurred
  @param[in]   Context  For EFI compatibility.  Not used.

**/
VOID
EFIAPI
VariableWriteProtocolCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS  Status;

  //
  // Fix incorrect state of MetaDataHmacVariable before any variable update.
  // This has to be done here due to the fact that this operation needs to
  // update NV storage but the FVB and FTW protocol might not be ready during
  // ProtectedVariableLibInitialize().
  //
  Status = FixupHmacVariable ();
  ASSERT_EFI_ERROR (Status);

  Status = ProtectedVariableLibWriteInit ();
  ASSERT_EFI_ERROR (Status);

  gBS->CloseEvent (Event);
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
  @retval EFI_COMPROMISED_DATA      If failed to check integrity of protected variables.
  @retval EFI_OUT_OF_RESOURCES      Fail to allocate enough resource.
  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibInitialize (
  IN  PROTECTED_VARIABLE_CONTEXT_IN  *ContextIn
  )
{
  EFI_STATUS                     Status;
  PROTECTED_VARIABLE_CONTEXT_IN  *ProtectedVarContext;
  PROTECTED_VARIABLE_GLOBAL      *OldGlobal;
  PROTECTED_VARIABLE_GLOBAL      *NewGlobal;
  VOID                           *VarWriteReg;

  if (  (ContextIn == NULL)
     || (ContextIn->StructVersion != PROTECTED_VARIABLE_CONTEXT_IN_STRUCT_VERSION)
     || (ContextIn->StructSize != sizeof (PROTECTED_VARIABLE_CONTEXT_IN))
     || (ContextIn->GetVariableInfo == NULL)
     || (ContextIn->GetNextVariableInfo == NULL)
     || (ContextIn->UpdateVariableStore == NULL)
     || (ContextIn->UpdateVariable == NULL))
  {
    ASSERT (ContextIn != NULL);
    ASSERT (ContextIn->StructVersion == PROTECTED_VARIABLE_CONTEXT_IN_STRUCT_VERSION);
    ASSERT (ContextIn->StructSize == sizeof (PROTECTED_VARIABLE_CONTEXT_IN));
    ASSERT (ContextIn->GetVariableInfo != NULL);
    ASSERT (ContextIn->GetNextVariableInfo != NULL);
    ASSERT (ContextIn->UpdateVariableStore != NULL);
    return EFI_INVALID_PARAMETER;
  }

  GetProtectedVariableGlobal (&NewGlobal);
  ProtectedVarContext = GET_CNTX (NewGlobal);
  CopyMem (ProtectedVarContext, ContextIn, sizeof (mVariableContextIn));
  ProtectedVarContext->VariableServiceUser = FromSmmModule;

  //
  // Get root key and HMAC key from HOB created by PEI variable driver.
  //
  Status = GetProtectedVariableGlobalFromHob (&OldGlobal);
  ASSERT_EFI_ERROR (Status);

  CopyMem ((VOID *)NewGlobal, (CONST VOID *)OldGlobal, sizeof (*OldGlobal));

  //
  // The keys must not be available outside SMM.
  //
  if (ProtectedVarContext->VariableServiceUser == FromSmmModule) {
    ZeroMem (OldGlobal->RootKey, sizeof (OldGlobal->RootKey));
    ZeroMem (OldGlobal->MetaDataHmacKey, sizeof (OldGlobal->MetaDataHmacKey));
  }

  //
  // Register variable write protocol notify function used to fix any
  // inconsistency in MetaDataHmacVariable before the first variable write
  // operation.
  //
  NewGlobal->Flags.WriteInit  = FALSE;
  NewGlobal->Flags.WriteReady = FALSE;

  EfiCreateProtocolNotifyEvent (
    &gEfiVariableWriteArchProtocolGuid,
    TPL_CALLBACK,
    VariableWriteProtocolCallback,
    NULL,
    &VarWriteReg
    );

  return EFI_SUCCESS;
}

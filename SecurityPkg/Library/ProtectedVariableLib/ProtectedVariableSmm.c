/** @file
  Implemention of ProtectedVariableLib for SMM variable services.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Uefi.h>

#include "Guid/SmmVariableCommon.h"

#include "Library/MmServicesTableLib.h"
#include "Library/MemoryAllocationLib.h"

#include "ProtectedVariableInternal.h"

PROTECTED_VARIABLE_CONTEXT_IN   mVariableContextIn = {
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

PROTECTED_VARIABLE_GLOBAL       mProtectedVariableGlobal = {
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
  {0, 0, 0, 0}
};

/**

  Callback function to call variable write.

  @param[in]  Protocol    Not Used.
  @param[in]  Interface   Not Used.
  @param[in]  Handle      Not Used.

  @retval EFI_SUCCESS     Protected variable write successful.
  @retval others          Protected variable write failed.

**/
EFI_STATUS
EFIAPI
VariableWriteProtocolCallback (
  IN CONST EFI_GUID                       *Protocol,
  IN VOID                                 *Interface,
  IN EFI_HANDLE                           Handle
  )
{
  EFI_STATUS      Status;

  Status = ProtectedVariableLibWriteInit ();
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
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
  IN  PROTECTED_VARIABLE_CONTEXT_IN   *ContextIn
  )
{
  EFI_STATUS                      Status;
  PROTECTED_VARIABLE_CONTEXT_IN   *ProtectedVarContext;
  PROTECTED_VARIABLE_GLOBAL       *OldGlobal;
  PROTECTED_VARIABLE_GLOBAL       *NewGlobal;
  VARIABLE_DIGEST                 *VarDig;
  VARIABLE_DIGEST                 *NewVarDig;
  EFI_PHYSICAL_ADDRESS            NewCacheIndex;
  UINTN                           VarSize;
  UNPROTECTED_VARIABLE_INDEX      Index;

  if (ContextIn == NULL
      || ContextIn->StructVersion != PROTECTED_VARIABLE_CONTEXT_IN_STRUCT_VERSION
      || ContextIn->StructSize != sizeof (PROTECTED_VARIABLE_CONTEXT_IN)
      || ContextIn->GetVariableInfo == NULL
      || ContextIn->GetNextVariableInfo == NULL
      || ContextIn->UpdateVariableStore == NULL
      || ContextIn->UpdateVariable == NULL)
  {
    ASSERT (ContextIn != NULL);
    ASSERT (ContextIn->StructVersion == PROTECTED_VARIABLE_CONTEXT_IN_STRUCT_VERSION);
    ASSERT (ContextIn->StructSize == sizeof (PROTECTED_VARIABLE_CONTEXT_IN));
    ASSERT (ContextIn->GetVariableInfo != NULL);
    ASSERT (ContextIn->GetNextVariableInfo != NULL);
    ASSERT (ContextIn->UpdateVariableStore != NULL);
    ASSERT (ContextIn->UpdateVariable != NULL);
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

  NewGlobal->Flags.WriteInit      = FALSE;
  NewGlobal->Flags.WriteReady     = FALSE;
  NewGlobal->LastAccessedVariable = 0;
  NewGlobal->VariableCache        = GET_ADRS(AllocateZeroPool(MAX_VARIABLE_SIZE));
  NewGlobal->DigestContext        = GET_ADRS(AllocateZeroPool(DIGEST_CONTEXT_SIZE));
  if (NewGlobal->DigestContext == 0) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Copy over variable from HOB to SMM memory
  //
  NewGlobal->VariableDigests = 0;
  VarDig = VAR_DIG_PTR (OldGlobal->VariableDigests);
  while (VarDig !=NULL) {
    //
    // Allocate new Var Digest in SMM memory
    //
    NewVarDig = (VARIABLE_DIGEST *)AllocateZeroPool (
                                  sizeof (VARIABLE_DIGEST) + VarDig->NameSize + METADATA_HMAC_SIZE
                                  );
    if (NewVarDig == NULL) {
      ASSERT (NewVarDig != NULL);
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem(NewVarDig, VarDig, sizeof(VARIABLE_DIGEST));
    NewVarDig->Prev = 0;
    NewVarDig->Next = 0;

    CopyMem (VAR_DIG_NAME (NewVarDig), VAR_DIG_NAME (VarDig), VarDig->NameSize);
    CopyMem (VAR_DIG_VALUE (NewVarDig), VAR_DIG_VALUE (VarDig), VarDig->DigestSize);

    VarSize =  VARIABLE_HEADER_SIZE (NewGlobal->Flags.Auth);
    VarSize += VarDig->NameSize + GET_PAD_SIZE (VarDig->NameSize);
    VarSize += VarDig->DataSize + GET_PAD_SIZE (VarDig->DataSize);
    VarSize = HEADER_ALIGN (VarSize);

    NewCacheIndex = GET_ADRS(AllocateZeroPool (VarSize));
    if (GET_BUFR(NewCacheIndex) == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem(GET_BUFR(NewCacheIndex), GET_BUFR(VarDig->CacheIndex), VarSize);
    NewVarDig->CacheIndex = NewCacheIndex;
    NewVarDig->Flags.Freeable = TRUE;

    for (Index = 0; Index < UnprotectedVarIndexMax; ++Index) {
      if (OldGlobal->Unprotected[Index] == VAR_DIG_ADR(VarDig)) {
        NewGlobal->Unprotected[Index] = VAR_DIG_ADR(NewVarDig);
      }
    }

    InsertVariableDigestNode (NewGlobal, NewVarDig, NULL);

    VarDig = VAR_DIG_NEXT (VarDig);
  }

  return Status;
}


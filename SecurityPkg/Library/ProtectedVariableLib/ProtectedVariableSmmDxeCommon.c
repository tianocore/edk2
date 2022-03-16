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
#include <Library/HobLib.h>

#include "ProtectedVariableInternal.h"

/**

  Get context and/or global data structure used to process protected variable.

  @param[out]   Global      Pointer to global configuration data.

  @retval EFI_SUCCESS         Get requested structure successfully.

**/
EFI_STATUS
GetProtectedVariableGlobal (
  OUT PROTECTED_VARIABLE_GLOBAL  **Global OPTIONAL
  )
{
  if (Global != NULL) {
    mProtectedVariableGlobal.ContextIn = GET_ADRS (&mVariableContextIn);
    *Global                            = &mProtectedVariableGlobal;
  }

  return EFI_SUCCESS;
}

/**
  Encrypt given variable data and generate new HMAC value against it.

  @param[in]      Global          Pointer to global configuration data.
  @param[in,out]  NewVarInfo      Pointer to buffer of new variable data.
  @param[in,out]  NewVarDig       Pointer to buffer of new variable digest.

  @retval EFI_SUCCESS           No error occurred during the encryption and HMC calculation.
  @retval EFI_ABORTED           Failed to do HMC calculation.
  @return EFI_OUT_OF_RESOURCES  Not enough resource to calculate HMC value.
  @return EFI_NOT_FOUND         The MetaDataHmacVar was not found in storage.

**/
STATIC
EFI_STATUS
UpdateVariableInternal (
  IN      PROTECTED_VARIABLE_GLOBAL  *Global,
  IN  OUT PROTECTED_VARIABLE_INFO    *NewVarInfo,
  IN  OUT VARIABLE_DIGEST            *NewVarDig
  )
{
  EFI_STATUS               Status;
  PROTECTED_VARIABLE_INFO  CachedVarInfo;
  VOID                     *Buffer;
  UINTN                    VarSize;

  if ((NewVarInfo == NULL) || (NewVarDig == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If Add or update variable, encrypt new data first.
  //
  if (NewVarInfo->Buffer != NULL) {
    Status = EFI_UNSUPPORTED;

    if (NewVarDig->Flags.Encrypted) {
      NewVarInfo->PlainData          = NULL;
      NewVarInfo->PlainDataSize      = 0;
      NewVarInfo->CipherData         = NULL;
      NewVarInfo->CipherDataSize     = 0;
      NewVarInfo->Key                = Global->RootKey;
      NewVarInfo->KeySize            = sizeof (Global->RootKey);
      NewVarInfo->Header.Attributes &= (~EFI_VARIABLE_APPEND_WRITE);
      Status                         = EncryptVariable (NewVarInfo);
      if (!EFI_ERROR (Status)) {
        //
        // Update new data size in variable header.
        //
        SET_VARIABLE_DATA_SIZE (NewVarInfo, NewVarInfo->CipherDataSize);
      } else if (Status != EFI_UNSUPPORTED) {
        ASSERT (FALSE);
        return Status;
      }
    }

    if (Status == EFI_UNSUPPORTED) {
      NewVarInfo->CipherData     = NewVarInfo->Header.Data;
      NewVarInfo->CipherDataSize = (UINT32)NewVarInfo->Header.DataSize;
      NewVarInfo->PlainData      = NULL;
      NewVarInfo->PlainDataSize  = 0;
    }
  } else {
    NewVarInfo->CipherData     = NULL;
    NewVarInfo->CipherDataSize = 0;
    NewVarInfo->PlainData      = NULL;
    NewVarInfo->PlainDataSize  = 0;
  }

  if (NewVarDig->CacheIndex != 0) {
    //
    // Update the cached copy.
    //
    ZeroMem ((VOID *)&CachedVarInfo, sizeof (CachedVarInfo));
    CachedVarInfo.Buffer     = GET_BUFR (NewVarDig->CacheIndex);
    CachedVarInfo.StoreIndex = VAR_INDEX_INVALID;
    CachedVarInfo.Flags.Auth = NewVarInfo->Flags.Auth;

    Status = GET_CNTX (Global)->GetVariableInfo (&CachedVarInfo);
    ASSERT_EFI_ERROR (Status);

    if ((CachedVarInfo.Header.DataSize != 0) && (NewVarInfo->CipherDataSize > CachedVarInfo.Header.DataSize)) {
      //
      // allocate new VarInfo buffer that is of greater CipherDataSize
      //
      VarSize  =  VARIABLE_HEADER_SIZE (NewVarDig->Flags.Auth);
      VarSize += NewVarInfo->Header.NameSize + GET_PAD_SIZE (NewVarInfo->Header.NameSize);
      VarSize += NewVarInfo->CipherDataSize + GET_PAD_SIZE (NewVarInfo->CipherDataSize);
      VarSize  = HEADER_ALIGN (VarSize);
      Buffer   = AllocateZeroPool (VarSize);
      if (Buffer != NULL) {
        VarSize  =  VARIABLE_HEADER_SIZE (NewVarDig->Flags.Auth);
        VarSize += CachedVarInfo.Header.NameSize + GET_PAD_SIZE (CachedVarInfo.Header.NameSize);
        VarSize += CachedVarInfo.Header.DataSize + GET_PAD_SIZE (CachedVarInfo.DataSize);
        VarSize  = HEADER_ALIGN (VarSize);

        CopyMem (
          Buffer,
          CachedVarInfo.Buffer,
          VarSize
          );

        FreePool (CachedVarInfo.Buffer);

        //
        // Update the cached copy.
        //
        ZeroMem ((VOID *)&CachedVarInfo, sizeof (CachedVarInfo));
        CachedVarInfo.Buffer     = Buffer;
        CachedVarInfo.StoreIndex = VAR_INDEX_INVALID;
        CachedVarInfo.Flags.Auth = NewVarInfo->Flags.Auth;
        Status                   = GET_CNTX (Global)->GetVariableInfo (&CachedVarInfo);
        ASSERT_EFI_ERROR (Status);
        NewVarDig->CacheIndex = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
      }
    }

    CopyMem (
      CachedVarInfo.Header.Data,
      NewVarInfo->CipherData,
      NewVarInfo->CipherDataSize
      );
    SET_VARIABLE_DATA_SIZE (&CachedVarInfo, NewVarInfo->CipherDataSize);

    NewVarDig->State    = VAR_ADDED;
    NewVarDig->DataSize = NewVarInfo->CipherDataSize;

    if (NewVarInfo->PlainDataSize > 0) {
      NewVarDig->PlainDataSize = NewVarInfo->PlainDataSize;
    } else {
      NewVarDig->PlainDataSize = NewVarDig->DataSize;
    }

    //
    // (Re-)Calculate the hash of the variable.
    //
    if (NewVarDig->Flags.Protected) {
      GetVariableDigest (Global, NewVarInfo, VAR_DIG_VALUE (NewVarDig));
    }
  }

  return EFI_SUCCESS;
}

/**

  Fix state of MetaDataHmacVar on NV variable storage, if there's failure at
  last boot during updating variable.

  This must be done before the first writing of variable in current boot,
  including storage reclaim.

  @retval EFI_UNSUPPORTED        Updating NV variable storage is not supported.
  @retval EFI_OUT_OF_RESOURCES   Not enough resource to complete the operation.
  @retval EFI_SUCCESS            Variable store was successfully updated.

**/
EFI_STATUS
FixupHmacVariable (
  VOID
  )
{
  EFI_STATUS                     Status;
  PROTECTED_VARIABLE_INFO        HmacVarInfo;
  PROTECTED_VARIABLE_GLOBAL      *Global;
  PROTECTED_VARIABLE_CONTEXT_IN  *ContextIn;
  VARIABLE_DIGEST                *VarDig;
  UINTN                          Index;

  Status = GetProtectedVariableGlobal (&Global);
  ASSERT_EFI_ERROR (Status);

  if (Global->Flags.WriteReady) {
    return EFI_SUCCESS;
  }

  ContextIn = GET_CNTX (Global);

  //
  // Delete invalid MetaDataHmacVar.
  //
  for (Index = 0; Index <= IndexHmacAdded; ++Index) {
    if (Global->Unprotected[Index] == VAR_INDEX_INVALID) {
      continue;
    }

    VarDig = VAR_DIG_PTR (Global->Unprotected[Index]);
    if (VarDig->Flags.Valid) {
      continue;
    }

    ZeroMem ((VOID *)&HmacVarInfo, sizeof (HmacVarInfo));
    HmacVarInfo.StoreIndex = VarDig->StoreIndex;
    HmacVarInfo.Flags.Auth = VarDig->Flags.Auth;

    Status = ContextIn->GetVariableInfo (&HmacVarInfo);
    if (!EFI_ERROR (Status) && (HmacVarInfo.Buffer != NULL)) {
      HmacVarInfo.Buffer->State &= VAR_DELETED;
      Status                     = ContextIn->UpdateVariableStore (
                                                &HmacVarInfo,
                                                OFFSET_OF (VARIABLE_HEADER, State),
                                                sizeof (HmacVarInfo.Buffer->State),
                                                &HmacVarInfo.Buffer->State
                                                );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    }

    //
    // Release the resource and update related states.
    //
    VarDig->State &= VAR_DELETED;
    RemoveVariableDigestNode (Global, VarDig, FALSE);
    Global->Unprotected[Index] = VAR_INDEX_INVALID;
  }

  //
  // There should be no MetaDataHmacVar if in variable storage recovery mode.
  //
  if (Global->Flags.RecoveryMode) {
    ASSERT (Global->Unprotected[IndexHmacAdded] == VAR_INDEX_INVALID);
    ASSERT (Global->Unprotected[IndexHmacInDel] == VAR_INDEX_INVALID);
  }

  Global->Flags.WriteReady = TRUE;

  return EFI_SUCCESS;
}

/**

  Prepare for variable update.

  This is needed only once during current boot to mitigate replay attack. Its
  major job is to advance RPMC (Replay Protected Monotonic Counter).

  @retval EFI_SUCCESS             Variable is ready to update hereafter.
  @retval EFI_UNSUPPORTED         Updating variable is not supported.
  @retval EFI_DEVICE_ERROR        Error in advancing RPMC.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibWriteInit (
  VOID
  )
{
  EFI_STATUS                     Status;
  PROTECTED_VARIABLE_GLOBAL      *Global;
  PROTECTED_VARIABLE_CONTEXT_IN  *ContextIn;
  PROTECTED_VARIABLE_INFO        VarInfo;

  (VOID)GetProtectedVariableGlobal (&Global);
  ContextIn = GET_CNTX (Global);

  //
  // HmacVarInfo should be here
  //
  if (Global->Flags.RecoveryMode) {
    //
    // Flush default variables to variable storage if in variable recovery mode.
    //
    Status = ContextIn->UpdateVariableStore (NULL, 0, (UINT32)-1, NULL);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  } else {
    //
    // RPMC1 is going to be one step ahead of RPMC2 here, in order to
    // refresh the MetaDataHmacVar in each boot before writing any other
    // variables.
    //
    Status = IncrementMonotonicCounter (RPMC_COUNTER_1);
    ASSERT_EFI_ERROR (Status);

    ContextIn = GET_CNTX (Global);

    //
    // Fix any wrong MetaDataHmacVar information before adding new one.
    //
    Status = FixupHmacVariable ();
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    if (!Global->Flags.WriteReady) {
      return EFI_NOT_READY;
    }

    //
    // Refresh MetaDataHmacVar with RPMC2 by 1 in each boot before any variable
    // update,  by deleting (attr == 0 && datasize == 0) the old one.
    //
    ZeroMem (&VarInfo, sizeof (PROTECTED_VARIABLE_INFO));  // Zero attr & datasize

    VarInfo.Flags.Auth          = Global->Flags.Auth;
    VarInfo.Header.VariableName = METADATA_HMAC_VARIABLE_NAME;
    VarInfo.Header.NameSize     = METADATA_HMAC_VARIABLE_NAME_SIZE;
    VarInfo.Header.VendorGuid   = &METADATA_HMAC_VARIABLE_GUID;

    //
    // Pretend to delete MetaDataHmacVar.
    //
    Status = ContextIn->UpdateVariable (&VarInfo.Header);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  mProtectedVariableGlobal.Flags.WriteInit = TRUE;

  return EFI_SUCCESS;
}

/**

  Update a variable with protection provided by this library.

  If variable encryption is employed, the new variable data will be encrypted
  before being written to NV variable storage.

  A special variable, called "MetaDataHmacVar", will always be updated along
  with variable being updated to reflect the changes (HMAC value) of all
  protected valid variables. The only exceptions, currently, is variable
  variable "VarErrorLog".

  The buffer passed by NewVariable must be double of maximum variable size,
  which allows to pass the "MetaDataHmacVar" back to caller along with encrypted
  new variable data, if any. This can make sure the new variable data and
  "MetaDataHmacVar" can be written at almost the same time to reduce the chance
  of compromising the integrity.

  If *NewVariableSize is zero, it means to delete variable passed by CurrVariable
  and/or CurrVariableInDel. "MetaDataHmacVar" will be updated as well in such
  case because of less variables in storage. NewVariable should be always passed
  in to convey new "MetaDataHmacVar" back.

  @param[in,out]  CurrVariable        Variable to be updated. It's NULL if
                                      adding a new variable.
  @param[in]      CurrVariableInDel   In-delete-transition copy of updating variable.
  @param[in,out]  NewVariable         Buffer of new variable data.
                                      Buffer of "MetaDataHmacVar" and new
                                      variable (encrypted).
  @param[in,out]  NewVariableSize     Size of NewVariable.
                                      Size of (encrypted) NewVariable and
                                      "MetaDataHmacVar".

  @retval EFI_SUCCESS             The variable is updated with protection successfully.
  @retval EFI_INVALID_PARAMETER   NewVariable is NULL.
  @retval EFI_NOT_FOUND           Information missing to finish the operation.
  @retval EFI_ABORTED             Failed to encrypt variable or calculate HMAC.
  @retval EFI_NOT_READY           The RPMC device is not yet initialized.
  @retval EFI_DEVICE_ERROR        The RPMC device has error in updating.
  @retval EFI_ACCESS_DENIED       The given variable is not allowed to update.
                                  Currently this only happens on updating
                                  "MetaDataHmacVar" from code outside of this
                                  library.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibUpdate (
  IN  OUT VARIABLE_HEADER  *CurrVariable,
  IN      VARIABLE_HEADER  *CurrVariableInDel,
  IN  OUT VARIABLE_HEADER  *NewVariable,
  IN  OUT UINTN            *NewVariableSize
  )
{
  EFI_STATUS                     Status;
  PROTECTED_VARIABLE_CONTEXT_IN  *ContextIn;
  PROTECTED_VARIABLE_GLOBAL      *Global;
  PROTECTED_VARIABLE_INFO        VarInfo;
  VARIABLE_DIGEST                *VarDig;
  VARIABLE_DIGEST                *CurrVarDig;
  VARIABLE_DIGEST                *NewVarDig;
  PROTECTED_VARIABLE_INFO        NewVarInfo;
  PROTECTED_VARIABLE_INFO        NewHmacVarInfo;
  UINTN                          VarSize;
  UINT64                         UnprotectedVarIndex;

  //
  // Advance RPMC
  //
  Status = IncrementMonotonicCounter (RPMC_COUNTER_1);
  ASSERT_EFI_ERROR (Status);

  //
  // Buffer for new variable is always needed, even this function is called to
  // delete an existing one, because we need to pass the MetaDataHmacVar back
  // which will be updated upon each variable addition or deletion.
  //
  if ((NewVariable == NULL) || (NewVariableSize == NULL)) {
    ASSERT (NewVariable != NULL);
    ASSERT (NewVariableSize != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = GetProtectedVariableGlobal (&Global);
  ASSERT_EFI_ERROR (Status);
  ContextIn = GET_CNTX (Global);

  if (!Global->Flags.WriteReady && !Global->Flags.WriteInit) {
    return EFI_NOT_READY;
  }

  VarSize             = 0;
  CurrVarDig          = NULL;
  NewVarDig           = NULL;
  UnprotectedVarIndex = VAR_INDEX_INVALID;

  //
  // Check existing copy of the same variable.
  //
  if (CurrVariable != NULL) {
    //
    // Find local cached copy, if possible.
    //
    ZeroMem (&VarInfo, sizeof (VarInfo));
    VarInfo.Buffer     = CurrVariable;
    VarInfo.StoreIndex = VAR_INDEX_INVALID;
    VarInfo.Flags.Auth = Global->Flags.Auth;

    Status = ContextIn->GetVariableInfo (&VarInfo); // Retrieve the name/guid
    ASSERT_EFI_ERROR (Status);

    UnprotectedVarIndex = CheckKnownUnprotectedVariable (Global, &VarInfo);
    if (UnprotectedVarIndex < UnprotectedVarIndexMax) {
      CurrVarDig = VAR_DIG_PTR (Global->Unprotected[UnprotectedVarIndex]);
    } else {
      CurrVarDig = FindVariableInternal (Global, &VarInfo, FALSE);
    }

    ASSERT (CurrVarDig != NULL);
    CurrVarDig->State &= VAR_DELETED;
  }

  //
  // The old copy of the variable might haven't been deleted completely.
  //
  if (CurrVariableInDel != NULL) {
    //
    // Find local cached copy, if possible.
    //
    ZeroMem (&VarInfo, sizeof (VarInfo));
    VarInfo.Buffer     = CurrVariableInDel;
    VarInfo.StoreIndex = VAR_INDEX_INVALID;
    VarInfo.Flags.Auth = Global->Flags.Auth;

    Status = ContextIn->GetVariableInfo (&VarInfo); // Retrieve the name/guid
    ASSERT_EFI_ERROR (Status);

    if (UnprotectedVarIndex == VAR_INDEX_INVALID) {
      UnprotectedVarIndex = CheckKnownUnprotectedVariable (Global, &VarInfo);
    }

    if (UnprotectedVarIndex < UnprotectedVarIndexMax) {
      VarDig = VAR_DIG_PTR (Global->Unprotected[UnprotectedVarIndex]);
    } else {
      VarDig = FindVariableInternal (Global, &VarInfo, FALSE);
    }

    if ((VarDig != NULL) && (VAR_DIG_ADR (VarDig) != VAR_INDEX_INVALID)) {
      VarDig->State &= VAR_DELETED;

      //
      // Just need one node for the same variable. So remove the one
      // in-del-transition.
      //
      if ((CurrVarDig != NULL) && (VarDig != CurrVarDig)) {
        RemoveVariableDigestNode (Global, VarDig, TRUE);
      } else {
        CurrVarDig = VarDig;  // Reuse the one in-del-transition.
      }
    }
  }

  //
  // New data of the variable or new variable to be added.
  //
  if (NewVariable != NULL) {
    //
    // Completely new variable?
    //
    if (UnprotectedVarIndex == VAR_INDEX_INVALID) {
      ZeroMem (&VarInfo, sizeof (VarInfo));
      VarInfo.Buffer     = NewVariable;
      VarInfo.StoreIndex = VAR_INDEX_INVALID;
      VarInfo.Flags.Auth = Global->Flags.Auth;

      Status = ContextIn->GetVariableInfo (&VarInfo); // Retrieve the name/guid
      ASSERT_EFI_ERROR (Status);

      UnprotectedVarIndex = CheckKnownUnprotectedVariable (Global, &VarInfo);
    }
  }

  //
  // Reserve space for MetaDataHmacVar (before the new variable so
  // that it can be written first).
  //
  ZeroMem (&NewVarInfo, sizeof (NewVarInfo));
  ZeroMem (&NewHmacVarInfo, sizeof (NewHmacVarInfo));

  //
  // Put the MetaDataHmacVar at the beginning of buffer.
  //
  NewHmacVarInfo.Buffer = NewVariable;

  if (*NewVariableSize == 0) {
    //
    // Delete variable (but not MetaDataHmacVar)
    //
    if (  (UnprotectedVarIndex != IndexHmacAdded)
       && (UnprotectedVarIndex != IndexHmacInDel))
    {
      RemoveVariableDigestNode (Global, CurrVarDig, TRUE);
    }

    NewVarInfo.Buffer = NULL;
  } else if (UnprotectedVarIndex >= IndexPlatformVar) {
    //
    // Add/update variable. Move new variable data to be after MetaDataHmacVar.
    //
    // TRICK: New MetaDataHmacVar will be put at the beginning of buffer
    //        for new variable so that they can be written into non-volatile
    //        variable storage in one call. This can avoid writing one variable
    //        (NewHmacVarInfo) in the middle of writing another variable
    //        (NewVarInfo), which will need two calls and introduce extra
    //        complexities (from temp variable buffer reservation to variable
    //        space reclaim, etc.) in current implementation of variable
    //        services. The caller must make sure there's enough space in
    //        variable buffer (i.e. at least 2 * MaxVariableSize).
    //
    NewVarInfo.Buffer = (VARIABLE_HEADER *)((UINTN)NewVariable
                                            + GetMetaDataHmacVarSize (Global->Flags.Auth));
    CopyMem ((VOID *)NewVarInfo.Buffer, (VOID *)NewVariable, *NewVariableSize);

    NewVarInfo.StoreIndex = VAR_INDEX_INVALID;   // Skip offset calculation (it's new one)
    NewVarInfo.Flags.Auth = Global->Flags.Auth;

    Status = ContextIn->GetVariableInfo (&NewVarInfo);
    ASSERT_EFI_ERROR (Status);

    if (CurrVarDig != NULL) {
      //
      // Update existing variable. Re-use the node.
      //
      NewVarDig = CurrVarDig;
    } else {
      //
      // Add new variable.
      //
      NewVarDig = CreateVariableDigestNode (
                    NewVarInfo.Header.VariableName,
                    NewVarInfo.Header.VendorGuid,
                    (UINT16)NewVarInfo.Header.NameSize,
                    (UINT32)NewVarInfo.Header.DataSize,
                    NewVarInfo.Flags.Auth,
                    Global
                    );
      if (NewVarDig == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      NewVarDig->Attributes = NewVarInfo.Header.Attributes;
      if (UnprotectedVarIndex < UnprotectedVarIndexMax) {
        NewVarDig->Flags.Protected               = FALSE;
        NewVarDig->Flags.Encrypted               = FALSE;
        Global->Unprotected[UnprotectedVarIndex] = VAR_DIG_ADR (NewVarDig);
      }

      //
      // copy new variable to CacheIndex
      //
      VarSize  =  VARIABLE_HEADER_SIZE (NewVarInfo.Flags.Auth);
      VarSize += NewVarInfo.Header.NameSize + GET_PAD_SIZE (NewVarInfo.Header.NameSize);
      VarSize += NewVarInfo.Header.DataSize + GET_PAD_SIZE (NewVarInfo.Header.DataSize);
      VarSize  = HEADER_ALIGN (VarSize);
      CopyMem (GET_BUFR (NewVarDig->CacheIndex), GET_BUFR (NewVarInfo.Buffer), VarSize);
      InsertVariableDigestNode (Global, NewVarDig, NULL);
    }
  }

  if (  (UnprotectedVarIndex == IndexHmacAdded)
     || (UnprotectedVarIndex == IndexHmacInDel))
  {
    //
    // MetaDataHmacVar should be managed only by this library. It's not
    // supposed to be updated by external users of variable service. The only
    // exception is that deleting it (not really delete but refresh the HMAC
    // value against RPMC+1) is allowed before WriteInit, as a way to always
    // increment RPMC once in current boot before any variable updates.
    //
    if ((NewVarInfo.Buffer != NULL) || Global->Flags.WriteInit) {
      return EFI_ACCESS_DENIED;
    }
  } else {
    //
    // Do encryption, if enabled.
    //
    if ((NewVarDig != NULL) && (NewVarInfo.Buffer != NULL)) {
      Status = UpdateVariableInternal (Global, &NewVarInfo, NewVarDig);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    }
  }

  //
  // Refresh MetaDataHmacVar.
  //
  Status = RefreshVariableMetadataHmac (Global, NULL, &NewHmacVarInfo);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // Return size for both MetaDataHmacVar and added/updated one.
  //
  VarSize          = VARIABLE_SIZE (&NewHmacVarInfo);
  *NewVariableSize = HEADER_ALIGN (VarSize);
  if (NewVarInfo.Buffer != NULL) {
    VarSize = VARIABLE_SIZE (&NewVarInfo);
    VarSize = HEADER_ALIGN (VarSize);

    if (VarSize > GET_CNTX (Global)->MaxVariableSize) {
      return EFI_BAD_BUFFER_SIZE;
    }

    *NewVariableSize += VarSize;
  }

  return EFI_SUCCESS;
}

/**

  Finalize a variable updating after it's written to NV variable storage
  successfully.

  This usually includes works like increasing RPMC, synchronizing local cache,
  updating new position of "MetaDataHmacVar", deleting old copy of "MetaDataHmacVar"
  completely, etc.

  @param[in]      NewVariable       Buffer of new variables and MetaDataHmacVar.
  @param[in]      VariableSize      Size of buffer pointed by NewVariable.
  @param[in]      StoreIndex        StoreIndex to NV variable storage from where the new
                                    variable and MetaDataHmacVar have been written.

  @retval EFI_SUCCESS         No problem in winding up the variable write operation.
  @retval Others              Failed to updating state of old copy of updated
                              variable, or failed to increase RPMC, etc.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibWriteFinal (
  IN  VARIABLE_HEADER  *NewVariable,
  IN  UINTN            VariableSize,
  IN  UINT64           StoreIndex
  )
{
  EFI_STATUS                     Status;
  PROTECTED_VARIABLE_INFO        VarInfo;
  PROTECTED_VARIABLE_CONTEXT_IN  *ContextIn;
  PROTECTED_VARIABLE_GLOBAL      *Global;
  UNPROTECTED_VARIABLE_INDEX     Index;
  VARIABLE_DIGEST                *VarDig;
  VOID                           *Buffer;
  UINTN                          VarSize;
  UINTN                          NewVarSize;

  Status = GetProtectedVariableGlobal (&Global);
  ASSERT_EFI_ERROR (Status);
  ContextIn = GET_CNTX (Global);

  ZeroMem ((VOID *)&VarInfo, sizeof (VarInfo));
  VarInfo.Buffer     = NewVariable;
  VarInfo.StoreIndex = VAR_INDEX_INVALID;
  VarInfo.Flags.Auth = Global->Flags.Auth;

  Status = ContextIn->GetVariableInfo (&VarInfo);
  ASSERT_EFI_ERROR (Status);

  Index = CheckKnownUnprotectedVariable (Global, &VarInfo);
  if (Index < UnprotectedVarIndexMax) {
    VarDig = VAR_DIG_PTR (Global->Unprotected[Index]);
  } else {
    VarDig = FindVariableInternal (Global, &VarInfo, FALSE);
  }

  if (Index == IndexHmacAdded) {
    //
    // Advance the RPMC to let it match new MetaDataHmacVar.
    //
    Status = IncrementMonotonicCounter (RPMC_COUNTER_2);
    ASSERT_EFI_ERROR (Status);

    if ((VarDig->StoreIndex != VAR_INDEX_INVALID) && (VarDig->State != VAR_ADDED)) {
      ZeroMem ((VOID *)&VarInfo, sizeof (VarInfo));
      VarInfo.StoreIndex = VarDig->StoreIndex;  // Still point to old copy
      VarInfo.Flags.Auth = Global->Flags.Auth;

      //
      // Delete variable completely.
      //
      Status = ContextIn->GetVariableInfo (&VarInfo);
      ASSERT_EFI_ERROR (Status);

      if (  (VarInfo.Buffer->State == VAR_ADDED)
         || (VarInfo.Buffer->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION)))
      {
        VarInfo.Buffer->State &= VAR_DELETED;
        Status                 = ContextIn->UpdateVariableStore (
                                              &VarInfo,
                                              OFFSET_OF (VARIABLE_HEADER, State),
                                              sizeof (VarInfo.Buffer->State),
                                              &VarInfo.Buffer->State
                                              );
        if (EFI_ERROR (Status)) {
          ASSERT_EFI_ERROR (Status);
          return Status;
        }
      }
    }
  }

  VarDig->StoreIndex = StoreIndex;
  VarDig->State      = VAR_ADDED;

  ZeroMem ((VOID *)&VarInfo, sizeof (VarInfo));
  VarInfo.Buffer     = NULL;
  VarInfo.StoreIndex = VarDig->StoreIndex;
  VarInfo.Flags.Auth = Global->Flags.Auth;
  Status             = ContextIn->GetVariableInfo (&VarInfo);

  //
  // Check if cache pool need re-allocation due to variable size increase
  //
  VarSize  =  VARIABLE_HEADER_SIZE (VarDig->Flags.Auth);
  VarSize += VarDig->NameSize + GET_PAD_SIZE (VarDig->NameSize);
  VarSize += VarDig->DataSize + GET_PAD_SIZE (VarDig->DataSize);
  VarSize  = HEADER_ALIGN (VarSize);

  NewVarSize  =  VARIABLE_HEADER_SIZE (VarInfo.Flags.Auth);
  NewVarSize += VarInfo.Header.NameSize + GET_PAD_SIZE (VarInfo.Header.NameSize);
  NewVarSize += VarInfo.Header.DataSize + GET_PAD_SIZE (VarInfo.Header.DataSize);
  NewVarSize  = HEADER_ALIGN (NewVarSize);

  if (VarSize < NewVarSize) {
    if (VarDig->Flags.Freeable == TRUE) {
      FreePool (GET_BUFR (VarDig->CacheIndex));
    }

    Buffer = AllocatePool (NewVarSize);
    if (Buffer != NULL) {
      VarDig->CacheIndex = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
    } else {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }
  }

  //
  // Update cached copy.
  //
  CopyMem (GET_BUFR (VarDig->CacheIndex), NewVariable, NewVarSize);

  //
  // Check if there is consecutive variable as part of the write or
  // is it just the MetaDataHmacVar variable
  //
  if (NewVarSize < VariableSize) {
    //
    // Advance to consecutive Variable
    //
    NewVariable = GET_BUFR (GET_ADRS (NewVariable) + NewVarSize);

    //
    // Update the StoreIndex of consecutive Variable
    //
    ZeroMem ((VOID *)&VarInfo, sizeof (VarInfo));
    VarInfo.Buffer     = NULL;
    VarInfo.StoreIndex = VarDig->StoreIndex;
    VarInfo.Flags.Auth = Global->Flags.Auth;
    Status             = ContextIn->GetNextVariableInfo (&VarInfo);
    StoreIndex         = VarInfo.StoreIndex;

    //
    // The new StoreIndex does not exist in the variable digest.
    // It is yet to be updated.
    // Therefore, find variable by Name & Guid instead.
    //
    VarInfo.StoreIndex = VAR_INDEX_INVALID;
    VarDig             = FindVariableInternal (Global, &VarInfo, FALSE);

    //
    // Check if cache pool need re-allocation due to variable size increase
    //
    VarSize  =  VARIABLE_HEADER_SIZE (VarDig->Flags.Auth);
    VarSize += VarDig->NameSize + GET_PAD_SIZE (VarDig->NameSize);
    VarSize += VarDig->DataSize + GET_PAD_SIZE (VarDig->DataSize);
    VarSize  = HEADER_ALIGN (VarSize);

    NewVarSize  =  VARIABLE_HEADER_SIZE (VarInfo.Flags.Auth);
    NewVarSize += VarInfo.Header.NameSize + GET_PAD_SIZE (VarInfo.Header.NameSize);
    NewVarSize += VarInfo.Header.DataSize + GET_PAD_SIZE (VarInfo.Header.DataSize);
    NewVarSize  = HEADER_ALIGN (NewVarSize);

    if (VarSize < NewVarSize) {
      if (VarDig->Flags.Freeable == TRUE) {
        FreePool (GET_BUFR (VarDig->CacheIndex));
      }

      Buffer = AllocatePool (NewVarSize);
      if (Buffer != NULL) {
        VarDig->CacheIndex = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
      } else {
        ASSERT (FALSE);
        return EFI_ABORTED;
      }
    }

    //
    // Update cached copy.
    //
    CopyMem (GET_BUFR (VarDig->CacheIndex), NewVariable, NewVarSize);
    VarDig->StoreIndex = StoreIndex;
  }

  return Status;
}

/**
  Refresh variable information changed by variable service.

  @param[in]  Variable         Pointer to buffer of the updated variable.
  @param[in]  VariableSize     Size of variable pointed by Variable.
  @param[in]  StoreIndex       New index of the variable in store.
  @param[in]  RefreshData      Flag to indicate if the variable has been updated.

  @return EFI_SUCCESS     No error occurred in updating.
  @return EFI_NOT_FOUND   The given variable was not found in
                          ProtectedVariableLib.
**/
EFI_STATUS
EFIAPI
ProtectedVariableLibRefresh (
  IN  VARIABLE_HEADER  *Variable,
  IN  UINTN            VariableSize,
  IN  UINT64           StoreIndex,
  IN  BOOLEAN          RefreshData
  )
{
  EFI_STATUS                 Status;
  PROTECTED_VARIABLE_INFO    VarInfo;
  PROTECTED_VARIABLE_GLOBAL  *Global;
  VARIABLE_DIGEST            *VarDig;

  (VOID)GetProtectedVariableGlobal (&Global);

  ZeroMem ((VOID *)&VarInfo, sizeof (VarInfo));
  VarInfo.Buffer     = Variable;
  VarInfo.StoreIndex = VAR_INDEX_INVALID;
  VarInfo.Flags.Auth = Global->Flags.Auth;

  Status = GET_CNTX (Global)->GetVariableInfo (&VarInfo);
  ASSERT_EFI_ERROR (Status);

  VarDig = FindVariableInternal (Global, &VarInfo, FALSE);
  if (VarDig == NULL) {
    ASSERT (VarDig != NULL);
    return EFI_NOT_FOUND;
  }

  if (StoreIndex != VAR_INDEX_INVALID) {
    VarDig->StoreIndex = StoreIndex;
  }

  if (RefreshData) {
    if (VarDig->CacheIndex == VAR_INDEX_INVALID) {
      VarDig->CacheIndex = (EFI_PHYSICAL_ADDRESS)(UINTN)
                           AllocatePool (MAX_VARIABLE_SIZE);
    }

    CopyMem (GET_BUFR (VarDig->CacheIndex), Variable, VariableSize);
  }

  //
  // Information should stay the same other than following ones.
  //
  VarDig->State    = VarInfo.Header.State;
  VarDig->DataSize = (UINT32)VarInfo.Header.DataSize;

  return EFI_SUCCESS;
}

/**

  Determine if the variable is the HMAC variable

  @param VariableName   Pointer to variable name.

  @return TRUE      Variable is HMAC variable
  @return FALSE     Variable is not HMAC variable

**/
BOOLEAN
ProtectedVariableLibIsHmac (
  IN CHAR16  *VariableName
  )
{
  INTN  Result;

  Result = StrnCmp (
             METADATA_HMAC_VARIABLE_NAME,
             VariableName,
             METADATA_HMAC_VARIABLE_NAME_SIZE
             );

  if (Result == 0) {
    return TRUE;
  }

  return FALSE;
}

/** @file
  The common protected variable operation routines.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <PiPei.h>

#include <Guid/VariableFormat.h>
#include <Guid/VarErrorFlag.h>

#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HashApiLib.h>
#include <Library/SortLib.h>

#include "ProtectedVariableInternal.h"

EFI_TIME             mDefaultTimeStamp       = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
VARIABLE_IDENTIFIER  mUnprotectedVariables[] = {
  {
    METADATA_HMAC_VARIABLE_NAME,
    &METADATA_HMAC_VARIABLE_GUID,
    VAR_ADDED & VAR_IN_DELETED_TRANSITION
  },
  {
    METADATA_HMAC_VARIABLE_NAME,
    &METADATA_HMAC_VARIABLE_GUID,
    VAR_ADDED
  },
  {
    VAR_ERROR_FLAG_NAME,
    &gEdkiiVarErrorFlagGuid,
    VAR_ADDED
  },
  {
    (CHAR16 *)PcdGetPtr (PcdPlatformVariableName),
    (EFI_GUID *)PcdGetPtr (PcdPlatformVariableGuid),
    VAR_ADDED
  }
};

/**
  Print variable information.

  @param[in]   Data8      Pointer to data.
  @param[in]   DataSize   Size of data.

**/
VOID
PrintVariableData (
  IN UINT8  *Data8,
  IN UINTN  DataSize
  )
{
  UINTN  Index;

  for (Index = 0; Index < DataSize; Index++) {
    if (Index % 0x10 == 0) {
      DEBUG ((DEBUG_INFO, "\n%08X:", Index));
    }

    DEBUG ((DEBUG_INFO, " %02X", *Data8++));
  }

  DEBUG ((DEBUG_INFO, "\n"));
}

/**

  Retrieve the context and global configuration data structure from HOB.

  Once protected NV variable storage is cached and verified in PEI phase,
  all related information are stored in a HOB which can be used by PEI variable
  service itself and passed to SMM along with the boot flow, which can avoid
  many duplicate works, like generating HMAC key, verifying NV variable storage,
  etc.

  The HOB can be identified by gEdkiiProtectedVariableGlobalGuid.

  @param[out]   Global      Pointer to global configuration data from PEI phase.

  @retval EFI_SUCCESS     The HOB was found, and Context and Global are retrieved.
  @retval EFI_NOT_FOUND   The HOB was not found.

**/
EFI_STATUS
EFIAPI
GetProtectedVariableGlobalFromHob (
  OUT PROTECTED_VARIABLE_GLOBAL  **Global
  )
{
  VOID                           *Data;
  UINTN                          DataSize;
  EFI_PEI_HOB_POINTERS           Hob;
  EFI_HOB_MEMORY_ALLOCATION      *MemoryAllocationHob;
  PROTECTED_VARIABLE_CONTEXT_IN  *ContextIn;
  EFI_PHYSICAL_ADDRESS           OldStart;
  VARIABLE_DIGEST                *VarDig;
  EFI_HOB_GUID_TYPE              *GuidHob;
  UINTN                          Index;

  Hob.Raw = GetFirstGuidHob (&gEdkiiProtectedVariableGlobalGuid);
  if (Hob.Raw != NULL) {
    Data     = GET_GUID_HOB_DATA (Hob);
    DataSize = GET_GUID_HOB_DATA_SIZE (Hob);
  } else {
    //
    // Search the global from allocated memory blob.
    //
    Data                = NULL;
    DataSize            = 0;
    MemoryAllocationHob = NULL;

    Hob.Raw = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
    while (Hob.Raw != NULL) {
      MemoryAllocationHob = (EFI_HOB_MEMORY_ALLOCATION *)Hob.Raw;
      if (CompareGuid (
            &MemoryAllocationHob->AllocDescriptor.Name,
            &gEdkiiProtectedVariableGlobalGuid
            ))
      {
        Data = (VOID *)(UINTN)
               MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress;
        DataSize = (UINTN)MemoryAllocationHob->AllocDescriptor.MemoryLength;
        break;
      }

      Hob.Raw = GET_NEXT_HOB (Hob);
      Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw);
    }
  }

  if (Data == NULL) {
    return EFI_NOT_FOUND;
  }

  if (Global != NULL) {
    GuidHob = GetFirstGuidHob (&gEdkiiProtectedVariableContextGuid);
    if (GuidHob != NULL) {
      ContextIn = (PROTECTED_VARIABLE_CONTEXT_IN *)GET_GUID_HOB_DATA (GuidHob);
    } else {
      ASSERT (GuidHob == NULL);
    }

    *Global = (PROTECTED_VARIABLE_GLOBAL *)((UINT8 *)Data);
    //
    // Fix pointers in the HOB (due to physical memory readiness)
    //
    if ((*Global)->GlobalSelf != (EFI_PHYSICAL_ADDRESS)(UINTN)(*Global)) {
      OldStart             = (*Global)->GlobalSelf;
      (*Global)->ContextIn = GET_ADRS (ContextIn);

      //
      // Mark Memory caching is available
      //
      (*Global)->Flags.CacheReady = TRUE;

      //
      // Re-allocate new minimum cache
      //
      (*Global)->VariableCache = GET_ADRS (Data)
                                 + ((*Global)->VariableCache - OldStart);

      (*Global)->DigestContext = GET_ADRS (((*Global) + 1));
      for (Index = 0; Index < UnprotectedVarIndexMax; Index++) {
        if ((*Global)->Unprotected[Index] != VAR_INDEX_INVALID) {
          (*Global)->Unprotected[Index] = GET_ADRS (Data)
                                          + ((*Global)->Unprotected[Index] - OldStart);
        }
      }

      (*Global)->LastAccessedVariable = GET_ADRS (Data)
                                        + ((*Global)->LastAccessedVariable - OldStart);

      //
      // Fix all linked-list pointers inside VARIABLE_SIGNATURE.
      //
      (*Global)->VariableDigests = GET_ADRS (Data)
                                   + ((*Global)->VariableDigests - OldStart);
      VarDig = VAR_DIG_PTR ((*Global)->VariableDigests);
      while (VarDig != NULL) {
        if (VarDig->Prev != 0) {
          VarDig->Prev = GET_ADRS (Data) + (VarDig->Prev - OldStart);
        }

        if (VarDig->Next != 0) {
          VarDig->Next = GET_ADRS (Data) + (VarDig->Next - OldStart);
        }

        VarDig = VAR_DIG_NEXT (VarDig);
      }

      (*Global)->GlobalSelf = (EFI_PHYSICAL_ADDRESS)(UINTN)(*Global);
    }
  }

  return EFI_SUCCESS;
}

/**

  Derive HMAC key from given variable root key.

  @param[in]  RootKey       Pointer to root key to derive from.
  @param[in]  RootKeySize   Size of root key.
  @param[out] HmacKey       Pointer to generated HMAC key.
  @param[in]  HmacKeySize   Size of HMAC key.

  @retval TRUE      The HMAC key is derived successfully.
  @retval FALSE     Failed to generate HMAC key from given root key.

**/
BOOLEAN
EFIAPI
GenerateMetaDataHmacKey (
  IN   CONST UINT8  *RootKey,
  IN   UINTN        RootKeySize,
  OUT  UINT8        *HmacKey,
  IN   UINTN        HmacKeySize
  )
{
  UINT8  Salt[AES_BLOCK_SIZE];

  return HkdfSha256ExtractAndExpand (
           RootKey,
           RootKeySize,
           Salt,
           0,
           (UINT8 *)METADATA_HMAC_KEY_NAME,
           METADATA_HMAC_KEY_NAME_SIZE,
           HmacKey,
           HmacKeySize
           );
}

/**

  Return the size of variable MetaDataHmacVar.

  @param[in] AuthFlag         Auth-variable indicator.

  @retval size of variable MetaDataHmacVar.

**/
UINTN
GetMetaDataHmacVarSize (
  IN      BOOLEAN  AuthFlag
  )
{
  UINTN  Size;

  if (AuthFlag) {
    Size = sizeof (AUTHENTICATED_VARIABLE_HEADER);
  } else {
    Size = sizeof (VARIABLE_HEADER);
  }

  Size += METADATA_HMAC_VARIABLE_NAME_SIZE;
  Size += GET_PAD_SIZE (Size);
  Size += METADATA_HMAC_SIZE;
  Size += GET_PAD_SIZE (Size);

  return Size;
}

/**

  Digests the given variable data and updates HMAC context.

  @param[in]      Context        Pointer to initialized HMAC context.
  @param[in]      VarInfo        Pointer to variable data.
  @param[in]      UpdateMethod   Function to run when updating variable digest.

  @retval TRUE    HMAC context was updated successfully.
  @retval FALSE   Failed to update HMAC context.

**/
STATIC
BOOLEAN
UpdateVariableDigestData (
  IN  VOID                     *Context,
  IN  PROTECTED_VARIABLE_INFO  *VarInfo,
  IN  DIGEST_UPDATE            UpdateMethod
  )
{
  VOID     *Buffer[12];
  UINT32   BufferSize[12];
  UINTN    Index;
  BOOLEAN  Status;

  //
  // Empty variable is legal here (e.g. variable deletion case or write-init case).
  //
  if ((VarInfo == NULL) ||
      (VarInfo->CipherData == NULL) ||
      (VarInfo->CipherDataSize == 0))
  {
    return TRUE;
  }

  //
  // HMAC (":" || VariableName)
  //
  Buffer[0]     = METADATA_HMAC_SEP;
  BufferSize[0] = METADATA_HMAC_SEP_SIZE;

  Buffer[1]     = VarInfo->Header.VariableName;
  BufferSize[1] = (UINT32)VarInfo->Header.NameSize;

  //
  // HMAC (":" || VendorGuid || Attributes || DataSize)
  //
  Buffer[2]     = METADATA_HMAC_SEP;
  BufferSize[2] = METADATA_HMAC_SEP_SIZE;

  Buffer[3]     = VarInfo->Header.VendorGuid;
  BufferSize[3] = sizeof (EFI_GUID);

  Buffer[4]     = &VarInfo->Header.Attributes;
  BufferSize[4] = sizeof (VarInfo->Header.Attributes);

  Buffer[5]     = &VarInfo->CipherDataSize;
  BufferSize[5] = sizeof (VarInfo->CipherDataSize);

  //
  // HMAC (":" || CipherData)
  //
  Buffer[6]     = METADATA_HMAC_SEP;
  BufferSize[6] = METADATA_HMAC_SEP_SIZE;

  Buffer[7]     = VarInfo->CipherData;
  BufferSize[7] = VarInfo->CipherDataSize;

  //
  // HMAC (":" || PubKeyIndex || AuthMonotonicCount || TimeStamp)
  //
  Buffer[8]     = METADATA_HMAC_SEP;
  BufferSize[8] = METADATA_HMAC_SEP_SIZE;

  Buffer[9]     = &VarInfo->Header.PubKeyIndex;
  BufferSize[9] = sizeof (VarInfo->Header.PubKeyIndex);

  Buffer[10]     = &VarInfo->Header.MonotonicCount;
  BufferSize[10] = sizeof (VarInfo->Header.MonotonicCount);

  Buffer[11] = (VarInfo->Header.TimeStamp != NULL) ?
               VarInfo->Header.TimeStamp : &mDefaultTimeStamp;
  BufferSize[11] = sizeof (EFI_TIME);

  for (Index = 0; Index < ARRAY_SIZE (Buffer); ++Index) {
    Status = UpdateMethod (Context, Buffer[Index], BufferSize[Index]);
    if (!Status) {
      ASSERT (FALSE);
      return FALSE;
    }
  }

  return TRUE;
}

/**

  Digests the given variable data and updates HMAC context.

  @param[in]      Context   Pointer to initialized HMAC context.
  @param[in]      VarInfo   Pointer to variable data.

  @retval TRUE    HMAC context was updated successfully.
  @retval FALSE   Failed to update HMAC context.

**/
BOOLEAN
UpdateVariableMetadataHmac (
  IN  VOID                     *Context,
  IN  PROTECTED_VARIABLE_INFO  *VarInfo
  )
{
  return UpdateVariableDigestData (Context, VarInfo, (DIGEST_UPDATE)HmacSha256Update);
}

/**

  Get the variable digest.

  @param[in]      Global        Pointer to global configuration data.
  @param[in]      VarInfo       Pointer to verified copy of protected variables.
  @param[in,out]  DigestValue   Pointer to variable digest value.

  @retval EFI_INVALID_PARAMETER  Invalid parameter was passed in.
  @retval EFI_OUT_OF_RESOURCES   Not enough resource to calculate hash.
  @retval EFI_ABORTED            An error was encountered.
  @retval EFI_COMPROMISED_DATA   The data was compromised.
  @retval EFI_SUCCESS            Variable digest was successfully verified.

**/
EFI_STATUS
GetVariableDigest (
  IN      PROTECTED_VARIABLE_GLOBAL  *Global,
  IN      PROTECTED_VARIABLE_INFO    *VarInfo,
  IN  OUT UINT8                      *DigestValue
  )
{
  EFI_STATUS  Status;
  VOID        *Context;

  if ((Global == NULL) || (VarInfo == NULL) || (DigestValue == NULL)) {
    ASSERT (Global != NULL);
    ASSERT (VarInfo != NULL);
    ASSERT (DigestValue != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Context = GET_BUFR (Global->DigestContext);
  if (!HashApiInit (Context)) {
    ASSERT (Context != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  if (VarInfo->CipherData == NULL) {
    VarInfo->CipherData     = VarInfo->Header.Data;
    VarInfo->CipherDataSize = (UINT32)VarInfo->Header.DataSize;
  }

  if (  !UpdateVariableDigestData (Context, VarInfo, HashApiUpdate)
     || !HashApiFinal (Context, DigestValue))
  {
    ASSERT (FALSE);
    Status = EFI_ABORTED;
  } else {
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**

  Verify the variable digest.

  @param[in]  Global      Pointer to global configuration data.
  @param[in]  VarInfo     Pointer to verified copy of protected variables.
  @param[in]  VarDig      Pointer to variable digest data.

  @retval EFI_INVALID_PARAMETER  Invalid parameter was passed in.
  @retval EFI_OUT_OF_RESOURCES   Not enough resource to calculate hash.
  @retval EFI_ABORTED            An error was encountered.
  @retval EFI_COMPROMISED_DATA   The data was compromised.
  @retval EFI_SUCCESS            Variable digest was successfully verified.

**/
EFI_STATUS
VerifyVariableDigest (
  IN  PROTECTED_VARIABLE_GLOBAL  *Global,
  IN  PROTECTED_VARIABLE_INFO    *VarInfo,
  IN  VARIABLE_DIGEST            *VarDig
  )
{
  EFI_STATUS  Status;
  UINT8       NewDigest[METADATA_HMAC_SIZE];

  if (Global->Flags.RecoveryMode || !VarDig->Flags.Protected) {
    return EFI_SUCCESS;
  }

  ASSERT (VarDig->DigestSize == sizeof (NewDigest));

  Status = GetVariableDigest (Global, VarInfo, NewDigest);
  if (!EFI_ERROR (Status)) {
    if (CompareMem (VAR_DIG_VALUE (VarDig), NewDigest, VarDig->DigestSize) != 0) {
      Status = EFI_COMPROMISED_DATA;
    }
  }

  return Status;
}

/**
  Initialize variable MetaDataHmacVar.

  @param[in,out]  Variable      Pointer to buffer of MetaDataHmacVar.
  @param[in]      AuthFlag      Variable format flag.

**/
VOID
InitMetadataHmacVariable (
  IN  OUT VARIABLE_HEADER  *Variable,
  IN      BOOLEAN          AuthFlag
  )
{
  UINT8                          *NamePtr;
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  Variable->StartId    = VARIABLE_DATA;
  Variable->State      = VAR_ADDED;
  Variable->Reserved   = 0;
  Variable->Attributes = VARIABLE_ATTRIBUTE_NV_BS_RT;

  if (AuthFlag) {
    AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;

    AuthVariable->NameSize       = METADATA_HMAC_VARIABLE_NAME_SIZE;
    AuthVariable->DataSize       = METADATA_HMAC_SIZE;
    AuthVariable->PubKeyIndex    = 0;
    AuthVariable->MonotonicCount = 0;

    ZeroMem (&AuthVariable->TimeStamp, sizeof (EFI_TIME));
    CopyMem (&AuthVariable->VendorGuid, &METADATA_HMAC_VARIABLE_GUID, sizeof (EFI_GUID));

    NamePtr = (UINT8 *)AuthVariable + sizeof (AUTHENTICATED_VARIABLE_HEADER);
  } else {
    Variable->NameSize = METADATA_HMAC_VARIABLE_NAME_SIZE;
    Variable->DataSize = METADATA_HMAC_SIZE;

    CopyMem (&Variable->VendorGuid, &METADATA_HMAC_VARIABLE_GUID, sizeof (EFI_GUID));

    NamePtr = (UINT8 *)Variable + sizeof (VARIABLE_HEADER);
  }

  CopyMem (NamePtr, METADATA_HMAC_VARIABLE_NAME, METADATA_HMAC_VARIABLE_NAME_SIZE);
}

/**
  Re-calculate HMAC based on new variable data and re-generate MetaDataHmacVar.

  @param[in]      Global          Pointer to global configuration data.
  @param[in]      NewVarInfo      Pointer to buffer of new variable data.
  @param[in,out]  NewHmacVarInfo  Pointer to buffer of new MetaDataHmacVar.

  @return EFI_SUCCESS           The HMAC value was updated successfully.
  @return EFI_ABORTED           Failed to calculate the HMAC value.
  @return EFI_OUT_OF_RESOURCES  Not enough resource to calculate HMC value.
  @return EFI_NOT_FOUND         The MetaDataHmacVar was not found in storage.

**/
EFI_STATUS
RefreshVariableMetadataHmac (
  IN      PROTECTED_VARIABLE_GLOBAL  *Global,
  IN      PROTECTED_VARIABLE_INFO    *NewVarInfo,
  IN  OUT PROTECTED_VARIABLE_INFO    *NewHmacVarInfo
  )
{
  EFI_STATUS                     Status;
  VOID                           *Context;
  UINT32                         Counter;
  PROTECTED_VARIABLE_INFO        VarInfo;
  PROTECTED_VARIABLE_INFO        CurrHmacVarInfo;
  UINT8                          *HmacValue;
  PROTECTED_VARIABLE_CONTEXT_IN  *ContextIn;
  VARIABLE_DIGEST                *VarDig;
  VARIABLE_DIGEST                *HmacVarDig;

  ZeroMem ((VOID *)&VarInfo, sizeof (VarInfo));
  ZeroMem ((VOID *)&CurrHmacVarInfo, sizeof (CurrHmacVarInfo));

  Status = RequestMonotonicCounter (RPMC_COUNTER_2, &Counter);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Counter  += 1;
  ContextIn = GET_CNTX (Global);

  //
  // Delete current MetaDataHmacVariable first, if any.
  //
  if (Global->Unprotected[IndexHmacAdded] != VAR_INDEX_INVALID) {
    HmacVarDig = VAR_DIG_PTR (Global->Unprotected[IndexHmacAdded]);

    CurrHmacVarInfo.Header.NameSize     = HmacVarDig->NameSize;
    CurrHmacVarInfo.Header.VariableName = VAR_DIG_NAME (HmacVarDig);
    CurrHmacVarInfo.Header.VendorGuid   = VAR_DIG_GUID (HmacVarDig);

    CurrHmacVarInfo.Buffer     = VAR_HDR_PTR (HmacVarDig->CacheIndex);
    CurrHmacVarInfo.StoreIndex = HmacVarDig->StoreIndex;
    CurrHmacVarInfo.Flags.Auth = HmacVarDig->Flags.Auth;
    //
    // Force marking current MetaDataHmacVariable as VAR_IN_DELETED_TRANSITION.
    //
    CurrHmacVarInfo.Buffer->State &= VAR_IN_DELETED_TRANSITION;
    HmacVarDig->State             &= VAR_IN_DELETED_TRANSITION;
    Status                         = ContextIn->UpdateVariableStore (
                                                  &CurrHmacVarInfo,
                                                  OFFSET_OF (VARIABLE_HEADER, State),
                                                  sizeof (CurrHmacVarInfo.Buffer->State),
                                                  &CurrHmacVarInfo.Buffer->State
                                                  );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  } else if (Global->Unprotected[IndexHmacInDel] != VAR_INDEX_INVALID) {
    HmacVarDig = VAR_DIG_PTR (Global->Unprotected[IndexHmacInDel]);
  } else {
    //
    // No MetaDataHmacVar. Allocate space to cache its value.
    //
    HmacVarDig = CreateVariableDigestNode (
                   METADATA_HMAC_VARIABLE_NAME,
                   &METADATA_HMAC_VARIABLE_GUID,
                   METADATA_HMAC_VARIABLE_NAME_SIZE,
                   METADATA_HMAC_SIZE,
                   Global->Flags.Auth,
                   Global
                   );
    if (HmacVarDig == NULL) {
      ASSERT (HmacVarDig != NULL);
      return EFI_OUT_OF_RESOURCES;
    }

    HmacVarDig->Flags.Protected = FALSE;
  }

  if (HmacVarDig->CacheIndex == VAR_INDEX_INVALID) {
    HmacVarDig->CacheIndex = (GET_ADRS (Global)) + (Global->StructSize - GetMetaDataHmacVarSize (Global->Flags.Auth));
  }

  //
  // Construct new MetaDataHmacVar.
  //
  if (NewHmacVarInfo == NULL) {
    NewHmacVarInfo         = &VarInfo;
    NewHmacVarInfo->Buffer = GET_BUFR (HmacVarDig->CacheIndex);
  }

  InitMetadataHmacVariable (NewHmacVarInfo->Buffer, Global->Flags.Auth);

  NewHmacVarInfo->StoreIndex = VAR_INDEX_INVALID;     // Skip calculating offset
  NewHmacVarInfo->Flags.Auth = Global->Flags.Auth;
  Status                     = ContextIn->GetVariableInfo (NewHmacVarInfo);
  ASSERT_EFI_ERROR (Status);
  HmacValue = NewHmacVarInfo->Header.Data;

  //
  // Re-calculate HMAC for all valid variables
  //
  Context = HmacSha256New ();
  if (Context == NULL) {
    ASSERT (Context != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EFI_ABORTED;
  if (!HmacSha256SetKey (
         Context,
         Global->MetaDataHmacKey,
         sizeof (Global->MetaDataHmacKey)
         ))
  {
    ASSERT (FALSE);
    goto Done;
  }

  //
  // HMAC (|| hash(Var1) || hash(Var2) || ... || hash(VarN))
  //
  VarDig = VAR_DIG_PTR (Global->VariableDigests);
  while (VarDig != NULL) {
    if (VarDig->Flags.Valid && VarDig->Flags.Protected) {
      HmacSha256Update (Context, VAR_DIG_VALUE (VarDig), VarDig->DigestSize);
    }

    VarDig = VAR_DIG_NEXT (VarDig);
  }

  //
  // HMAC (RpmcMonotonicCounter)
  //
  if (!HmacSha256Update (Context, &Counter, sizeof (Counter))) {
    ASSERT (FALSE);
    goto Done;
  }

  if (!HmacSha256Final (Context, HmacValue)) {
    ASSERT (FALSE);
    goto Done;
  }

  //
  // Update HMAC value in cache.
  //
  CopyMem (VAR_DIG_VALUE (HmacVarDig), HmacValue, HmacVarDig->DataSize);
  if ((HmacVarDig->Prev == 0) && (HmacVarDig->Next == 0)) {
    InsertVariableDigestNode (Global, HmacVarDig, NULL);
  }

  //
  // Just one MetaDataHmacVar is needed for normal operation.
  //
  Global->Unprotected[IndexHmacAdded] = VAR_DIG_ADR (HmacVarDig);
  Global->Unprotected[IndexHmacInDel] = VAR_INDEX_INVALID;

  Status = EFI_SUCCESS;

Done:
  if (Context != NULL) {
    HmacSha256Free (Context);
  }

  return Status;
}

/**

  Check if a given variable is unprotected variable specified in advance
  and return its index ID.

  @param[in] Global     Pointer to global configuration data.
  @param[in] VarInfo    Pointer to variable information data.

  @retval IndexHmacInDel    Variable is MetaDataHmacVar in delete-transition state.
  @retval IndexHmacAdded    Variable is MetaDataHmacVar in valid state.
  @retval IndexErrorFlag    Variable is VarErrorLog.
  @retval Others            Variable is not any known unprotected variables.

**/
UNPROTECTED_VARIABLE_INDEX
CheckKnownUnprotectedVariable (
  IN  PROTECTED_VARIABLE_GLOBAL  *Global,
  IN  PROTECTED_VARIABLE_INFO    *VarInfo
  )
{
  UNPROTECTED_VARIABLE_INDEX  Index;

  if ((VarInfo == NULL) || (  (VarInfo->StoreIndex == VAR_INDEX_INVALID)
                           && (  (VarInfo->Header.VariableName == NULL)
                              || (VarInfo->Header.VendorGuid == NULL))))
  {
    ASSERT (VarInfo != NULL);
    ASSERT (VarInfo->StoreIndex != VAR_INDEX_INVALID);
    ASSERT (VarInfo->Header.VariableName != NULL);
    ASSERT (VarInfo->Header.VendorGuid != NULL);
    return UnprotectedVarIndexMax;
  }

  for (Index = 0; Index < UnprotectedVarIndexMax; ++Index) {
    if (  (Global->Unprotected[Index] != VAR_INDEX_INVALID)
       && (VarInfo->StoreIndex != VAR_INDEX_INVALID))
    {
      if (VarInfo->StoreIndex == VAR_DIG_PTR (Global->Unprotected[Index])->StoreIndex) {
        break;
      }
    } else if (IS_VARIABLE (
                 &VarInfo->Header,
                 mUnprotectedVariables[Index].VariableName,
                 mUnprotectedVariables[Index].VendorGuid
                 ) && (VarInfo->Header.State == mUnprotectedVariables[Index].State))
    {
      break;
    }
  }

  return Index;
}

/**

  Compare variable name and Guid

  @param[in]  Name1      Name of first variable.
  @param[in]  Name1Size  Size of first variable.
  @param[in]  Name2      Name of second variable.
  @param[in]  Name2Size  Size of second variable.
  @param[in]  Guid1      Guid for first variable.
  @param[in]  Guid2      Guid for second variable.

  @retval 0         First name is identical to Second name.
  @return others    First name is not identical to Second name.

**/
INTN
CompareVariableNameAndGuid (
  IN CONST CHAR16  *Name1,
  IN UINTN         Name1Size,
  IN CONST CHAR16  *Name2,
  IN UINTN         Name2Size,
  IN EFI_GUID      *Guid1,
  IN EFI_GUID      *Guid2
  )
{
  INTN  Result;

  Result = StrnCmp (
             Name1,
             Name2,
             MIN (Name1Size, Name2Size) / sizeof (CHAR16)
             );
  if (Result == 0) {
    if (Name1Size != Name2Size) {
      //
      // Longer name is 'bigger' than shorter one.
      //
      Result = (INTN)Name1Size - (INTN)Name2Size;
    } else {
      //
      // The variable name is the same. Compare the GUID.
      //
      Result = CompareMem ((VOID *)Guid1, (VOID *)Guid2, sizeof (EFI_GUID));
    }
  }

  return Result;
}

/**

  Compare variable digest.

  @param[in]  Variable1     Pointer to first variable digest.
  @param[in]  Variable2     Pointer to second variable digest.

  @retval 0         Variables are identical.
  @return others    Variables are not identical.

**/
INTN
CompareVariableDigestInfo (
  IN  VARIABLE_DIGEST  *Variable1,
  IN  VARIABLE_DIGEST  *Variable2
  )
{
  return CompareVariableNameAndGuid (
           VAR_DIG_NAME (Variable1),
           Variable1->NameSize,
           VAR_DIG_NAME (Variable2),
           Variable2->NameSize,
           &Variable1->VendorGuid,
           &Variable2->VendorGuid
           );
}

/**

  Move a node backward in the order controlled by SortMethod.

  @param[in,out]  Node          Pointer to node to be moved.
  @param[in]      SortMethod    Method used to compare node in list.

**/
VOID
MoveNodeBackward (
  IN  OUT VARIABLE_DIGEST  *Node,
  IN      SORT_METHOD      SortMethod
  )
{
  VARIABLE_DIGEST  *Curr;
  VARIABLE_DIGEST  *Prev;
  INTN             Result;

  Curr = Node;
  while (Curr != NULL) {
    Prev = VAR_DIG_PREV (Curr);
    if (Prev == NULL) {
      Result = -1;
    } else {
      Result = SortMethod (Prev, Node);
    }

    //
    // 'Result > 0' means the 'Prev' is 'bigger' than 'Node'. Continue to check
    // previous node til a node 'smaller' than 'Node' found.
    //
    if (Result > 0) {
      Curr = Prev;
      continue;
    }

    if (Curr != Node) {
      //
      // Remove Node first
      //
      if (VAR_DIG_PREV (Node) != NULL) {
        VAR_DIG_PREV (Node)->Next = Node->Next;
      }

      if (VAR_DIG_NEXT (Node) != NULL) {
        VAR_DIG_NEXT (Node)->Prev = Node->Prev;
      }

      //
      // Insert Node before Curr.
      //
      Node->Prev = Curr->Prev;
      Node->Next = VAR_DIG_ADR (Curr);

      if (Curr->Prev != 0) {
        VAR_DIG_PREV (Curr)->Next = VAR_DIG_ADR (Node);
      }

      Curr->Prev = VAR_DIG_ADR (Node);
    }

    //
    // If there're two identical variables in storage, one of them must be
    // "in-delete-transition" state. Mark it as "deleted" anyway.
    //
    if (Result == 0) {
      if (Curr->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION)) {
        Curr->State &= VAR_DELETED;
      }

      if (Prev->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION)) {
        Prev->State &= VAR_DELETED;
      }
    }

    break;
  }
}

/**

  Create variable digest node.

  @param[in]  VariableName      Name of variable.
  @param[in]  VendorGuid        Guid of variable.
  @param[in]  NameSize          Size of variable name.
  @param[in]  DataSize          Size of variable data.
  @param[in]  AuthVar           Authenticated variable flag.
  @param[in]  Global            Pointer to global configuration data.

  @retval Ptr   Pointer to variable digest

**/
VARIABLE_DIGEST *
CreateVariableDigestNode (
  IN CHAR16                     *VariableName,
  IN EFI_GUID                   *VendorGuid,
  IN UINT16                     NameSize,
  IN UINT32                     DataSize,
  IN BOOLEAN                    AuthVar,
  IN PROTECTED_VARIABLE_GLOBAL  *Global
  )
{
  VARIABLE_DIGEST  *VarDig;
  VOID             *Buffer;
  UINTN            VarSize;

  VarDig = (VARIABLE_DIGEST *)AllocateZeroPool (
                                sizeof (VARIABLE_DIGEST) + NameSize + METADATA_HMAC_SIZE
                                );
  if ((VarDig == NULL) || (Global == NULL)) {
    ASSERT (VarDig != NULL);
    ASSERT (Global != NULL);
    return NULL;
  }

  VarDig->DataSize        = DataSize;
  VarDig->NameSize        = NameSize;
  VarDig->DigestSize      = METADATA_HMAC_SIZE;
  VarDig->State           = VAR_ADDED;
  VarDig->Attributes      = VARIABLE_ATTRIBUTE_NV_BS_RT;
  VarDig->Flags.Auth      = AuthVar;
  VarDig->Flags.Valid     = TRUE;
  VarDig->Flags.Freeable  = TRUE;
  VarDig->Flags.Protected = PcdGetBool (PcdProtectedVariableIntegrity);
  VarDig->Flags.Encrypted = PcdGetBool (PcdProtectedVariableConfidentiality);
  VarDig->StoreIndex      = VAR_INDEX_INVALID;
  VarDig->CacheIndex      = VAR_INDEX_INVALID;

  if (Global->Flags.CacheReady == TRUE) {
    VarSize  =  VARIABLE_HEADER_SIZE (VarDig->Flags.Auth);
    VarSize += VarDig->NameSize + GET_PAD_SIZE (VarDig->NameSize);
    VarSize += VarDig->DataSize + GET_PAD_SIZE (VarDig->DataSize);
    VarSize  = HEADER_ALIGN (VarSize);

    Buffer = AllocateZeroPool (VarSize);
    if (Buffer != NULL) {
      VarDig->CacheIndex = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
    }
  }

  CopyMem (VAR_DIG_NAME (VarDig), VariableName, NameSize);
  CopyMem (VAR_DIG_GUID (VarDig), VendorGuid, sizeof (EFI_GUID));

  return VarDig;
}

/**

  Remove variable digest node.

  @param[in,out]  Global        Pointer to global configuration data.
  @param[in,out]  VarDig        Pointer to variable digest value.
  @param[in]      FreeResource  Flag to indicate whether to free resource.

**/
VOID
RemoveVariableDigestNode (
  IN  OUT PROTECTED_VARIABLE_GLOBAL  *Global,
  IN  OUT VARIABLE_DIGEST            *VarDig,
  IN      BOOLEAN                    FreeResource
  )
{
  VARIABLE_DIGEST  *Prev;
  VARIABLE_DIGEST  *Next;

  Prev = VAR_DIG_PREV (VarDig);
  Next = VAR_DIG_NEXT (VarDig);

  if (Global->VariableDigests == VAR_DIG_ADR (VarDig)) {
    Global->VariableDigests = VAR_DIG_ADR (Next);
  }

  if (Prev != NULL) {
    Prev->Next = VAR_DIG_ADR (Next);
  }

  if (Next != NULL) {
    Next->Prev = VAR_DIG_ADR (Prev);
  }

  VarDig->Prev        = 0;
  VarDig->Next        = 0;
  VarDig->Flags.Valid = FALSE;

  if (FreeResource && VarDig->Flags.Freeable) {
    if ((VarDig->CacheIndex != 0) && (VarDig->CacheIndex != VAR_INDEX_INVALID)) {
      VarDig->CacheIndex = VAR_INDEX_INVALID;
    }
  }
}

/**

  Insert variable digest node.

  @param[in,out]  Global        Pointer to global configuration data.
  @param[in]      VarDig        Pointer to variable digest value.
  @param[in]      SortMethod    Method for sorting.

**/
VOID
InsertVariableDigestNode (
  IN  OUT PROTECTED_VARIABLE_GLOBAL  *Global,
  IN      VARIABLE_DIGEST            *VarDig,
  IN      SORT_METHOD                SortMethod
  )
{
  VARIABLE_DIGEST  *Curr;
  VARIABLE_DIGEST  *Prev;
  BOOLEAN          DoReplace;
  INTN             Result;

  if (SortMethod == NULL) {
    SortMethod = CompareVariableDigestInfo;
  }

  DoReplace = FALSE;
  Curr      = VAR_DIG_PTR (Global->VariableDigests);
  if (Curr == NULL) {
    //
    // First one.
    //
    VarDig->Prev            = 0;
    VarDig->Next            = 0;
    Global->VariableDigests = VAR_DIG_ADR (VarDig);
    return;
  }

  while (Curr != NULL && Curr != VarDig) {
    Result = SortMethod (VarDig, Curr);

    if (Result <= 0) {
      ASSERT (VarDig->StoreIndex != Curr->StoreIndex);

      //
      // The same variable already in list?
      //
      if (Result == 0) {
        //
        // Keep only the same new one, unless states are different. In such
        // situation, the one with no VAR_ADDED will be deleted.
        //
        if (VarDig->State >= Curr->State) {
          DoReplace         = TRUE;
          Curr->Flags.Valid = FALSE;    // to-be-deleted
        } else {
          DoReplace           = FALSE;
          VarDig->Flags.Valid = FALSE;  // to-be-deleted
        }
      }

      //
      // Put VarDig before Curr
      //
      VarDig->Next = VAR_DIG_ADR (Curr);
      VarDig->Prev = Curr->Prev;

      if (VAR_DIG_PREV (Curr) != NULL) {
        VAR_DIG_PREV (Curr)->Next = VAR_DIG_ADR (VarDig);
      }

      Curr->Prev = VAR_DIG_ADR (VarDig);

      if (DoReplace) {
        RemoveVariableDigestNode (Global, Curr, TRUE);
      }

      break;
    }

    Prev = Curr;
    Curr = VAR_DIG_NEXT (Curr);
    if (Curr == NULL) {
      Prev->Next = VAR_DIG_ADR (VarDig);

      VarDig->Prev = VAR_DIG_ADR (Prev);
      VarDig->Next = 0;
    }
  }

  //
  // Update the head node if necessary.
  //
  if (VAR_DIG_PTR (VarDig->Prev) == NULL) {
    Global->VariableDigests = VAR_DIG_ADR (VarDig);
  }
}

/**

  Find the specified variable digest

  @param[in]  Global        Pointer to global configuration data.
  @param[in]  VarInfo       Pointer to variable data.
  @param[in]  FindNext      Flag to continue looking for variable.

**/
VARIABLE_DIGEST *
FindVariableInternal (
  IN PROTECTED_VARIABLE_GLOBAL  *Global,
  IN PROTECTED_VARIABLE_INFO    *VarInfo,
  IN BOOLEAN                    FindNext
  )
{
  VARIABLE_DIGEST  *VarDig;
  VARIABLE_DIGEST  *Found;
  VARIABLE_DIGEST  *FirstStoreIndexVar;
  BOOLEAN          ByIndex;
  INTN             FwdOrBwd;

  //
  // If VarInfo->StoreIndex is valid, use it to find the variable. Otherwise,
  // use the variable name and guid instead, if given. If no clue at all, return
  // the variable with lowest StoreIndex.
  //
  if (  (VarInfo->StoreIndex != VAR_INDEX_INVALID)
     || (VarInfo->Header.VariableName == NULL)
     || (VarInfo->Header.VendorGuid == NULL))
  {
    ByIndex = TRUE;
  } else {
    ByIndex = FALSE;
  }

  Found              = NULL;
  VarDig             = VAR_DIG_PTR (Global->VariableDigests);
  FirstStoreIndexVar = VarDig;
  FwdOrBwd           = 1;

  //
  // Discover variable with first/smallest store index
  //
  while (VarDig != NULL) {
    if (VarDig->StoreIndex < FirstStoreIndexVar->StoreIndex) {
      FirstStoreIndexVar = VAR_DIG_PTR (VarDig);
    }

    VarDig = VAR_DIG_NEXT (VarDig);
  }

  //
  // Input variable is NULL than return first variable
  // with smallest store index from the variable digest list.
  //
  if (((VarInfo->Header.VariableName == NULL) ||
       (VarInfo->Header.VendorGuid == NULL)) &&
      (ByIndex == FALSE))
  {
    return FirstStoreIndexVar;
  }

  //
  // Start with first entry
  //
  VarDig = VAR_DIG_PTR (Global->VariableDigests);
  while (VarDig != NULL) {
    if (ByIndex) {
      if (FindNext) {
        if (VarDig->StoreIndex == VarInfo->StoreIndex) {
          Found = VarDig = VAR_DIG_NEXT (VarDig);
          break;
        }
      } else if (VarDig->StoreIndex == VarInfo->StoreIndex) {
        Found = VarDig;
        break;
      }
    } else {
      //
      // Match given variable name and vendor guid.
      //
      if (IS_VARIABLE (&VarInfo->Header, VAR_DIG_NAME (VarDig), VAR_DIG_GUID (VarDig))) {
        Found = (FindNext) ? VAR_DIG_NEXT (VarDig) : VarDig;
        break;
      }
    }

    VarDig = (FwdOrBwd > 0) ? VAR_DIG_NEXT (VarDig) : VAR_DIG_PREV (VarDig);
    if (VarDig == NULL) {
    }
  }

  return Found;
}

/**

  Synchronize the RPMC counters

  @param[in]  Global      Pointer to global configuration data.
  @param[in]  VarInfo     Pointer to variable data.
  @param[in]  FindNext    Flag to continue looking for variable.

  @retval EFI_SUCCESS     Successfully sync RPMC counters.
  @return others          Failed to sync RPMC counters.

**/
EFI_STATUS
SyncRpmcCounter (
  VOID
  )
{
  UINT32      Counter1;
  UINT32      Counter2;
  EFI_STATUS  Status;

  //
  // Sync RPMC1 & RPMC2.
  //
  Status = RequestMonotonicCounter (RPMC_COUNTER_1, &Counter1);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = RequestMonotonicCounter (RPMC_COUNTER_2, &Counter2);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  while (Counter1 < Counter2) {
    Status = IncrementMonotonicCounter (RPMC_COUNTER_1);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    ++Counter1;
  }

  while (Counter2 < Counter1) {
    Status = IncrementMonotonicCounter (RPMC_COUNTER_2);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    ++Counter2;
  }

  return EFI_SUCCESS;
}

/**

  An alternative version of ProtectedVariableLibGetData to get plain data from
  given variable, if encrypted.

  @param[in]          Global        Pointer to global configuration data.
  @param[in,out]      VarInfo       Pointer to structure containing variable
                                    information. VarInfo->Header.Data must point
                                    to the original variable data.

  @retval EFI_SUCCESS               Found the specified variable.
  @retval EFI_INVALID_PARAMETER     VarInfo is NULL or both VarInfo->Buffer and
                                    VarInfo->Offset are invalid.
  @retval EFI_NOT_FOUND             The specified variable could not be found.

**/
STATIC
EFI_STATUS
ProtectedVariableLibGetDataInternal (
  IN      PROTECTED_VARIABLE_GLOBAL  *Global,
  IN  OUT PROTECTED_VARIABLE_INFO    *VarInfo
  )
{
  EFI_STATUS                     Status;
  PROTECTED_VARIABLE_CONTEXT_IN  *ContextIn;
  VOID                           *Buffer;
  UINTN                          BufferSize;

  if ((Global == NULL) || (VarInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ContextIn = GET_CNTX (Global);

  //
  // Check if the data has been decrypted or not.
  //
  VarInfo->CipherData     = NULL;
  VarInfo->CipherDataSize = 0;
  VarInfo->PlainData      = NULL;
  VarInfo->PlainDataSize  = 0;
  Status                  = GetCipherDataInfo (VarInfo);

  if ((Status == EFI_UNSUPPORTED) || (Status == EFI_NOT_FOUND)) {
    VarInfo->Flags.DecryptInPlace = TRUE;
    VarInfo->PlainDataSize        = (UINT32)VarInfo->Header.DataSize;
    VarInfo->PlainData            = VarInfo->Header.Data;
    VarInfo->CipherDataType       = 0;
    VarInfo->CipherHeaderSize     = 0;
    Status                        = EFI_SUCCESS;
  } else if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // If the variable data is cipher data, decrypt it inplace if possible.
  //
  if ((VarInfo->PlainData == NULL) && (VarInfo->CipherData != NULL)) {
    VarInfo->Key     = Global->RootKey;
    VarInfo->KeySize = sizeof (Global->RootKey);

    switch (ContextIn->VariableServiceUser) {
      case FromPeiModule:
        VarInfo->Flags.DecryptInPlace = FALSE;
        //
        // In PEI VariableCache holds Cipher header + Cipher data
        // Do not override Cipher header data during decrypt operation
        //
        VarInfo->PlainData = GET_BUFR (Global->VariableCache + VarInfo->CipherHeaderSize);

        Status = DecryptVariable (VarInfo);
        if (Status == EFI_UNSUPPORTED) {
          VarInfo->PlainData        = VarInfo->Header.Data;
          VarInfo->PlainDataSize    = (UINT32)VarInfo->Header.DataSize;
          VarInfo->CipherDataType   = 0;
          VarInfo->CipherHeaderSize = 0;

          Status = EFI_SUCCESS;
        }

        break;

      case FromSmmModule:
        VarInfo->Flags.DecryptInPlace = FALSE;
        VarInfo->PlainData            = GET_BUFR (Global->VariableCache);

        Status = DecryptVariable (VarInfo);
        if (Status == EFI_UNSUPPORTED) {
          VarInfo->PlainData        = VarInfo->Header.Data;
          VarInfo->PlainDataSize    = (UINT32)VarInfo->Header.DataSize;
          VarInfo->CipherDataType   = 0;
          VarInfo->CipherHeaderSize = 0;

          Status = EFI_SUCCESS;
        }

        break;

      case FromBootServiceModule:
      case FromRuntimeModule:
        //
        // The SMM passes back only decrypted data. We re-use the original cipher
        // data buffer to keep the plain data along with the cipher header.
        //
        VarInfo->Flags.DecryptInPlace = TRUE;
        Buffer                        = (VOID *)((UINTN)VarInfo->CipherData + VarInfo->CipherHeaderSize);
        BufferSize                    = VarInfo->PlainDataSize;
        Status                        = ContextIn->FindVariableSmm (
                                                     VarInfo->Header.VariableName,
                                                     VarInfo->Header.VendorGuid,
                                                     &VarInfo->Header.Attributes,
                                                     &BufferSize,
                                                     Buffer
                                                     );
        if (!EFI_ERROR (Status)) {
          //
          // Flag the payload as plain data to avoid re-decrypting.
          //
          VarInfo->CipherDataType = ENC_TYPE_NULL;
          VarInfo->PlainDataSize  = (UINT32)BufferSize;
          VarInfo->PlainData      = Buffer;

          Status = SetCipherDataInfo (VarInfo);
          if (Status == EFI_UNSUPPORTED) {
            Status = EFI_SUCCESS;
          }
        }

        break;

      default:
        Status = EFI_UNSUPPORTED;
        break;
    }

    VarInfo->CipherData     = NULL;
    VarInfo->CipherDataSize = 0;
  }

  return Status;
}

/**

  An alternative version of ProtectedVariableLibGetData to get plain data, if
  encrypted, from given variable, for different use cases.

  @param[in,out]      VarInfo     Pointer to structure containing variable information.

  @retval EFI_SUCCESS               Found the specified variable.
  @retval EFI_INVALID_PARAMETER     VarInfo is NULL or both VarInfo->Buffer and
                                    VarInfo->Offset are invalid.
  @retval EFI_NOT_FOUND             The specified variable could not be found.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetByInfo (
  IN  OUT PROTECTED_VARIABLE_INFO  *VarInfo
  )
{
  EFI_STATUS                 Status;
  PROTECTED_VARIABLE_GLOBAL  *Global;
  VOID                       **Buffer;
  UINT32                     BufferSize;

  Status = GetProtectedVariableGlobal (&Global);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // Save the output data buffer because below call
  // call will use this struct field internally.
  //
  Buffer     = VarInfo->PlainData;
  BufferSize = VarInfo->PlainDataSize;

  Status = ProtectedVariableLibGetDataInternal (Global, VarInfo);
  if (EFI_ERROR (Status)) {
    //
    // Return with caller provided buffer with zero DataSize
    //
    VarInfo->PlainData     = Buffer;
    VarInfo->PlainDataSize = 0;
    return Status;
  }

  if ((Buffer == NULL) || ((BufferSize) < VarInfo->PlainDataSize)) {
    //
    // Return with caller provided buffer with true PlainDataSize
    //
    VarInfo->PlainData = Buffer;
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Copy Plain data to ouput data buffer
  //
  CopyMem (Buffer, VarInfo->PlainData, VarInfo->PlainDataSize);
  VarInfo->PlainData = Buffer;

  return Status;
}

/**

  Retrieve plain data, if encrypted, of given variable.

  If variable encryption is employed, this function will initiate a SMM request
  to get the plain data. Due to security consideration, the decryption can only
  be done in SMM environment.

  @param[in]      Variable           Pointer to header of a Variable.
  @param[in,out]  Data               Pointer to plain data of the given variable.
  @param[in,out]  DataSize           Size of data returned or data buffer needed.
  @param[in]      AuthFlag           Auth-variable indicator.

  @retval EFI_SUCCESS                Found the specified variable.
  @retval EFI_INVALID_PARAMETER      Invalid parameter.
  @retval EFI_NOT_FOUND              The specified variable could not be found.
  @retval EFI_BUFFER_TOO_SMALL       If *DataSize is smaller than needed.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetByBuffer (
  IN      VARIABLE_HEADER  *Variable,
  IN  OUT VOID             *Data,
  IN  OUT UINT32           *DataSize,
  IN      BOOLEAN          AuthFlag
  )
{
  EFI_STATUS                     Status;
  PROTECTED_VARIABLE_INFO        VarInfo;
  PROTECTED_VARIABLE_GLOBAL      *Global;
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;
  VOID                           *Buffer;

  if ((Variable == NULL) || (DataSize == NULL)) {
    ASSERT (Variable != NULL);
    ASSERT (DataSize != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = GetProtectedVariableGlobal (&Global);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  ZeroMem (&VarInfo, sizeof (VarInfo));

  VarInfo.Buffer     = Variable;
  VarInfo.Flags.Auth = AuthFlag;

  if (VarInfo.Flags.Auth == TRUE) {
    AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;

    VarInfo.Header.VariableName = (CHAR16 *)((UINTN)Variable + sizeof (AUTHENTICATED_VARIABLE_HEADER));
    VarInfo.Header.NameSize     = AuthVariable->NameSize;
    VarInfo.Header.VendorGuid   = &AuthVariable->VendorGuid;
    VarInfo.Header.Attributes   = AuthVariable->Attributes;
    VarInfo.Header.DataSize     = AuthVariable->DataSize;
  } else {
    VarInfo.Header.VariableName = (CHAR16 *)((UINTN)Variable + sizeof (VARIABLE_HEADER));
    VarInfo.Header.NameSize     = Variable->NameSize;
    VarInfo.Header.VendorGuid   = &Variable->VendorGuid;
    VarInfo.Header.Attributes   = Variable->Attributes;
    VarInfo.Header.DataSize     = Variable->DataSize;
  }

  Buffer              = VARIABLE_NAME (VarInfo.Buffer, VarInfo.Flags.Auth);
  Buffer              = GET_BUFR (GET_ADRS (Buffer) + VarInfo.Header.NameSize);
  Buffer              = GET_BUFR (GET_ADRS (Buffer) + GET_PAD_SIZE (VarInfo.Header.NameSize));
  VarInfo.Header.Data = Buffer;

  Status = ProtectedVariableLibGetDataInternal (Global, &VarInfo);
  if (!EFI_ERROR (Status)) {
    if ((*DataSize) < VarInfo.PlainDataSize) {
      *DataSize = VarInfo.PlainDataSize;
      return EFI_BUFFER_TOO_SMALL;
    }

    *DataSize = VarInfo.PlainDataSize;
    CopyMem (Data, VarInfo.PlainData, VarInfo.PlainDataSize);
  }

  return Status;
}

/**
  This service retrieves a variable's value using its name and GUID.

  Read the specified variable from the UEFI variable store. If the Data
  buffer is too small to hold the contents of the variable, the error
  EFI_BUFFER_TOO_SMALL is returned and DataSize is set to the required buffer
  size to obtain the data.

  @param  VariableName          A pointer to a null-terminated string that is the variable's name.
  @param  VariableGuid          A pointer to an EFI_GUID that is the variable's GUID. The combination of
                                VariableGuid and VariableName must be unique.
  @param  Attributes            If non-NULL, on return, points to the variable's attributes.
  @param  DataSize              On entry, points to the size in bytes of the Data buffer.
                                On return, points to the size of the data returned in Data.
  @param  Data                  Points to the buffer which will hold the returned variable value.
                                May be NULL with a zero DataSize in order to determine the size of the buffer needed.

  @retval EFI_SUCCESS           The variable was read successfully.
  @retval EFI_NOT_FOUND         The variable was be found.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the resulting data.
                                DataSize is updated with the size required for
                                the specified variable.
  @retval EFI_INVALID_PARAMETER VariableName, VariableGuid, DataSize or Data is NULL.
  @retval EFI_DEVICE_ERROR      The variable could not be retrieved because of a device error.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetByName (
  IN      CONST  CHAR16    *VariableName,
  IN      CONST  EFI_GUID  *VariableGuid,
  OUT UINT32               *Attributes,
  IN  OUT UINTN            *DataSize,
  OUT VOID                 *Data OPTIONAL
  )
{
  EFI_STATUS                     Status;
  PROTECTED_VARIABLE_CONTEXT_IN  *ContextIn;
  PROTECTED_VARIABLE_GLOBAL      *Global;
  VARIABLE_DIGEST                *VarDig;
  PROTECTED_VARIABLE_INFO        VarInfo;
  EFI_TIME                       TimeStamp;
  VOID                           *DataBuffer;

  if ((VariableName == NULL) || (VariableGuid == NULL) || (DataSize == NULL)) {
    ASSERT (VariableName != NULL);
    ASSERT (VariableGuid != NULL);
    ASSERT (DataSize != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = GetProtectedVariableGlobal (&Global);

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  ContextIn = GET_CNTX (Global);

  ZeroMem (&VarInfo, sizeof (VarInfo));
  VarInfo.StoreIndex          = VAR_INDEX_INVALID;
  VarInfo.Header.VariableName = (CHAR16 *)VariableName;
  VarInfo.Header.NameSize     = StrSize (VariableName);
  VarInfo.Header.VendorGuid   = (EFI_GUID *)VariableGuid;

  VarDig = FindVariableInternal (Global, &VarInfo, FALSE);
  if (VarDig == NULL) {
    return EFI_NOT_FOUND;
  }

  if (Attributes != NULL) {
    *Attributes = VarDig->Attributes;
  }

  if ((Data == NULL) || (*DataSize < VarDig->PlainDataSize)) {
    *DataSize = VarDig->PlainDataSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  VarInfo.Flags.Auth      = VarDig->Flags.Auth;
  VarInfo.Flags.Protected = VarDig->Flags.Protected;

  //
  // Verify digest before copy the data back, if the variable is not in cache.
  //
  if (VarDig->CacheIndex != VAR_INDEX_INVALID) {
    VarInfo.Header.VariableName = NULL;
    VarInfo.Header.VendorGuid   = NULL;
    VarInfo.Buffer              = GET_BUFR (VarDig->CacheIndex);

    Status = ContextIn->GetVariableInfo (&VarInfo);
    ASSERT_EFI_ERROR (Status);
  } else {
    //
    // A buffer for at least one variable data (<=PcdMax(Auth)VariableSize)
    // must be reserved in advance.
    //
    ASSERT (
      Global->VariableCache != 0
           && Global->VariableCacheSize >= VarDig->DataSize
      );
    DataBuffer = GET_BUFR (Global->VariableCache);
    //
    // Note name and GUID are already there.
    //
    VarInfo.StoreIndex = VarDig->StoreIndex;

    VarInfo.Header.VariableName = NULL; // Prevent name from being retrieved again.
    VarInfo.Header.NameSize     = 0;
    VarInfo.Header.VendorGuid   = NULL; // Prevent guid from being retrieved again.
    VarInfo.Header.TimeStamp    = &TimeStamp;
    VarInfo.Header.Data         = DataBuffer;
    VarInfo.Header.DataSize     = VarDig->DataSize;

    //
    // Get detailed information about the variable.
    //
    Status = ContextIn->GetVariableInfo (&VarInfo);
    ASSERT_EFI_ERROR (Status);

    //
    // The variable must be validated its digest value to avoid TOCTOU, if it's
    // not been cached yet.
    //
    VarInfo.Header.VariableName = VAR_DIG_NAME (VarDig);
    VarInfo.Header.NameSize     = VarDig->NameSize;
    VarInfo.Header.VendorGuid   = &VarDig->VendorGuid;
    Status                      = VerifyVariableDigest (Global, &VarInfo, VarDig);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Decrypt the data, if necessary.
  //
  Status = ProtectedVariableLibGetDataInternal (Global, &VarInfo);
  if (!EFI_ERROR (Status)) {
    if (*DataSize < VarInfo.PlainDataSize) {
      *DataSize = VarInfo.PlainDataSize;
      return EFI_BUFFER_TOO_SMALL;
    }

    *DataSize = VarInfo.PlainDataSize;
    CopyMem (Data, VarInfo.PlainData, VarInfo.PlainDataSize);
  }

  return Status;
}

/**

  This function is used to enumerate the variables managed by current
  ProtectedVariableLib.

  If the VarInfo->StoreIndex is invalid (VAR_INDEX_INVALID), the first variable
  with the smallest StoreIndex will be returned. Otherwise, the variable with
  StoreIndex just after than the VarInfo->StoreIndex will be returned.

  @param[in,out]      VarInfo     Pointer to structure containing variable information.

  @retval EFI_SUCCESS               Found the specified variable.
  @retval EFI_INVALID_PARAMETER     VarInfo is NULL.
  @retval EFI_NOT_FOUND             The specified variable could not be found.

**/
STATIC
EFI_STATUS
GetNextVariableInternal (
  IN  OUT PROTECTED_VARIABLE_INFO  *VarInfo
  )
{
  EFI_STATUS                 Status;
  PROTECTED_VARIABLE_GLOBAL  *Global;
  VARIABLE_DIGEST            *Found;

  if (VarInfo == NULL) {
    ASSERT (VarInfo != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = GetProtectedVariableGlobal (&Global);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Found = FindVariableInternal (Global, VarInfo, TRUE);
  if (Found == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Return all cached data.
  //
  VarInfo->Header.VariableName = VAR_DIG_NAME (Found);
  VarInfo->Header.VendorGuid   = VAR_DIG_GUID (Found);
  VarInfo->Header.NameSize     = Found->NameSize;
  VarInfo->Header.DataSize     = Found->DataSize;
  VarInfo->Header.Attributes   = Found->Attributes;

  VarInfo->PlainDataSize = Found->PlainDataSize;
  VarInfo->StoreIndex    = Found->StoreIndex;
  if (Found->CacheIndex != VAR_INDEX_INVALID) {
    VarInfo->Buffer = GET_BUFR (Found->CacheIndex);
  }

  VarInfo->Flags.Auth = Found->Flags.Auth;

  return EFI_SUCCESS;
}

/**

  Find the request variable.

  @param[in, out]  VarInfo      Pointer to variable data.

  @retval EFI_SUCCESS           The variable was read successfully.
  @retval EFI_NOT_FOUND         The variable could not be found.
  @retval EFI_INVALID_PARAMETER Variable info is NULL.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibFind (
  IN  OUT PROTECTED_VARIABLE_INFO  *VarInfo
  )
{
  EFI_STATUS                 Status;
  PROTECTED_VARIABLE_GLOBAL  *Global;
  VARIABLE_DIGEST            *Found;

  if (VarInfo == NULL) {
    ASSERT (VarInfo != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = GetProtectedVariableGlobal (&Global);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Found = FindVariableInternal (Global, VarInfo, FALSE);
  if (Found == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Return all cached data.
  //
  VarInfo->Header.VariableName = VAR_DIG_NAME (Found);
  VarInfo->Header.VendorGuid   = VAR_DIG_GUID (Found);
  VarInfo->Header.NameSize     = Found->NameSize;
  VarInfo->Header.DataSize     = Found->DataSize;
  VarInfo->Header.Attributes   = Found->Attributes;

  VarInfo->PlainDataSize = Found->PlainDataSize;
  VarInfo->StoreIndex    = Found->StoreIndex;
  if (Found->CacheIndex != VAR_INDEX_INVALID) {
    VarInfo->Buffer = GET_BUFR (Found->CacheIndex);
  }

  VarInfo->Flags.Auth = Found->Flags.Auth;

  return EFI_SUCCESS;
}

/**
  Return the next variable name and GUID.

  This function is called multiple times to retrieve the VariableName
  and VariableGuid of all variables currently available in the system.
  On each call, the previous results are passed into the interface,
  and, on return, the interface returns the data for the next
  interface. When the entire variable list has been returned,
  EFI_NOT_FOUND is returned.

  @param  VariableNameSize  On entry, points to the size of the buffer pointed to by VariableName.
                            On return, the size of the variable name buffer.
  @param  VariableName      On entry, a pointer to a null-terminated string that is the variable's name.
                            On return, points to the next variable's null-terminated name string.
  @param  VariableGuid      On entry, a pointer to an EFI_GUID that is the variable's GUID.
                            On return, a pointer to the next variable's GUID.

  @retval EFI_SUCCESS           The variable was read successfully.
  @retval EFI_NOT_FOUND         The variable could not be found.
  @retval EFI_BUFFER_TOO_SMALL  The VariableNameSize is too small for the resulting
                                data. VariableNameSize is updated with the size
                                required for the specified variable.
  @retval EFI_INVALID_PARAMETER VariableName, VariableGuid or
                                VariableNameSize is NULL.
  @retval EFI_DEVICE_ERROR      The variable could not be retrieved because of a device error.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibFindNext (
  IN OUT UINTN     *VariableNameSize,
  IN OUT CHAR16    *VariableName,
  IN OUT EFI_GUID  *VariableGuid
  )
{
  EFI_STATUS                 Status;
  PROTECTED_VARIABLE_INFO    VarInfo;
  PROTECTED_VARIABLE_GLOBAL  *Global;
  VARIABLE_DIGEST            *VarDig;
  UINTN                      Size;

  if ((VariableNameSize == NULL) || (VariableName == NULL) || (VariableGuid == NULL)) {
    ASSERT (VariableNameSize != NULL);
    ASSERT (VariableName != NULL);
    ASSERT (VariableGuid != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = GetProtectedVariableGlobal (&Global);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SetMem (&VarInfo, sizeof (VarInfo), 0);
  Size = StrSize (VariableName);

  if (Size <= 2) {
    VarDig = VAR_DIG_PTR (Global->VariableDigests);
  } else {
    VarInfo.Header.VariableName = VariableName;
    VarInfo.Header.NameSize     = Size;
    VarInfo.Header.VendorGuid   = VariableGuid;

    VarInfo.StoreIndex = VAR_INDEX_INVALID;

    VarDig = FindVariableInternal (Global, &VarInfo, TRUE);
  }

  if (VarDig == NULL) {
    return EFI_NOT_FOUND;
  }

  if (VarDig->NameSize > *VariableNameSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (VariableName, VAR_DIG_NAME (VarDig), VarDig->NameSize);
  CopyGuid (VariableGuid, &VarDig->VendorGuid);
  *VariableNameSize = VarInfo.Header.NameSize;

  return EFI_SUCCESS;
}

/**

  Return the next variable name and GUID.

  @param[in, out]  VarInfo        Pointer to variable data.

  @retval EFI_SUCCESS             The variable was read successfully.
  @retval EFI_INVALID_PARAMETER   VarInfo is NULL.
  @retval EFI_NOT_FOUND           The specified variable could not be found.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibFindNextEx (
  IN  OUT PROTECTED_VARIABLE_INFO  *VarInfo
  )
{
  return GetNextVariableInternal (VarInfo);
}

/**

  Return the max count of a variable.

  @return   max count of a variable.

**/
UINTN
ProtectedVariableLibGetMaxVariablesCount (
  VOID
  )
{
  PROTECTED_VARIABLE_GLOBAL  *Global;
  PROTECTED_VARIABLE_INFO    VarInfo;
  VARIABLE_DIGEST            *VarDig;
  EFI_STATUS                 Status;
  UINTN                      Count;

  Status = GetProtectedVariableGlobal (&Global);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return 0;
  }

  Count = 0;
  ZeroMem (&VarInfo, sizeof (VarInfo));

  //
  // Start with first entry
  //
  VarDig                      = VAR_DIG_PTR (Global->VariableDigests);
  VarInfo.Header.VariableName = VAR_DIG_NAME (VarDig);
  VarInfo.Header.VendorGuid   = VAR_DIG_GUID (VarDig);
  VarInfo.StoreIndex          = VarDig->StoreIndex;

  do {
    VarInfo.Buffer = NULL;
    Status         = ProtectedVariableLibFindNextEx (&VarInfo);
    if (EFI_ERROR (Status)) {
      return Count;
    }

    Count++;
  } while (TRUE);
}

/**
  The function is called by PerformQuickSort to sort.

  @param[in] Left            The pointer to first buffer.
  @param[in] Right           The pointer to second buffer.

  @retval 0                  Buffer1 equal to Buffer2.
  @return < 0                Buffer1 is less than Buffer2.
  @return > 0                Buffer1 is greater than Buffer2.

**/
INTN
EFIAPI
CompareStoreIndex (
  IN CONST VOID  *Left,
  IN CONST VOID  *Right
  )
{
  EFI_PHYSICAL_ADDRESS  StoreIndex1;
  EFI_PHYSICAL_ADDRESS  StoreIndex2;

  StoreIndex1 = (*(EFI_PHYSICAL_ADDRESS *)Left);
  StoreIndex2 = (*(EFI_PHYSICAL_ADDRESS *)Right);

  if (StoreIndex1 == StoreIndex2) {
    return (0);
  }

  if (StoreIndex1 < StoreIndex2) {
    return (-1);
  }

  return (1);
}

/**
  Refresh variable information changed by variable service.

  @param Buffer           Pointer to a pointer of buffer.
  @param NumElements      Pointer to number of elements in list.


  @return EFI_SUCCESS     Successfully retrieved sorted list.
  @return others          Unsuccessful.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetSortedList (
  IN  OUT  EFI_PHYSICAL_ADDRESS  **Buffer,
  IN  OUT  UINTN                 *NumElements
  )
{
  EFI_STATUS                 Status;
  UINTN                      Count;
  UINTN                      StoreIndexTableSize;
  EFI_PHYSICAL_ADDRESS       *StoreIndexTable;
  PROTECTED_VARIABLE_INFO    VarInfo;
  PROTECTED_VARIABLE_GLOBAL  *Global;
  VARIABLE_DIGEST            *VarDig;

  Status = GetProtectedVariableGlobal (&Global);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Count = 0;
  ZeroMem (&VarInfo, sizeof (VarInfo));
  StoreIndexTableSize = ProtectedVariableLibGetMaxVariablesCount ();
  StoreIndexTable     = AllocateZeroPool (sizeof (EFI_PHYSICAL_ADDRESS) * StoreIndexTableSize);

  //
  // Start with first entry
  //
  VarDig                      = VAR_DIG_PTR (Global->VariableDigests);
  VarInfo.Header.VariableName = VAR_DIG_NAME (VarDig);
  VarInfo.Header.VendorGuid   = VAR_DIG_GUID (VarDig);
  VarInfo.StoreIndex          = VarDig->StoreIndex;
  StoreIndexTable[Count]      = VarInfo.StoreIndex;
  Count++;

  //
  // Populate the un-sorted table
  //
  do {
    VarInfo.Buffer = NULL;
    Status         = ProtectedVariableLibFindNextEx (&VarInfo);
    if (EFI_ERROR (Status)) {
      break;
    }

    StoreIndexTable[Count] = VarInfo.StoreIndex;
    Count++;
  } while (TRUE);

  PerformQuickSort (
    StoreIndexTable,
    Count,
    sizeof (EFI_PHYSICAL_ADDRESS),
    (SORT_COMPARE)CompareStoreIndex
    );

  *Buffer      = StoreIndexTable;
  *NumElements = Count;

  return EFI_SUCCESS;
}

/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <PiPei.h>

#include <Guid/VariableFormat.h>
#include <Ppi/MemoryDiscovered.h>

#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ReportStatusCodeLib.h>

#include "ProtectedVariableInternal.h"

/**
  Function allocates a global buffer.

  This function allocates a buffer with the specified size.

  @param[in] Size           Size of buffer to allocate.
  @param[in] AllocatePage   Whether to allocate pages.

  @retval Buffer             Pointer to the Buffer allocated.
  @retval NULL               if no Buffer was found.

**/
VOID *
AllocateGlobalBuffer (
  IN UINT32   Size,
  IN BOOLEAN  AllocatePage
  )
{
  VOID                       *Buffer;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryAllocationHob;
  EFI_PEI_HOB_POINTERS       Hob;

  Buffer = NULL;
  if (!AllocatePage) {
    Buffer = BuildGuidHob (&gEdkiiProtectedVariableGlobalGuid, Size);
  }

  if (Buffer == NULL) {
    //
    // Use the AllocatePages() to get over size limit of general GUID-ed HOB.
    //
    Buffer = AllocatePages (EFI_SIZE_TO_PAGES (Size));
    if (Buffer == NULL) {
      ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
      return NULL;
    }

    //
    // Mark the HOB holding the pages just allocated so that it can be
    // identified later.
    //
    MemoryAllocationHob = NULL;
    Hob.Raw             = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
    while (Hob.Raw != NULL) {
      MemoryAllocationHob = (EFI_HOB_MEMORY_ALLOCATION *)Hob.Raw;
      if ((UINTN)Buffer == (UINTN)MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress) {
        CopyGuid (
          &MemoryAllocationHob->AllocDescriptor.Name,
          &gEdkiiProtectedVariableGlobalGuid
          );
        break;
      }

      Hob.Raw = GET_NEXT_HOB (Hob);
      Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw);
    }
  }

  return Buffer;
}

/**
  Callback use to re-verify all variables and cache them in memory.

  @param[in] PeiServices          General purpose services available to every PEIM.
  @param[in] NotifyDescriptor     The notification structure this PEIM registered on install.
  @param[in] Ppi                  The memory discovered PPI.  Not used.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval others                  There's error in MP initialization.
**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                     Status;
  PROTECTED_VARIABLE_CONTEXT_IN  *ContextIn;
  PROTECTED_VARIABLE_GLOBAL      *Global;
  VARIABLE_DIGEST                *VarDig;
  PROTECTED_VARIABLE_INFO        VarInfo;
  VOID                           *Buffer;
  UINT32                         VarSize;
  INTN                           Result;

  Status = GetProtectedVariableGlobal (&Global);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  ContextIn = GET_CNTX (Global);

  VarSize = Global->VariableNumber * MAX_VARIABLE_SIZE;
  Buffer  = AllocateGlobalBuffer (VarSize, TRUE);

  //
  // It's safe to store the pointer in Global->VariableCache, which has been
  // cleared by above calling of GetProtectedVariableGlobal().
  //
  Global->VariableCache     = GET_ADRS (Buffer);
  Global->VariableCacheSize = EFI_SIZE_TO_PAGES (VarSize);

  //
  // Traverse all valid variables.
  //
  VarDig = VAR_DIG_PTR (Global->VariableDigests);
  while (VarDig != NULL) {
    if (VarDig->CacheIndex == VAR_INDEX_INVALID) {
      ASSERT (VarDig->StoreIndex != VAR_INDEX_INVALID);

      VarSize  =  VARIABLE_HEADER_SIZE (Global->Flags.Auth);
      VarSize += VarDig->NameSize + GET_PAD_SIZE (VarDig->NameSize);
      VarSize += VarDig->DataSize + GET_PAD_SIZE (VarDig->DataSize);
      VarSize  = HEADER_ALIGN (VarSize);

      //
      // Note the variable might be in unconsecutive space.
      //
      ZeroMem (&VarInfo, sizeof (VarInfo));
      VarInfo.StoreIndex = VarDig->StoreIndex;
      VarInfo.Buffer     = Buffer;
      VarInfo.Flags.Auth = VarDig->Flags.Auth;

      Status = ContextIn->GetVariableInfo (&VarInfo);
      ASSERT_EFI_ERROR (Status);
      //
      // VerifyVariableDigest() refers to CipherData for raw data.
      //
      VarInfo.CipherData     = VarInfo.Header.Data;
      VarInfo.CipherDataSize = (UINT32)VarInfo.Header.DataSize;

      //
      // Make sure that the cached copy is not compromised.
      //
      Status = VerifyVariableDigest (Global, &VarInfo, VarDig);
      if (EFI_ERROR (Status)) {
        REPORT_STATUS_CODE (
          EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED,
          (PcdGet32 (PcdStatusCodeVariableIntegrity) | (Status & 0xFF))
          );
        ASSERT_EFI_ERROR (Status);
        CpuDeadLoop ();
      }

      //
      // Simply use the cache address as CacheIndex of the variable.
      //
      VarDig->CacheIndex = GET_ADRS (Buffer);
      Buffer             = (UINT8 *)Buffer + MAX_VARIABLE_SIZE;
    } else {
      Result = StrnCmp (
                 VAR_DIG_NAME (VarDig),
                 METADATA_HMAC_VARIABLE_NAME,
                 METADATA_HMAC_VARIABLE_NAME_SIZE
                 );
      if (Result == 0) {
        CopyMem (
          Buffer,
          GET_BUFR (Global->GlobalSelf + (Global->StructSize - GetMetaDataHmacVarSize (Global->Flags.Auth))),
          GetMetaDataHmacVarSize (Global->Flags.Auth)
          );

        //
        // Simply use the cache address as CacheIndex of the variable.
        //
        VarDig->CacheIndex = GET_ADRS (Buffer);
        Buffer             = (UINT8 *)Buffer + MAX_VARIABLE_SIZE;
      }
    }

    VarDig = VAR_DIG_NEXT (VarDig);
  }

  return EFI_SUCCESS;
}

/**
  Callback use to perform variable integrity check.

  @param[in] PeiServices          General purpose services available to every PEIM.
  @param[in] NotifyDescriptor     The notification structure this PEIM registered on install.
  @param[in] Ppi                  The memory discovered PPI.  Not used.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval others                  There's error in MP initialization.
**/
EFI_STATUS
EFIAPI
VariableStoreDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                     Status;
  EFI_HOB_GUID_TYPE              *GuidHob;
  PROTECTED_VARIABLE_CONTEXT_IN  *ContextIn;

  GuidHob = GetFirstGuidHob (&gEdkiiProtectedVariableContextGuid);
  if (GuidHob != NULL) {
    ContextIn = (PROTECTED_VARIABLE_CONTEXT_IN *)GET_GUID_HOB_DATA (GuidHob);
  } else {
    ASSERT (GuidHob == NULL);
  }

  Status = ContextIn->IsHobVariableStoreAvailable ();

  if (Status == EFI_NOT_READY) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = PerformVariableIntegrityCheck (ContextIn);

  return Status;
}

EFI_PEI_NOTIFY_DESCRIPTOR  mPostMemNotifyList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMemoryDiscoveredPpiGuid,
    MemoryDiscoveredPpiNotifyCallback
  }
};

EFI_PEI_NOTIFY_DESCRIPTOR  mVariableStoreNotifyList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiVariableStoreDiscoveredPpiGuid,
    VariableStoreDiscoveredPpiNotifyCallback
  }
};

/**

  Get global data structure used to process protected variable.

  @param[out]   Global      Pointer to global configuration data.

  @retval EFI_SUCCESS         Get requested structure successfully.

**/
EFI_STATUS
EFIAPI
GetProtectedVariableGlobal (
  OUT PROTECTED_VARIABLE_GLOBAL  **Global OPTIONAL
  )
{
  return GetProtectedVariableGlobalFromHob (Global);
}

/**

  Get context data structure used to process protected variable.

  @param[out]   ContextIn   Pointer to context provided by variable runtime services.

  @retval EFI_SUCCESS         Get requested structure successfully.

**/
EFI_STATUS
EFIAPI
GetProtectedVariableContext (
  PROTECTED_VARIABLE_CONTEXT_IN  **ContextIn  OPTIONAL
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  GuidHob = GetFirstGuidHob (&gEdkiiProtectedVariableContextGuid);
  if (GuidHob != NULL) {
    *ContextIn = (PROTECTED_VARIABLE_CONTEXT_IN *)GET_GUID_HOB_DATA (GuidHob);
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Verify the HMAC value stored in MetaDataHmacVar against all valid and
  protected variables in storage.

  @param[in,out]  Global          Pointer to global configuration data.

  @retval   EFI_SUCCESS           The HMAC value matches.
  @retval   EFI_ABORTED           Error in HMAC value calculation.
  @retval   EFI_VOLUME_CORRUPTED  Inconsistency found in NV variable storage.
  @retval   EFI_COMPROMISED_DATA  The HMAC value doesn't match.

**/
EFI_STATUS
VerifyMetaDataHmac (
  IN OUT  PROTECTED_VARIABLE_GLOBAL  *Global
  )
{
  EFI_STATUS       Status;
  VARIABLE_DIGEST  *VariableDig;
  UINT32           Counter1;
  UINT32           Counter2;
  VOID             *Hmac1;
  VOID             *Hmac2;
  UINT8            HmacVal1[METADATA_HMAC_SIZE];
  UINT8            HmacVal2[METADATA_HMAC_SIZE];

  Hmac1 = NULL;
  Hmac2 = HmacSha256New ();
  if (Hmac2 == NULL) {
    ASSERT (Hmac2 != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  if (!HmacSha256SetKey (Hmac2, Global->MetaDataHmacKey, sizeof (Global->MetaDataHmacKey))) {
    ASSERT (FALSE);
    Status = EFI_ABORTED;
    goto Done;
  }

  //
  // Retrieve the RPMC counter value.
  //
  Status = RequestMonotonicCounter (RPMC_COUNTER_1, &Counter1);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto Done;
  }

  Status = RequestMonotonicCounter (RPMC_COUNTER_2, &Counter2);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto Done;
  }

  //
  // Counter1 must be either equal to Counter2 or just one step ahead of Counter2.
  //
  if ((Counter1 > Counter2) && ((Counter1 - Counter2) > 1)) {
    Status = EFI_COMPROMISED_DATA;
    goto Done;
  }

  VariableDig = VAR_DIG_PTR (Global->VariableDigests);
  while (VariableDig != NULL) {
    //
    // Only take valid protected variables into account.
    //
    if (VariableDig->Flags.Protected && VariableDig->Flags.Valid) {
      if (!HmacSha256Update (
             Hmac2,
             VAR_DIG_VALUE (VariableDig),
             VariableDig->DigestSize
             ))
      {
        ASSERT (FALSE);
        Status = EFI_ABORTED;
        goto Done;
      }
    }

    VariableDig = VAR_DIG_NEXT (VariableDig);
  }

  //
  // If two MetaDataHmacVariable were found, check which one is valid. We might
  // need two HMAC values to check against: one for Counter1, one for Counter2.
  //
  if (  (Global->Unprotected[IndexHmacAdded] != VAR_INDEX_INVALID)
     && (Global->Unprotected[IndexHmacInDel] != VAR_INDEX_INVALID)
     && (Counter1 != Counter2))
  {
    //
    // Might need to check Counter1. There must be something wrong in last boot.
    //
    Hmac1 = HmacSha256New ();
    if ((Hmac1 == NULL) || !HmacSha256Duplicate (Hmac2, Hmac1)) {
      ASSERT (FALSE);
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    if (  !HmacSha256Update (Hmac1, &Counter1, sizeof (Counter1))
       || !HmacSha256Final (Hmac1, HmacVal1))
    {
      ASSERT (FALSE);
      Status = EFI_ABORTED;
      goto Done;
    }
  }

  //
  // Always check Counter2.
  //
  if (  !HmacSha256Update (Hmac2, &Counter2, sizeof (Counter2))
     || !HmacSha256Final (Hmac2, HmacVal2))
  {
    ASSERT (FALSE);
    Status = EFI_ABORTED;
    goto Done;
  }

  //
  //  When writing (update or add) a variable, there must be following steps
  //  performed:
  //
  //    A - Increment Counter1
  //    B - Mark old MetaDataHmacVar as VAR_IN_DELETED_TRANSITION
  //    C - Calculate new HMAC value against Counter2+1,
  //        and force-add a new MetaDataHmacVar with state of VAR_ADDED
  //    D - Write the new protected variable
  //    E - Increment Counter2
  //    F - Mark old MetaDataHmacVar as VAR_DELETED
  //
  Status = EFI_COMPROMISED_DATA;
  if (  (Global->Unprotected[IndexHmacAdded] != VAR_INDEX_INVALID)
     && (Global->Unprotected[IndexHmacInDel] == VAR_INDEX_INVALID))
  {
    if (CompareMem (
          VAR_DIG_VALUE (VAR_DIG_PTR (Global->Unprotected[IndexHmacAdded])),
          HmacVal2,
          METADATA_HMAC_SIZE
          ) == 0)
    {
      //
      //
      // + A - Increment Counter1
      //   B - Mark old MetaDataHmacVar as VAR_IN_DELETED_TRANSITION
      //   C - Calculate new HMAC value against Counter2+1,
      //       and force-add a new MetaDataHmacVar with state of VAR_ADDED
      //   D - Write the new protected variable
      //   E - Increment Counter2
      //   F - Mark old MetaDataHmacVar as VAR_DELETED
      //
      // or,
      //
      // + A - Increment Counter1
      // + B - Mark old MetaDataHmacVar as VAR_IN_DELETED_TRANSITION
      // + C - Calculate new HMAC value against Counter2+1,
      //       and force-add a new MetaDataHmacVar with state of VAR_ADDED
      // + D - Write the new protected variable
      // + E - Increment Counter2
      // + F - Mark old MetaDataHmacVar as VAR_DELETED
      //
      Status = EFI_SUCCESS;

      VAR_DIG_PTR (Global->Unprotected[IndexHmacAdded])->Flags.Valid = TRUE;
    }
  } else if (  (Global->Unprotected[IndexHmacAdded] == VAR_INDEX_INVALID)
            && (Global->Unprotected[IndexHmacInDel] != VAR_INDEX_INVALID))
  {
    if (CompareMem (
          VAR_DIG_VALUE (VAR_DIG_PTR (Global->Unprotected[IndexHmacInDel])),
          HmacVal2,
          METADATA_HMAC_SIZE
          ) == 0)
    {
      //
      // + A - Increment Counter1
      // + B - Mark old MetaDataHmacVar as VAR_IN_DELETED_TRANSITION
      //   C - Calculate new HMAC value against Counter2+1,
      //       and force-add a new MetaDataHmacVar with state of VAR_ADDED
      //   D - Write the new protected variable
      //   E - Increment Counter2
      //   F - Mark old MetaDataHmacVar as VAR_DELETED
      //
      Status = EFI_SUCCESS;

      VAR_DIG_PTR (Global->Unprotected[IndexHmacInDel])->Flags.Valid = TRUE;
    }
  } else if (  (Global->Unprotected[IndexHmacAdded] != VAR_INDEX_INVALID)
            && (Global->Unprotected[IndexHmacInDel] != VAR_INDEX_INVALID))
  {
    if (Counter1 > Counter2) {
      if (CompareMem (
            VAR_DIG_VALUE (VAR_DIG_PTR (Global->Unprotected[IndexHmacInDel])),
            HmacVal2,
            METADATA_HMAC_SIZE
            ) == 0)
      {
        //
        // + A - Increment Counter1
        // + B - Mark old MetaDataHmacVar as VAR_IN_DELETED_TRANSITION
        // + C - Calculate new HMAC value against Counter2+1,
        //       and force-add a new MetaDataHmacVar with state VAR_ADDED
        //   D - Write the new protected variable
        //   E - Increment Counter2
        //   F - Mark old MetaDataHmacVar as VAR_DELETED
        //
        Status = EFI_SUCCESS;

        VAR_DIG_PTR (Global->Unprotected[IndexHmacAdded])->Flags.Valid = FALSE;
        VAR_DIG_PTR (Global->Unprotected[IndexHmacInDel])->Flags.Valid = TRUE;
      } else if (CompareMem (
                   VAR_DIG_VALUE (VAR_DIG_PTR (Global->Unprotected[IndexHmacAdded])),
                   HmacVal1,
                   METADATA_HMAC_SIZE
                   ) == 0)
      {
        //
        // + A - Increment Counter1
        // + B - Mark old MetaDataHmacVar as VAR_IN_DELETED_TRANSITION
        // + C - Calculate new HMAC value against Counter2+1,
        //       and force-add a new MetaDataHmacVar with state of VAR_ADDED
        // + D - Write the new protected variable
        //   E - Increment Counter2
        //   F - Mark old MetaDataHmacVar as VAR_DELETED
        //
        Status = EFI_SUCCESS;

        VAR_DIG_PTR (Global->Unprotected[IndexHmacAdded])->Flags.Valid = TRUE;
        VAR_DIG_PTR (Global->Unprotected[IndexHmacInDel])->Flags.Valid = FALSE;
      }
    } else {
      if (CompareMem (
            VAR_DIG_VALUE (VAR_DIG_PTR (Global->Unprotected[IndexHmacAdded])),
            HmacVal2,
            METADATA_HMAC_SIZE
            ) == 0)
      {
        //
        // + A - Increment Counter1
        // + B - Mark old MetaDataHmacVar as VAR_IN_DELETED_TRANSITION
        // + C - Calculate new HMAC value against Counter2+1,
        //       and force-add a new MetaDataHmacVar with state of VAR_ADDED
        // + D - Write the new protected variable
        // + E - Increment Counter2
        //   F - Mark old MetaDataHmacVar as VAR_DELETED
        //
        Status = EFI_SUCCESS;

        VAR_DIG_PTR (Global->Unprotected[IndexHmacAdded])->Flags.Valid = TRUE;
        VAR_DIG_PTR (Global->Unprotected[IndexHmacInDel])->Flags.Valid = FALSE;
      }
    }
  } else {
    //
    // There must be logic error or variable written to storage skipped
    // the protected variable service, if code reaches here.
    //
    ASSERT (FALSE);
  }

Done:
  if (Hmac1 != NULL) {
    HmacSha256Free (Hmac1);
  }

  if (Hmac2 != NULL) {
    HmacSha256Free (Hmac2);
  }

  return Status;
}

/**
  Collect variable digest information.

  This information is collected to be used to for integrity check.

  @param[in]       Global             Pointer to global configuration data.
  @param[in]       ContextIn          Pointer to variable service context needed by
                                      protected variable.
  @param[in, out]  DigestBuffer       Base address of digest of each variable.
  @param[out]      DigestBufferSize   Digest size of one variable if DigestBuffer is NULL.
                                      Size of DigestBuffer if DigestBuffer is NOT NULL.
  @param[out]      VariableNumber     Number of valid variables.

  @retval   EFI_SUCCESS             Successfully retreived variable digest.
  @retval   EFI_INVALID_PARAMETER   One ore more parameters are invalid.
  @retval   EFI_OUT_OF_RESOURCES    Unable to allocate memory.
  @retval   EFI_BUFFER_TOO_SMALL    The DigestBufferSize pass in is too small.

**/
EFI_STATUS
CollectVariableDigestInfo (
  IN      PROTECTED_VARIABLE_GLOBAL      *Global,
  IN      PROTECTED_VARIABLE_CONTEXT_IN  *ContextIn,
  IN  OUT VOID                           *DigestBuffer OPTIONAL,
  OUT UINT32                             *DigestBufferSize OPTIONAL,
  OUT UINT32                             *VariableNumber OPTIONAL
  )
{
  EFI_STATUS                  Status;
  PROTECTED_VARIABLE_INFO     VarInfo;
  UINT32                      VarNum;
  UINT32                      DigSize;
  VARIABLE_DIGEST             *VarDig;
  EFI_TIME                    TimeStamp;
  UNPROTECTED_VARIABLE_INDEX  VarIndex;

  //
  // This function might be called before Global is initialized. In that case,
  // Global must be NULL but not ContextIn.
  //
  if ((Global == NULL) && (ContextIn == NULL)) {
    ASSERT (Global != NULL || ContextIn != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if ((Global == NULL) && (DigestBuffer != NULL)) {
    ASSERT (Global != NULL && DigestBuffer != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (  (DigestBuffer != NULL)
     && ((DigestBufferSize == NULL) || (*DigestBufferSize == 0)))
  {
    ASSERT (
      DigestBuffer != NULL
           && DigestBufferSize != NULL && *DigestBufferSize > 0
      );
    return EFI_INVALID_PARAMETER;
  }

  if ((Global != NULL) && (ContextIn == NULL)) {
    ContextIn = GET_CNTX (Global);
  }

  DigSize = 0;
  VarNum  = 0;
  VarDig  = NULL;

  ZeroMem (&VarInfo, sizeof (VarInfo));
  VarInfo.StoreIndex = VAR_INDEX_INVALID; // To get the first variable.

  if ((Global != NULL) &&
      (Global->VariableCache != 0) &&
      (Global->VariableCacheSize > 0))
  {
    //
    // Use the variable cache to hold a copy of one variable.
    //
    VarInfo.Buffer = GET_BUFR (Global->VariableCache);
  } else {
    //
    // Allocate a buffer to hold a copy of one variable
    //
    VarInfo.Buffer = AllocatePages (EFI_SIZE_TO_PAGES (MAX_VARIABLE_SIZE));
    if (VarInfo.Buffer == NULL) {
      ASSERT (VarInfo.Buffer != NULL);
      return EFI_OUT_OF_RESOURCES;
    }
  }

  if ((DigestBuffer != NULL) && (*DigestBufferSize > 0)) {
    VarDig = DigestBuffer;
  }

  while (TRUE) {
    if (VarDig != NULL) {
      if (DigSize >= (*DigestBufferSize)) {
        //
        // Out of buffer.
        //
        break;
      }

      VarInfo.Header.VendorGuid   = &VarDig->VendorGuid;
      VarInfo.Header.VariableName = VAR_DIG_NAME (VarDig);
      VarInfo.Header.NameSize     = (UINTN)DigestBuffer + (UINTN)*DigestBufferSize
                                    - (UINTN)VarInfo.Header.VariableName;
      VarInfo.Header.TimeStamp = &TimeStamp;
      VarInfo.Header.Data      = NULL;
    } else {
      ZeroMem ((VOID *)&VarInfo.Header, sizeof (VarInfo.Header));
    }

    Status = ContextIn->GetNextVariableInfo (&VarInfo);
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Skip deleted variables.
    //
    if (  (VarInfo.Header.State != VAR_ADDED)
       && (VarInfo.Header.State != (VAR_ADDED & VAR_IN_DELETED_TRANSITION)))
    {
      continue;
    }

    if (Global != NULL) {
      Global->Flags.Auth &= VarInfo.Flags.Auth;
    }

    VarNum  += 1;
    DigSize += (UINT32)(sizeof (VARIABLE_DIGEST)
                        + VarInfo.Header.NameSize
                        + METADATA_HMAC_SIZE);
    if ((DigestBuffer != NULL) && (DigSize > *DigestBufferSize)) {
      ASSERT (DigSize <= *DigestBufferSize);
      return EFI_BUFFER_TOO_SMALL;
    }

    if (VarDig != NULL) {
      VarDig->Prev       = 0;
      VarDig->Next       = 0;
      VarDig->State      = VarInfo.Header.State;
      VarDig->Attributes = VarInfo.Header.Attributes;
      VarDig->DataSize   = (UINT32)VarInfo.Header.DataSize;
      VarDig->NameSize   = (UINT16)VarInfo.Header.NameSize;
      VarDig->DigestSize = METADATA_HMAC_SIZE;
      VarDig->StoreIndex = VarInfo.StoreIndex;

      if ((VarInfo.Buffer != NULL) && ((UINTN)VarInfo.Buffer != Global->VariableCache)) {
        VarDig->CacheIndex = GET_ADRS (VarInfo.Buffer);
      } else {
        VarDig->CacheIndex = VAR_INDEX_INVALID;
      }

      VarDig->Flags.Auth  = VarInfo.Flags.Auth;
      VarDig->Flags.Valid = TRUE;

      VarIndex = CheckKnownUnprotectedVariable (Global, &VarInfo);
      if (VarIndex >= UnprotectedVarIndexMax) {
        //
        // Check information relating to encryption, if enabled.
        //
        VarDig->Flags.Encrypted = FALSE;
        if ((VarInfo.Header.Data != NULL) && (VarInfo.Header.DataSize > 0)) {
          VarInfo.CipherData     = NULL;
          VarInfo.CipherDataSize = 0;
          VarInfo.PlainData      = NULL;
          VarInfo.PlainDataSize  = 0;
          Status                 = GetCipherDataInfo (&VarInfo);
          if (!EFI_ERROR (Status)) {
            //
            // Discovered encrypted variable mark variable to be
            // encrypted on the next SetVariable() operation
            //
            VarDig->Flags.Encrypted = PcdGetBool (PcdProtectedVariableConfidentiality);
          } else {
            VarInfo.PlainData        = VarInfo.Header.Data;
            VarInfo.PlainDataSize    = (UINT32)VarInfo.Header.DataSize;
            VarInfo.CipherDataType   = 0;
            VarInfo.CipherHeaderSize = 0;
            if (Status == EFI_NOT_FOUND) {
              //
              // Found variable that is not encrypted mark variable to be
              // encrypted on the next SetVariable() operation
              //
              VarDig->Flags.Encrypted = PcdGetBool (PcdProtectedVariableConfidentiality);
            }
          }
        }

        //
        // Variable is protected
        //
        VarDig->Flags.Protected = PcdGetBool (PcdProtectedVariableIntegrity);
        VarDig->PlainDataSize   = VarInfo.PlainDataSize;

        //
        // Calculate digest only for protected variable.
        //
        Status = GetVariableDigest (Global, &VarInfo, VAR_DIG_VALUE (VarDig));
        if (EFI_ERROR (Status)) {
          return Status;
        }

        //
        // Keep the VarDig in an ordered list.
        //
        InsertVariableDigestNode (Global, VarDig, CompareVariableDigestInfo);
      } else {
        VarDig->Flags.Protected = FALSE;
        VarDig->Flags.Encrypted = FALSE;
        VarDig->PlainDataSize   = VarDig->DataSize;

        //
        // Make use of VARIABLE_DIGEST->DigestValue to cache HMAC value from
        // MetaDataHmacVar, which doesn't need a digest value (only protected
        // variables need it for integrity check).
        //
        if ((VarIndex == IndexHmacInDel) || (VarIndex == IndexHmacAdded)) {
          if (VarDig->State == VAR_ADDED) {
            VarIndex = IndexHmacAdded;
          } else {
            VarIndex = IndexHmacInDel;
          }
        }

        Global->Unprotected[VarIndex] = VAR_DIG_ADR (VarDig);

        if ((VarInfo.Header.Data != NULL) && (VarDig->DataSize <= VarDig->DigestSize)) {
          CopyMem (VAR_DIG_VALUE (VarDig), VarInfo.Header.Data, VarDig->DataSize);
        }

        //
        // Don't add the VarDig for MetaDataHmacVar into the linked list now.
        // Do it after the HMAC has been validated.
        //
        if ((VarIndex != IndexHmacInDel) || (VarIndex != IndexHmacAdded)) {
          InsertVariableDigestNode (Global, VarDig, CompareVariableDigestInfo);
        }
      }

      VarDig = (VARIABLE_DIGEST *)((UINTN)VarDig + VAR_DIG_END (VarDig));
    }
  }

  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    return Status;
  }

  if (DigestBufferSize != NULL) {
    *DigestBufferSize = DigSize;
  }

  if (VariableNumber != NULL) {
    *VariableNumber = VarNum;
  }

  if ((Global == NULL) && (VarInfo.Buffer != NULL)) {
    //
    // Free Buffer
    //
    FreePages (VarInfo.Buffer, EFI_SIZE_TO_PAGES (MAX_VARIABLE_SIZE));
  }

  return EFI_SUCCESS;
}

/**

  Perform for protected variable integrity check.

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
PerformVariableIntegrityCheck (
  IN  PROTECTED_VARIABLE_CONTEXT_IN  *ContextIn
  )
{
  EFI_STATUS                 Status;
  UINT32                     HobDataSize;
  UINT32                     VarNumber;
  VOID                       *Buffer;
  PROTECTED_VARIABLE_GLOBAL  *Global;
  VARIABLE_DIGEST            *DigBuffer;
  UINT32                     DigBufferSize;
  UINT32                     HmacMetaDataSize;
  UINTN                      Index;
  BOOLEAN                    PreviousKey;
  EFI_HOB_GUID_TYPE          *GuidHob;

  if ((ContextIn == NULL) || (ContextIn->GetNextVariableInfo == NULL)) {
    ASSERT (ContextIn != NULL);
    ASSERT (ContextIn->GetNextVariableInfo != NULL);
    return EFI_INVALID_PARAMETER;
  }

  ContextIn->StructSize = (ContextIn->StructSize == 0) ? sizeof (*ContextIn)
                                                       : ContextIn->StructSize;

  //
  // Enumerate all variables first to collect info for resource allocation.
  //
  DigBufferSize = 0;
  Status        = CollectVariableDigestInfo (
                    NULL,
                    ContextIn,
                    NULL,
                    &DigBufferSize,
                    &VarNumber
                    );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return EFI_VOLUME_CORRUPTED;
  }

  //
  // Allocate buffer for Global. Memory layout:
  //
  //      Global
  //      Digest context
  //      Variable Digest List
  //      HmacMetaData
  //
  // To save precious NEM space of processor, variable cache will not be
  // allocated at this point until physical memory is ready for use.
  //
  HmacMetaDataSize = (UINT32)GetMetaDataHmacVarSize (TRUE);
  HobDataSize      = sizeof (PROTECTED_VARIABLE_GLOBAL)
                     + (UINT32)DIGEST_CONTEXT_SIZE
                     + DigBufferSize
                     + HmacMetaDataSize;
  Buffer = AllocateGlobalBuffer (HobDataSize, FALSE);
  if (Buffer == NULL) {
    ASSERT (Buffer != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  Global                = (PROTECTED_VARIABLE_GLOBAL *)((UINTN)Buffer);
  Global->DigestContext = GET_ADRS (Global + 1);

  if (DigBufferSize > 0) {
    DigBuffer = (VARIABLE_DIGEST *)(UINTN)(Global->DigestContext + DIGEST_CONTEXT_SIZE);
    ZeroMem (DigBuffer, DigBufferSize);
  } else {
    DigBuffer = NULL;
  }

  //
  // Keep a copy of ContextIn in HOB for later uses.
  //
  Global->GlobalSelf = GET_ADRS (Global);
  Global->ContextIn  = GET_ADRS (ContextIn);

  Global->StructVersion = PROTECTED_VARIABLE_CONTEXT_OUT_STRUCT_VERSION;
  Global->StructSize    = HobDataSize;

  Global->VariableNumber  = VarNumber;
  Global->VariableDigests = 0;

  Global->Flags.Auth       = TRUE;
  Global->Flags.WriteInit  = FALSE;
  Global->Flags.WriteReady = FALSE;

  GuidHob = GetFirstGuidHob (&gEfiAuthenticatedVariableGuid);
  if (GuidHob == NULL) {
    GuidHob = GetFirstGuidHob (&gEfiVariableGuid);
    if (GuidHob != NULL) {
      Global->Flags.Auth = FALSE;
    }
  }

  Global->Flags.RecoveryMode = (GuidHob != NULL);

  //
  // Before physical memory is ready, we cannot cache all variables in the very
  // limited NEM space. But we still need to reserve buffer to hold data of
  // one variable as well as context for integrity check (HMAC calculation).
  //
  Global->VariableCacheSize = MAX_VARIABLE_SIZE;
  Buffer                    = AllocatePages (EFI_SIZE_TO_PAGES (Global->VariableCacheSize));
  if (Buffer == NULL) {
    ASSERT (Buffer != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  Global->VariableCache        = GET_ADRS (Buffer);
  Global->LastAccessedVariable = VAR_INDEX_INVALID;

  for (Index = 0; Index < UnprotectedVarIndexMax; ++Index) {
    Global->Unprotected[Index] = VAR_INDEX_INVALID;
  }

  //
  // Re-enumerate all NV variables and build digest list.
  //
  Status = CollectVariableDigestInfo (
             Global,
             ContextIn,
             DigBuffer,
             &DigBufferSize,
             &VarNumber
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  ASSERT (Global->VariableNumber == VarNumber);

  //
  // Fix-up number of valid protected variables (i.e. exclude unprotected ones)
  //
  for (Index = 0; VarNumber != 0 && Index < UnprotectedVarIndexMax; ++Index) {
    if (Global->Unprotected[Index] != VAR_INDEX_INVALID) {
      --VarNumber;
    }
  }

  //
  // Get root key and generate HMAC key.
  //
  PreviousKey = FALSE;
  Status      = GetVariableKey ((VOID *)Global->RootKey, sizeof (Global->RootKey));
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    Status = EFI_COMPROMISED_DATA;
  }

  //
  // Derive the MetaDataHmacKey from root key
  //
  if (!GenerateMetaDataHmacKey (
         Global->RootKey,
         sizeof (Global->RootKey),
         Global->MetaDataHmacKey,
         sizeof (Global->MetaDataHmacKey)
         ))
  {
    ASSERT (FALSE);
    Status = EFI_COMPROMISED_DATA;
  }

  //
  // Check the integrity of all NV variables, if any.
  //
  if ((  (Global->Unprotected[IndexHmacAdded] != VAR_INDEX_INVALID)
      || (Global->Unprotected[IndexHmacInDel] != VAR_INDEX_INVALID)))
  {
    //
    // Validate the HMAC stored in variable MetaDataHmacVar.
    //
    Status = VerifyMetaDataHmac (Global);
    if (EFI_ERROR (Status)) {
      //
      // Try again with the previous root key if the latest key failed the HMAC validation.
      //
      Status = GetVariableKey ((VOID *)Global->RootKey, sizeof (Global->RootKey));
      if (!EFI_ERROR (Status)) {
        //
        // Derive the MetaDataHmacKey from previous root key
        //
        if (GenerateMetaDataHmacKey (
              Global->RootKey,
              sizeof (Global->RootKey),
              Global->MetaDataHmacKey,
              sizeof (Global->MetaDataHmacKey)
              ) == TRUE)
        {
          //
          // Validate the HMAC stored in variable MetaDataHmacVar.
          //
          Status = VerifyMetaDataHmac (Global);
          if (!EFI_ERROR (Status)) {
            Status = EFI_COMPROMISED_DATA;
          }
        } else {
          Status = EFI_COMPROMISED_DATA;
        }
      }
    }
  } else if (Global->Flags.RecoveryMode) {
    //
    // Generate the first version of MetaDataHmacVar.
    //
    Status = SyncRpmcCounter ();
    if (!EFI_ERROR (Status)) {
      Status = RefreshVariableMetadataHmac (Global, NULL, NULL);
      if (!EFI_ERROR (Status)) {
        //
        // MetaDataHmacVar is always calculated against Counter2+1. Updating
        // RPMCs to match it.
        //
        (VOID)IncrementMonotonicCounter (RPMC_COUNTER_1);
        (VOID)IncrementMonotonicCounter (RPMC_COUNTER_2);
      }
    }
  } else if ((VarNumber > 0) && !Global->Flags.RecoveryMode) {
    //
    // There's no MetaDataHmacVar found for protected variables. Suppose
    // the variable storage is compromised.
    //
    Status = EFI_COMPROMISED_DATA;
  }

  if (EFI_ERROR (Status)) {
    //
    // The integrity of variables have been compromised. The platform has to do
    // something to recover the variable store. But the boot should not go on
    // anyway this time.
    //
    DEBUG ((DEBUG_ERROR, "%a: %d Integrity check Status = %r\n", __FUNCTION__, __LINE__, Status));
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED,
      (PcdGet32 (PcdStatusCodeVariableIntegrity) | EFI_SW_PEI_PC_RECOVERY_BEGIN)
      );
 #if defined (EDKII_UNIT_TEST_FRAMEWORK_ENABLED) // Avoid test malfunctioning.
    return Status;
 #else
    ASSERT_EFI_ERROR (Status);
    CpuDeadLoop ();
 #endif
  }

  //
  // Everything's OK.
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    PcdGet32 (PcdStatusCodeVariableIntegrity)
    );

  if (GET_BUFR (Global->VariableCacheSize) != NULL) {
    //
    // Free Buffer
    //
    FreePages (Buffer, EFI_SIZE_TO_PAGES (Global->VariableCacheSize));
  }

  //
  // Keep the valid MetaDataHmacVar in the list.
  //
  for (Index = 0; Index < IndexPlatformVar; ++Index) {
    if (  (Global->Unprotected[Index] != VAR_INDEX_INVALID)
       && VAR_DIG_PTR (Global->Unprotected[Index])->Flags.Valid)
    {
      InsertVariableDigestNode (
        Global,
        VAR_DIG_PTR (Global->Unprotected[Index]),
        NULL
        );
    }
  }

  //
  // Restore the key to the latest one.
  //
  if (PreviousKey) {
    Status = GetVariableKey ((VOID *)Global->RootKey, sizeof (Global->RootKey));
    ASSERT_EFI_ERROR (Status);

    //
    // Derive the MetaDataHmacKey from root key
    //
    if (!GenerateMetaDataHmacKey (
           Global->RootKey,
           sizeof (Global->RootKey),
           Global->MetaDataHmacKey,
           sizeof (Global->MetaDataHmacKey)
           ))
    {
      ASSERT (FALSE);
    }
  }

  //
  // Make sure that the RPMC counter is in-sync.
  //
  Status = SyncRpmcCounter ();

  //
  // Setup a hook to migrate data in Global once physical memory is ready.
  //
  Status = PeiServicesNotifyPpi (mPostMemNotifyList);

  return Status;
}

/**

  Initialization for protected variable services.

  If the variable store is available than perform integrity check.
  Otherwise, defer integrity check until variable store is available.


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
  EFI_STATUS               Status;
  VOID                     *ContextInHob;
  PROTECTED_VARIABLE_INFO  VarInfo;

  if ((ContextIn == NULL) || (ContextIn->GetNextVariableInfo == NULL)) {
    ASSERT (ContextIn != NULL);
    ASSERT (ContextIn->GetNextVariableInfo != NULL);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Keep a copy of ContextIn in HOB for later uses.
  //
  ContextIn->StructSize = (ContextIn->StructSize == 0) ? sizeof (*ContextIn)
                                                       : ContextIn->StructSize;
  ContextInHob = BuildGuidHob (&gEdkiiProtectedVariableContextGuid, ContextIn->StructSize);
  CopyMem (ContextInHob, ContextIn, ContextIn->StructSize);

  //
  // Discover if Variable Store Info Hob has been published by platform driver.
  // It contains information regards to HOB or NV Variable Store availability
  //
  ZeroMem ((VOID *)&VarInfo.Header, sizeof (VarInfo.Header));
  VarInfo.StoreIndex = VAR_INDEX_INVALID;
  VarInfo.Buffer     = AllocatePages (EFI_SIZE_TO_PAGES (MAX_VARIABLE_SIZE));
  if (VarInfo.Buffer == NULL) {
    ASSERT (VarInfo.Buffer != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  FreePages (VarInfo.Buffer, EFI_SIZE_TO_PAGES ((MAX_VARIABLE_SIZE)));

  Status = ContextIn->GetNextVariableInfo (&VarInfo);
  if (EFI_ERROR (Status)) {
    //
    // Register for platform driver callback when Variable Store is available.
    //
    DEBUG ((DEBUG_INFO, "Variable Store is not available. Register for a integrity check callback\n"));
    Status = PeiServicesNotifyPpi (mVariableStoreNotifyList);
    return Status;
  }

  //
  // HOB Variable store is not available
  // Assume NV Variable store is available instead
  // Perform integrity check on NV Variable Store
  //
  DEBUG ((DEBUG_INFO, "NV Variable Store is available. Perform integrity check\n"));
  Status = PerformVariableIntegrityCheck (ContextInHob);
  return Status;
}

/**

  Prepare for variable update.

  (Not suppported in PEI phase.)

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

  Not supported in PEI phase.

  @param[in,out]  CurrVariable        Variable to be updated. It's NULL if
                                      adding a new variable.
  @param[in]      CurrVariableInDel   In-delete-transition copy of updating variable.
  @param[in,out]  NewVariable         Buffer of new variable data.
                                      Buffer of "MetaDataHmacVar" and new
                                      variable (encrypted).
  @param[in,out]  NewVariableSize     Size of NewVariable.
                                      Size of (encrypted) NewVariable and
                                      "MetaDataHmacVar".

  @retval EFI_UNSUPPORTED         Not support updating variable in PEI phase.

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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/**

  Finalize a variable updating after it's written to NV variable storage
  successfully.

  @param[in]      NewVariable       Buffer of new variables and MetaDataHmacVar.
  @param[in]      VariableSize      Size of buffer pointed by NewVariable.
  @param[in]      Offset            Offset to NV variable storage from where the new
                                    variable and MetaDataHmacVar have been written.

  @retval EFI_UNSUPPORTED           Not support updating variable in PEI phase.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibWriteFinal (
  IN  VARIABLE_HEADER  *NewVariable,
  IN  UINTN            VariableSize,
  IN  UINT64           StoreIndex
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/** @file
  This library is the PeilessSec version of the HashLib. It will
  initiate a hash on each supported hash algorithm via the TPM or
  TransferList.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  Copyright (c), Microsoft Corporation

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Guid/TcgEventHob.h>
#include <Guid/TpmInstance.h>
#include <Guid/TransferListHob.h>

#include <IndustryStandard/UefiTcgPlatform.h>

#include <Library/ArmTransferListLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/HashLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/Tpm2HelpLib.h>

/**
  Get transfer list header.

  @param[out] TransferList  Transfer list header

  @retval EFI_SUCCESS      Transfer list is found.
  @retval EFI_NOT_FOUND    Transfer list is not found.

**/
STATIC
EFI_STATUS
EFIAPI
GetTransferList (
  OUT TRANSFER_LIST_HEADER  **TransferList
  )
{
  VOID               *HobList;
  EFI_HOB_GUID_TYPE  *GuidHob;
  UINTN              *GuidHobData;

  *TransferList = NULL;

  HobList = GetHobList ();
  if (HobList == NULL) {
    return EFI_NOT_FOUND;
  }

  GuidHob = GetNextGuidHob (&gArmTransferListHobGuid, HobList);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  GuidHobData = GET_GUID_HOB_DATA (GuidHob);

  *TransferList = (TRANSFER_LIST_HEADER *)(*GuidHobData);

  return EFI_SUCCESS;
}

/**
  Get supported hash bitmap from the Transfer List and by
  querying the TPM for supported hashing algorithms.

  @param[out] SupportedHashBitmap

  @retval EFI_SUCCESS            Bitmap populated
  @retval EFI_INVALID_PARAMETER  Invalid pointer
  @retval EFI_NOT_FOUND          Error accessing data
  @retval EFI_DEVICE_ERROR       TPM device error

**/
STATIC
EFI_STATUS
EFIAPI
GetSupportedHashBitmap (
  OUT UINT32  *SupportedHashBitmap
  )
{
  EFI_STATUS                       Status;
  TRANSFER_LIST_HEADER             *TransferList;
  VOID                             *EventLog;
  UINTN                            EventLogSize;
  TCG_EfiSpecIDEventStruct         *TcgEfiSpecIdEventStruct;
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  UINTN                            Idx;
  UINT32                           NumberOfAlgorithms;
  UINT32                           TpmHashBitmap;
  UINT32                           PcrHashBitmap;
  BOOLEAN                          UseTlHashBitmap;

  if (SupportedHashBitmap == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Tpm2RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: TPM2 not detected!\n", __func__));
    return Status;
  }

  Status = Tpm2GetCapabilitySupportedAndActivePcrs (&TpmHashBitmap, &PcrHashBitmap);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Tpm capability... Status: %r\n", __func__, Status));
    return Status;
  }

  // NOTE: TpmHashBitmap is what the TPM supports, PcrHashBitmap is what is currently active
  DEBUG ((DEBUG_INFO, "TpmHashBitmap: %x, PcrHashBitmap: %x\n", TpmHashBitmap, PcrHashBitmap));

  UseTlHashBitmap = FALSE;
  Status          = GetTransferList (&TransferList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Unable to acquire Transfer list...\n", __func__));
    goto Exit;
  }

  if (TransferListCheckHeader (TransferList) == TRANSFER_LIST_OPS_INVALID) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Transfer list...\n", __func__));
    goto Exit;
  }

  Status = TransferListGetEventLog (TransferList, &EventLog, &EventLogSize, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: No data for TPM event log...\n", __func__));
    goto Exit;
  }

  UseTlHashBitmap         = TRUE;
  TcgEfiSpecIdEventStruct = (TCG_EfiSpecIDEventStruct *)
                            (EventLog + OFFSET_OF (TCG_PCR_EVENT, Event));

  CopyMem (&NumberOfAlgorithms, TcgEfiSpecIdEventStruct + 1, sizeof (NumberOfAlgorithms));
  DigestSize = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct + sizeof (*TcgEfiSpecIdEventStruct) + sizeof (NumberOfAlgorithms));
  DEBUG ((DEBUG_INFO, "%a: Transfer list TPM event log available\n", __func__));

  // Update the supported hash bitmap based on the info from the TCG event log
  for (Idx = 0; Idx < NumberOfAlgorithms; Idx++) {
    *SupportedHashBitmap |= GetHashMaskFromAlgo (DigestSize[Idx].algorithmId);
  }

  // The active PCR banks should match what is reported in the TCG event log
  if (PcrHashBitmap != *SupportedHashBitmap) {
    DEBUG ((DEBUG_ERROR, "Active PCRs & Transfer List mismatch!\n"));
    UseTlHashBitmap = FALSE;
  }

Exit:
  if (!UseTlHashBitmap) {
    // Use the information from the TPM to update the supported hash bitmap
    *SupportedHashBitmap = TpmHashBitmap;
    DEBUG ((DEBUG_INFO, "%a: No Transfer List or TPM event log available\n", __func__));
  }

  *SupportedHashBitmap &= PcrHashBitmap;
  if (*SupportedHashBitmap == 0x00) {
    DEBUG ((DEBUG_ERROR, "%a: No supported Hash algorithm with event log Spec...!\n", __func__));
  }

  return EFI_SUCCESS;
}

/**
  Start hash sequence.

  @param HashHandle Hash handle.

  @retval EFI_SUCCESS          Hash sequence start and HandleHandle returned.
  @retval EFI_OUT_OF_RESOURCES No enough resource to start hash.

**/
EFI_STATUS
EFIAPI
HashStart (
  OUT HASH_HANDLE  *HashHandle
  )
{
  EFI_STATUS   Status;
  TPM_ALG_ID   AlgoId;
  UINT32       Idx;
  UINT32       SupportedHashBitmap;
  HASH_HANDLE  *HashCtx;
  UINTN        HashInfoSize;

  SupportedHashBitmap = 0;
  Status              = GetSupportedHashBitmap (&SupportedHashBitmap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SupportedHashBitmap == 0) {
    return EFI_DEVICE_ERROR;
  }

  HashInfoSize = Tpm2GetHashInfoSize ();
  HashCtx      = AllocatePool (HashInfoSize * sizeof (HASH_HANDLE));
  if (HashCtx == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Idx = 0; Idx < HashInfoSize; Idx++) {
    if ((Tpm2GetHashMaskAtIndex (Idx) & SupportedHashBitmap) == 0) {
      continue;
    }

    AlgoId = Tpm2GetHashAlgoFromMask (Tpm2GetHashMaskAtIndex (Idx));
    if (AlgoId == TPM_ALG_ERROR) {
      return EFI_UNSUPPORTED;
    }

    Status = Tpm2HashSequenceStart (AlgoId, (TPMI_DH_OBJECT *)&HashCtx[Idx]);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  *HashHandle = (HASH_HANDLE)HashCtx;

  return Status;
}

/**
  Update hash sequence data.

  @param HashHandle    Hash handle.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.

  @retval EFI_SUCCESS     Hash sequence updated.

**/
EFI_STATUS
EFIAPI
HashUpdate (
  IN HASH_HANDLE  HashHandle,
  IN VOID         *DataToHash,
  IN UINTN        DataToHashLen
  )
{
  EFI_STATUS        Status;
  UINT32            Idx;
  UINT8             *Buffer;
  UINT64            HashLen;
  TPM2B_MAX_BUFFER  HashBuffer;
  UINT32            SupportedHashBitmap;
  HASH_HANDLE       *HashCtx;
  UINTN             HashInfoSize;

  SupportedHashBitmap = 0;
  Status              = GetSupportedHashBitmap (&SupportedHashBitmap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SupportedHashBitmap == 0) {
    return EFI_DEVICE_ERROR;
  }

  HashCtx = (HASH_HANDLE *)HashHandle;

  HashInfoSize = Tpm2GetHashInfoSize ();
  for (Idx = 0; Idx < HashInfoSize; Idx++) {
    if ((Tpm2GetHashMaskAtIndex (Idx) & SupportedHashBitmap) == 0) {
      continue;
    }

    Buffer = (UINT8 *)(UINTN)DataToHash;
    for (HashLen = DataToHashLen; HashLen > sizeof (HashBuffer.buffer); HashLen -= sizeof (HashBuffer.buffer)) {
      HashBuffer.size = sizeof (HashBuffer.buffer);
      CopyMem (HashBuffer.buffer, Buffer, sizeof (HashBuffer.buffer));
      Buffer += sizeof (HashBuffer.buffer);

      Status = Tpm2SequenceUpdate ((TPMI_DH_OBJECT)HashCtx[Idx], &HashBuffer);
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }
    }

    // Last one
    HashBuffer.size = (UINT16)HashLen;
    CopyMem (HashBuffer.buffer, Buffer, (UINTN)HashLen);
    Status = Tpm2SequenceUpdate ((TPMI_DH_OBJECT)HashCtx[Idx], &HashBuffer);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
  }

  return Status;
}

/**
  Hash sequence complete and extend to PCR.

  @param HashHandle    Hash handle.
  @param PcrIndex      PCR to be extended.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.
  @param DigestList    Digest list.

  @retval EFI_SUCCESS     Hash sequence complete and DigestList is returned.

**/
EFI_STATUS
EFIAPI
HashCompleteAndExtend (
  IN HASH_HANDLE          HashHandle,
  IN TPMI_DH_PCR          PcrIndex,
  IN VOID                 *DataToHash,
  IN UINTN                DataToHashLen,
  OUT TPML_DIGEST_VALUES  *DigestList
  )
{
  EFI_STATUS        Status;
  UINT32            Idx;
  UINT32            DigestIdx;
  UINT8             *Buffer;
  UINT64            HashLen;
  TPM2B_MAX_BUFFER  HashBuffer;
  TPM_ALG_ID        AlgoId;
  TPM2B_DIGEST      Result;
  UINT32            SupportedHashBitmap;
  HASH_HANDLE       *HashCtx;
  UINTN             HashInfoSize;

  SupportedHashBitmap = 0;
  Status              = GetSupportedHashBitmap (&SupportedHashBitmap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SupportedHashBitmap == 0) {
    return EFI_DEVICE_ERROR;
  }

  ZeroMem (DigestList, sizeof (*DigestList));
  DigestList->count = HASH_COUNT;
  DigestIdx         = 0;
  HashCtx           = (HASH_HANDLE *)HashHandle;

  HashInfoSize = Tpm2GetHashInfoSize ();
  for (Idx = 0; Idx < HashInfoSize; Idx++) {
    if ((Tpm2GetHashMaskAtIndex (Idx) & SupportedHashBitmap) == 0) {
      continue;
    }

    Buffer = (UINT8 *)(UINTN)DataToHash;
    for (HashLen = DataToHashLen; HashLen > sizeof (HashBuffer.buffer); HashLen -= sizeof (HashBuffer.buffer)) {
      HashBuffer.size = sizeof (HashBuffer.buffer);
      CopyMem (HashBuffer.buffer, Buffer, sizeof (HashBuffer.buffer));
      Buffer += sizeof (HashBuffer.buffer);

      Status = Tpm2SequenceUpdate ((TPMI_DH_OBJECT)HashCtx[Idx], &HashBuffer);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
    }

    // Last one
    HashBuffer.size = (UINT16)HashLen;
    CopyMem (HashBuffer.buffer, Buffer, (UINTN)HashLen);

    Status = Tpm2SequenceComplete ((TPMI_DH_OBJECT)HashCtx[Idx], &HashBuffer, &Result);
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    AlgoId = Tpm2GetHashAlgoFromMask (Tpm2GetHashMaskAtIndex (Idx));
    if (AlgoId == TPM_ALG_ERROR) {
      Status = EFI_UNSUPPORTED;
      goto Error;
    }

    // Copy the result of hash.
    CopyMem (&DigestList->digests[DigestIdx].digest, Result.buffer, Result.size);
    DigestList->digests[DigestIdx].hashAlg = AlgoId;
    DigestIdx++;
  }

  DigestList->count = DigestIdx;

  Status = Tpm2PcrExtend (PcrIndex, DigestList);

Error:
  FreePool (HashCtx);

  return Status;
}

/**
  Hash data and extend to PCR.

  @param PcrIndex      PCR to be extended.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.
  @param DigestList    Digest list.

  @retval EFI_SUCCESS     Hash data and DigestList is returned.

**/
EFI_STATUS
EFIAPI
HashAndExtend (
  IN TPMI_DH_PCR          PcrIndex,
  IN VOID                 *DataToHash,
  IN UINTN                DataToHashLen,
  OUT TPML_DIGEST_VALUES  *DigestList
  )
{
  EFI_STATUS        Status;
  UINT8             *Buffer;
  UINT64            HashLen;
  TPMI_DH_OBJECT    SequenceHandle;
  TPM2B_MAX_BUFFER  HashBuffer;
  TPM2B_DIGEST      Result;
  TPM_ALG_ID        AlgoId;
  UINT32            Idx;
  UINT32            DigestIdx;
  UINT32            SupportedHashBitmap;
  UINT32            HashInfoSize;

  DEBUG ((DEBUG_VERBOSE, "\n HashAndExtend Entry \n"));

  SupportedHashBitmap = 0;
  Status              = GetSupportedHashBitmap (&SupportedHashBitmap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SupportedHashBitmap == 0x00) {
    return EFI_DEVICE_ERROR;
  }

  ZeroMem (DigestList, sizeof (*DigestList));
  DigestList->count = HASH_COUNT;
  DigestIdx         = 0;

  HashInfoSize = Tpm2GetHashInfoSize ();
  for (Idx = 0; Idx < HashInfoSize; Idx++) {
    if ((Tpm2GetHashMaskAtIndex (Idx) & SupportedHashBitmap) == 0) {
      continue;
    }

    DEBUG ((DEBUG_INFO, "Hashing with Mask: %x\n", Tpm2GetHashMaskAtIndex (Idx)));
    AlgoId = Tpm2GetHashAlgoFromMask (Tpm2GetHashMaskAtIndex (Idx));
    if (AlgoId == TPM_ALG_ERROR) {
      return EFI_UNSUPPORTED;
    }

    Status = Tpm2HashSequenceStart (AlgoId, &SequenceHandle);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    DEBUG ((DEBUG_VERBOSE, "\n Tpm2HashSequenceStart Success \n"));
    DEBUG ((DEBUG_INFO, "Hashing %d bytes of data\n", DataToHashLen));

    Buffer = (UINT8 *)(UINTN)DataToHash;
    for (HashLen = DataToHashLen; HashLen > sizeof (HashBuffer.buffer); HashLen -= sizeof (HashBuffer.buffer)) {
      HashBuffer.size = sizeof (HashBuffer.buffer);
      CopyMem (HashBuffer.buffer, Buffer, sizeof (HashBuffer.buffer));
      Buffer += sizeof (HashBuffer.buffer);

      Status = Tpm2SequenceUpdate (SequenceHandle, &HashBuffer);
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }
    }

    DEBUG ((DEBUG_VERBOSE, "\n Tpm2SequenceUpdate Success \n"));

    HashBuffer.size = (UINT16)HashLen;
    CopyMem (HashBuffer.buffer, Buffer, (UINTN)HashLen);

    Status = Tpm2SequenceComplete (SequenceHandle, &HashBuffer, &Result);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    DEBUG ((DEBUG_VERBOSE, "\n Tpm2SequenceComplete Success \n"));

    CopyMem (&DigestList->digests[DigestIdx].digest, Result.buffer, Result.size);
    DigestList->digests[DigestIdx].hashAlg = AlgoId;
    DigestIdx++;
  }

  DigestList->count = DigestIdx;

  DEBUG ((DEBUG_INFO, "Extending to PCR%d\n", PcrIndex));
  Status = Tpm2PcrExtend (PcrIndex, DigestList);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  DEBUG ((DEBUG_VERBOSE, "\n Tpm2PcrExtend Success \n"));

  return EFI_SUCCESS;
}

/**
  This service register Hash.

  @param HashInterface  Hash interface

  @retval EFI_SUCCESS          This hash interface is registered successfully.
  @retval EFI_UNSUPPORTED      System does not support register this interface.
  @retval EFI_ALREADY_STARTED  System already register this interface.

**/
EFI_STATUS
EFIAPI
RegisterHashInterfaceLib (
  IN HASH_INTERFACE  *HashInterface
  )
{
  return EFI_UNSUPPORTED;
}

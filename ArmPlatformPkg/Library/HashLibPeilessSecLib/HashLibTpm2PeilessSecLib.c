/** @file

  Copyright (c) 2025, Arm Limited. All rights reserved.

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

#define HASH_ALG_ERROR   0x00
#define BAD_SEQ_HANDLE   0xFFFFFFFF
#define BAD_HASH_HANDLE  0x00

typedef struct {
  TPM_ALG_ID        AlgoId;
  UINT32            Mask;
  TPMI_DH_OBJECT    SequenceHandle;
} TPM2_HASH_MASK;

TPM2_HASH_MASK  mTpm2HashMask[] = {
  { TPM_ALG_SHA1,   HASH_ALG_SHA1,   BAD_SEQ_HANDLE },
  { TPM_ALG_SHA256, HASH_ALG_SHA256, BAD_SEQ_HANDLE },
  { TPM_ALG_SHA384, HASH_ALG_SHA384, BAD_SEQ_HANDLE },
  { TPM_ALG_SHA512, HASH_ALG_SHA512, BAD_SEQ_HANDLE },
};

STATIC UINT32   mSupportedHashBitmap;
STATIC BOOLEAN  mHashLibDisabled;

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
  The function get algorithm from hash mask info.

  @param[in]  HashMask

  @return Hash algorithm

**/
STATIC
TPM_ALG_ID
EFIAPI
Tpm2GetAlgoFromHashMask (
  IN UINT32  HashMask
  )
{
  UINTN  Idx;

  for (Idx = 0; Idx < ARRAY_SIZE (mTpm2HashMask); Idx++) {
    if (mTpm2HashMask[Idx].Mask == HashMask) {
      return mTpm2HashMask[Idx].AlgoId;
    }
  }

  return TPM_ALG_ERROR;
}

/**
  The function get hashmask from algorithm info.

  @param[in]  AlgoId

  @return Hash mask

**/
STATIC
UINT32
EFIAPI
Tpm2GetHashMaskFromAlgo (
  TPM_ALG_ID  AlgoId
  )
{
  UINTN  Idx;

  for (Idx = 0; Idx < ARRAY_SIZE (mTpm2HashMask); Idx++) {
    if (mTpm2HashMask[Idx].AlgoId == AlgoId) {
      return mTpm2HashMask[Idx].Mask;
    }
  }

  return HASH_ALG_ERROR;
}

/**
  Validate hash handle.

  @param[in]   HashHandle        HashHandle

  @return EFI_SUCCESS
  @return EFI_INVALID_PARAMETER  Invalidate HashHandle

**/
STATIC
EFI_STATUS
EFIAPI
ValidateHashHandle (
  IN HASH_HANDLE  HashHandle
  )
{
  UINT32  Idx;
  UINT32  HashMask;

  for (Idx = 0; Idx < ARRAY_SIZE (mTpm2HashMask); Idx++) {
    HashMask = 1 << Idx;

    if ((HashHandle & HashMask) == 0x00) {
      continue;
    }

    if (((mSupportedHashBitmap & HashMask) == 0x00) ||
        (mTpm2HashMask[Idx].SequenceHandle == BAD_SEQ_HANDLE))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/**
  Clear Sequence Handles.

**/
STATIC
VOID
ClearSequenceHandles (
  IN VOID
  )
{
  UINT32  Idx;

  for (Idx = 0; Idx < ARRAY_SIZE (mTpm2HashMask); Idx++) {
    mTpm2HashMask[Idx].SequenceHandle = BAD_SEQ_HANDLE;
  }
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
  UINT32       HashMask;
  HASH_HANDLE  Handle;

  if (mHashLibDisabled) {
    return EFI_UNSUPPORTED;
  }

  Handle = BAD_HASH_HANDLE;

  for (Idx = 0; Idx < ARRAY_SIZE (mTpm2HashMask); Idx++) {
    HashMask = 1 << Idx;

    if ((mSupportedHashBitmap & HashMask) == 0x00) {
      continue;
    }

    if (mTpm2HashMask[Idx].SequenceHandle != BAD_SEQ_HANDLE) {
      Status = EFI_ALREADY_STARTED;
      Handle = BAD_HASH_HANDLE;
      goto ErrorHandler;
    }

    AlgoId = Tpm2GetAlgoFromHashMask (HashMask);
    ASSERT (AlgoId != TPM_ALG_ERROR);

    Status =  Tpm2HashSequenceStart (AlgoId, &mTpm2HashMask[Idx].SequenceHandle);
    if (EFI_ERROR (Status)) {
      Handle = BAD_HASH_HANDLE;
      goto ErrorHandler;
    }

    Handle |= HashMask;
  }

ErrorHandler:
  *HashHandle = Handle;
  if (Handle == BAD_HASH_HANDLE) {
    ClearSequenceHandles ();
  }

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
  UINT32            HashMask;
  UINT8             *Buffer;
  UINT64            HashLen;
  TPM2B_MAX_BUFFER  HashBuffer;

  if (mHashLibDisabled) {
    return EFI_UNSUPPORTED;
  }

  Status = ValidateHashHandle (HashHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Idx = 0; Idx < ARRAY_SIZE (mTpm2HashMask); Idx++) {
    HashMask = 1 << Idx;

    if ((HashHandle & HashMask) == 0x00) {
      continue;
    }

    Buffer = (UINT8 *)(UINTN)DataToHash;
    for (HashLen = DataToHashLen; HashLen > sizeof (HashBuffer.buffer); HashLen -= sizeof (HashBuffer.buffer)) {
      HashBuffer.size = sizeof (HashBuffer.buffer);
      CopyMem (HashBuffer.buffer, Buffer, sizeof (HashBuffer.buffer));
      Buffer += sizeof (HashBuffer.buffer);

      Status = Tpm2SequenceUpdate (mTpm2HashMask[Idx].SequenceHandle, &HashBuffer);
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }
    }

    // Last one
    HashBuffer.size = (UINT16)HashLen;
    CopyMem (HashBuffer.buffer, Buffer, (UINTN)HashLen);
    Status = Tpm2SequenceUpdate (mTpm2HashMask[Idx].SequenceHandle, &HashBuffer);
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
  UINT32            HashMask;
  UINT8             *Buffer;
  UINT64            HashLen;
  TPM2B_MAX_BUFFER  HashBuffer;
  TPM_ALG_ID        AlgoId;
  TPM2B_DIGEST      Result;

  if (mHashLibDisabled) {
    return EFI_UNSUPPORTED;
  }

  Status = ValidateHashHandle (HashHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (DigestList, sizeof (*DigestList));
  DigestList->count = HASH_COUNT;
  DigestIdx         = 0;

  for (Idx = 0; Idx < ARRAY_SIZE (mTpm2HashMask); Idx++) {
    HashMask = 1 << Idx;

    if ((HashHandle & HashMask) == 0x00) {
      continue;
    }

    Buffer = (UINT8 *)(UINTN)DataToHash;
    for (HashLen = DataToHashLen; HashLen > sizeof (HashBuffer.buffer); HashLen -= sizeof (HashBuffer.buffer)) {
      HashBuffer.size = sizeof (HashBuffer.buffer);
      CopyMem (HashBuffer.buffer, Buffer, sizeof (HashBuffer.buffer));
      Buffer += sizeof (HashBuffer.buffer);

      Status = Tpm2SequenceUpdate (mTpm2HashMask[Idx].SequenceHandle, &HashBuffer);
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }
    }

    // Last one
    HashBuffer.size = (UINT16)HashLen;
    CopyMem (HashBuffer.buffer, Buffer, (UINTN)HashLen);

    Status = Tpm2SequenceComplete (
               mTpm2HashMask[Idx].SequenceHandle,
               &HashBuffer,
               &Result
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    AlgoId = Tpm2GetAlgoFromHashMask (HashMask);
    ASSERT (AlgoId != TPM_ALG_ERROR);

    // Copy the result of hash.
    CopyMem (&DigestList->digests[DigestIdx].digest, Result.buffer, Result.size);
    DigestList->digests[DigestIdx].hashAlg = AlgoId;
    DigestIdx++;
  }

  DigestList->count = DigestIdx;

  Status = Tpm2PcrExtend (
             PcrIndex,
             DigestList
             );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  ClearSequenceHandles ();

  return EFI_SUCCESS;
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
  UINT32            HashMask;

  if (mHashLibDisabled) {
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_VERBOSE, "\n HashAndExtend Entry \n"));

  if (mSupportedHashBitmap == 0x00) {
    return EFI_DEVICE_ERROR;
  }

  ZeroMem (DigestList, sizeof (*DigestList));
  DigestList->count = HASH_COUNT;
  DigestIdx         = 0;

  for (Idx = 0; Idx < ARRAY_SIZE (mTpm2HashMask); Idx++) {
    HashMask = 1 << Idx;

    if ((mSupportedHashBitmap & HashMask) == 0x00) {
      continue;
    }

    SequenceHandle = BAD_SEQ_HANDLE; // Know bad value
    AlgoId         = Tpm2GetAlgoFromHashMask (HashMask);
    ASSERT (AlgoId != TPM_ALG_ERROR);

    Status = Tpm2HashSequenceStart (AlgoId, &SequenceHandle);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    DEBUG ((DEBUG_VERBOSE, "\n Tpm2HashSequenceStart Success \n"));

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

    Status = Tpm2SequenceComplete (
               SequenceHandle,
               &HashBuffer,
               &Result
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    DEBUG ((DEBUG_VERBOSE, "\n Tpm2SequenceComplete Success \n"));

    CopyMem (&DigestList->digests[DigestIdx].digest, Result.buffer, Result.size);
    DigestList->digests[DigestIdx].hashAlg = AlgoId;
    DigestIdx++;
  }

  DigestList->count = DigestIdx;

  Status = Tpm2PcrExtend (
             PcrIndex,
             DigestList
             );
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

/**
  Constructor of HashLibTpm2PeilessSecLibConstructor.

**/
EFI_STATUS
EFIAPI
HasLibTpm2PeilessSecLibConstructor (
  VOID
  )
{
  EFI_STATUS                       Status;
  TRANSFER_LIST_HEADER             *TransferList;
  VOID                             *EventLog;
  UINTN                            EventLogSize;
  TCG_PCR_EVENT                    *TcgPcrEvent;
  TCG_EfiSpecIDEventStruct         *TcgEfiSpecIdEventStruct;
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  UINTN                            Idx;
  UINT32                           NumberOfAlgorithms;
  UINT32                           TpmHashBitmap;
  UINT32                           PcrHashBitmap;

  mHashLibDisabled = TRUE;

  Status = GetTransferList (&TransferList);
  if (EFI_ERROR (Status)) {
    goto DisableHandler;
  }

  if (TransferListCheckHeader (TransferList) == TRANSFER_LIST_OPS_INVALID) {
    DEBUG ((DEBUG_ERROR, "Invalid Transfer list..\n"));
    goto DisableHandler;
  }

  Status = TransferListGetEventLog (TransferList, &EventLog, &EventLogSize, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: No data for Tpm event log...\n", __func__));
    goto DisableHandler;
  }

  TcgPcrEvent             = (TCG_PCR_EVENT *)EventLog;
  TcgEfiSpecIdEventStruct = (TCG_EfiSpecIDEventStruct *)
                            (EventLog + OFFSET_OF (TCG_PCR_EVENT, Event));

  CopyMem (&NumberOfAlgorithms, TcgEfiSpecIdEventStruct + 1, sizeof (NumberOfAlgorithms));
  DigestSize = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct + sizeof (*TcgEfiSpecIdEventStruct) + sizeof (NumberOfAlgorithms));

  for (Idx = 0; Idx < NumberOfAlgorithms; Idx++) {
    mSupportedHashBitmap |= Tpm2GetHashMaskFromAlgo (DigestSize[Idx].algorithmId);
  }

  Status = Tpm2RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: TPM2 not detected!\n", __func__));
    BuildGuidHob (&gTpmErrorHobGuid, 0);
    goto DisableHandler;
  }

  Status = Tpm2GetCapabilitySupportedAndActivePcrs (&TpmHashBitmap, &PcrHashBitmap);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Tpm capability... Status: %r\n", __func__, Status));
    goto DisableHandler;
  }

  mSupportedHashBitmap &= PcrHashBitmap;
  if (mSupportedHashBitmap == 0x00) {
    DEBUG ((DEBUG_ERROR, "%a: No supported Hash algorithm with event log Spec...!\n", __func__));
    BuildGuidHob (&gTpmErrorHobGuid, 0);
    goto DisableHandler;
  }

  mHashLibDisabled = FALSE;

DisableHandler:
  return EFI_SUCCESS;
}

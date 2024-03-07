/** @file
  TdxHelper Functions which are used in SEC phase

  Copyright (c) 2022 - 2023, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/IntelTdx.h>
#include <Library/TdxLib.h>
#include <Library/TdxHelperLib.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/CcMeasurement.h>

UINT32  mCcEventLogCount;

/**
 * Calculate the sha384 of input Data and extend it to RTMR register.
 *
 * @param RtmrIndex       Index of the RTMR register
 * @param DataToHash      Data to be hashed
 * @param DataToHashLen   Length of the data
 * @param Digest          Hash value of the input data
 * @param DigestLen       Length of the hash value
 *
 * @retval EFI_SUCCESS    Successfully hash and extend to RTMR
 * @retval Others         Other errors as indicated
 */
EFI_STATUS
HashAndExtendToRtmr (
  IN UINT32  RtmrIndex,
  IN VOID    *DataToHash,
  IN UINTN   DataToHashLen,
  OUT UINT8  *Digest,
  IN  UINTN  DigestLen
  )
{
  EFI_STATUS  Status;

  if ((DataToHash == NULL) || (DataToHashLen == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Digest == NULL) || (DigestLen != SHA384_DIGEST_SIZE)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate the sha384 of the data
  //
  if (!Sha384HashAll (DataToHash, DataToHashLen, Digest)) {
    return EFI_ABORTED;
  }

  //
  // Extend to RTMR
  //
  Status = TdExtendRtmr (
             (UINT32 *)Digest,
             SHA384_DIGEST_SIZE,
             (UINT8)RtmrIndex
             );

  ASSERT (!EFI_ERROR (Status));
  return Status;
}

EFI_STATUS
BuildCcEventVariable (
  UINT32  RtmrIndex,
  UINT32  EventType,
  UINT8   *EventData,
  UINT32  EventSize,
  UINT8   *HashValue,
  UINT32  HashSize
  )
{
  EFI_STATUS          Status;
  UINT8               *Ptr;
  UINT8               *VariableData;
  UINTN               VariableDataSize;
  TPML_DIGEST_VALUES  *TdxDigest;

  EFI_CC_EVENT_VARIABLE_HEADER  CcEventVariableHeader;

  CcEventVariableHeader.LogIndex = mCcEventLogCount;
  VariableData                   = NULL;
  if (HashSize != SHA384_DIGEST_SIZE) {
    return EFI_INVALID_PARAMETER;
  }

  #define TDX_DIGEST_VALUE_LEN  (sizeof (UINT32) + sizeof (TPMI_ALG_HASH) + SHA384_DIGEST_SIZE)

  CcEventVariableHeader.LogSize = sizeof (TCG_PCRINDEX) + sizeof (TCG_EVENTTYPE) + TDX_DIGEST_VALUE_LEN + sizeof (UINT32) + EventSize;
  VariableDataSize              = sizeof (CcEventVariableHeader) + CcEventVariableHeader.LogSize;
  VariableData                  = AllocatePool (VariableDataSize);
  if (VariableData == NULL ) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr = VariableData;

  CopyMem (Ptr, &CcEventVariableHeader, sizeof (CcEventVariableHeader));
  Ptr += sizeof (CcEventVariableHeader);
  //
  // There are 2 types of measurement registers in TDX: MRTD and RTMR[0-3].
  // According to UEFI Spec 2.10 Section 38.4.1, RTMR[0-3] is mapped to MrIndex[1-4].
  // So RtmrIndex must be increased by 1 before the event log is created.
  //
  RtmrIndex++;
  CopyMem (Ptr, &RtmrIndex, sizeof (UINT32));
  Ptr += sizeof (UINT32);

  CopyMem (Ptr, &EventType, sizeof (TCG_EVENTTYPE));
  Ptr += sizeof (TCG_EVENTTYPE);

  TdxDigest                     = (TPML_DIGEST_VALUES *)Ptr;
  TdxDigest->count              = 1;
  TdxDigest->digests[0].hashAlg = TPM_ALG_SHA384;
  CopyMem (
    TdxDigest->digests[0].digest.sha384,
    HashValue,
    SHA384_DIGEST_SIZE
    );
  Ptr += TDX_DIGEST_VALUE_LEN;

  CopyMem (Ptr, &EventSize, sizeof (UINT32));
  Ptr += sizeof (UINT32);

  CopyMem (Ptr, (VOID *)EventData, EventSize);
  Ptr += EventSize;

  Status = gRT->SetVariable (
                  OVMF_CC_MEASUREMENT_EVENT_VARIABLE_NAME,
                  &gCcEventVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_APPEND_WRITE,
                  VariableDataSize,
                  VariableData
                  );

  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
  }

  if (VariableData) {
    FreePool (VariableData);
  }

  mCcEventLogCount++;
  return Status;
}

/**
 * In Tdx guest, OVMF uses FW_CFG_SELECTOR(0x510) and FW_CFG_IO_DATA(0x511)
 * to get configuration infomation from QEMU. From the security perspective,
 * that should be measured before it is consumed.

  @retval EFI_SUCCESS  The measurement is successfully
  @retval Others       Other errors as indicated
**/
EFI_STATUS
EFIAPI
TdxHelperMeasureFwCfgData (
  IN VOID    *EventLog,
  IN UINT32  LogLen,
  IN VOID    *HashData,
  IN UINT64  HashDataLen
  )
{
  EFI_STATUS  Status;
  UINT8       Digest[SHA384_DIGEST_SIZE];

  if ((EventLog == NULL) || (HashData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!TdIsEnabled ()) {
    return EFI_UNSUPPORTED;
  }

  Status = HashAndExtendToRtmr (
             0,
             (UINT8 *)HashData,
             (UINTN)HashDataLen,
             Digest,
             SHA384_DIGEST_SIZE
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: HashAndExtendToRtmr failed with %r\n", __func__, Status));
    return Status;
  }

  Status = BuildCcEventVariable (
             0,
             EV_PLATFORM_CONFIG_FLAGS,
             EventLog,
             LogLen,
             Digest,
             SHA384_DIGEST_SIZE
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: BuildTdxMeasurementGuidHob failed with %r\n", __func__, Status));
  }

  return Status;
}

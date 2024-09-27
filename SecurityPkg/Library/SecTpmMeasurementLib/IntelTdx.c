/** @file
  Extend to RTMR and Build GuidHob for tdx measurement.

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PrintLib.h>
#include <Library/TdxLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/TpmMeasurementLib.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/TdxMeasurementLib.h>

/**
  Do a hash operation on a data buffer, extend a specific RTMR with the hash result,
  and add an entry to the Event Log.

  @param[in]      PcrIndex      PCRIndex Index of the TPM PCR
  @param[in]      EventType     Type of the Event.
  @param[in]      EventLog      Physical address of the start of the data buffer.
  @param[in]      EventSize     The length, in bytes, of the buffer referenced by EventLog.
  @param[in]      HashData      Physical address of the start of the data buffer
                                to be hashed, extended, and logged.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced by HashData

  @retval EFI_SUCCESS  The measurement is successful
  @retval Others       Other errors as indicated
**/
EFI_STATUS
EFIAPI
TdxHashLogExtendEvent (
  IN UINT32  PcrIndex,
  IN UINT32  EventType,
  IN VOID    *EventLog,
  IN UINT32  LogLen,
  IN VOID    *HashData,
  IN UINT64  HashDataLen
  )
{
  EFI_STATUS  Status;
  UINT8       Digest[SHA384_DIGEST_SIZE];
  UINT32      MrIndex;

  if ((EventLog == NULL) || (HashData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!TdIsEnabled ()) {
    return EFI_UNSUPPORTED;
  }

  MrIndex = TdxMeasurementMapPcrToMrIndex (PcrIndex);
  if (MrIndex == CC_MR_INDEX_INVALID) {
    return EFI_INVALID_PARAMETER;
  }

  Status = TdxMeasurementHashAndExtendToRtmr (
             MrIndex - 1,
             (UINT8 *)HashData,
             (UINTN)HashDataLen,
             Digest,
             SHA384_DIGEST_SIZE
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: TdxMeasurementHashAndExtendToRtmr failed with %r\n", __func__, Status));
    return Status;
  }

  Status = TdxMeasurementBuildGuidHob (
             MrIndex - 1,
             EventType,
             EventLog,
             LogLen,
             Digest,
             SHA384_DIGEST_SIZE
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: TdxMeasurementBuildGuidHob failed with %r\n", __func__, Status));
  }

  return Status;
}

/** @file
  TpmMeasurementLib SEC implementation.

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/TpmMeasurementLib.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/CcProbeLib.h>

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
  );

/**
  Tpm measure and log data, and extend the measurement result into a specific PCR.

  @param[in]  PcrIndex         PCR Index.
  @param[in]  EventType        Event type.
  @param[in]  EventLog         Measurement event log.
  @param[in]  LogLen           Event log length in bytes.
  @param[in]  HashData         The start of the data buffer to be hashed, extended.
  @param[in]  HashDataLen      The length, in bytes, of the buffer referenced by HashData

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_UNSUPPORTED       TPM device not available.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.
**/
EFI_STATUS
EFIAPI
TpmMeasureAndLogData (
  IN UINT32  PcrIndex,
  IN UINT32  EventType,
  IN VOID    *EventLog,
  IN UINT32  LogLen,
  IN VOID    *HashData,
  IN UINT64  HashDataLen
  )
{
  if (CcProbe () == CcGuestTypeIntelTdx) {
    return TdxHashLogExtendEvent (PcrIndex, EventType, EventLog, LogLen, HashData, HashDataLen);
  }

  return EFI_UNSUPPORTED;
}

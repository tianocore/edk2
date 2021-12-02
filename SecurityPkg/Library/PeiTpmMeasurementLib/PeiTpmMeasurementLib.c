/** @file
  This library is used by other modules to measure data to TPM.

Copyright (c) 2020, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/TpmMeasurementLib.h>

#include <Ppi/Tcg.h>
#include <IndustryStandard/UefiTcgPlatform.h>

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
  EFI_STATUS         Status;
  EDKII_TCG_PPI      *TcgPpi;
  TCG_PCR_EVENT_HDR  TcgEventHdr;

  Status = PeiServicesLocatePpi (
             &gEdkiiTcgPpiGuid,
             0,
             NULL,
             (VOID **)&TcgPpi
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TcgEventHdr.PCRIndex  = PcrIndex;
  TcgEventHdr.EventType = EventType;
  TcgEventHdr.EventSize = LogLen;

  Status = TcgPpi->HashLogExtendEvent (
                     TcgPpi,
                     0,
                     HashData,
                     (UINTN)HashDataLen,
                     &TcgEventHdr,
                     EventLog
                     );
  return Status;
}

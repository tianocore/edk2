/** @file
  This library is used by other modules to measure data to TPM and Confidential
  Computing (CC) measure registers.

Copyright (c) 2012 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/TcgService.h>
#include <Protocol/Tcg2Protocol.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/TpmMeasurementLib.h>

#include <Guid/Acpi.h>
#include <IndustryStandard/Acpi.h>
#include <Protocol/CcMeasurement.h>

/**
  Tpm12 measure and log data, and extend the measurement result into a specific PCR.

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
STATIC
EFI_STATUS
Tpm12MeasureAndLogData (
  IN UINT32  PcrIndex,
  IN UINT32  EventType,
  IN VOID    *EventLog,
  IN UINT32  LogLen,
  IN VOID    *HashData,
  IN UINT64  HashDataLen
  )
{
  EFI_STATUS            Status;
  EFI_TCG_PROTOCOL      *TcgProtocol;
  TCG_PCR_EVENT         *TcgEvent;
  EFI_PHYSICAL_ADDRESS  EventLogLastEntry;
  UINT32                EventNumber;

  TcgEvent = NULL;

  //
  // Tpm activation state is checked in HashLogExtendEvent
  //
  Status = gBS->LocateProtocol (&gEfiTcgProtocolGuid, NULL, (VOID **)&TcgProtocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TcgEvent = (TCG_PCR_EVENT *)AllocateZeroPool (sizeof (TCG_PCR_EVENT_HDR) + LogLen);
  if (TcgEvent == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TcgEvent->PCRIndex  = PcrIndex;
  TcgEvent->EventType = EventType;
  TcgEvent->EventSize = LogLen;
  CopyMem (&TcgEvent->Event[0], EventLog, LogLen);
  EventNumber = 1;
  Status      = TcgProtocol->HashLogExtendEvent (
                               TcgProtocol,
                               (EFI_PHYSICAL_ADDRESS)(UINTN)HashData,
                               HashDataLen,
                               TPM_ALG_SHA,
                               TcgEvent,
                               &EventNumber,
                               &EventLogLastEntry
                               );

  FreePool (TcgEvent);

  return Status;
}

/**
  Tpm20 measure and log data, and extend the measurement result into a specific PCR.

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
STATIC
EFI_STATUS
Tpm20MeasureAndLogData (
  IN UINT32  PcrIndex,
  IN UINT32  EventType,
  IN VOID    *EventLog,
  IN UINT32  LogLen,
  IN VOID    *HashData,
  IN UINT64  HashDataLen
  )
{
  EFI_STATUS         Status;
  EFI_TCG2_PROTOCOL  *Tcg2Protocol;
  EFI_TCG2_EVENT     *Tcg2Event;

  //
  // TPMPresentFlag is checked in HashLogExtendEvent
  //
  Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **)&Tcg2Protocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Tcg2Event = (EFI_TCG2_EVENT *)AllocateZeroPool (LogLen + sizeof (EFI_TCG2_EVENT));
  if (Tcg2Event == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Tcg2Event->Size                 = (UINT32)LogLen + sizeof (EFI_TCG2_EVENT) - sizeof (Tcg2Event->Event);
  Tcg2Event->Header.HeaderSize    = sizeof (EFI_TCG2_EVENT_HEADER);
  Tcg2Event->Header.HeaderVersion = EFI_TCG2_EVENT_HEADER_VERSION;
  Tcg2Event->Header.PCRIndex      = PcrIndex;
  Tcg2Event->Header.EventType     = EventType;
  CopyMem (&Tcg2Event->Event[0], EventLog, LogLen);

  Status = Tcg2Protocol->HashLogExtendEvent (
                           Tcg2Protocol,
                           0,
                           (EFI_PHYSICAL_ADDRESS)(UINTN)HashData,
                           HashDataLen,
                           Tcg2Event
                           );
  FreePool (Tcg2Event);

  return Status;
}

/**
  Cc measure and log data, and extend the measurement result into a
  specific CC MR.

  @param[in]  CcProtocol       Instance of CC measurement protocol
  @param[in]  PcrIndex         PCR Index.
  @param[in]  EventType        Event type.
  @param[in]  EventLog         Measurement event log.
  @param[in]  LogLen           Event log length in bytes.
  @param[in]  HashData         The start of the data buffer to be hashed, extended.
  @param[in]  HashDataLen      The length, in bytes, of the buffer referenced by HashData

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_UNSUPPORTED       CC guest not available.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.
  @retval EFI_INVALID_PARAMETER The input parameter is invalid.
**/
STATIC
EFI_STATUS
CcMeasureAndLogData (
  IN EFI_CC_MEASUREMENT_PROTOCOL  *CcProtocol,
  IN UINT32                       PcrIndex,
  IN UINT32                       EventType,
  IN VOID                         *EventLog,
  IN UINT32                       LogLen,
  IN VOID                         *HashData,
  IN UINT64                       HashDataLen
  )
{
  EFI_STATUS       Status;
  EFI_CC_EVENT     *EfiCcEvent;
  EFI_CC_MR_INDEX  MrIndex;

  if (CcProtocol == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = CcProtocol->MapPcrToMrIndex (CcProtocol, PcrIndex, &MrIndex);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  EfiCcEvent = (EFI_CC_EVENT *)AllocateZeroPool (LogLen + sizeof (EFI_CC_EVENT));
  if (EfiCcEvent == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EfiCcEvent->Size                 = (UINT32)LogLen + sizeof (EFI_CC_EVENT) - sizeof (EfiCcEvent->Event);
  EfiCcEvent->Header.HeaderSize    = sizeof (EFI_CC_EVENT_HEADER);
  EfiCcEvent->Header.HeaderVersion = EFI_CC_EVENT_HEADER_VERSION;
  EfiCcEvent->Header.MrIndex       = MrIndex;
  EfiCcEvent->Header.EventType     = EventType;
  CopyMem (&EfiCcEvent->Event[0], EventLog, LogLen);

  Status = CcProtocol->HashLogExtendEvent (
                         CcProtocol,
                         0,
                         (EFI_PHYSICAL_ADDRESS)(UINTN)HashData,
                         HashDataLen,
                         EfiCcEvent
                         );
  FreePool (EfiCcEvent);

  return Status;
}

/**
  Tpm measure and log data, and extend the measurement result into a specific PCR.

  @param[in]  PcrIndex         PCR Index.
  @param[in]  EventType        Event type.
  @param[in]  EventLog         Measurement event log.
  @param[in]  LogLen           Event log length in bytes.
  @param[in]  HashData         The start of the data buffer to be hashed, extended.
  @param[in]  HashDataLen      The length, in bytes, of the buffer referenced by HashData

  @retval EFI_SUCCESS               Operation completed successfully.
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
  EFI_STATUS                   Status;
  EFI_CC_MEASUREMENT_PROTOCOL  *CcProtocol;

  Status = gBS->LocateProtocol (&gEfiCcMeasurementProtocolGuid, NULL, (VOID **)&CcProtocol);
  if (!EFI_ERROR (Status)) {
    //
    // Try to measure using Cc measurement protocol
    //
    Status = CcMeasureAndLogData (
               CcProtocol,
               PcrIndex,
               EventType,
               EventLog,
               LogLen,
               HashData,
               HashDataLen
               );
  } else {
    //
    // Try to measure using Tpm20 protocol
    //
    Status = Tpm20MeasureAndLogData (
               PcrIndex,
               EventType,
               EventLog,
               LogLen,
               HashData,
               HashDataLen
               );

    if (EFI_ERROR (Status)) {
      //
      // Try to measure using Tpm1.2 protocol
      //
      Status = Tpm12MeasureAndLogData (
                 PcrIndex,
                 EventType,
                 EventLog,
                 LogLen,
                 HashData,
                 HashDataLen
                 );
    }
  }

  return Status;
}

/** @file
  This library is used by other modules to measure data to TPM.

Copyright (c) 2012 - 2015, Intel Corporation. All rights reserved. <BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
EFI_STATUS
Tpm12MeasureAndLogData (
  IN UINT32             PcrIndex,
  IN UINT32             EventType,
  IN VOID               *EventLog,
  IN UINT32             LogLen,
  IN VOID               *HashData,
  IN UINT64             HashDataLen
  )
{
  EFI_STATUS                Status;
  EFI_TCG_PROTOCOL          *TcgProtocol;
  TCG_PCR_EVENT             *TcgEvent;
  EFI_PHYSICAL_ADDRESS      EventLogLastEntry;
  UINT32                    EventNumber;

  TcgEvent = NULL;

  //
  // Tpm active/deactive state is checked in HashLogExtendEvent
  //
  Status = gBS->LocateProtocol (&gEfiTcgProtocolGuid, NULL, (VOID **) &TcgProtocol);
  if (EFI_ERROR(Status)){
    return Status;
  }

  TcgEvent = (TCG_PCR_EVENT *)AllocateZeroPool (sizeof (TCG_PCR_EVENT_HDR) + LogLen);
  if(TcgEvent == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TcgEvent->PCRIndex  = PcrIndex;
  TcgEvent->EventType = EventType;
  TcgEvent->EventSize = LogLen;
  CopyMem (&TcgEvent->Event[0], EventLog, LogLen);
  EventNumber = 1;
  Status = TcgProtocol->HashLogExtendEvent (
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
EFI_STATUS
Tpm20MeasureAndLogData (
  IN UINT32             PcrIndex,
  IN UINT32             EventType,
  IN VOID               *EventLog,
  IN UINT32             LogLen,
  IN VOID               *HashData,
  IN UINT64             HashDataLen
  )
{
  EFI_STATUS                Status;
  EFI_TCG2_PROTOCOL         *Tcg2Protocol;
  EFI_TCG2_EVENT            *Tcg2Event;

  //
  // TPMPresentFlag is checked in HashLogExtendEvent
  //
  Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, (VOID **) &Tcg2Protocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Tcg2Event = (EFI_TCG2_EVENT *) AllocateZeroPool (LogLen + sizeof (EFI_TCG2_EVENT));
  if(Tcg2Event == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Tcg2Event->Size = (UINT32)LogLen + sizeof (EFI_TCG2_EVENT) - sizeof(Tcg2Event->Event);
  Tcg2Event->Header.HeaderSize    = sizeof(EFI_TCG2_EVENT_HEADER);
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
  IN UINT32             PcrIndex,
  IN UINT32             EventType,
  IN VOID               *EventLog,
  IN UINT32             LogLen,
  IN VOID               *HashData,
  IN UINT64             HashDataLen
  )
{
  EFI_STATUS  Status;

  //
  // Try to measure using Tpm1.2 protocol
  //
  Status = Tpm12MeasureAndLogData(
               PcrIndex,
               EventType,
               EventLog,
               LogLen,
               HashData,
               HashDataLen
               );
  if (EFI_ERROR (Status)) {
    //
    // Try to measure using Tpm20 protocol
    //
    Status = Tpm20MeasureAndLogData(
               PcrIndex,
               EventType,
               EventLog,
               LogLen,
               HashData,
               HashDataLen
               );
  }

  return Status;
}

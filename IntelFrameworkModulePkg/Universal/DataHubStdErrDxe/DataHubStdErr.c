/** @file
  Data Hub filter driver that takes DEBUG () info from Data Hub and writes it
  to StdErr if it exists.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <FrameworkDxe.h>
#include <Guid/DataHubStatusCodeRecord.h>
#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/StatusCodeDataTypeDebug.h>
#include <Protocol/DataHub.h>
#include <Protocol/SimpleTextOut.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_DATA_HUB_PROTOCOL *mDataHub = NULL;

EFI_EVENT             mDataHubStdErrEvent;

/**
  Event handler registered with the Data Hub to parse EFI_DEBUG_CODE. This
  handler reads the Data Hub and sends any DEBUG info to StdErr.

  @param Event      The event that occured, not used
  @param Context    DataHub Protocol Pointer
**/
VOID
EFIAPI
DataHubStdErrEventHandler (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  EFI_STATUS                           Status;
  EFI_DATA_HUB_PROTOCOL                *DataHub;
  EFI_DATA_RECORD_HEADER               *Record;
  DATA_HUB_STATUS_CODE_DATA_RECORD     *DataRecord;
  UINT64                               Mtc;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL      *Sto;
  INT32                                OldAttribute;

  DataHub = (EFI_DATA_HUB_PROTOCOL *) Context;

  //
  // If StdErr is not yet initialized just return a DEBUG print in the BDS
  // after consoles are connect will make sure data gets flushed properly
  // when StdErr is available.
  //
  if (gST == NULL) {
    return ;
  }

  if (gST->StdErr == NULL) {
    return ;
  }

  //
  // Mtc of zero means return the next record that has not been read by the
  // event handler.
  //
  Mtc = 0;
  do {
    Status = DataHub->GetNextRecord (DataHub, &Mtc, &mDataHubStdErrEvent, &Record);
    if (!EFI_ERROR (Status)) {
      if (CompareGuid (&Record->DataRecordGuid, &gEfiDataHubStatusCodeRecordGuid)) {
        DataRecord = (DATA_HUB_STATUS_CODE_DATA_RECORD *) (((CHAR8 *) Record) + Record->HeaderSize);

        if (DataRecord->Data.HeaderSize > 0) {
          if (CompareGuid (&DataRecord->Data.Type, &gEfiStatusCodeDataTypeDebugGuid)) {
            //
            // If the Data record is from a DEBUG () then send it to Standard Error
            //
            Sto           = gST->StdErr;
            OldAttribute  = Sto->Mode->Attribute;
            Sto->SetAttribute (Sto, EFI_TEXT_ATTR (EFI_MAGENTA, EFI_BLACK));
            Sto->OutputString (Sto, (CHAR16 *) (DataRecord + 1));
            Sto->SetAttribute (Sto, OldAttribute);
          }
        }
      }
    }
  } while ((Mtc != 0) && !EFI_ERROR (Status));
}

/**
  Register an event handler with the Data Hub to parse EFI_DEBUG_CODE. This
  handler reads the Data Hub and sends any DEBUG info to StdErr.

  @param ImageHandle                Image handle of this driver.
  @param SystemTable                Pointer to EFI system table.

  @retval EFI_SUCCESS               The event handler was registered.
  @retval EFI_OUT_OF_RESOURCES      The event hadler was not registered due to lack of system resources.
**/
EFI_STATUS
EFIAPI
DataHubStdErrInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT64      DataClass;

  gBS->LocateProtocol (&gEfiDataHubProtocolGuid, NULL, (VOID **) &mDataHub);
  //
  // Should never fail due to Depex grammer.
  //
  ASSERT (mDataHub != NULL);

  //
  // Create an event and register it with the filter driver
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  DataHubStdErrEventHandler,
                  mDataHub,
                  &mDataHubStdErrEvent
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DataClass = EFI_DATA_RECORD_CLASS_DEBUG | EFI_DATA_RECORD_CLASS_ERROR;
  Status = mDataHub->RegisterFilterDriver (
                      mDataHub,
                      mDataHubStdErrEvent,
                      TPL_CALLBACK,
                      DataClass,
                      NULL
                      );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (mDataHubStdErrEvent);
  }

  return Status;
}


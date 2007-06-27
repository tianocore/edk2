/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  MiscSubclassDriverEntryPoint.c

Abstract:

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

--*/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "MiscSubclassDriver.h"


extern UINT8  MiscSubclassStrings[];

VOID
EFIAPI
WinNtIoProtocolNotifyFunction (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  );

//
//
//
EFI_STATUS
LogRecordDataToDataHub (
  EFI_DATA_HUB_PROTOCOL *DataHub,
  UINT32                RecordType,
  UINT32                RecordLen,
  VOID                  *RecordData
  )
/*++
Description:

Parameters:

  DataHub
    %%TBD

  RecordType
    %%TBD

  RecordLen
    %%TBD

  RecordData
    %%TBD

Returns:

  EFI_INVALID_PARAMETER

  EFI_SUCCESS

  Other Data Hub errors

--*/
{
  EFI_MISC_SUBCLASS_DRIVER_DATA MiscSubclass;
  EFI_STATUS                    EfiStatus;

  //
  // Do nothing if data parameters are not valid.
  //
  if (RecordLen == 0 || RecordData == NULL) {
    DEBUG (
      (EFI_D_ERROR,
      "RecordLen == %d  RecordData == %xh\n",
      RecordLen,
      RecordData)
      );

    return EFI_INVALID_PARAMETER;
  }
  //
  // Assemble Data Hub record.
  //
  MiscSubclass.Header.Version     = EFI_MISC_SUBCLASS_VERSION;
  MiscSubclass.Header.HeaderSize  = sizeof (EFI_SUBCLASS_TYPE1_HEADER);
  MiscSubclass.Header.Instance    = 1;
  MiscSubclass.Header.SubInstance = 1;
  MiscSubclass.Header.RecordType  = RecordType;

  CopyMem (
    &MiscSubclass.Record,
    RecordData,
    RecordLen
    );

  //
  // Log Data Hub record.
  //
  EfiStatus = DataHub->LogData (
                        DataHub,
                        &gEfiMiscSubClassGuid,
                        &gEfiMiscSubClassGuid,
                        EFI_DATA_RECORD_CLASS_DATA,
                        &MiscSubclass,
                        sizeof (EFI_SUBCLASS_TYPE1_HEADER) + RecordLen
                        );

  if (EFI_ERROR (EfiStatus)) {
    DEBUG (
      (EFI_D_ERROR,
      "LogData(%d bytes) == %r\n",
      sizeof (EFI_SUBCLASS_TYPE1_HEADER) + RecordLen,
      EfiStatus)
      );
  }

  return EfiStatus;
}


EFI_STATUS
EFIAPI
MiscSubclassDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++
Description:

  Standard EFI driver point.  This driver parses the mMiscSubclassDataTable
  structure and reports any generated data to the DataHub.

Arguments:

  ImageHandle
    Handle for the image of this driver

  SystemTable
    Pointer to the EFI System Table

Returns:

  EFI_SUCCESS
    The data was successfully reported to the Data Hub.

--*/
{
  EFI_MISC_SUBCLASS_DRIVER_DATA RecordData;
  EFI_DATA_HUB_PROTOCOL         *DataHub;
  EFI_HII_PROTOCOL              *Hii;
  EFI_HII_PACKAGES              *PackageList;
  EFI_HII_HANDLE                HiiHandle;
  EFI_STATUS                    EfiStatus;
  UINTN                         Index;
  BOOLEAN                       LogRecordData;
  EFI_EVENT                     Event;
  VOID                          *Registration;


  //
  // Initialize constant portion of subclass header.
  //
  RecordData.Header.Version     = EFI_MISC_SUBCLASS_VERSION;
  RecordData.Header.HeaderSize  = sizeof (EFI_SUBCLASS_TYPE1_HEADER);
  RecordData.Header.Instance    = 1;
  RecordData.Header.SubInstance = 1;

  //
  // Locate data hub protocol.
  //
  EfiStatus = gBS->LocateProtocol (&gEfiDataHubProtocolGuid, NULL, &DataHub);

  if (EFI_ERROR (EfiStatus)) {
    DEBUG ((EFI_D_ERROR, "Could not locate DataHub protocol.  %r\n", EfiStatus));
    return EfiStatus;
  } else if (DataHub == NULL) {
    DEBUG ((EFI_D_ERROR, "LocateProtocol(DataHub) returned NULL pointer!\n"));
    return EFI_DEVICE_ERROR;
  }
  //
  // Locate hii protocol.
  //
  EfiStatus = gBS->LocateProtocol (&gEfiHiiProtocolGuid, NULL, &Hii);

  if (EFI_ERROR (EfiStatus)) {
    DEBUG ((EFI_D_ERROR, "Could not locate Hii protocol.  %r\n", EfiStatus));
    return EfiStatus;
  } else if (Hii == NULL) {
    DEBUG ((EFI_D_ERROR, "LocateProtocol(Hii) returned NULL pointer!\n"));
    return EFI_DEVICE_ERROR;
  }
  //
  // Add our default strings to the HII database. They will be modified later.
  //
  PackageList = PreparePackages (1, &gEfiMiscSubClassGuid, MiscSubclassStrings);
  EfiStatus   = Hii->NewPack (Hii, PackageList, &HiiHandle);
  FreePool (PackageList);

  if (EFI_ERROR (EfiStatus)) {
    DEBUG ((EFI_D_ERROR, "Could not log default strings to Hii.  %r\n", EfiStatus));
    return EfiStatus;
  }
  //
  //
  //
  for (Index = 0; Index < mMiscSubclassDataTableEntries; ++Index) {
    //
    // Stupidity check!  Do nothing if RecordLen is zero.
    // %%TBD - Should this be an error or a mechanism for ignoring
    // records in the Data Table?
    //
    if (mMiscSubclassDataTable[Index].RecordLen == 0) {
      DEBUG (
        (EFI_D_ERROR,
        "mMiscSubclassDataTable[%d].RecordLen == 0\n",
        Index)
        );

      continue;
    }
    //
    // Initialize per-record portion of subclass header and
    // copy static data into data portion of subclass record.
    //
    RecordData.Header.RecordType = mMiscSubclassDataTable[Index].RecordType;

    if (mMiscSubclassDataTable[Index].RecordData == NULL) {
      ZeroMem (
        &RecordData.Record,
        mMiscSubclassDataTable[Index].RecordLen
        );
    } else {
      CopyMem (
        &RecordData.Record,
        mMiscSubclassDataTable[Index].RecordData,
        mMiscSubclassDataTable[Index].RecordLen
        );
    }
    //
    // If the entry does not have a function pointer, just log the data.
    //
    if (mMiscSubclassDataTable[Index].Function == NULL) {
      //
      // Log RecordData to Data Hub.
      //
      EfiStatus = DataHub->LogData (
                            DataHub,
                            &gEfiMiscSubClassGuid,
                            &gEfiMiscSubClassGuid,
                            EFI_DATA_RECORD_CLASS_DATA,
                            &RecordData,
                            sizeof (EFI_SUBCLASS_TYPE1_HEADER) + mMiscSubclassDataTable[Index].RecordLen
                            );

      if (EFI_ERROR (EfiStatus)) {
        DEBUG (
          (EFI_D_ERROR,
          "LogData(%d bytes) == %r\n",
          sizeof (EFI_SUBCLASS_TYPE1_HEADER) + mMiscSubclassDataTable[Index].RecordLen,
          EfiStatus)
          );
      }

      continue;
    }
    //
    // The entry has a valid function pointer.
    // Keep calling the function and logging data until there
    // is no more data to log.
    //
    for (;;) {
      //
      //
      //
      EfiStatus = (*mMiscSubclassDataTable[Index].Function)
        (
          mMiscSubclassDataTable[Index].RecordType, &mMiscSubclassDataTable[Index].RecordLen, &RecordData.Record, &
            LogRecordData
        );

      //
      //
      //
      if (EFI_ERROR (EfiStatus)) {
        break;
      }

      if (!LogRecordData) {
        break;
      }
      //
      //
      //
      EfiStatus = DataHub->LogData (
                            DataHub,
                            &gEfiMiscSubClassGuid,
                            &gEfiMiscSubClassGuid,
                            EFI_DATA_RECORD_CLASS_DATA,
                            &RecordData,
                            sizeof (EFI_SUBCLASS_TYPE1_HEADER) + mMiscSubclassDataTable[Index].RecordLen
                            );

      if (EFI_ERROR (EfiStatus)) {
        DEBUG (
          (EFI_D_ERROR,
          "LogData(%d bytes) == %r\n",
          sizeof (EFI_SUBCLASS_TYPE1_HEADER) + mMiscSubclassDataTable[Index].RecordLen,
          EfiStatus)
          );
      }
    }
  }
  //
  // Install notify function to fetch memory data through WinNtIo protocol and store to data hub.
  //
  EfiStatus = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    WinNtIoProtocolNotifyFunction,
                    ImageHandle,
                    &Event
                    );
  ASSERT (!EFI_ERROR (EfiStatus));

  EfiStatus = gBS->RegisterProtocolNotify (
                    &gEfiWinNtIoProtocolGuid,
                    Event,
                    &Registration
                    );
  ASSERT (!EFI_ERROR (EfiStatus));

  return EFI_SUCCESS;
}

UINTN
Atoi (
  CHAR16  *String
  )
/*++

Routine Description:
  Convert a unicode string to a UINTN

Arguments:
  String - Unicode string.

Returns:
  UINTN of the number represented by String.

--*/
{
  UINTN   Number;
  CHAR16  *Str;

  //
  // skip preceeding white space
  //
  Str = String;
  while ((*Str) && (*Str == ' ' || *Str == '"')) {
    Str++;
  }
  //
  // Convert ot a Number
  //
  Number = 0;
  while (*Str != '\0') {
    if ((*Str >= '0') && (*Str <= '9')) {
      Number = (Number * 10) +*Str - '0';
    } else {
      break;
    }

    Str++;
  }

  return Number;
}

VOID
EFIAPI
WinNtIoProtocolNotifyFunction (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
/*++

Routine Description:
  This function will log memory size data to data hub.

Arguments:
Event        - Event whose notification function is being invoked.
Context      - Pointer to the notification function's context.

Returns:
    EFI_STATUS.

--*/
{
  EFI_STATUS                      Status;
  EFI_MEMORY_SUBCLASS_DRIVER_DATA MemorySubClassData;
  EFI_DATA_RECORD_HEADER          *Record;
  EFI_SUBCLASS_TYPE1_HEADER       *DataHeader;
  UINTN                           HandleCount;
  UINTN                           HandleIndex;
  UINT64                          MonotonicCount;
  BOOLEAN                         RecordFound;
  EFI_HANDLE                      *HandleBuffer;
  EFI_WIN_NT_IO_PROTOCOL          *WinNtIo;
  EFI_DATA_HUB_PROTOCOL           *DataHub;
  UINT64                          TotalMemorySize;

  DataHub         = NULL;
  MonotonicCount  = 0;
  RecordFound     = FALSE;

  //
  // Retrieve the list of all handles from the handle database.
  //
  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  &gEfiWinNtIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }
  //
  // Locate DataHub protocol.
  //
  Status = gBS->LocateProtocol (&gEfiDataHubProtocolGuid, NULL, &DataHub);
  if (EFI_ERROR (Status)) {
    return ;
  }
  //
  // Search the Handle array to find the meory size information.
  //
  for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
    Status = gBS->OpenProtocol (
                    HandleBuffer[HandleIndex],
                    &gEfiWinNtIoProtocolGuid,
                    &WinNtIo,
                    Context,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if ((WinNtIo->WinNtThunk->Signature == EFI_WIN_NT_THUNK_PROTOCOL_SIGNATURE) &&
        CompareGuid (WinNtIo->TypeGuid, &gEfiWinNtMemoryGuid)
          ) {
      //
      // Check if this record has been stored in data hub.
      //
      do {
        Status = DataHub->GetNextRecord (DataHub, &MonotonicCount, NULL, &Record);
        if (Record->DataRecordClass == EFI_DATA_RECORD_CLASS_DATA) {
          DataHeader = (EFI_SUBCLASS_TYPE1_HEADER *) (Record + 1);
          if (CompareGuid (&Record->DataRecordGuid, &gEfiProcessorSubClassGuid) &&
              (DataHeader->RecordType == EFI_MEMORY_ARRAY_START_ADDRESS_RECORD_NUMBER)
              ) {
            RecordFound = TRUE;
          }
        }
      } while (MonotonicCount != 0);

      if (RecordFound) {
        RecordFound = FALSE;
        continue;
      }
      //
      // Initialize data record.
      //
      MemorySubClassData.Header.Instance    = 1;
      MemorySubClassData.Header.SubInstance = EFI_SUBCLASS_INSTANCE_NON_APPLICABLE;
      MemorySubClassData.Header.RecordType  = EFI_MEMORY_ARRAY_START_ADDRESS_RECORD_NUMBER;

      TotalMemorySize                       = (UINT64) Atoi (WinNtIo->EnvString);

      MemorySubClassData.Record.ArrayStartAddress.MemoryArrayStartAddress               = 0;
      MemorySubClassData.Record.ArrayStartAddress.MemoryArrayEndAddress                 = LShiftU64 (TotalMemorySize, 20) - 1;
      MemorySubClassData.Record.ArrayStartAddress.PhysicalMemoryArrayLink.ProducerName  = gEfiMemoryProducerGuid;
      MemorySubClassData.Record.ArrayStartAddress.PhysicalMemoryArrayLink.Instance      = 1;
      MemorySubClassData.Record.ArrayStartAddress.PhysicalMemoryArrayLink.SubInstance = EFI_SUBCLASS_INSTANCE_NON_APPLICABLE;
      MemorySubClassData.Record.ArrayStartAddress.MemoryArrayPartitionWidth = 0;

      //
      // Store memory size data record to data hub.
      //
      Status = DataHub->LogData (
                          DataHub,
                          &gEfiMemorySubClassGuid,
                          &gEfiMemoryProducerGuid,
                          EFI_DATA_RECORD_CLASS_DATA,
                          &MemorySubClassData,
                          sizeof (EFI_SUBCLASS_TYPE1_HEADER) + sizeof (EFI_MEMORY_ARRAY_START_ADDRESS_DATA)
                          );
    }

    gBS->CloseProtocol (
          HandleBuffer[HandleIndex],
          &gEfiWinNtIoProtocolGuid,
          Context,
          NULL
          );
  }
}


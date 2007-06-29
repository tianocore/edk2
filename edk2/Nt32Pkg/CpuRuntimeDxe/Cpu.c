/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Cpu.c

Abstract:

  NT Emulation Architectural Protocol Driver as defined in Tiano.
  This CPU module abstracts the interrupt subsystem of a platform and
  the CPU-specific setjump/long pair.  Other services are not implemented
  in this driver.

--*/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "CpuDriver.h"

#define EFI_CPU_DATA_MAXIMUM_LENGTH 0x100

EFI_STATUS
EFIAPI
InitializeCpu (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

VOID
EFIAPI
WinNtIoProtocolNotifyFunction (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  );

typedef union {
  EFI_CPU_DATA_RECORD *DataRecord;
  UINT8               *Raw;
} EFI_CPU_DATA_RECORD_BUFFER;

EFI_SUBCLASS_TYPE1_HEADER mCpuDataRecordHeader = {
  EFI_PROCESSOR_SUBCLASS_VERSION,       // Version
  sizeof (EFI_SUBCLASS_TYPE1_HEADER),   // Header Size
  0,                                    // Instance, Initialize later
  EFI_SUBCLASS_INSTANCE_NON_APPLICABLE, // SubInstance
  0                                     // RecordType, Initialize later
};

//
// Service routines for the driver
//
STATIC
EFI_STATUS
EFIAPI
WinNtFlushCpuDataCache (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  IN EFI_PHYSICAL_ADDRESS   Start,
  IN UINT64                 Length,
  IN EFI_CPU_FLUSH_TYPE     FlushType
  )
/*++

Routine Description:

  This routine would provide support for flushing the CPU data cache.
  In the case of NT emulation environment, this flushing is not necessary and
  is thus not implemented.

Arguments:

  Pointer to CPU Architectural Protocol interface
  Start adddress in memory to flush
  Length of memory to flush
  Flush type

Returns:

  Status
    EFI_SUCCESS

--*/
// TODO:    This - add argument and description to function comment
// TODO:    FlushType - add argument and description to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
{
  if (FlushType == EfiCpuFlushTypeWriteBackInvalidate) {
    //
    // Only WB flush is supported. We actually need do nothing on NT emulator
    // environment. Classify this to follow EFI spec
    //
    return EFI_SUCCESS;
  }
  //
  // Other flush types are not supported by NT emulator
  //
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
WinNtEnableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL  *This
  )
/*++

Routine Description:

  This routine provides support for emulation of the interrupt enable of the
  the system.  For our purposes, CPU enable is just a BOOLEAN that the Timer
  Architectural Protocol observes in order to defer behaviour while in its
  emulated interrupt, or timer tick.

Arguments:

  Pointer to CPU Architectural Protocol interface

Returns:

  Status
    EFI_SUCCESS

--*/
// TODO:    This - add argument and description to function comment
{
  CPU_ARCH_PROTOCOL_PRIVATE *Private;

  Private                 = CPU_ARCH_PROTOCOL_PRIVATE_DATA_FROM_THIS (This);
  Private->InterruptState = TRUE;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
WinNtDisableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL  *This
  )
/*++

Routine Description:

  This routine provides support for emulation of the interrupt disable of the
  the system.  For our purposes, CPU enable is just a BOOLEAN that the Timer
  Architectural Protocol observes in order to defer behaviour while in its
  emulated interrupt, or timer tick.

Arguments:

  Pointer to CPU Architectural Protocol interface

Returns:

  Status
    EFI_SUCCESS

--*/
// TODO:    This - add argument and description to function comment
{
  CPU_ARCH_PROTOCOL_PRIVATE *Private;

  Private                 = CPU_ARCH_PROTOCOL_PRIVATE_DATA_FROM_THIS (This);
  Private->InterruptState = FALSE;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
WinNtGetInterruptState (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  OUT BOOLEAN               *State
  )
/*++

Routine Description:

  This routine provides support for emulation of the interrupt disable of the
  the system.  For our purposes, CPU enable is just a BOOLEAN that the Timer
  Architectural Protocol observes in order to defer behaviour while in its
  emulated interrupt, or timer tick.

Arguments:

  Pointer to CPU Architectural Protocol interface

Returns:

  Status
    EFI_SUCCESS

--*/
// TODO:    This - add argument and description to function comment
// TODO:    State - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  CPU_ARCH_PROTOCOL_PRIVATE *Private;

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = CPU_ARCH_PROTOCOL_PRIVATE_DATA_FROM_THIS (This);
  *State  = Private->InterruptState;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
WinNtInit (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  IN EFI_CPU_INIT_TYPE      InitType
  )
/*++

Routine Description:

  This routine would support generation of a CPU INIT.  At
  present, this code does not provide emulation.

Arguments:

  Pointer to CPU Architectural Protocol interface
  INIT Type

Returns:

  Status
    EFI_UNSUPPORTED - not yet implemented

--*/
// TODO:    This - add argument and description to function comment
// TODO:    InitType - add argument and description to function comment
{
  CPU_ARCH_PROTOCOL_PRIVATE *Private;

  Private = CPU_ARCH_PROTOCOL_PRIVATE_DATA_FROM_THIS (This);
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
WinNtRegisterInterruptHandler (
  IN EFI_CPU_ARCH_PROTOCOL      *This,
  IN EFI_EXCEPTION_TYPE         InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER  InterruptHandler
  )
/*++

Routine Description:

  This routine would support registration of an interrupt handler.  At
  present, this code does not provide emulation.

Arguments:

  Pointer to CPU Architectural Protocol interface
  Pointer to interrupt handlers
  Interrupt type

Returns:

  Status
    EFI_UNSUPPORTED - not yet implemented

--*/
// TODO:    This - add argument and description to function comment
// TODO:    InterruptType - add argument and description to function comment
// TODO:    InterruptHandler - add argument and description to function comment
{
  CPU_ARCH_PROTOCOL_PRIVATE *Private;

  //
  // Do parameter checking for EFI spec conformance
  //
  if (InterruptType < 0 || InterruptType > 0xff) {
    return EFI_UNSUPPORTED;
  }
  //
  // Do nothing for Nt32 emulation
  //
  Private = CPU_ARCH_PROTOCOL_PRIVATE_DATA_FROM_THIS (This);
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
WinNtGetTimerValue (
  IN  EFI_CPU_ARCH_PROTOCOL *This,
  IN  UINT32                TimerIndex,
  OUT UINT64                *TimerValue,
  OUT UINT64                *TimerPeriod OPTIONAL
  )
/*++

Routine Description:

  This routine would support querying of an on-CPU timer.  At present,
  this code does not provide timer emulation.

Arguments:

  This        - Pointer to CPU Architectural Protocol interface
  TimerIndex  - Index of given CPU timer
  TimerValue  - Output of the timer
  TimerPeriod - Output of the timer period

Returns:

  EFI_UNSUPPORTED       - not yet implemented
  EFI_INVALID_PARAMETER - TimeValue is NULL

--*/
{
  if (TimerValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // No timer supported
  //
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
WinNtSetMemoryAttributes (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  IN EFI_PHYSICAL_ADDRESS   BaseAddress,
  IN UINT64                 Length,
  IN UINT64                 Attributes
  )
/*++

Routine Description:

  This routine would support querying of an on-CPU timer.  At present,
  this code does not provide timer emulation.

Arguments:

  Pointer to CPU Architectural Protocol interface
  Start address of memory region
  The size in bytes of the memory region
  The bit mask of attributes to set for the memory region

Returns:

  Status
    EFI_UNSUPPORTED - not yet implemented

--*/
// TODO:    This - add argument and description to function comment
// TODO:    BaseAddress - add argument and description to function comment
// TODO:    Length - add argument and description to function comment
// TODO:    Attributes - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  CPU_ARCH_PROTOCOL_PRIVATE *Private;

  //
  // Check for invalid parameter for Spec conformance
  //
  if (Length == 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Do nothing for Nt32 emulation
  //
  Private = CPU_ARCH_PROTOCOL_PRIVATE_DATA_FROM_THIS (This);
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
InitializeCpu (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

  Initialize the state information for the CPU Architectural Protocol

Arguments:

  ImageHandle of the loaded driver
  Pointer to the System Table

Returns:

  Status

  EFI_SUCCESS           - protocol instance can be published
  EFI_OUT_OF_RESOURCES  - cannot allocate protocol data structure
  EFI_DEVICE_ERROR      - cannot create the thread

--*/
// TODO:    SystemTable - add argument and description to function comment
{
  EFI_STATUS                Status;
  EFI_EVENT                 Event;
  CPU_ARCH_PROTOCOL_PRIVATE *Private;
  VOID                      *Registration;

  Private = AllocatePool (sizeof (CPU_ARCH_PROTOCOL_PRIVATE));
  ASSERT (Private != NULL);

  Private->Signature                    = CPU_ARCH_PROT_PRIVATE_SIGNATURE;
  Private->Cpu.FlushDataCache           = WinNtFlushCpuDataCache;
  Private->Cpu.EnableInterrupt          = WinNtEnableInterrupt;
  Private->Cpu.DisableInterrupt         = WinNtDisableInterrupt;
  Private->Cpu.GetInterruptState        = WinNtGetInterruptState;
  Private->Cpu.Init                     = WinNtInit;
  Private->Cpu.RegisterInterruptHandler = WinNtRegisterInterruptHandler;
  Private->Cpu.GetTimerValue            = WinNtGetTimerValue;
  Private->Cpu.SetMemoryAttributes      = WinNtSetMemoryAttributes;

  Private->Cpu.NumberOfTimers           = 0;
  Private->Cpu.DmaBufferAlignment       = 4;

  Private->InterruptState               = TRUE;

  Private->CpuIo.Mem.Read   = CpuMemoryServiceRead;
  Private->CpuIo.Mem.Write  = CpuMemoryServiceWrite;
  Private->CpuIo.Io.Read    = CpuIoServiceRead;
  Private->CpuIo.Io.Write   = CpuIoServiceWrite;


  Private->Handle                       = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->Handle,
                  &gEfiCpuArchProtocolGuid,   &Private->Cpu,
                  &gEfiCpuIoProtocolGuid,     &Private->CpuIo,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Install notify function to store processor data to HII database and data hub.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  WinNtIoProtocolNotifyFunction,
                  ImageHandle,
                  &Event
                  );
  ASSERT (!EFI_ERROR (Status));

  Status = gBS->RegisterProtocolNotify (
                  &gEfiWinNtIoProtocolGuid,
                  Event,
                  &Registration
                  );
  ASSERT (!EFI_ERROR (Status));

  //
  // Should be at EFI_D_INFO, but lets us now things are running
  //
  DEBUG ((EFI_D_ERROR, "CPU Architectural Protocol Loaded\n"));



  return Status;
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
  This function will log processor version and frequency data to data hub.

Arguments:
  Event        - Event whose notification function is being invoked.
  Context      - Pointer to the notification function's context.

Returns:
  None.

--*/
{
  EFI_STATUS                  Status;
  EFI_CPU_DATA_RECORD_BUFFER  RecordBuffer;
  EFI_DATA_RECORD_HEADER      *Record;
  EFI_SUBCLASS_TYPE1_HEADER   *DataHeader;
  UINT32                      HeaderSize;
  UINT32                      TotalSize;
  UINTN                       HandleCount;
  UINTN                       HandleIndex;
  UINT64                      MonotonicCount;
  BOOLEAN                     RecordFound;
  EFI_HANDLE                  *HandleBuffer;
  EFI_WIN_NT_IO_PROTOCOL      *WinNtIo;
  EFI_DATA_HUB_PROTOCOL       *DataHub;
  EFI_HII_PROTOCOL            *Hii;
  EFI_HII_HANDLE              StringHandle;
  EFI_HII_PACKAGES            *PackageList;
  STRING_REF                  Token;

  DataHub         = NULL;
  Token           = 0;
  MonotonicCount  = 0;
  RecordFound     = FALSE;

  //
  // Retrieve the list of all handles from the handle database
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
  // Locate HII protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiProtocolGuid, NULL, &Hii);
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
  // Initialize data record header
  //
  mCpuDataRecordHeader.Instance = 1;
  HeaderSize                    = sizeof (EFI_SUBCLASS_TYPE1_HEADER);

  RecordBuffer.Raw              = AllocatePool (HeaderSize + EFI_CPU_DATA_MAXIMUM_LENGTH);
  if (RecordBuffer.Raw == NULL) {
    return ;
  }

  CopyMem (RecordBuffer.Raw, &mCpuDataRecordHeader, HeaderSize);

  //
  // Search the Handle array to find the CPU model and speed information
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
        CompareGuid (WinNtIo->TypeGuid, &gEfiWinNtCPUModelGuid)
          ) {
      //
      // Check if this record has been stored in data hub
      //
      do {
        Status = DataHub->GetNextRecord (DataHub, &MonotonicCount, NULL, &Record);
        if (Record->DataRecordClass == EFI_DATA_RECORD_CLASS_DATA) {
          DataHeader = (EFI_SUBCLASS_TYPE1_HEADER *) (Record + 1);
          if (CompareGuid (&Record->DataRecordGuid, &gEfiProcessorSubClassGuid) &&
              (DataHeader->RecordType == ProcessorVersionRecordType)
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
      // Initialize strings to HII database
      //
      PackageList = PreparePackages (1, &gEfiProcessorProducerGuid, CpuStrings);

      Status      = Hii->NewPack (Hii, PackageList, &StringHandle);
      ASSERT (!EFI_ERROR (Status));

      FreePool (PackageList);

      //
      // Store processor version data record to data hub
      //
      Status = Hii->NewString (Hii, NULL, StringHandle, &Token, WinNtIo->EnvString);
      ASSERT (!EFI_ERROR (Status));

      RecordBuffer.DataRecord->DataRecordHeader.RecordType      = ProcessorVersionRecordType;
      RecordBuffer.DataRecord->VariableRecord.ProcessorVersion  = Token;
      TotalSize = HeaderSize + sizeof (EFI_PROCESSOR_VERSION_DATA);

      Status = DataHub->LogData (
                          DataHub,
                          &gEfiProcessorSubClassGuid,
                          &gEfiProcessorProducerGuid,
                          EFI_DATA_RECORD_CLASS_DATA,
                          RecordBuffer.Raw,
                          TotalSize
                          );
    }

    if ((WinNtIo->WinNtThunk->Signature == EFI_WIN_NT_THUNK_PROTOCOL_SIGNATURE) &&
        CompareGuid (WinNtIo->TypeGuid, &gEfiWinNtCPUSpeedGuid)
          ) {
      //
      // Check if this record has been stored in data hub
      //
      do {
        Status = DataHub->GetNextRecord (DataHub, &MonotonicCount, NULL, &Record);
        if (Record->DataRecordClass == EFI_DATA_RECORD_CLASS_DATA) {
          DataHeader = (EFI_SUBCLASS_TYPE1_HEADER *) (Record + 1);
          if (CompareGuid (&Record->DataRecordGuid, &gEfiProcessorSubClassGuid) &&
              (DataHeader->RecordType == ProcessorCoreFrequencyRecordType)
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
      // Store CPU frequency data record to data hub
      //
      RecordBuffer.DataRecord->DataRecordHeader.RecordType                    = ProcessorCoreFrequencyRecordType;
      RecordBuffer.DataRecord->VariableRecord.ProcessorCoreFrequency.Value    = (UINT16) Atoi (WinNtIo->EnvString);
      RecordBuffer.DataRecord->VariableRecord.ProcessorCoreFrequency.Exponent = 6;
      TotalSize = HeaderSize + sizeof (EFI_PROCESSOR_CORE_FREQUENCY_DATA);

      Status = DataHub->LogData (
                          DataHub,
                          &gEfiProcessorSubClassGuid,
                          &gEfiProcessorProducerGuid,
                          EFI_DATA_RECORD_CLASS_DATA,
                          RecordBuffer.Raw,
                          TotalSize
                          );

      FreePool (RecordBuffer.Raw);
    }

    gBS->CloseProtocol (
          HandleBuffer[HandleIndex],
          &gEfiWinNtIoProtocolGuid,
          Context,
          NULL
          );
  }
}

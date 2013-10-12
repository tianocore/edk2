/**@file

Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
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

**/


#include "CpuDriver.h"

UINT64  mTimerPeriod;

CPU_ARCH_PROTOCOL_PRIVATE mCpuTemplate = {
  CPU_ARCH_PROT_PRIVATE_SIGNATURE,
  NULL,
  {
    WinNtFlushCpuDataCache,
    WinNtEnableInterrupt,
    WinNtDisableInterrupt,
    WinNtGetInterruptState,
    WinNtInit,
    WinNtRegisterInterruptHandler,
    WinNtGetTimerValue,
    WinNtSetMemoryAttributes,
    1,
    4
  },
  {
    CpuMemoryServiceRead,
    CpuMemoryServiceWrite,
    CpuIoServiceRead,
    CpuIoServiceWrite
  },
  0,
  TRUE
};

#define EFI_CPU_DATA_MAXIMUM_LENGTH 0x100



//
// Service routines for the driver
//
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
  return EFI_UNSUPPORTED;
}


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

  //
  // Do parameter checking for EFI spec conformance
  //
  if (InterruptType < 0 || InterruptType > 0xff) {
    return EFI_UNSUPPORTED;
  }
  //
  // Do nothing for Nt32 emulation
  //
  return EFI_UNSUPPORTED;
}


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
  
  if (TimerIndex != 0) {
    return EFI_INVALID_PARAMETER;
  }
  
  gWinNt->QueryPerformanceCounter ((LARGE_INTEGER *)TimerValue);
  
  if (TimerPeriod != NULL) {
    *TimerPeriod = mTimerPeriod;
  }
  
  return EFI_SUCCESS;
}


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
  //
  // Check for invalid parameter for Spec conformance
  //
  if (Length == 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Do nothing for Nt32 emulation
  //
  return EFI_UNSUPPORTED;
}



/**
  Logs SMBIOS record.

  @param  Smbios   Pointer to SMBIOS protocol instance.
  @param  Buffer   Pointer to the data buffer.

**/
VOID
LogSmbiosData (
  IN  EFI_SMBIOS_PROTOCOL        *Smbios,
  IN  UINT8                      *Buffer
  )
{
  EFI_STATUS         Status;
  EFI_SMBIOS_HANDLE  SmbiosHandle;
  
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios->Add (
                     Smbios,
                     NULL,
                     &SmbiosHandle,
                     (EFI_SMBIOS_TABLE_HEADER*)Buffer
                     );
  ASSERT_EFI_ERROR (Status);
}


VOID
CpuUpdateSmbios (
  VOID
  )
/*++

Routine Description:
  This function will log processor version and frequency data to Smbios.

Arguments:
  Event        - Event whose notification function is being invoked.
  Context      - Pointer to the notification function's context.

Returns:
  None.

--*/
{
  EFI_STATUS                  Status;
  UINT32                      TotalSize;
  EFI_SMBIOS_PROTOCOL         *Smbios;
  EFI_HII_HANDLE              HiiHandle;
  STRING_REF                  Token;
  UINTN                       CpuVerStrLen;
  EFI_STRING                  CpuVerStr;
  SMBIOS_TABLE_TYPE4          *SmbiosRecord;
  CHAR8                       *OptionalStrStart;

  //
  // Locate Smbios protocol.
  //
  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&Smbios);
  
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Initialize strings to HII database
  //
  HiiHandle = HiiAddPackages (
                &gEfiCallerIdGuid,
                NULL,
                CpuStrings,
                NULL
                );
  ASSERT (HiiHandle != NULL);

  Token  = STRING_TOKEN (STR_PROCESSOR_VERSION);
  CpuVerStr = HiiGetPackageString(&gEfiCallerIdGuid, Token, NULL);
  CpuVerStrLen = StrLen(CpuVerStr);
  ASSERT (CpuVerStrLen <= SMBIOS_STRING_MAX_LENGTH);


  TotalSize = (UINT32)(sizeof(SMBIOS_TABLE_TYPE4) + CpuVerStrLen + 1 + 1);
  SmbiosRecord = AllocatePool(TotalSize);
  ZeroMem(SmbiosRecord, TotalSize);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE4);
  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle = 0;  
  //
  // Processor version is the 1st string.
  //
  SmbiosRecord->ProcessorVersion = 1;
  //
  // Store CPU frequency data record to data hub - It's an emulator so make up a value
  //
  SmbiosRecord->CurrentSpeed  = 1234;

  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStr(CpuVerStr, OptionalStrStart);

  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  LogSmbiosData(Smbios, (UINT8 *) SmbiosRecord);
  FreePool (SmbiosRecord);

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
{
  EFI_STATUS  Status;
  UINT64      Frequency;

  //
  // Retrieve the frequency of the performance counter in Hz.
  //  
  gWinNt->QueryPerformanceFrequency ((LARGE_INTEGER *)&Frequency);
  
  //
  // Convert frequency in Hz to a clock period in femtoseconds.
  //
  mTimerPeriod = DivU64x64Remainder (1000000000000000, Frequency, NULL);
  
  CpuUpdateSmbios ();

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mCpuTemplate.Handle,
                  &gEfiCpuArchProtocolGuid,   &mCpuTemplate.Cpu,
                  &gEfiCpuIo2ProtocolGuid,    &mCpuTemplate.CpuIo,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/*++ @file
  Emu driver to produce CPU Architectural Protocol.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuDriver.h"

UINT64  mTimerPeriod;

CPU_ARCH_PROTOCOL_PRIVATE mCpuTemplate = {
  CPU_ARCH_PROT_PRIVATE_SIGNATURE,
  NULL,
  {
    EmuFlushCpuDataCache,
    EmuEnableInterrupt,
    EmuDisableInterrupt,
    EmuGetInterruptState,
    EmuInit,
    EmuRegisterInterruptHandler,
    EmuGetTimerValue,
    EmuSetMemoryAttributes,
    0,
    4
  },
  {
    {
      CpuMemoryServiceRead,
      CpuMemoryServiceWrite
    },
    {
      CpuIoServiceRead,
      CpuIoServiceWrite
    }
  },
  TRUE
};

#define EFI_CPU_DATA_MAXIMUM_LENGTH 0x100



//
// Service routines for the driver
//
EFI_STATUS
EFIAPI
EmuFlushCpuDataCache (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  IN EFI_PHYSICAL_ADDRESS   Start,
  IN UINT64                 Length,
  IN EFI_CPU_FLUSH_TYPE     FlushType
  )
{
  if (FlushType == EfiCpuFlushTypeWriteBackInvalidate) {
    //
    // Only WB flush is supported. We actually need do nothing on Emu emulator
    // environment. Classify this to follow EFI spec
    //
    return EFI_SUCCESS;
  }
  //
  // Other flush types are not supported by Emu emulator
  //
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
EmuEnableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL  *This
  )
{
  CPU_ARCH_PROTOCOL_PRIVATE *Private;

  Private                 = CPU_ARCH_PROTOCOL_PRIVATE_DATA_FROM_THIS (This);
  Private->InterruptState = TRUE;
  gEmuThunk->EnableInterrupt ();
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
EmuDisableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL  *This
  )
{
  CPU_ARCH_PROTOCOL_PRIVATE *Private;

  Private                 = CPU_ARCH_PROTOCOL_PRIVATE_DATA_FROM_THIS (This);
  Private->InterruptState = FALSE;
  gEmuThunk->DisableInterrupt ();
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
EmuGetInterruptState (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  OUT BOOLEAN               *State
  )
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
EmuInit (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  IN EFI_CPU_INIT_TYPE      InitType
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
EmuRegisterInterruptHandler (
  IN EFI_CPU_ARCH_PROTOCOL      *This,
  IN EFI_EXCEPTION_TYPE         InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER  InterruptHandler
  )
{
  //
  // Do parameter checking for EFI spec conformance
  //
  if (InterruptType < 0 || InterruptType > 0xff) {
    return EFI_UNSUPPORTED;
  }
  //
  // Do nothing for Emu emulation
  //
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
EmuGetTimerValue (
  IN  EFI_CPU_ARCH_PROTOCOL *This,
  IN  UINT32                TimerIndex,
  OUT UINT64                *TimerValue,
  OUT UINT64                *TimerPeriod OPTIONAL
  )
{
  if (TimerValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (TimerIndex != 0) {
    return EFI_INVALID_PARAMETER;
  }

  *TimerValue = gEmuThunk->QueryPerformanceCounter ();

  if (TimerPeriod != NULL) {
    *TimerPeriod = mTimerPeriod;
  }

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
EmuSetMemoryAttributes (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  IN EFI_PHYSICAL_ADDRESS   BaseAddress,
  IN UINT64                 Length,
  IN UINT64                 Attributes
  )
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

  TotalSize = sizeof(SMBIOS_TABLE_TYPE4) + CpuVerStrLen + 1 + 1;
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



/**
  Callback function for idle events.

  @param  Event                 Event whose notification function is being invoked.
  @param  Context               The pointer to the notification function's context,
                                which is implementation-dependent.

**/
VOID
EFIAPI
IdleLoopEventCallback (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  gEmuThunk->CpuSleep ();
}


EFI_STATUS
EFIAPI
InitializeCpu (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  UINT64        Frequency;
  EFI_EVENT     IdleLoopEvent;

  //
  // Retrieve the frequency of the performance counter in Hz.
  //
  Frequency = gEmuThunk->QueryPerformanceFrequency ();

  //
  // Convert frequency in Hz to a clock period in femtoseconds.
  //
  mTimerPeriod = DivU64x64Remainder (1000000000000000ULL, Frequency, NULL);

  CpuUpdateSmbios ();

  CpuMpServicesInit ();

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IdleLoopEventCallback,
                  NULL,
                  &gIdleLoopEventGuid,
                  &IdleLoopEvent
                  );
  ASSERT_EFI_ERROR (Status);


  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mCpuTemplate.Handle,
                  &gEfiCpuArchProtocolGuid,   &mCpuTemplate.Cpu,
                  &gEfiCpuIo2ProtocolGuid,    &mCpuTemplate.CpuIo,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

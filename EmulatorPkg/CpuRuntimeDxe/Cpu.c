/*++ @file
  Emu driver to produce CPU Architectural Protocol.

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011 - 2012, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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

SMBIOS_TABLE_TYPE4 mCpuSmbiosType4 = {
  { EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE4), 0},
  1,                    // Socket String
  ProcessorOther,       // ProcessorType;          ///< The enumeration value from PROCESSOR_TYPE_DATA.
  ProcessorFamilyOther, // ProcessorFamily;        ///< The enumeration value from PROCESSOR_FAMILY_DATA.
  2,                    // ProcessorManufacture String;
  {                     // ProcessorId;
    {  // PROCESSOR_SIGNATURE
      0, //  ProcessorSteppingId:4;
      0, //  ProcessorModel:     4;
      0, //  ProcessorFamily:    4;
      0, //  ProcessorType:      2;
      0, //  ProcessorReserved1: 2;
      0, //  ProcessorXModel:    4;
      0, //  ProcessorXFamily:   8;
      0, //  ProcessorReserved2: 4;
    },
    {  // PROCESSOR_FEATURE_FLAGS
      0, //  ProcessorFpu       :1;
      0, //  ProcessorVme       :1;
      0, //  ProcessorDe        :1;
      0, //  ProcessorPse       :1;
      0, //  ProcessorTsc       :1;
      0, //  ProcessorMsr       :1;
      0, //  ProcessorPae       :1;
      0, //  ProcessorMce       :1;
      0, //  ProcessorCx8       :1;
      0, //  ProcessorApic      :1;
      0, //  ProcessorReserved1 :1;
      0, //  ProcessorSep       :1;
      0, //  ProcessorMtrr      :1;
      0, //  ProcessorPge       :1;
      0, //  ProcessorMca       :1;
      0, //  ProcessorCmov      :1;
      0, //  ProcessorPat       :1;
      0, //  ProcessorPse36     :1;
      0, //  ProcessorPsn       :1;
      0, //  ProcessorClfsh     :1;
      0, //  ProcessorReserved2 :1;
      0, //  ProcessorDs        :1;
      0, //  ProcessorAcpi      :1;
      0, //  ProcessorMmx       :1;
      0, //  ProcessorFxsr      :1;
      0, //  ProcessorSse       :1;
      0, //  ProcessorSse2      :1;
      0, //  ProcessorSs        :1;
      0, //  ProcessorReserved3 :1;
      0, //  ProcessorTm        :1;
      0, //  ProcessorReserved4 :2;
    }
  },
  3,                    // ProcessorVersion String;
  {                     // Voltage;
    1,  // ProcessorVoltageCapability5V        :1;
    1,  // ProcessorVoltageCapability3_3V      :1;
    1,  // ProcessorVoltageCapability2_9V      :1;
    0,  // ProcessorVoltageCapabilityReserved  :1; ///< Bit 3, must be zero.
    0,  // ProcessorVoltageReserved            :3; ///< Bits 4-6, must be zero.
    0   // ProcessorVoltageIndicateLegacy      :1;
  },
  0,                      // ExternalClock;
  0,                      // MaxSpeed;
  0,                      // CurrentSpeed;
  0x41,                   // Status;
  ProcessorUpgradeOther,  // ProcessorUpgrade;      ///< The enumeration value from PROCESSOR_UPGRADE.
  0,                      // L1CacheHandle;
  0,                      // L2CacheHandle;
  0,                      // L3CacheHandle;
  4,                      // SerialNumber;
  5,                      // AssetTag;
  6,                      // PartNumber;
  0,                      // CoreCount;
  0,                      // EnabledCoreCount;
  0,                      // ThreadCount;
  0,                      // ProcessorCharacteristics;
  0,                      // ProcessorFamily2;
};

CHAR8 *mCpuSmbiosType4Strings[] = {
  "Socket",
  "http://www.tianocore.org/edk2/",
  "Emulated Processor",
  "1.0",
  "1.0",
  "1.0",
  NULL
};


/**
  Create SMBIOS record.

  Converts a fixed SMBIOS structure and an array of pointers to strings into
  an SMBIOS record where the strings are cat'ed on the end of the fixed record
  and terminated via a double NULL and add to SMBIOS table.

  SMBIOS_TABLE_TYPE32 gSmbiosType12 = {
    { EFI_SMBIOS_TYPE_SYSTEM_CONFIGURATION_OPTIONS, sizeof (SMBIOS_TABLE_TYPE12), 0 },
    1 // StringCount
  };
  CHAR8 *gSmbiosType12Strings[] = {
    "Not Found",
    NULL
  };

  ...
  LogSmbiosData (
    (EFI_SMBIOS_TABLE_HEADER*)&gSmbiosType12,
    gSmbiosType12Strings
    );

  @param  Template    Fixed SMBIOS structure, required.
  @param  StringArray Array of strings to convert to an SMBIOS string pack.
                      NULL is OK.

**/
EFI_STATUS
LogSmbiosData (
  IN  EFI_SMBIOS_TABLE_HEADER *Template,
  IN  CHAR8                   **StringPack
  )
{
  EFI_STATUS                Status;
  EFI_SMBIOS_PROTOCOL       *Smbios;
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER   *Record;
  UINTN                     Index;
  UINTN                     StringSize;
  UINTN                     Size;
  CHAR8                     *Str;

  //
  // Locate Smbios protocol.
  //
  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&Smbios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Calculate the size of the fixed record and optional string pack
  Size = Template->Length;
  if (StringPack == NULL) {
    // At least a double null is required
    Size += 2;
  } else {
    for (Index = 0; StringPack[Index] != NULL; Index++) {
      StringSize = AsciiStrSize (StringPack[Index]);
      Size += StringSize;
    }
    if (StringPack[0] == NULL) {
      // At least a double null is required
      Size += 1;
    }
    // Don't forget the terminating double null
    Size += 1;
  }

  // Copy over Template
  Record = (EFI_SMBIOS_TABLE_HEADER *)AllocateZeroPool (Size);
  if (Record == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (Record, Template, Template->Length);

  // Append string pack
  Str = ((CHAR8 *)Record) + Record->Length;
  for (Index = 0; StringPack[Index] != NULL; Index++) {
    StringSize = AsciiStrSize (StringPack[Index]);
    CopyMem (Str, StringPack[Index], StringSize);
    Str += StringSize;
  }
  *Str = 0;

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios->Add (
                     Smbios,
                     gImageHandle,
                     &SmbiosHandle,
                     Record
                     );
  ASSERT_EFI_ERROR (Status);

  FreePool (Record);
  return Status;
}




VOID
CpuUpdateSmbios (
  IN UINTN  MaxCpus
  )
{
  mCpuSmbiosType4.CoreCount        = (UINT8) MaxCpus;
  mCpuSmbiosType4.EnabledCoreCount = (UINT8) MaxCpus;
  mCpuSmbiosType4.ThreadCount      = (UINT8) MaxCpus;
  //
  // The value of 1234 is fake value for CPU frequency
  //
  mCpuSmbiosType4.CurrentSpeed = 1234;
  LogSmbiosData ((EFI_SMBIOS_TABLE_HEADER *)&mCpuSmbiosType4, mCpuSmbiosType4Strings);
}


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
  UINTN         MaxCpu;

  //
  // Retrieve the frequency of the performance counter in Hz.
  //
  Frequency = gEmuThunk->QueryPerformanceFrequency ();

  //
  // Convert frequency in Hz to a clock period in femtoseconds.
  //
  mTimerPeriod = DivU64x64Remainder (1000000000000000ULL, Frequency, NULL);

  CpuMpServicesInit (&MaxCpu);

  CpuUpdateSmbios (MaxCpu);


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

/** @file

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/MemoryTypeInformation.h>
#include "UefiPayloadEntry.h"

STATIC UINT32  mTopOfLowerUsableDram = 0;

EFI_MEMORY_TYPE_INFORMATION  mDefaultMemoryTypeInformation[] = {
  { EfiACPIReclaimMemory,   FixedPcdGet32 (PcdMemoryTypeEfiACPIReclaimMemory)   },
  { EfiACPIMemoryNVS,       FixedPcdGet32 (PcdMemoryTypeEfiACPIMemoryNVS)       },
  { EfiReservedMemoryType,  FixedPcdGet32 (PcdMemoryTypeEfiReservedMemoryType)  },
  { EfiRuntimeServicesData, FixedPcdGet32 (PcdMemoryTypeEfiRuntimeServicesData) },
  { EfiRuntimeServicesCode, FixedPcdGet32 (PcdMemoryTypeEfiRuntimeServicesCode) },
  { EfiMaxMemoryType,       0                                                   }
};

/**
   Callback function to build resource descriptor HOB

   This function build a HOB based on the memory map entry info.
   It creates only EFI_RESOURCE_MEMORY_MAPPED_IO and EFI_RESOURCE_MEMORY_RESERVED
   resources.

   @param MemoryMapEntry         Memory map entry info got from bootloader.
   @param Params                 A pointer to ACPI_BOARD_INFO.

  @retval EFI_SUCCESS            Successfully build a HOB.
  @retval EFI_INVALID_PARAMETER  Invalid parameter provided.
**/
EFI_STATUS
MemInfoCallbackMmio (
  IN MEMORY_MAP_ENTRY  *MemoryMapEntry,
  IN VOID              *Params
  )
{
  EFI_PHYSICAL_ADDRESS         Base;
  EFI_RESOURCE_TYPE            Type;
  UINT64                       Size;
  EFI_RESOURCE_ATTRIBUTE_TYPE  Attribue;
  ACPI_BOARD_INFO              *AcpiBoardInfo;

  AcpiBoardInfo = (ACPI_BOARD_INFO *)Params;
  if (AcpiBoardInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Skip types already handled in MemInfoCallback
  //
  if ((MemoryMapEntry->Type == E820_RAM) || (MemoryMapEntry->Type == E820_ACPI)) {
    return EFI_SUCCESS;
  }

  if (MemoryMapEntry->Base == AcpiBoardInfo->PcieBaseAddress) {
    //
    // MMCONF is always MMIO
    //
    Type = EFI_RESOURCE_MEMORY_MAPPED_IO;
  } else if (MemoryMapEntry->Base < mTopOfLowerUsableDram) {
    //
    // It's in DRAM and thus must be reserved
    //
    Type = EFI_RESOURCE_MEMORY_RESERVED;
  } else if ((MemoryMapEntry->Base < 0x100000000ULL) && (MemoryMapEntry->Base >= mTopOfLowerUsableDram)) {
    //
    // It's not in DRAM, must be MMIO
    //
    Type = EFI_RESOURCE_MEMORY_MAPPED_IO;
  } else {
    Type = EFI_RESOURCE_MEMORY_RESERVED;
  }

  Base = MemoryMapEntry->Base;
  Size = MemoryMapEntry->Size;

  Attribue = EFI_RESOURCE_ATTRIBUTE_PRESENT |
             EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
             EFI_RESOURCE_ATTRIBUTE_TESTED |
             EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE;

  BuildResourceDescriptorHob (Type, Attribue, (EFI_PHYSICAL_ADDRESS)Base, Size);
  DEBUG ((DEBUG_INFO, "buildhob: base = 0x%lx, size = 0x%lx, type = 0x%x\n", Base, Size, Type));

  if ((MemoryMapEntry->Type == E820_UNUSABLE) ||
      (MemoryMapEntry->Type == E820_DISABLED))
  {
    BuildMemoryAllocationHob (Base, Size, EfiUnusableMemory);
  } else if (MemoryMapEntry->Type == E820_PMEM) {
    BuildMemoryAllocationHob (Base, Size, EfiPersistentMemory);
  }

  return EFI_SUCCESS;
}

/**
   Callback function to find TOLUD (Top of Lower Usable DRAM)

   Estimate where TOLUD (Top of Lower Usable DRAM) resides. The exact position
   would require platform specific code.

   @param MemoryMapEntry         Memory map entry info got from bootloader.
   @param Params                 Not used for now.

  @retval EFI_SUCCESS            Successfully updated mTopOfLowerUsableDram.
**/
EFI_STATUS
FindToludCallback (
  IN MEMORY_MAP_ENTRY  *MemoryMapEntry,
  IN VOID              *Params
  )
{
  //
  // This code assumes that the memory map on this x86 machine below 4GiB is continous
  // until TOLUD. In addition it assumes that the bootloader provided memory tables have
  // no "holes" and thus the first memory range not covered by e820 marks the end of
  // usable DRAM. In addition it's assumed that every reserved memory region touching
  // usable RAM is also covering DRAM, everything else that is marked reserved thus must be
  // MMIO not detectable by bootloader/OS
  //

  //
  // Skip memory types not RAM or reserved
  //
  if ((MemoryMapEntry->Type == E820_UNUSABLE) || (MemoryMapEntry->Type == E820_DISABLED) ||
      (MemoryMapEntry->Type == E820_PMEM))
  {
    return EFI_SUCCESS;
  }

  //
  // Skip resources above 4GiB
  //
  if ((MemoryMapEntry->Base + MemoryMapEntry->Size) > 0x100000000ULL) {
    return EFI_SUCCESS;
  }

  if ((MemoryMapEntry->Type == E820_RAM) || (MemoryMapEntry->Type == E820_ACPI) ||
      (MemoryMapEntry->Type == E820_NVS))
  {
    //
    // It's usable DRAM. Update TOLUD.
    //
    if (mTopOfLowerUsableDram < (MemoryMapEntry->Base + MemoryMapEntry->Size)) {
      mTopOfLowerUsableDram = (UINT32)(MemoryMapEntry->Base + MemoryMapEntry->Size);
    }
  } else {
    //
    // It might be 'reserved DRAM' or 'MMIO'.
    //
    // If it touches usable DRAM at Base assume it's DRAM as well,
    // as it could be bootloader installed tables, TSEG, GTT, ...
    //
    if (mTopOfLowerUsableDram == MemoryMapEntry->Base) {
      mTopOfLowerUsableDram = (UINT32)(MemoryMapEntry->Base + MemoryMapEntry->Size);
    }
  }

  return EFI_SUCCESS;
}

/**
   Callback function to build resource descriptor HOB

   This function build a HOB based on the memory map entry info.
   Only add EFI_RESOURCE_SYSTEM_MEMORY.

   @param MemoryMapEntry         Memory map entry info got from bootloader.
   @param Params                 Not used for now.

  @retval RETURN_SUCCESS        Successfully build a HOB.
**/
EFI_STATUS
MemInfoCallback (
  IN MEMORY_MAP_ENTRY  *MemoryMapEntry,
  IN VOID              *Params
  )
{
  EFI_PHYSICAL_ADDRESS         Base;
  EFI_RESOURCE_TYPE            Type;
  UINT64                       Size;
  EFI_RESOURCE_ATTRIBUTE_TYPE  Attribue;

  //
  // Skip everything not known to be usable DRAM.
  // It will be added later.
  //
  if ((MemoryMapEntry->Type != E820_RAM) && (MemoryMapEntry->Type != E820_ACPI) &&
      (MemoryMapEntry->Type != E820_NVS))
  {
    return RETURN_SUCCESS;
  }

  Type = EFI_RESOURCE_SYSTEM_MEMORY;
  Base = MemoryMapEntry->Base;
  Size = MemoryMapEntry->Size;

  Attribue = EFI_RESOURCE_ATTRIBUTE_PRESENT |
             EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
             EFI_RESOURCE_ATTRIBUTE_TESTED |
             EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE;

  BuildResourceDescriptorHob (Type, Attribue, (EFI_PHYSICAL_ADDRESS)Base, Size);
  DEBUG ((DEBUG_INFO, "buildhob: base = 0x%lx, size = 0x%lx, type = 0x%x\n", Base, Size, Type));

  if (MemoryMapEntry->Type == E820_ACPI) {
    BuildMemoryAllocationHob (Base, Size, EfiACPIReclaimMemory);
  } else if (MemoryMapEntry->Type == E820_NVS) {
    BuildMemoryAllocationHob (Base, Size, EfiACPIMemoryNVS);
  }

  return RETURN_SUCCESS;
}

/**
  It will build HOBs based on information from bootloaders.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required HOBs.
**/
EFI_STATUS
BuildHobFromBl (
  VOID
  )
{
  EFI_STATUS                        Status;
  ACPI_BOARD_INFO                   *AcpiBoardInfo;
  EFI_PEI_GRAPHICS_INFO_HOB         GfxInfo;
  EFI_PEI_GRAPHICS_INFO_HOB         *NewGfxInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB  GfxDeviceInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB  *NewGfxDeviceInfo;
  UNIVERSAL_PAYLOAD_SMBIOS_TABLE    *SmBiosTableHob;
  UNIVERSAL_PAYLOAD_ACPI_TABLE      *AcpiTableHob;

  //
  // First find TOLUD
  //
  DEBUG ((DEBUG_INFO, "Guessing Top of Lower Usable DRAM:\n"));
  Status = ParseMemoryInfo (FindToludCallback, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Assuming TOLUD = 0x%x\n", mTopOfLowerUsableDram));

  //
  // Parse memory info and build memory HOBs for Usable RAM
  //
  DEBUG ((DEBUG_INFO, "Building ResourceDescriptorHobs for usable memory:\n"));
  Status = ParseMemoryInfo (MemInfoCallback, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create guid hob for frame buffer information
  //
  Status = ParseGfxInfo (&GfxInfo);
  if (!EFI_ERROR (Status)) {
    NewGfxInfo = BuildGuidHob (&gEfiGraphicsInfoHobGuid, sizeof (GfxInfo));
    ASSERT (NewGfxInfo != NULL);
    CopyMem (NewGfxInfo, &GfxInfo, sizeof (GfxInfo));
    DEBUG ((DEBUG_INFO, "Created graphics info hob\n"));
  }

  Status = ParseGfxDeviceInfo (&GfxDeviceInfo);
  if (!EFI_ERROR (Status)) {
    NewGfxDeviceInfo = BuildGuidHob (&gEfiGraphicsDeviceInfoHobGuid, sizeof (GfxDeviceInfo));
    ASSERT (NewGfxDeviceInfo != NULL);
    CopyMem (NewGfxDeviceInfo, &GfxDeviceInfo, sizeof (GfxDeviceInfo));
    DEBUG ((DEBUG_INFO, "Created graphics device info hob\n"));
  }

  //
  // Creat SmBios table Hob
  //
  SmBiosTableHob = BuildGuidHob (&gUniversalPayloadSmbiosTableGuid, sizeof (UNIVERSAL_PAYLOAD_SMBIOS_TABLE));
  ASSERT (SmBiosTableHob != NULL);
  SmBiosTableHob->Header.Revision = UNIVERSAL_PAYLOAD_SMBIOS_TABLE_REVISION;
  SmBiosTableHob->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_SMBIOS_TABLE);
  DEBUG ((DEBUG_INFO, "Create smbios table gUniversalPayloadSmbiosTableGuid guid hob\n"));
  Status = ParseSmbiosTable (SmBiosTableHob);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Detected Smbios Table at 0x%lx\n", SmBiosTableHob->SmBiosEntryPoint));
  }

  //
  // Creat ACPI table Hob
  //
  AcpiTableHob = BuildGuidHob (&gUniversalPayloadAcpiTableGuid, sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE));
  ASSERT (AcpiTableHob != NULL);
  AcpiTableHob->Header.Revision = UNIVERSAL_PAYLOAD_ACPI_TABLE_REVISION;
  AcpiTableHob->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE);
  DEBUG ((DEBUG_INFO, "Create ACPI table gUniversalPayloadAcpiTableGuid guid hob\n"));
  Status = ParseAcpiTableInfo (AcpiTableHob);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Detected ACPI Table at 0x%lx\n", AcpiTableHob->Rsdp));
  }

  //
  // Create guid hob for acpi board information
  //
  AcpiBoardInfo = BuildHobFromAcpi (AcpiTableHob->Rsdp);
  ASSERT (AcpiBoardInfo != NULL);

  //
  // Parse memory info and build memory HOBs for reserved DRAM and MMIO
  //
  DEBUG ((DEBUG_INFO, "Building ResourceDescriptorHobs for reserved memory:\n"));
  Status = ParseMemoryInfo (MemInfoCallbackMmio, AcpiBoardInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Parse the misc info provided by bootloader
  //
  Status = ParseMiscInfo ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Error when parsing misc info, Status = %r\n", Status));
  }

  //
  // Parse platform specific information.
  //
  Status = ParsePlatformInfo ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error when parsing platform info, Status = %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  This function will build some generic HOBs that doesn't depend on information from bootloaders.

**/
VOID
BuildGenericHob (
  VOID
  )
{
  UINT32                       RegEax;
  UINT8                        PhysicalAddressBits;
  EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute;

  // The UEFI payload FV
  BuildMemoryAllocationHob (PcdGet32 (PcdPayloadFdMemBase), PcdGet32 (PcdPayloadFdMemSize), EfiBootServicesData);

  //
  // Build CPU memory space and IO space hob
  //
  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
    PhysicalAddressBits = (UINT8)RegEax;
  } else {
    PhysicalAddressBits = 36;
  }

  BuildCpuHob (PhysicalAddressBits, 16);

  //
  // Report Local APIC range, cause sbl HOB to be NULL, comment now
  //
  ResourceAttribute = (
                       EFI_RESOURCE_ATTRIBUTE_PRESENT |
                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
                       EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
                       EFI_RESOURCE_ATTRIBUTE_TESTED
                       );
  BuildResourceDescriptorHob (EFI_RESOURCE_MEMORY_MAPPED_IO, ResourceAttribute, 0xFEC80000, SIZE_512KB);
  BuildMemoryAllocationHob (0xFEC80000, SIZE_512KB, EfiMemoryMappedIO);
}

/**
  Entry point to the C language phase of UEFI payload.

  @param[in]   BootloaderParameter    The starting address of bootloader parameter block.

  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.
**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN UINTN  BootloaderParameter
  )
{
  EFI_STATUS                          Status;
  PHYSICAL_ADDRESS                    DxeCoreEntryPoint;
  UINTN                               MemBase;
  UINTN                               HobMemBase;
  UINTN                               HobMemTop;
  EFI_PEI_HOB_POINTERS                Hob;
  SERIAL_PORT_INFO                    SerialPortInfo;
  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO  *UniversalSerialPort;

  Status = PcdSet64S (PcdBootloaderParameter, BootloaderParameter);
  ASSERT_EFI_ERROR (Status);

  // Initialize floating point operating environment to be compliant with UEFI spec.
  InitializeFloatingPointUnits ();

  // HOB region is used for HOB and memory allocation for this module
  MemBase    = PcdGet32 (PcdPayloadFdMemBase);
  HobMemBase = ALIGN_VALUE (MemBase + PcdGet32 (PcdPayloadFdMemSize), SIZE_1MB);
  HobMemTop  = HobMemBase + FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);

  HobConstructor ((VOID *)MemBase, (VOID *)HobMemTop, (VOID *)HobMemBase, (VOID *)HobMemTop);

  //
  // Build serial port info
  //
  Status = ParseSerialInfo (&SerialPortInfo);
  if (!EFI_ERROR (Status)) {
    UniversalSerialPort = BuildGuidHob (&gUniversalPayloadSerialPortInfoGuid, sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO));
    ASSERT (UniversalSerialPort != NULL);
    UniversalSerialPort->Header.Revision = UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO_REVISION;
    UniversalSerialPort->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO);
    UniversalSerialPort->UseMmio         = (SerialPortInfo.Type == 1) ? FALSE : TRUE;
    UniversalSerialPort->RegisterBase    = SerialPortInfo.BaseAddr;
    UniversalSerialPort->BaudRate        = SerialPortInfo.Baud;
    UniversalSerialPort->RegisterStride  = (UINT8)SerialPortInfo.RegWidth;
    // Set PCD here (vs in PlatformHookLib.c) to avoid adding a new field to UniversalSerialPort struct
    if (SerialPortInfo.InputHertz > 0) {
      Status = PcdSet32S (PcdSerialClockRate, SerialPortInfo.InputHertz);
      if (RETURN_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to set PcdSerialClockRate; Status = %r\n", Status));
        return Status;
      }
    }
  }

  // The library constructors might depend on serial port, so call it after serial port hob
  ProcessLibraryConstructorList ();
  DEBUG ((DEBUG_INFO, "sizeof(UINTN) = 0x%x\n", sizeof (UINTN)));

  // Build HOB based on information from Bootloader
  Status = BuildHobFromBl ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "BuildHobFromBl Status = %r\n", Status));
    return Status;
  }

  // Build other HOBs required by DXE
  BuildGenericHob ();

  //
  // Create Memory Type Information HOB
  //
  BuildGuidDataHob (
    &gEfiMemoryTypeInformationGuid,
    mDefaultMemoryTypeInformation,
    sizeof (mDefaultMemoryTypeInformation)
    );

  // Load the DXE Core
  Status = LoadDxeCore (&DxeCoreEntryPoint);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "DxeCoreEntryPoint = 0x%lx\n", DxeCoreEntryPoint));

  //
  // Mask off all legacy 8259 interrupt sources
  //
  IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0xFF);
  IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, 0xFF);

  Hob.HandoffInformationTable = (EFI_HOB_HANDOFF_INFO_TABLE *)GetFirstHob (EFI_HOB_TYPE_HANDOFF);
  HandOffToDxeCore (DxeCoreEntryPoint, Hob);

  // Should not get here
  CpuDeadLoop ();
  return EFI_SUCCESS;
}

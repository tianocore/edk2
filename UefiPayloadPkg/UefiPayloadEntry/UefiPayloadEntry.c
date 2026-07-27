/** @file

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/MemoryTypeInformation.h>
#include <Library/BaseArchLibSupport.h>
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
  EFI_RESOURCE_ATTRIBUTE_TYPE  Attribute;
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

  Attribute = EFI_RESOURCE_ATTRIBUTE_PRESENT |
              EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
              EFI_RESOURCE_ATTRIBUTE_TESTED |
              EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
              EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
              EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
              EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE;

  BuildResourceDescriptorHob (Type, Attribute, (EFI_PHYSICAL_ADDRESS)Base, Size);
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
   Callback function to find free and usable DRAM for HOB
   The memory region returned will have at least PcdSystemMemoryUefiRegionSize bytes
   and will be aligned to 1 MiB.

   The caller must initialize HobMemBase to zero.

   @param MemoryMapEntry         Memory map entry info got from bootloader.
   @param Params                 Pointer to HobMemBase

  @retval EFI_SUCCESS            Continue walking the memory map
  @retval EFI_ALREADY_STARTED    HobMemBase is not zero

**/
EFI_STATUS
FindFreeMemForHobCallback (
  IN MEMORY_MAP_ENTRY  *MemoryMapEntry,
  IN VOID              *Params
  )
{
  EFI_STATUS        Status;
  MEMORY_MAP_ENTRY  Entry;
  MEMORY_MAP_ENTRY  MemoryMapEntrySplit;
  UINTN             *HobMemBase = (UINTN *)Params;

  //
  // Found new base, nothing to do
  //
  if (*HobMemBase != 0) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Skip memory types not RAM
  //
  if (MemoryMapEntry->Type != E820_RAM) {
    return EFI_SUCCESS;
  }

  //
  // Operate on a copy so the caller's memory map is not modified.
  // SblParseLib passes pointers into the bootloader's HOB and the same
  // array is walked again later to publish system memory resource HOBs.
  //
  Entry = *MemoryMapEntry;

  //
  // Align on 1 MiB
  //
  if (ALIGN_VALUE (Entry.Base, SIZE_1MB) > Entry.Base) {
    //
    // Skip too small
    //
    if (ALIGN_VALUE (Entry.Base, SIZE_1MB) >= (Entry.Base + Entry.Size)) {
      return EFI_SUCCESS;
    }

    Entry.Size -= ALIGN_VALUE (Entry.Base, SIZE_1MB) - Entry.Base;
    Entry.Base  = ALIGN_VALUE (Entry.Base, SIZE_1MB);
  }

  //
  // Skip resources above 4GiB on x86_32
  //
  if ((sizeof (UINTN) == 4) && (Entry.Base >= 0x100000000ULL)) {
    return EFI_SUCCESS;
  }

  if ((sizeof (UINTN) == 4) && ((Entry.Base + Entry.Size) > 0x100000000ULL)) {
    Entry.Size = 0x100000000ULL - Entry.Base;
  }

  //
  // Skip too small
  //
  if (Entry.Size < FixedPcdGet32 (PcdSystemMemoryUefiRegionSize)) {
    return EFI_SUCCESS;
  }

  //
  // Overlaps UefiPayload, split into smaller chunks
  //
  if ((Entry.Base <= PcdGet32 (PcdPayloadFdMemBase)) &&
      ((Entry.Base + Entry.Size) >= PcdGet32 (PcdPayloadFdMemBase)))
  {
    MemoryMapEntrySplit.Type = E820_RAM;
    MemoryMapEntrySplit.Base = Entry.Base;
    MemoryMapEntrySplit.Size = PcdGet32 (PcdPayloadFdMemBase) - MemoryMapEntrySplit.Base;
    Status                   = FindFreeMemForHobCallback (&MemoryMapEntrySplit, Params);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((Entry.Base + Entry.Size) > (PcdGet32 (PcdPayloadFdMemBase) + PcdGet32 (PcdPayloadFdMemSize))) {
      MemoryMapEntrySplit.Base = PcdGet32 (PcdPayloadFdMemBase) + PcdGet32 (PcdPayloadFdMemSize);
      MemoryMapEntrySplit.Size = (Entry.Base + Entry.Size) - MemoryMapEntrySplit.Base;
      Status                   = FindFreeMemForHobCallback (&MemoryMapEntrySplit, Params);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    return EFI_SUCCESS;
  }

  *HobMemBase = Entry.Base;

  return EFI_ALREADY_STARTED;
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
  EFI_RESOURCE_ATTRIBUTE_TYPE  Attribute;

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

  Attribute = EFI_RESOURCE_ATTRIBUTE_PRESENT |
              EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
              EFI_RESOURCE_ATTRIBUTE_TESTED |
              EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
              EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
              EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
              EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE;

  BuildResourceDescriptorHob (Type, Attribute, (EFI_PHYSICAL_ADDRESS)Base, Size);
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
  SMMSTORE_INFO                     SmmStoreInfo;
  SMMSTORE_INFO                     *NewSmmStoreInfo;
  FIRMWARE_INFO                     FirmwareInfo;
  FIRMWARE_INFO                     *NewFirmwareInfo;
  EFI_PEI_GRAPHICS_INFO_HOB         GfxInfo;
  EFI_PEI_GRAPHICS_INFO_HOB         *NewGfxInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB  GfxDeviceInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB  *NewGfxDeviceInfo;
  UNIVERSAL_PAYLOAD_SMBIOS_TABLE    SmBiosTable;
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
  // Create guid hob for SmmStore
  //
  Status = ParseSmmStoreInfo (&SmmStoreInfo);
  if (!EFI_ERROR (Status)) {
    NewSmmStoreInfo = BuildGuidHob (&gEfiSmmStoreInfoHobGuid, sizeof (SmmStoreInfo));
    ASSERT (NewSmmStoreInfo != NULL);
    CopyMem (NewSmmStoreInfo, &SmmStoreInfo, sizeof (SmmStoreInfo));
    DEBUG ((DEBUG_INFO, "Created SmmStore info hob\n"));
  }

  //
  // Create guid hob for firmware information
  //
  Status = ParseFirmwareInfo (&FirmwareInfo);
  if (!EFI_ERROR (Status)) {
    NewFirmwareInfo = BuildGuidHob (&gEfiFirmwareInfoHobGuid, sizeof (FirmwareInfo));
    ASSERT (NewFirmwareInfo != NULL);
    CopyMem (NewFirmwareInfo, &FirmwareInfo, sizeof (FirmwareInfo));
    DEBUG ((DEBUG_INFO, "Created firmware info hob\n"));
  }

  //
  // Create SmBios table Hob
  //
  ZeroMem (&SmBiosTable, sizeof (SmBiosTable));
  Status = ParseSmbiosTable (&SmBiosTable);
  if (!EFI_ERROR (Status)) {
    SmBiosTableHob = BuildGuidHob (&gUniversalPayloadSmbiosTableGuid, sizeof (UNIVERSAL_PAYLOAD_SMBIOS_TABLE));
    ASSERT (SmBiosTableHob != NULL);
    SmBiosTableHob->Header.Revision  = UNIVERSAL_PAYLOAD_SMBIOS_TABLE_REVISION;
    SmBiosTableHob->Header.Length    = sizeof (UNIVERSAL_PAYLOAD_SMBIOS_TABLE);
    SmBiosTableHob->SmBiosEntryPoint = SmBiosTable.SmBiosEntryPoint;
    DEBUG ((DEBUG_INFO, "Detected Smbios Table at 0x%lx\n", SmBiosTableHob->SmBiosEntryPoint));
  }

  //
  // Create ACPI table Hob
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

  //
  // Import update capsules, if there are any.
  //
  Status = ParseCapsules (BuildCvHob);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error when importing update capsules, Status = %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Locate the bootloader's ExtraData HOB and report how many entries it
  actually has room for.

  Only bootloaders that hand over a PEI-format HOB list (Slim Bootloader
  and compatible frontends such as ChainloadApp) can carry an ExtraData
  HOB. coreboot passes a coreboot table pointer, which fails the handoff
  header check and returns NULL.

  @param[in]  BootloaderParameter  Bootloader-provided argument.
  @param[out] Count                Entry count, clamped to what the HOB's
                                   own data size can hold.

  @return  The ExtraData structure, or NULL if the bootloader did not hand
           over a PEI HOB list carrying one.
**/
STATIC
UNIVERSAL_PAYLOAD_EXTRA_DATA *
FindBootloaderExtraDataHob (
  IN  UINTN  BootloaderParameter,
  OUT UINTN  *Count
  )
{
  EFI_PEI_HOB_POINTERS          BlHob;
  UNIVERSAL_PAYLOAD_EXTRA_DATA  *ExtraData;
  UINTN                         DataSize;

  BlHob.Raw = (UINT8 *)BootloaderParameter;
  if ((BlHob.Raw == NULL) ||
      (BlHob.Header->HobType != EFI_HOB_TYPE_HANDOFF) ||
      (BlHob.Header->HobLength != sizeof (EFI_HOB_HANDOFF_INFO_TABLE)))
  {
    return NULL;
  }

  BlHob.Raw = GetNextGuidHob (&gUniversalPayloadExtraDataGuid, BlHob.Raw);
  if (BlHob.Raw == NULL) {
    return NULL;
  }

  ExtraData = (UNIVERSAL_PAYLOAD_EXTRA_DATA *)GET_GUID_HOB_DATA (BlHob.Raw);
  DataSize  = GET_GUID_HOB_DATA_SIZE (BlHob.Raw);
  if (DataSize < sizeof (UNIVERSAL_PAYLOAD_EXTRA_DATA)) {
    return NULL;
  }

  *Count = MIN (
             (UINTN)ExtraData->Count,
             (DataSize - sizeof (UNIVERSAL_PAYLOAD_EXTRA_DATA)) /
             sizeof (UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY)
             );

  return ExtraData;
}

/**
  Locate the payload FV base and size from the bootloader's ExtraData
  HOB, if it published one.

  @param[in]  BootloaderParameter  Bootloader-provided argument.
  @param[out] FvBase               ExtraData "uefi_fv" entry base.
  @param[out] FvSize               ExtraData "uefi_fv" entry size.

  @retval TRUE   The bootloader supplied an ExtraData FV location.
  @retval FALSE  No usable ExtraData "uefi_fv" entry.
**/
STATIC
BOOLEAN
FindPayloadFvFromBootloader (
  IN  UINTN  BootloaderParameter,
  OUT UINTN  *FvBase,
  OUT UINTN  *FvSize
  )
{
  UNIVERSAL_PAYLOAD_EXTRA_DATA  *ExtraData;
  UINTN                         Count;
  UINTN                         Index;

  Count     = 0;
  ExtraData = FindBootloaderExtraDataHob (BootloaderParameter, &Count);
  if (ExtraData == NULL) {
    return FALSE;
  }

  for (Index = 0; Index < Count; Index++) {
    if (AsciiStrnCmp (
          ExtraData->Entry[Index].Identifier,
          "uefi_fv",
          sizeof (ExtraData->Entry[Index].Identifier)
          ) == 0)
    {
      if (ExtraData->Entry[Index].Size == 0) {
        DEBUG ((DEBUG_ERROR, "%a: uefi_fv ExtraData entry has zero size\n", __func__));
        return FALSE;
      }

      *FvBase = (UINTN)ExtraData->Entry[Index].Base;
      *FvSize = (UINTN)ExtraData->Entry[Index].Size;

      return TRUE;
    }
  }

  return FALSE;
}

/**
  This function will build the HOBs that every payload needs regardless of
  which bootloader it was launched from: the payload FV reservation and the
  CPU HOB, plus the Local APIC range on x86.

  @param[in] PayloadFvBase  Base of the payload FV to reserve.
  @param[in] PayloadFvSize  Size of the payload FV to reserve.
**/
VOID
BuildGenericHob (
  IN UINTN  PayloadFvBase,
  IN UINTN  PayloadFvSize
  )
{
  UINT8                        PhysicalAddressBits;
  EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute;

  // The UEFI payload FV
  BuildMemoryAllocationHob (PayloadFvBase, PayloadFvSize, EfiBootServicesData);

  PhysicalAddressBits = ArchGetPhysicalAddressBits ();
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
  EFI_HOB_HANDOFF_INFO_TABLE          *HobInfo;
  UNIVERSAL_PAYLOAD_EXTRA_DATA        *ExtraData;
  UNIVERSAL_PAYLOAD_EXTRA_DATA        *NewExtraData;
  UINTN                               ExtraDataSize;
  UINTN                               ExtraDataCount;
  UINTN                               PayloadFvBase;
  UINTN                               PayloadFvSize;

  Status = PcdSet64S (PcdBootloaderParameter, BootloaderParameter);
  ASSERT_EFI_ERROR (Status);

  // Initialize floating point operating environment to be compliant with UEFI spec.
  InitializeFloatingPointUnits ();

  //
  // Determine the payload FV location. If the bootloader relocated the
  // FV and published an ExtraData HOB, honour that; otherwise fall back
  // to the build-time PCD.
  //
  if (!FindPayloadFvFromBootloader (BootloaderParameter, &PayloadFvBase, &PayloadFvSize)) {
    PayloadFvBase = PcdGet32 (PcdPayloadFdMemBase);
    PayloadFvSize = PcdGet32 (PcdPayloadFdMemSize);
  }

  // HOB region is used for HOB and memory allocation for this module
  MemBase    = PayloadFvBase;
  HobMemBase = 0;

  //
  // Find a good place for HOB and memory allocation
  //
  ParseMemoryInfo (FindFreeMemForHobCallback, &HobMemBase);

  ASSERT (HobMemBase != 0);
  if (HobMemBase == 0) {
    HobMemBase = ALIGN_VALUE (MemBase + PayloadFvSize, SIZE_1MB);
  }

  HobMemTop = HobMemBase + FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);

  HobInfo = HobConstructor ((VOID *)MemBase, (VOID *)HobMemTop, (VOID *)HobMemBase, (VOID *)HobMemTop);

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
  DEBUG ((DEBUG_INFO, "MemBase       = 0x%llx\n", (UINT64)MemBase));
  DEBUG ((DEBUG_INFO, "HobMemBase    = 0x%llx\n", (UINT64)HobMemBase));

  // Build HOB based on information from Bootloader
  Status = BuildHobFromBl ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "BuildHobFromBl Status = %r\n", Status));
    return Status;
  }

  //
  // Republish the ExtraData HOB in the new HOB list so that DXE-phase
  // consumers can locate the relocated payload FV.  LoadDxeCore() does
  // not read it: the FV was resolved once, above.
  //
  ExtraDataCount = 0;
  ExtraData      = FindBootloaderExtraDataHob (BootloaderParameter, &ExtraDataCount);
  if (ExtraData != NULL) {
    ExtraDataSize = sizeof (UNIVERSAL_PAYLOAD_EXTRA_DATA) +
                    ExtraDataCount * sizeof (UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY);
    NewExtraData = BuildGuidHob (&gUniversalPayloadExtraDataGuid, ExtraDataSize);
    ASSERT (NewExtraData != NULL);
    if (NewExtraData == NULL) {
      //
      // Not fatal: the FV was already resolved and reserved.  Only
      // DXE-phase consumers of the HOB lose out, so say so and go on.
      //
      DEBUG ((
        DEBUG_ERROR,
        "%a: failed to build ExtraData HOB of 0x%x bytes\n",
        __func__,
        (UINT32)ExtraDataSize
        ));
    } else {
      CopyMem (NewExtraData, ExtraData, ExtraDataSize);
      NewExtraData->Count         = (UINT32)ExtraDataCount;
      NewExtraData->Header.Length = (UINT16)ExtraDataSize;
      DEBUG ((DEBUG_INFO, "Copied ExtraData HOB with %u entries\n", (UINT32)ExtraDataCount));
    }
  }

  // Build other HOBs required by DXE
  BuildGenericHob (PayloadFvBase, PayloadFvSize);

  //
  // Create Memory Type Information HOB
  //
  BuildGuidDataHob (
    &gEfiMemoryTypeInformationGuid,
    mDefaultMemoryTypeInformation,
    sizeof (mDefaultMemoryTypeInformation)
    );

  // Load the DXE Core
  Status = LoadDxeCore ((EFI_FIRMWARE_VOLUME_HEADER *)PayloadFvBase, &DxeCoreEntryPoint);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "DxeCoreEntryPoint = 0x%lx\n", DxeCoreEntryPoint));

  //
  // Switch to update mode if there is at least one capsule.
  //
  if (GetFirstHob (EFI_HOB_TYPE_UEFI_CAPSULE) != NULL) {
    HobInfo->BootMode = BOOT_ON_FLASH_UPDATE;
  }

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

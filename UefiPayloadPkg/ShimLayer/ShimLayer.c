/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ShimLayer.h"

STATIC UINT32  mTopOfLowerUsableDram = 0;

extern char  _binary_UniversalPayload_elf_start[];

/**
  Allocates one or more pages of type EfiBootServicesData.

  Allocates the number of pages of MemoryType and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.
  If Pages is 0, then NULL is returned.
  If there is not enough memory availble to satisfy the request, then NULL
  is returned.

  @param   Pages                 The number of 4 KB pages to allocate.
  @return  A pointer to the allocated buffer or NULL if allocation fails.
**/
VOID *
AllocatePages (
  IN UINTN  Pages
  )
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_PHYSICAL_ADDRESS        Offset;
  EFI_HOB_HANDOFF_INFO_TABLE  *HobTable;

  Hob.Raw  = GetHobList ();
  HobTable = Hob.HandoffInformationTable;

  if (Pages == 0) {
    return NULL;
  }

  // Make sure allocation address is page alligned.
  Offset = HobTable->EfiFreeMemoryTop & EFI_PAGE_MASK;
  if (Offset != 0) {
    HobTable->EfiFreeMemoryTop -= Offset;
  }

  //
  // Check available memory for the allocation
  //
  if (HobTable->EfiFreeMemoryTop - ((Pages * EFI_PAGE_SIZE) + sizeof (EFI_HOB_MEMORY_ALLOCATION)) < HobTable->EfiFreeMemoryBottom) {
    return NULL;
  }

  HobTable->EfiFreeMemoryTop -= Pages * EFI_PAGE_SIZE;
  BuildMemoryAllocationHob (HobTable->EfiFreeMemoryTop, Pages * EFI_PAGE_SIZE, EfiBootServicesData);

  return (VOID *)(UINTN)HobTable->EfiFreeMemoryTop;
}

/**
  Acquire the coreboot memory table with the given table id

  @param  TableId            Table id to be searched
  @param  MemTable           Pointer to the base address of the memory table
  @param  MemTableSize       Pointer to the size of the memory table

  @retval RETURN_SUCCESS     Successfully find out the memory table.
  @retval RETURN_INVALID_PARAMETER  Invalid input parameters.
  @retval RETURN_NOT_FOUND   Failed to find the memory table.

**/
RETURN_STATUS
ParseCbmemInfo (
  IN  UINT32  TableId,
  OUT VOID    **MemTable,
  OUT UINT32  *MemTableSize
  )
{
  EFI_STATUS               Status;
  CB_MEMORY                *Rec;
  struct cb_memory_range   *Range;
  UINT64                   Start;
  UINT64                   Size;
  UINTN                    Index;
  struct cbmem_root        *CbMemLgRoot;
  VOID                     *CbMemSmRoot;
  VOID                     *CbMemSmRootTable;
  UINT32                   SmRootTableSize;
  struct imd_root_pointer  *SmRootPointer;

  if (MemTable == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  *MemTable = NULL;
  Status    = RETURN_NOT_FOUND;

  //
  // Get the coreboot memory table
  //
  Rec = (CB_MEMORY *)FindCbTag (CB_TAG_MEMORY);
  if (Rec == NULL) {
    return Status;
  }

  for (Index = 0; Index < MEM_RANGE_COUNT (Rec); Index++) {
    Range = MEM_RANGE_PTR (Rec, Index);
    Start = cb_unpack64 (Range->start);
    Size  = cb_unpack64 (Range->size);

    if ((Range->type == CB_MEM_TABLE) && (Start > 0x1000)) {
      CbMemLgRoot = (struct  cbmem_root *)(UINTN)(Start + Size - DYN_CBMEM_ALIGN_SIZE);
      Status      = FindCbMemTable (CbMemLgRoot, TableId, MemTable, MemTableSize);
      if (!EFI_ERROR (Status)) {
        break;
      } else {
        /* Try to locate small root table and find the target CBMEM entry in small root table */
        Status        = FindCbMemTable (CbMemLgRoot, CBMEM_ID_IMD_SMALL, &CbMemSmRootTable, &SmRootTableSize);
        SmRootPointer = (struct imd_root_pointer *)(UINTN)((UINTN)CbMemSmRootTable + SmRootTableSize - sizeof (struct imd_root_pointer));
        CbMemSmRoot   = (struct cbmem_root *)(UINTN)(SmRootPointer->root_offset + (UINTN)SmRootPointer);
        if (!EFI_ERROR (Status)) {
          Status = FindCbMemTable ((struct cbmem_root *)CbMemSmRoot, TableId, MemTable, MemTableSize);
          if (!EFI_ERROR (Status)) {
            break;
          }
        }
      }
    }
  }

  return Status;
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

  //
  // Skip types already handled in MemInfoCallback
  //
  if ((MemoryMapEntry->Type == E820_RAM) || (MemoryMapEntry->Type == E820_ACPI)) {
    return EFI_SUCCESS;
  }

  if (MemoryMapEntry->Base < mTopOfLowerUsableDram) {
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
  Parse and handle the misc info provided by bootloader

  @retval RETURN_SUCCESS           The misc information was parsed successfully.
  @retval RETURN_NOT_FOUND         Could not find required misc info.
  @retval RETURN_OUT_OF_RESOURCES  Insufficant memory space.

**/
RETURN_STATUS
EFIAPI
ParseRootBridgeInfo (
  OUT UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE  *PciRootBridge,
  OUT UINT32                             *PciRootBridgeCount
  )
{
  EFI_STATUS         Status;
  UINTN              *FspHobListAddr;
  UINT32             HobLength;
  EFI_HOB_GUID_TYPE  *GuidHob;
  IIO_UDS            *IioUdsHob;
  EFI_GUID           UniversalDataGuid = IIO_UNIVERSAL_DATA_GUID;
  UINT32             Index;
  UINT32             Count = 0;
  UDS_STACK_RES      *StackRes;
  UINT32             IIONum;
  UINT32             PciRootIndex;

  Status = ParseCbmemInfo (CBMEM_ID_FSP_RUNTIME, (VOID *)&FspHobListAddr, &HobLength);
  DEBUG ((DEBUG_INFO, "Find FspHosList at 0x%x\n", *FspHobListAddr));
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  GuidHob = GetNextGuidHob (&UniversalDataGuid, (VOID *)(*FspHobListAddr));
  if (GuidHob != NULL) {
    IioUdsHob = (IIO_UDS *)GET_GUID_HOB_DATA (GuidHob);
    for (IIONum = 0; IIONum <= IioUdsHob->PlatformData.numofIIO; IIONum++) {
      for (Index = 0; Index < MAX_LOGIC_IIO_STACK; Index++) {
        StackRes = &IioUdsHob->PlatformData.IIO_resource[IIONum].StackRes[Index];
        // Payload only needs to handle the TYPE_IIO type stack info
        if ((StackRes->BusBase <= StackRes->BusLimit) && (StackRes->Personality == TYPE_IIO)) {
          for (PciRootIndex = 0; PciRootIndex < MAX_IIO_PCIROOTS_PER_STACK; PciRootIndex++) {
            if (StackRes->PciRoot[PciRootIndex].BusBase != StackRes->BusBase) {
              continue;
            }

            DEBUG ((DEBUG_INFO, "============================================================================\n"));
            DEBUG ((DEBUG_INFO, "StackRes->PciRoot[PciRootIndex].BusBase    : %x\n", StackRes->PciRoot[PciRootIndex].BusBase));
            DEBUG ((DEBUG_INFO, "StackRes->PciRoot[PciRootIndex].BusLimit   : %x\n", StackRes->PciRoot[PciRootIndex].BusLimit));
            DEBUG ((DEBUG_INFO, "StackRes->PciRoot[PciRootIndex].IoBase     : %x\n", StackRes->PciRoot[PciRootIndex].IoBase));
            DEBUG ((DEBUG_INFO, "StackRes->PciRoot[PciRootIndex].IoLimit    : %x\n", StackRes->PciRoot[PciRootIndex].IoLimit));
            DEBUG ((DEBUG_INFO, "StackRes->PciRoot[PciRootIndex].Mmio32Base : %x\n", StackRes->PciRoot[PciRootIndex].Mmio32Base));
            DEBUG ((DEBUG_INFO, "StackRes->PciRoot[PciRootIndex].Mmio32Limit: %x\n", StackRes->PciRoot[PciRootIndex].Mmio32Limit));
            DEBUG ((DEBUG_INFO, "StackRes->PciRoot[PciRootIndex].Mmio64Base : %x\n", StackRes->PciRoot[PciRootIndex].Mmio64Base));
            DEBUG ((DEBUG_INFO, "StackRes->PciRoot[PciRootIndex].Mmio64Limit: %x\n", StackRes->PciRoot[PciRootIndex].Mmio64Limit));
            PciRootBridge[Count].Segment                 = IioUdsHob->PlatformData.CpuQpiInfo[IIONum].PcieSegment;
            PciRootBridge[Count].Supports                = EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO | EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO | EFI_PCI_ATTRIBUTE_ISA_IO_16 | EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16 | EFI_PCI_ATTRIBUTE_VGA_MEMORY | EFI_PCI_ATTRIBUTE_VGA_IO_16;
            PciRootBridge[Count].Attributes              = 0;
            PciRootBridge[Count].DmaAbove4G              = FALSE;
            PciRootBridge[Count].NoExtendedConfigSpace   = FALSE;
            PciRootBridge[Count].AllocationAttributes    = EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM | EFI_PCI_HOST_BRIDGE_MEM64_DECODE;
            PciRootBridge[Count].Bus.Base                = StackRes->PciRoot[PciRootIndex].BusBase;
            PciRootBridge[Count].Bus.Limit               = StackRes->PciRoot[PciRootIndex].BusLimit;
            PciRootBridge[Count].Bus.Translation         = 0;
            PciRootBridge[Count].Io.Base                 = StackRes->PciRoot[PciRootIndex].IoBase;
            PciRootBridge[Count].Io.Limit                = StackRes->PciRoot[PciRootIndex].IoLimit;
            PciRootBridge[Count].Io.Translation          = 0;
            PciRootBridge[Count].Mem.Base                = StackRes->PciRoot[PciRootIndex].Mmio32Base;
            PciRootBridge[Count].Mem.Limit               = StackRes->PciRoot[PciRootIndex].Mmio32Limit;
            PciRootBridge[Count].Mem.Translation         = 0;
            PciRootBridge[Count].MemAbove4G.Base         = StackRes->PciRoot[PciRootIndex].Mmio64Base;
            PciRootBridge[Count].MemAbove4G.Limit        = StackRes->PciRoot[PciRootIndex].Mmio64Limit;
            PciRootBridge[Count].MemAbove4G.Translation  = 0;
            PciRootBridge[Count].PMem.Base               = 0xFFFFFFFFFFFFFFFF;
            PciRootBridge[Count].PMem.Limit              = 0;
            PciRootBridge[Count].PMem.Translation        = 0;
            PciRootBridge[Count].PMemAbove4G.Base        = 0xFFFFFFFFFFFFFFFF;
            PciRootBridge[Count].PMemAbove4G.Limit       = 0;
            PciRootBridge[Count].PMemAbove4G.Translation = 0;
            PciRootBridge[Count].HID                     = EISA_PNP_ID (0x0A03);
            PciRootBridge[Count].UID                     = Count;
            Count++;
          }
        }
      }
    }
  }

  *PciRootBridgeCount = Count;

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
  EFI_STATUS                          Status;
  EFI_PEI_GRAPHICS_INFO_HOB           GfxInfo;
  EFI_PEI_GRAPHICS_INFO_HOB           *NewGfxInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB    GfxDeviceInfo;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB    *NewGfxDeviceInfo;
  UNIVERSAL_PAYLOAD_SMBIOS_TABLE      *SmBiosTableHob;
  UNIVERSAL_PAYLOAD_ACPI_TABLE        *AcpiTableHob;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE   PciRootBridge[84];
  UINT32                              PciRootBridgeCount;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES  *NewPciRootBridgeInfo;
  UINT32                              Length;

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
  // Creat PciRootBridge Hob
  //
  ZeroMem (PciRootBridge, sizeof (PciRootBridge));
  Status = ParseRootBridgeInfo (&PciRootBridge[0], &PciRootBridgeCount);
  if (!EFI_ERROR (Status)) {
    Length               = PciRootBridgeCount * sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE);
    NewPciRootBridgeInfo = BuildGuidHob (&gUniversalPayloadPciRootBridgeInfoGuid, sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES) + Length);
    ASSERT (NewPciRootBridgeInfo != NULL);

    NewPciRootBridgeInfo->Count            = PciRootBridgeCount;
    NewPciRootBridgeInfo->Header.Length    = sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES) + Length;
    NewPciRootBridgeInfo->Header.Revision  = UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION;
    NewPciRootBridgeInfo->ResourceAssigned = TRUE;

    CopyMem (&(NewPciRootBridgeInfo->RootBridge[0]), &PciRootBridge, Length);
    DEBUG ((DEBUG_INFO, "Created PCI root bridge info hob\n"));
  }

  //
  // Parse memory info and build memory HOBs for reserved DRAM and MMIO
  //
  DEBUG ((DEBUG_INFO, "Building ResourceDescriptorHobs for reserved memory:\n"));
  Status = ParseMemoryInfo (MemInfoCallbackMmio, NULL);
  if (EFI_ERROR (Status)) {
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

  // Memory allocation hob for the Shim Layer
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

EFI_STATUS
ConvertCbmemToHob (
  VOID
  )
{
  UINTN                               MemBase;
  UINTN                               HobMemBase;
  UINTN                               HobMemTop;
  EFI_STATUS                          Status;
  SERIAL_PORT_INFO                    SerialPortInfo;
  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO  *UniversalSerialPort;

  MemBase    = PcdGet32 (PcdPayloadFdMemBase);
  HobMemBase = ALIGN_VALUE (MemBase + PcdGet32 (PcdPayloadFdMemSize), SIZE_1MB);
  HobMemTop  = HobMemBase + PcdGet32 (PcdSystemMemoryUefiRegionSize);
  HobConstructor ((VOID *)MemBase, (VOID *)HobMemTop, (VOID *)HobMemBase, (VOID *)HobMemTop);

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
  }

  ProcessLibraryConstructorList ();
  DEBUG ((DEBUG_INFO, "sizeof(UINTN) = 0x%x\n", sizeof (UINTN)));
  Status = BuildHobFromBl ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "BuildHobFromBl Status = %r\n", Status));
    return Status;
  }

  BuildGenericHob ();
  return EFI_SUCCESS;
}

/**
  This function relocate elf file to an align base address.
**/
EFI_STATUS
RelocateElfImage (
  IN OUT ELF_IMAGE_CONTEXT  *ElfCt
  )
{
  UINTN  DestSize;
  VOID   *DestAddress;

  DestSize    = ElfCt->FileSize;
  DestAddress = AllocatePages (EFI_SIZE_TO_PAGES (DestSize));
  CopyMem (DestAddress, ElfCt->FileBase, DestSize);

  //
  // Replace Address
  //
  ElfCt->FileBase = DestAddress;

  return EFI_SUCCESS;
}

EFI_STATUS
LoadPayload (
  OUT    EFI_PHYSICAL_ADDRESS  *ImageAddressArg   OPTIONAL,
  OUT    UINT64                *ImageSizeArg,
  OUT    PHYSICAL_ADDRESS      *UniversalPayloadEntry
  )
{
  EFI_STATUS                    Status;
  UINT32                        Index;
  UINT16                        ExtraDataIndex;
  CHAR8                         *SectionName;
  UINTN                         Offset;
  UINTN                         Size;
  UINTN                         Length;
  UINT32                        ExtraDataCount;
  ELF_IMAGE_CONTEXT             Context;
  UNIVERSAL_PAYLOAD_EXTRA_DATA  *ExtraData;

  Status = ParseElfImage (_binary_UniversalPayload_elf_start, &Context);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Relocated Elf to an align base address
  //
  RelocateElfImage (&Context);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "Payload File Size: 0x%08X, Mem Size: 0x%08x, Reload: %d\n",
    Context.FileSize,
    Context.ImageSize,
    Context.ReloadRequired
    ));

  //
  // Get UNIVERSAL_PAYLOAD_INFO_HEADER and number of additional PLD sections.
  //

  ExtraDataCount = 0;
  for (Index = 0; Index < Context.ShNum; Index++) {
    Status = GetElfSectionName (&Context, Index, &SectionName);
    if (EFI_ERROR (Status)) {
      continue;
    }

    DEBUG ((DEBUG_INFO, "Payload Section[%d]: %a\n", Index, SectionName));
    if (AsciiStrnCmp (SectionName, UNIVERSAL_PAYLOAD_EXTRA_SEC_NAME_PREFIX, UNIVERSAL_PAYLOAD_EXTRA_SEC_NAME_PREFIX_LENGTH) == 0) {
      Status = GetElfSectionPos (&Context, Index, &Offset, &Size);
      if (!EFI_ERROR (Status)) {
        ExtraDataCount++;
      }
    }
  }

  //
  // Report the additional PLD sections through HOB.
  //
  Length    = sizeof (UNIVERSAL_PAYLOAD_EXTRA_DATA) + ExtraDataCount * sizeof (UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY);
  ExtraData = BuildGuidHob (
                &gUniversalPayloadExtraDataGuid,
                Length
                );
  ExtraData->Count           = ExtraDataCount;
  ExtraData->Header.Revision = UNIVERSAL_PAYLOAD_EXTRA_DATA_REVISION;
  ExtraData->Header.Length   = (UINT16)Length;
  if (ExtraDataCount != 0) {
    for (ExtraDataIndex = 0, Index = 0; Index < Context.ShNum; Index++) {
      Status = GetElfSectionName (&Context, Index, &SectionName);
      if (EFI_ERROR (Status)) {
        continue;
      }

      if (AsciiStrnCmp (SectionName, UNIVERSAL_PAYLOAD_EXTRA_SEC_NAME_PREFIX, UNIVERSAL_PAYLOAD_EXTRA_SEC_NAME_PREFIX_LENGTH) == 0) {
        Status = GetElfSectionPos (&Context, Index, &Offset, &Size);
        if (!EFI_ERROR (Status)) {
          ASSERT (ExtraDataIndex < ExtraDataCount);
          AsciiStrCpyS (
            ExtraData->Entry[ExtraDataIndex].Identifier,
            sizeof (ExtraData->Entry[ExtraDataIndex].Identifier),
            SectionName + UNIVERSAL_PAYLOAD_EXTRA_SEC_NAME_PREFIX_LENGTH
            );
          ExtraData->Entry[ExtraDataIndex].Base = (UINTN)(Context.FileBase + Offset);
          ExtraData->Entry[ExtraDataIndex].Size = Size;
          ExtraDataIndex++;
        }
      }
    }
  }

  if (Context.ReloadRequired || (Context.PreferredImageAddress != Context.FileBase)) {
    Context.ImageAddress = AllocatePages (EFI_SIZE_TO_PAGES (Context.ImageSize));
  } else {
    Context.ImageAddress = Context.FileBase;
  }

  //
  // Load ELF into the required base
  //
  Status = LoadElfImage (&Context);
  if (!EFI_ERROR (Status)) {
    *ImageAddressArg       = (UINTN)Context.ImageAddress;
    *UniversalPayloadEntry = Context.EntryPoint;
    *ImageSizeArg          = Context.ImageSize;
  }

  return Status;
}

EFI_STATUS
HandOffToPayload (
  IN     PHYSICAL_ADDRESS      UniversalPayloadEntry,
  IN     EFI_PEI_HOB_POINTERS  Hob
  )
{
  UINTN  HobList;

  HobList = (UINTN)(VOID *)Hob.Raw;
  typedef VOID (EFIAPI *PayloadEntry)(UINTN);
  ((PayloadEntry)(UINTN)UniversalPayloadEntry)(HobList);

  CpuDeadLoop ();
  return EFI_SUCCESS;
}

/**

  Entry point to the C language phase of Shim Layer before UEFI payload.

  @param[in]   BootloaderParameter    The starting address of bootloader parameter block.

  @retval      It will not return if SUCCESS, and return error when passing bootloader parameter.

**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN UINTN  BootloaderParameter
  )
{
  EFI_STATUS            Status;
  EFI_PEI_HOB_POINTERS  Hob;
  PHYSICAL_ADDRESS      ImageAddress;
  UINT64                ImageSize;
  PHYSICAL_ADDRESS      UniversalPayloadEntry;

  Status = PcdSet64S (PcdBootloaderParameter, BootloaderParameter);
  ASSERT_EFI_ERROR (Status);

  Status = ConvertCbmemToHob ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ConvertCbmemToHob Status = %r\n", Status));
    return Status;
  }

  Status = LoadPayload (&ImageAddress, &ImageSize, &UniversalPayloadEntry);
  ASSERT_EFI_ERROR (Status);
  BuildMemoryAllocationHob (ImageAddress, ImageSize, EfiBootServicesData);

  Hob.HandoffInformationTable = (EFI_HOB_HANDOFF_INFO_TABLE *)GetFirstHob (EFI_HOB_TYPE_HANDOFF);
  HandOffToPayload (UniversalPayloadEntry, Hob);

  // Should not get here
  CpuDeadLoop ();
  return EFI_SUCCESS;
}

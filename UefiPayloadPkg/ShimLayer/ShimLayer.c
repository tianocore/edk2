/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ShimLayer.h"

STATIC UINT32  mTopOfLowerUsableDram = 0;

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

  //
  // Make sure allocation address is page alligned.
  //
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
  EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute;

  // Memory allocaion hob for the Shim Layer
  BuildMemoryAllocationHob (0x800000, 0x100000, EfiBootServicesData);

  //
  // Build CPU memory space and IO space hob
  //
  BuildCpuHob (36, 16);

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
  This function will put the contents of CBMEM into a Hob.
**/
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

  MemBase = 0x00800000;
  // Assume the size of shimlayer.elf is 0x100000.
  HobMemBase = ALIGN_VALUE (MemBase + 0x100000, SIZE_1MB);
  HobMemTop  = HobMemBase + 0x800000;
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
  This function will decompress the payload located at a fixed offset.
**/
EFI_STATUS
LocateAndDecompressPayload (
  VOID
  )
{
  EFI_STATUS                   Status;
  PHYSICAL_ADDRESS             SourceAddress;
  UINT64                       ImageSize;
  PHYSICAL_ADDRESS             DestAddress;
  PHYSICAL_ADDRESS             CBFSAddress;
  VOID                         *FMapEntry;
  UINT32                       FMapEntrySize;
  struct fmap_area             *FMapArea;
  VOID                         *MCacheEntryBase;
  union mcache_entry           *MCacheEntry;
  UINT32                       MCacheEntrySize;
  struct cbfs_payload_segment  *FirstSegment;
  UINT32                       Index;

  DestAddress   = 0x1600000;
  SourceAddress = 0;
  CBFSAddress   = 0;

  Status = ParseCbmemInfo (CBMEM_ID_FMAP, &FMapEntry, &FMapEntrySize);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  /*Locate fmap from CBMEM*/
  FMapArea = (struct fmap_area *)((UINTN)FMapEntry + sizeof (struct fmap));
  for (Index = 0; Index < ((struct fmap *)FMapEntry)->nareas; Index++) {
    DEBUG ((DEBUG_INFO, "Coreboot fmap is %a\n", (CONST CHAR8 *)FMapArea->name));
    if (AsciiStrCmp ((CONST CHAR8 *)FMapArea->name, "COREBOOT") == 0) {
      CBFSAddress = ((struct fmap *)FMapEntry)->base + FMapArea->offset;
      break;
    }

    FMapArea = (struct fmap_area *)((UINTN)FMapArea + sizeof (struct fmap_area));
  }

  if (!CBFSAddress) {
    return EFI_NOT_FOUND;
  }

  /*Locate MCACHE from CBMEM. Parse payload address from MCACHE.*/
  Status = ParseCbmemInfo (CBMEM_ID_CBFS_RO_MCACHE, &MCacheEntryBase, &MCacheEntrySize);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  MCacheEntry = (union mcache_entry *)MCacheEntryBase;
  while (((UINTN)MCacheEntry + CBFS_MCACHE_ALIGNMENT) <= ((UINTN)MCacheEntryBase + MCacheEntrySize)) {
    if (MCacheEntry->magic == MCACHE_MAGIC_FILE) {
      if (AsciiStrCmp (MCacheEntry->file.h.filename, CBFS_UNIVERSAL_PAYLOAD) == 0) {
        FirstSegment  = &((struct cbfs_payload *)(UINTN)(CBFSAddress + MCacheEntry->offset + SWAP32 (MCacheEntry->file.h.offset)))->segments;
        SourceAddress = (UINTN)FirstSegment + SWAP32 (FirstSegment->offset);
        ImageSize     = SWAP32 (FirstSegment->len);
        ASSERT (SWAP32 (FirstSegment->type)              == PAYLOAD_SEGMENT_CODE);
        ASSERT (SWAP32 ((FirstSegment + 1)->type)        == PAYLOAD_SEGMENT_ENTRY);
        break;
      }
    }

    MCacheEntry = (union mcache_entry *)((UINTN)MCacheEntry + ALIGN_UP (SWAP32 (MCacheEntry->file.h.offset), CBFS_MCACHE_ALIGNMENT));
  }

  if (!SourceAddress) {
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "Locate Payload at 0x%x\n", SourceAddress));
  Status = LzmaUefiDecompress ((VOID *)(UINTN)SourceAddress, ImageSize, (VOID *)(UINTN)DestAddress, (VOID *)(UINTN)0x2000000);
  if (EFI_ERROR (Status)) {
    return RETURN_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Load the payload.

  @param  ImageAddressArg         Address of the image
  @param  ImageSizeArg            Size of the image
  @param  UniversalPayloadEntry   Address of the payload entry

  @retval RETURN_SUCCESS     Payload was loaded

**/
EFI_STATUS
LoadPayload (
  OUT    EFI_PHYSICAL_ADDRESS  *ImageAddressArg   OPTIONAL,
  OUT    UINT64                *ImageSizeArg,
  OUT    PHYSICAL_ADDRESS      *UniversalPayloadEntry
  )
{
  EFI_STATUS                     Status;
  UINT32                         Index;
  UINT16                         ExtraDataIndex;
  CHAR8                          *SectionName;
  UINTN                          Offset;
  UINTN                          Size;
  UINTN                          Length;
  UINT32                         ExtraDataCount;
  VOID                           *Elf;
  ELF_IMAGE_CONTEXT              Context;
  UNIVERSAL_PAYLOAD_EXTRA_DATA   *ExtraData;
  UNIVERSAL_PAYLOAD_INFO_HEADER  *PldInfo;

  /* Hard code for now */
  Elf    = (VOID *)0x1600000;
  Status = LocateAndDecompressPayload ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ParseElfImage (Elf, &Context);
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
  PldInfo        = NULL;
  ExtraDataCount = 0;
  for (Index = 0; Index < Context.ShNum; Index++) {
    Status = GetElfSectionName (&Context, Index, &SectionName);
    if (EFI_ERROR (Status)) {
      continue;
    }

    DEBUG ((DEBUG_INFO, "Payload Section[%d]: %a\n", Index, SectionName));
    if (AsciiStrCmp (SectionName, UNIVERSAL_PAYLOAD_INFO_SEC_NAME) == 0) {
      Status = GetElfSectionPos (&Context, Index, &Offset, &Size);
      if (!EFI_ERROR (Status)) {
        PldInfo = (UNIVERSAL_PAYLOAD_INFO_HEADER *)(Context.FileBase + Offset);
        DEBUG ((DEBUG_INFO, "PldInfo: %d\n", PldInfo));
      }
    } else if (AsciiStrnCmp (SectionName, UNIVERSAL_PAYLOAD_EXTRA_SEC_NAME_PREFIX, UNIVERSAL_PAYLOAD_EXTRA_SEC_NAME_PREFIX_LENGTH) == 0) {
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

/**
  Hand off to payload.

**/
EFI_STATUS
HandOffToPayload (
  IN     PHYSICAL_ADDRESS      UniversalPayloadEntry,
  IN     EFI_PEI_HOB_POINTERS  Hob
  )
{
  EFI_STATUS  Status;
  UINTN       HobList;

  HobList = (UINTN)(VOID *)Hob.Raw;
  typedef EFI_STATUS (EFIAPI *PayloadEntry)(UINTN);
  Status = ((PayloadEntry)(UINTN)UniversalPayloadEntry)(HobList);
  DEBUG ((DEBUG_ERROR, "HandOffToPayload Status = %r\n", Status));

  CpuDeadLoop ();
//return Status;
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

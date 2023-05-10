/** @file
  This library will parse the coreboot table in memory and extract those required
  information.

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PeCoffLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Guid/MemoryAllocationHob.h>
#include <Library/IoLib.h>
#include <Library/PeCoffLib.h>
#include <Library/PlatformSupportLib.h>
#include <Library/CpuLib.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include <Guid/SerialPortInfoGuid.h>
#include <Guid/MemoryMapInfoGuid.h>
#include <Guid/AcpiBoardInfoGuid.h>
#include <UniversalPayload/SmbiosTable.h>
#include <UniversalPayload/AcpiTable.h>
#include <UniversalPayload/UniversalPayload.h>
#include <UniversalPayload/ExtraData.h>
#include <UniversalPayload/SerialPortInfo.h>
#include <Guid/PcdDataBaseSignatureGuid.h>
#include <Library/BaseRiscVSbiLib.h>
#include <libfdt.h>

#define E820_RAM        1
#define E820_RESERVED   2
#define E820_ACPI       3
#define E820_NVS        4
#define E820_UNUSABLE   5
#define E820_DISABLED   6
#define E820_PMEM       7
#define E820_UNDEFINED  8
#define GET_OCCUPIED_SIZE(ActualSize, Alignment) \
  ((ActualSize) + (((Alignment) - ((ActualSize) & ((Alignment) - 1))) & ((Alignment) - 1)))

/**
  Auto-generated function that calls the library constructors for all of the module's
  dependent libraries.
**/
VOID
EFIAPI
ProcessLibraryConstructorList (
  VOID
  );

/**
  Add a new HOB to the HOB List.

  @param HobType            Type of the new HOB.
  @param HobLength          Length of the new HOB to allocate.

  @return  NULL if there is no space to create a hob.
  @return  The address point to the new created hob.

**/
VOID *
EFIAPI
CreateHob (
  IN  UINT16  HobType,
  IN  UINT16  HobLength
  );

/**
  Build a Handoff Information Table HOB.

  This function initialize a HOB region from EfiMemoryBegin to
  EfiMemoryTop. And EfiFreeMemoryBottom and EfiFreeMemoryTop should
  be inside the HOB region.

  @param[in] EfiMemoryBottom       Total memory start address
  @param[in] EfiMemoryTop          Total memory end address.
  @param[in] EfiFreeMemoryBottom   Free memory start address
  @param[in] EfiFreeMemoryTop      Free memory end address.

  @return   The pointer to the handoff HOB table.
**/
EFI_HOB_HANDOFF_INFO_TABLE *
EFIAPI
HobConstructor (
  IN VOID  *EfiMemoryBottom,
  IN VOID  *EfiMemoryTop,
  IN VOID  *EfiFreeMemoryBottom,
  IN VOID  *EfiFreeMemoryTop
  );

/**
  Build ACPI board info HOB using infomation from ACPI table.

  @param  AcpiTableBase      ACPI table start address in memory
  @retval  A pointer to ACPI board HOB ACPI_BOARD_INFO. Null if build HOB failure.
**/
ACPI_BOARD_INFO *
BuildHobFromAcpi (
  IN   UINT64  AcpiTableBase
  );

/**
   Callback function to build resource descriptor HOB.

   This function build a HOB based on the memory map entry info.
   Only add EFI_RESOURCE_SYSTEM_MEMORY.

   @param MemoryMapEntry         Memory map entry info got from bootloader.

   @retval RETURN_SUCCESS        Successfully build a HOB.
**/
EFI_STATUS
MemInfoCallback (
  IN MEMORY_MAP_ENTRY  *MemoryMapEntry
  )
{
  EFI_RESOURCE_ATTRIBUTE_TYPE  Attribue;

  Attribue = EFI_RESOURCE_ATTRIBUTE_PRESENT |
             EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
             EFI_RESOURCE_ATTRIBUTE_TESTED |
             EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE;

  BuildResourceDescriptorHob (MemoryMapEntry->Type, Attribue, (EFI_PHYSICAL_ADDRESS)MemoryMapEntry->Base, MemoryMapEntry->Size);
  DEBUG ((DEBUG_INFO, "buildhob: base = 0x%lx, size = 0x%lx, type = 0x%x\n", MemoryMapEntry->Base, MemoryMapEntry->Size, MemoryMapEntry->Type));

  if (MemoryMapEntry->Type == E820_ACPI) {
    BuildMemoryAllocationHob (MemoryMapEntry->Base, MemoryMapEntry->Size, EfiACPIReclaimMemory);
  } else if (MemoryMapEntry->Type == E820_NVS) {
    BuildMemoryAllocationHob (MemoryMapEntry->Base, MemoryMapEntry->Size, EfiACPIMemoryNVS);
  }

  return RETURN_SUCCESS;
}

/**
  Build memory hob from FDT.

  @param  Fdt
  @retval  Status: Success/Failure
**/
RETURN_STATUS
EFIAPI
BuildMemHobFromFDT (
  CONST VOID  *Fdt
  )
{
  MEMORY_MAP_ENTRY  MemoryMap;
  MEMORY_MAP_ENTRY  RsvdMemoryMap;
  CONST INT32       *Prop;
  INT32             AddressCells;
  INT32             SizeCells;
  INT32             Length;
  INT32             MemoryNode;

  if (fdt_check_header (Fdt) != 0) {
    return FALSE;
  }

  DEBUG ((DEBUG_INFO, "Fdt Arg = 0x%x. fdt_check_header() = %d\n", Fdt, fdt_check_header ((VOID *)Fdt)));
  DEBUG ((DEBUG_INFO, "fdt_num_mem_rsv() = %d\n", fdt_num_mem_rsv ((VOID *)Fdt)));

  //
  // Look for a node called "memory" at the lowest level of the tree
  //
  MemoryNode = fdt_path_offset (Fdt, "/reserved-memory/mmode_resv0@80000000");
  if (MemoryNode <= 0) {
    return FALSE;
  }

  //
  // Retrieve the #address-cells and #size-cells properties
  // from the root node, or use the default if not provided.
  //
  AddressCells = 1;
  SizeCells    = 1;

  Prop = fdt_getprop (Fdt, 0, "#address-cells", &Length);
  if (Length == 4) {
    AddressCells = fdt32_to_cpu (*Prop);
  }

  Prop = fdt_getprop (Fdt, 0, "#size-cells", &Length);
  if (Length == 4) {
    SizeCells = fdt32_to_cpu (*Prop);
  }

  //
  // Now find the 'reg' property of the /memory node, and read the first
  // range listed.
  //
  Prop = fdt_getprop (Fdt, MemoryNode, "reg", &Length);

  if (Length < (AddressCells + SizeCells) * sizeof (INT32)) {
    return FALSE;
  }

  UINT32  Ranges;

  Ranges = Length / ((AddressCells + SizeCells) * sizeof (INT32));

  UINT32  Range;

  for (Range = 0; Range < Ranges; Range++) {
    UINT64  Address, Size;
    Address = fdt32_to_cpu (Prop[0]);
    if (AddressCells > 1) {
      Address = (Address << 32) | fdt32_to_cpu (Prop[1]);
    }

    Prop += AddressCells;
    Size  = fdt32_to_cpu (Prop[0]);
    if (SizeCells > 1) {
      Size = (Size << 32) | fdt32_to_cpu (Prop[1]);
    }

    Prop += SizeCells;

    RsvdMemoryMap.Base = Address;
    RsvdMemoryMap.Size = Size;
    RsvdMemoryMap.Type = EFI_RESOURCE_MEMORY_RESERVED;
    RsvdMemoryMap.Flag = 0;
    MemInfoCallback (&RsvdMemoryMap);
  }

  //
  // Look for a node called "memory" at the lowest level of the tree
  //
  MemoryNode = fdt_path_offset (Fdt, "/memory");
  if (MemoryNode <= 0) {
    return FALSE;
  }

  //
  // Now find the 'reg' property of the /memory node, and read the first
  // range listed.
  //
  Prop = fdt_getprop (Fdt, MemoryNode, "reg", &Length);
  if (Length < (AddressCells + SizeCells) * sizeof (INT32)) {
    return FALSE;
  }

  Ranges = Length / ((AddressCells + SizeCells) * sizeof (INT32));

  for (Range = 0; Range < Ranges; Range++) {
    UINT64  Address, Size;
    Address = fdt32_to_cpu (Prop[0]);
    if (AddressCells > 1) {
      Address = (Address << 32) | fdt32_to_cpu (Prop[1]);
    }

    Prop += AddressCells;

    Size = fdt32_to_cpu (Prop[0]);
    if (SizeCells > 1) {
      Size = (Size << 32) | fdt32_to_cpu (Prop[1]);
    }

    Prop += SizeCells;

    /* For now make an assumption that we will have one mem rsvd
          region from BL. FDT seems to create overlapping regions as in
          total mem range includes rsvd range as well. So we need to
          adjust the available mem base accordingly otherwise GCD does
          not like it */

    MemoryMap.Base = Address + RsvdMemoryMap.Size; // address;
    MemoryMap.Size = Size - RsvdMemoryMap.Size;
    MemoryMap.Type = EFI_RESOURCE_SYSTEM_MEMORY;
    MemoryMap.Flag = 0;
    MemInfoCallback (&MemoryMap);
  }

  return RETURN_SUCCESS;
}

/**
  Build Serial info HOB using infomation from FDT.

  @param  Fdt
  @param  UniversalSerialPort

  @retval  Pointer to Universal Serial Port.
**/
RETURN_STATUS
EFIAPI
BuildSerialHobFromFDT (
  IN CONST VOID                           *Fdt,
  OUT UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO  *UniversalSerialPort
  )
{
  CONST INT32  *Prop;
  INT32        Length;
  INT32        SerialNode;
  UINT32       Freq;
  UINT64       Address;

  if (fdt_check_header (Fdt) != 0) {
    return FALSE;
  }

  //
  // Look for "compatible property with value "ns16550a"
  //
  SerialNode = fdt_path_offset (Fdt, "/soc");
  if (SerialNode <= 0) {
    return FALSE;
  }

  SerialNode = fdt_node_offset_by_prop_value (Fdt, SerialNode, "compatible", "ns16550a", strlen ("ns16550a")+1);

  if (SerialNode <= 0) {
    return FALSE;
  }

  //
  // Now find the 'reg' property of the /memory node, and read the first
  // range listed.
  //
  Prop    = fdt_getprop (Fdt, SerialNode, "reg", &Length);
  Address = fdt32_to_cpu (Prop[0]);
  Address = (Address << 32) | fdt32_to_cpu (Prop[1]);

  Prop = fdt_getprop (Fdt, SerialNode, "clock-frequency", &Length);
  Freq = fdt32_to_cpu (Prop[0]);

  UniversalSerialPort->Header.Revision = UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO_REVISION;
  UniversalSerialPort->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO);
  UniversalSerialPort->UseMmio         = TRUE;
  UniversalSerialPort->RegisterBase    = Address;
  UniversalSerialPort->BaudRate        = Freq;
  UniversalSerialPort->RegisterStride  = 1;

  return TRUE;
}

/**
  This function searchs a given file type with a given Guid within a valid FV.
  If input Guid is NULL, will locate the first section having the given file type

  @param FvHeader        A pointer to firmware volume header that contains the set of files
                         to be searched.
  @param FileType        File type to be searched.
  @param Guid            Will ignore if it is NULL.
  @param FileHeader      A pointer to the discovered file, if successful.

  @retval EFI_SUCCESS    Successfully found FileType
  @retval EFI_NOT_FOUND  File type can't be found.
**/
STATIC EFI_STATUS
FvFindFileByTypeGuid (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader,
  IN  EFI_FV_FILETYPE             FileType,
  IN  EFI_GUID                    *Guid  OPTIONAL,
  OUT EFI_FFS_FILE_HEADER         **FileHeader
  )
{
  EFI_PHYSICAL_ADDRESS  CurrentAddress;
  EFI_PHYSICAL_ADDRESS  EndOfFirmwareVolume;
  EFI_FFS_FILE_HEADER   *File;
  UINT32                Size;
  EFI_PHYSICAL_ADDRESS  EndOfFile;

  CurrentAddress      = (EFI_PHYSICAL_ADDRESS)(UINTN)FvHeader;
  EndOfFirmwareVolume = CurrentAddress + FvHeader->FvLength;

  //
  // Loop through the FFS files
  //
  for (EndOfFile = CurrentAddress + FvHeader->HeaderLength; ; ) {
    CurrentAddress = (EndOfFile + 7) & 0xfffffffffffffff8ULL;
    if (CurrentAddress > EndOfFirmwareVolume) {
      break;
    }

    File = (EFI_FFS_FILE_HEADER *)(UINTN)CurrentAddress;
    if (IS_FFS_FILE2 (File)) {
      Size = FFS_FILE2_SIZE (File);
      if (Size <= 0x00FFFFFF) {
        break;
      }
    } else {
      Size = FFS_FILE_SIZE (File);
      if (Size < sizeof (EFI_FFS_FILE_HEADER)) {
        break;
      }
    }

    EndOfFile = CurrentAddress + Size;
    if (EndOfFile > EndOfFirmwareVolume) {
      break;
    }

    //
    // Look for file type
    //
    if (File->Type == FileType) {
      if ((Guid == NULL) || CompareGuid (&File->Name, Guid)) {
        *FileHeader = File;
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  This function searchs a given section type within a valid FFS file.

  @param  FileHeader            A pointer to the file header that contains the set of sections to
                                be searched.
  @param  SectionType            The value of the section type to search.
  @param  SectionData           A pointer to the discovered section, if successful.

  @retval EFI_SUCCESS           The section was found.
  @retval EFI_NOT_FOUND         The section was not found.

**/
STATIC EFI_STATUS
FileFindSection (
  IN EFI_FFS_FILE_HEADER  *FileHeader,
  IN EFI_SECTION_TYPE     SectionType,
  OUT VOID                **SectionData
  )
{
  UINT32                     FileSize;
  EFI_COMMON_SECTION_HEADER  *Section;
  UINT32                     SectionSize;
  UINT32                     Index;

  if (IS_FFS_FILE2 (FileHeader)) {
    FileSize = FFS_FILE2_SIZE (FileHeader);
  } else {
    FileSize = FFS_FILE_SIZE (FileHeader);
  }

  FileSize -= sizeof (EFI_FFS_FILE_HEADER);

  Section = (EFI_COMMON_SECTION_HEADER *)(FileHeader + 1);
  Index   = 0;
  while (Index < FileSize) {
    if (Section->Type == SectionType) {
      if (IS_SECTION2 (Section)) {
        *SectionData = (VOID *)((UINT8 *)Section + sizeof (EFI_COMMON_SECTION_HEADER2));
      } else {
        *SectionData = (VOID *)((UINT8 *)Section + sizeof (EFI_COMMON_SECTION_HEADER));
      }

      return EFI_SUCCESS;
    }

    if (IS_SECTION2 (Section)) {
      SectionSize = SECTION2_SIZE (Section);
    } else {
      SectionSize = SECTION_SIZE (Section);
    }

    SectionSize = GET_OCCUPIED_SIZE (SectionSize, 4);
    ASSERT (SectionSize != 0);
    Index += SectionSize;

    Section = (EFI_COMMON_SECTION_HEADER *)((UINT8 *)Section + SectionSize);
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
BuildBlHobs (
  IN  UINTN                       Param1,
  IN  UINTN                       Param2,
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  );

/**
  It will build HOBs based on information from bootloaders.

  @param[in]  Param1   Hard ID
  @param[in]  Param2   FDT blob pointer
  @param[out] DxeFv    The pointer to the DXE FV in memory.

  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
BuildBlHobs (
  IN  UINTN                       Param1,
  IN  UINTN                       Param2,
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  )
{
  UINTN                               MinimalNeededSize;
  EFI_PHYSICAL_ADDRESS                FreeMemoryBottom;
  EFI_PHYSICAL_ADDRESS                FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS                MemoryBottom;
  UINT8                               *GuidHob;
  UNIVERSAL_PAYLOAD_ACPI_TABLE        *AcpiTable;
  ACPI_BOARD_INFO                     *AcpiBoardInfo;
  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO  *UniversalSerialPort;
  VOID                                *NewBase;
  UINTN                               FdtSize;
  UINTN                               FdtPages;
  UINT64                              *FdtHobData;
  CONST VOID                          *Fdt;
  EFI_FFS_FILE_HEADER                 *FileHeader;
  EFI_FIRMWARE_VOLUME_HEADER          *DxeCoreFv;
  EFI_STATUS                          Status;
  EFI_RISCV_FIRMWARE_CONTEXT          FirmwareContext;

  FirmwareContext.BootHartId          = Param1;
  FirmwareContext.FlattenedDeviceTree = Param2;
  SetFirmwareContextPointer (&FirmwareContext);

  Fdt               = (VOID *)Param2;
  MinimalNeededSize = FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);

  ASSERT ((UINT8 *)Fdt != NULL);

  // HOB region is used for HOB and memory allocation for this module
  MemoryBottom     = PcdGet32 (PcdPayloadFdMemBase);
  FreeMemoryBottom = ALIGN_VALUE (MemoryBottom + PcdGet32 (PcdPayloadFdMemSize), SIZE_1MB);
  FreeMemoryTop    = FreeMemoryBottom + FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);

  HobConstructor ((VOID *)(UINTN)MemoryBottom, (VOID *)(UINTN)FreeMemoryTop, (VOID *)(UINTN)FreeMemoryBottom, (VOID *)(UINTN)FreeMemoryTop);
  //
  // Build serial port info
  //
  UniversalSerialPort = BuildGuidHob (&gUniversalPayloadSerialPortInfoGuid, sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO));
  ASSERT (UniversalSerialPort != NULL);
  BuildSerialHobFromFDT (Fdt, UniversalSerialPort);
  BuildMemHobFromFDT (Fdt);

  // Build the CPU HOB with guest RAM size dependent address width and 16-bits
  // of IO space. (Side note: unlike other HOBs, the CPU HOB is needed during
  // S3 resume as well, so we build it unconditionally.)
  //
  // TODO: Determine this dynamically from the platform
  // setting or the HART configuration.
  //
  BuildCpuHob (48, 32);

  ASSERT ((UINT8 *)Fdt != NULL);
  ASSERT (fdt_check_header (Fdt) == 0);

  FdtSize  = fdt_totalsize (Fdt);
  FdtPages = EFI_SIZE_TO_PAGES (FdtSize);
  NewBase  = AllocatePages (FdtPages);
  ASSERT (NewBase != NULL);
  fdt_open_into (Fdt, NewBase, EFI_PAGES_TO_SIZE (FdtPages));

  FdtHobData = BuildGuidHob (&gFdtHobGuid, sizeof *FdtHobData);
  ASSERT (FdtHobData != NULL);
  *FdtHobData = (UINTN)NewBase;

  //
  // Create guid hob for acpi board information
  //
  GuidHob = GetFirstGuidHob (&gUniversalPayloadAcpiTableGuid);
  if (GuidHob != NULL) {
    AcpiTable     = (UNIVERSAL_PAYLOAD_ACPI_TABLE *)GET_GUID_HOB_DATA (GuidHob);
    AcpiBoardInfo = BuildHobFromAcpi ((UINT64)AcpiTable->Rsdp);
    ASSERT (AcpiBoardInfo != NULL);
  }

  DxeCoreFv = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdPayloadFdMemBase);

  Status = FvFindFileByTypeGuid (DxeCoreFv, 0x0B, ((void *)0), &FileHeader);
  if ((((INTN)(RETURN_STATUS)(Status)) < 0)) {
    return Status;
  }

  Status = FileFindSection (FileHeader, 0x17, (void **)DxeFv);
  if ((((INTN)(RETURN_STATUS)(Status)) < 0)) {
    return Status;
  }

  BuildFvHob ((EFI_PHYSICAL_ADDRESS)(UINTN)DxeCoreFv, DxeCoreFv->FvLength);

  return EFI_SUCCESS;
}

/**
  Acquire SMBIOS table from Boot Loader.Get a pointer from BL via FDT.

  @param  SmbiosTable               Pointer to the SMBIOS table info.

  @retval RETURN_SUCCESS            Successfully find out the tables.
  @retval RETURN_NOT_FOUND          Failed to find the tables.

**/
RETURN_STATUS
EFIAPI
ParseSmbiosTable (
  OUT UNIVERSAL_PAYLOAD_SMBIOS_TABLE  *SmbiosTable
  )
{
  return RETURN_SUCCESS;
}

/**
  Acquire ACPI table from Boot loader.Get a pointer from BL via FDT

  @param  AcpiTableHob              Pointer to the ACPI table info.

  @retval RETURN_SUCCESS            Successfully find out the tables.
  @retval RETURN_NOT_FOUND          Failed to find the tables.

**/
RETURN_STATUS
EFIAPI
ParseAcpiTableInfo (
  OUT UNIVERSAL_PAYLOAD_ACPI_TABLE  *AcpiTableHob
  )
{
  return RETURN_SUCCESS;
}

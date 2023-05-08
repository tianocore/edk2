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
#include <libfdt.h>

#define E820_RAM        1
#define E820_RESERVED   2
#define E820_ACPI       3
#define E820_NVS        4
#define E820_UNUSABLE   5
#define E820_DISABLED   6
#define E820_PMEM       7
#define E820_UNDEFINED  8
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
  Build a Handoff Information Table HOB

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
  Build ACPI board info HOB using infomation from ACPI table

  @param  AcpiTableBase      ACPI table start address in memory

  @retval  A pointer to ACPI board HOB ACPI_BOARD_INFO. Null if build HOB failure.
**/
ACPI_BOARD_INFO *
BuildHobFromAcpi (
  IN   UINT64  AcpiTableBase
  );

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
  IN MEMORY_MAP_ENTRY  *MemoryMapEntry
  )
{
//  EFI_PHYSICAL_ADDRESS         Base;
//  EFI_RESOURCE_TYPE            Type;
//  UINT64                       Size;
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

RETURN_STATUS
EFIAPI
BuildMemHobFromFDT (
  const void *Fdt
  )
{
  UINTN                   Index;
  MEMORY_MAP_ENTRY        MemoryMap;
  MEMORY_MAP_ENTRY        RsvdMemoryMap;
  CONST INT32  *Prop;
  INT32        AddressCells;
  INT32        SizeCells;
  INT32        Length;
  INT32        MemoryNode;

  if (fdt_check_header (Fdt) != 0) {
    return FALSE;
  }

  DEBUG ((DEBUG_INFO, "Fdt Arg = 0x%x. fdt_check_header() = %d\n", Fdt, fdt_check_header((void *)Fdt)));
  DEBUG ((DEBUG_INFO, "fdt_num_mem_rsv() = %d\n",   fdt_num_mem_rsv((void *)Fdt)));

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

  int ranges = Length / ((AddressCells + SizeCells) * sizeof(INT32));

  for (int range = 0; range < ranges; range++) {

    UINT64 address, size;
    address = fdt32_to_cpu (Prop[0]);
    if (AddressCells > 1) {
      address = (address << 32) | fdt32_to_cpu (Prop[1]);
    }

    Prop += AddressCells;
    size = fdt32_to_cpu (Prop[0]);
    if (SizeCells > 1) {
      size = (size << 32) | fdt32_to_cpu (Prop[1]);
    }

    DEBUG ((
      DEBUG_INFO,
      "%d. %016lx - %016lx\n",
      Index++,
      address + size,
      size
      ));
    Prop += SizeCells;

    RsvdMemoryMap.Base = address;
    RsvdMemoryMap.Size = size;
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

  ranges = Length / ((AddressCells + SizeCells) * sizeof(INT32));

  for (int range = 0; range < ranges; range++) {

    UINT64 address, size;
    address = fdt32_to_cpu (Prop[0]);
    if (AddressCells > 1) {
      address = (address << 32) | fdt32_to_cpu (Prop[1]);
    }

    Prop += AddressCells;

    size = fdt32_to_cpu (Prop[0]);
    if (SizeCells > 1) {
      size = (size << 32) | fdt32_to_cpu (Prop[1]);
    }
    DEBUG ((
      DEBUG_INFO,
      "%d. %016lx - %016lx\n",
      Index++,
      address + size,
      size
      ));
    Prop += SizeCells;

/* For now make an assumption that we will have one mem rsvd
   region from BL. FDT seems to create overlapping regions as in
   total mem range includes rsvd range as well. So we need to
   adjust the available mem base accordingly otherwise GCD does
   not like it */

    MemoryMap.Base = address + RsvdMemoryMap.Size; //address;
    MemoryMap.Size = size - RsvdMemoryMap.Size;
    MemoryMap.Type = EFI_RESOURCE_SYSTEM_MEMORY;
    MemoryMap.Flag = 0;
    MemInfoCallback (&MemoryMap);
  }

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
BuildSerialHobFromFDT (
  const void *Fdt,
  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO  *UniversalSerialPort
  )
{
  CONST INT32  *Prop;
  INT32        Length;
  INT32        SerialNode;
  UINT32       freq;
  UINT64       address;

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

  SerialNode = fdt_node_offset_by_prop_value(Fdt, SerialNode, "compatible", "ns16550a", strlen("ns16550a")+1);

  if (SerialNode <= 0) {
    return FALSE;
  }
  //
  // Now find the 'reg' property of the /memory node, and read the first
  // range listed.
  //
  Prop = fdt_getprop (Fdt, SerialNode, "reg", &Length);
  address = fdt32_to_cpu (Prop[0]);
  address = (address << 32) | fdt32_to_cpu (Prop[1]);

  Prop = fdt_getprop (Fdt, SerialNode, "clock-frequency", &Length);
  freq = fdt32_to_cpu (Prop[0]);

  UniversalSerialPort->Header.Revision = UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO_REVISION;
  UniversalSerialPort->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO);
  UniversalSerialPort->UseMmio         = TRUE;
  UniversalSerialPort->RegisterBase    = address;
  UniversalSerialPort->BaudRate        = freq;
  UniversalSerialPort->RegisterStride  = 1;

  return TRUE;
}

EFI_STATUS
BuildBlHobs (
  IN  UINTN              Param1,
  IN  UINTN              Param2,
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  );
/**
  It will build HOBs based on information from bootloaders.

  @param[in]  fdt   The starting memory address of bootloader parameter block.
  @param[out] DxeFv                 The pointer to the DXE FV in memory.

  @retval EFI_SUCCESS        If it completed successfully.
  @retval Others             If it failed to build required HOBs.
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

  const void* fdt = (void*) Param2;
  MinimalNeededSize = FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);

  ASSERT ((UINT8 *)fdt != NULL);

  // HOB region is used for HOB and memory allocation for this module
  MemoryBottom    = PcdGet32 (PcdPayloadFdMemBase);
  FreeMemoryBottom = ALIGN_VALUE (MemoryBottom + PcdGet32 (PcdPayloadFdMemSize), SIZE_1MB);
  FreeMemoryTop  = FreeMemoryBottom + FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);

  HobConstructor ((VOID *)(UINTN)MemoryBottom, (VOID *)(UINTN)FreeMemoryTop, (VOID *)(UINTN)FreeMemoryBottom, (VOID *)(UINTN)FreeMemoryTop);
  //
  // Build serial port info
  //
  UniversalSerialPort = BuildGuidHob (&gUniversalPayloadSerialPortInfoGuid, sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO));
  ASSERT (UniversalSerialPort != NULL);
  BuildSerialHobFromFDT(fdt,UniversalSerialPort);

  BuildMemHobFromFDT(fdt);
  // Build the CPU HOB with guest RAM size dependent address width and 16-bits
  // of IO space. (Side note: unlike other HOBs, the CPU HOB is needed during
  // S3 resume as well, so we build it unconditionally.)
  //
  // TODO: Determine this dynamically from the platform
  // setting or the HART configuration.
  //
  BuildCpuHob (48, 32);

  ASSERT ((UINT8 *)fdt != NULL);
  ASSERT (fdt_check_header (fdt) == 0);

  FdtSize  = fdt_totalsize (fdt);
  FdtPages = EFI_SIZE_TO_PAGES (FdtSize);
  NewBase  = AllocatePages (FdtPages);
  ASSERT (NewBase != NULL);
  fdt_open_into (fdt, NewBase, EFI_PAGES_TO_SIZE (FdtPages));

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

  *DxeFv = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdPayloadFdMemBase);
  return EFI_SUCCESS;
}

/**
  Acquire SMBIOS table from Boot Loader.Get a pointer from BL via FDT

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

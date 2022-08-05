/** @file

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SHIMLAYER_H__
#define __SHIMLAYER_H__

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
#include <Library/BlParseLib.h>
#include <Library/PlatformSupportLib.h>
#include <Library/UefiCpuLib.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include <Guid/SerialPortInfoGuid.h>
#include <Guid/MemoryMapInfoGuid.h>
#include <Guid/AcpiBoardInfoGuid.h>
#include <Guid/GraphicsInfoHob.h>
#include <UniversalPayload/SmbiosTable.h>
#include <UniversalPayload/AcpiTable.h>
#include <UniversalPayload/UniversalPayload.h>
#include <UniversalPayload/ExtraData.h>
#include <UniversalPayload/SerialPortInfo.h>
#include <Guid/PcdDataBaseSignatureGuid.h>
#include <Coreboot.h>
#include <Library/ElfLoaderLib.h>
#include <Library/PciHostBridgeLib.h>
#include <UniversalPayload/PciRootBridges.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include "HobIioUds.h"

#define SHIMLAYER_SIZE                    SIZE_1MB
#define SHIMLAYER_REGION                  SIZE_16MB
#define LEGACY_8259_MASK_REGISTER_MASTER  0x21
#define LEGACY_8259_MASK_REGISTER_SLAVE   0xA1
#define GET_OCCUPIED_SIZE(ActualSize, Alignment) \
  ((ActualSize) + (((Alignment) - ((ActualSize) & ((Alignment) - 1))) & ((Alignment) - 1)))
#define ALIGN_UP(x, a)  (((x)+(a - 1))&~(a-1))
#define SWAP32(x) \
  ((unsigned int)( \
    (((unsigned int)(x) & 0x000000ffUL) << 24) | \
    (((unsigned int)(x) & 0x0000ff00UL) <<  8) | \
    (((unsigned int)(x) & 0x00ff0000UL) >>  8) | \
    (((unsigned int)(x) & 0xff000000UL) >> 24)))

#define E820_RAM        1
#define E820_RESERVED   2
#define E820_ACPI       3
#define E820_NVS        4
#define E820_UNUSABLE   5
#define E820_DISABLED   6
#define E820_PMEM       7
#define E820_UNDEFINED  8

RETURN_STATUS
EFIAPI
LzmaUefiDecompressGetInfo (
  IN  CONST VOID  *Source,
  IN  UINT32      SourceSize,
  OUT UINT32      *DestinationSize,
  OUT UINT32      *ScratchSize
  );

VOID *
EFIAPI
CreateHob (
  IN  UINT16  HobType,
  IN  UINT16  HobLength
  );

/**
  Update the Stack Hob if the stack has been moved

  @param  BaseAddress   The 64 bit physical address of the Stack.
  @param  Length        The length of the stack in bytes.

**/
VOID
EFIAPI
UpdateStackHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
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
  This function searchs a given section type within a valid FFS file.

  @param  FileHeader            A pointer to the file header that contains the set of sections to
                                be searched.
  @param  SearchType            The value of the section type to search.
  @param  SectionData           A pointer to the discovered section, if successful.

  @retval EFI_SUCCESS           The section was found.
  @retval EFI_NOT_FOUND         The section was not found.

**/
EFI_STATUS
FileFindSection (
  IN EFI_FFS_FILE_HEADER  *FileHeader,
  IN EFI_SECTION_TYPE     SectionType,
  OUT VOID                **SectionData
  );

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
EFI_STATUS
FvFindFileByTypeGuid (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader,
  IN  EFI_FV_FILETYPE             FileType,
  IN  EFI_GUID                    *Guid           OPTIONAL,
  OUT EFI_FFS_FILE_HEADER         **FileHeader
  );

RETURN_STATUS
EFIAPI
LzmaUefiDecompress (
  IN CONST VOID  *Source,
  IN UINTN       SourceSize,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch
  );

/**
  Auto-generated function that calls the library constructors for all of the module's
  dependent libraries.  This function must be called by the SEC Core once a stack has
  been established.

**/
VOID
EFIAPI
ProcessLibraryConstructorList (
  VOID
  );

/**
  Find coreboot record with given Tag.

  @param  Tag                The tag id to be found

  @retval NULL              The Tag is not found.
  @retval Others            The pointer to the record found.

**/
VOID *
FindCbTag (
  IN  UINT32  Tag
  );

/**
  Find the given table with TableId from the given coreboot memory Root.

  @param  Root               The coreboot memory table to be searched in
  @param  TableId            Table id to be found
  @param  MemTable           To save the base address of the memory table found
  @param  MemTableSize       To save the size of memory table found

  @retval RETURN_SUCCESS            Successfully find out the memory table.
  @retval RETURN_INVALID_PARAMETER  Invalid input parameters.
  @retval RETURN_NOT_FOUND          Failed to find the memory table.

**/
RETURN_STATUS
FindCbMemTable (
  IN  struct cbmem_root  *Root,
  IN  UINT32             TableId,
  OUT VOID               **MemTable,
  OUT UINT32             *MemTableSize
  );

/**
  Convert a packed value from cbuint64 to a UINT64 value.

  @param  val      The pointer to packed data.

  @return          the UNIT64 value after conversion.

**/
UINT64
cb_unpack64 (
  IN struct cbuint64  val
  );

#endif

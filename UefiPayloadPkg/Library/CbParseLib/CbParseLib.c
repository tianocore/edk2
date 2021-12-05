/** @file
  This library will parse the coreboot table in memory and extract those required
  information.

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/BlParseLib.h>
#include <IndustryStandard/Acpi.h>
#include <Coreboot.h>

/**
  Convert a packed value from cbuint64 to a UINT64 value.

  @param  val      The pointer to packed data.

  @return          the UNIT64 value after conversion.

**/
UINT64
cb_unpack64 (
  IN struct cbuint64  val
  )
{
  return LShiftU64 (val.hi, 32) | val.lo;
}

/**
  Returns the sum of all elements in a buffer of 16-bit values.  During
  calculation, the carry bits are also been added.

  @param  Buffer      The pointer to the buffer to carry out the sum operation.
  @param  Length      The size, in bytes, of Buffer.

  @return Sum         The sum of Buffer with carry bits included during additions.

**/
UINT16
CbCheckSum16 (
  IN UINT16  *Buffer,
  IN UINTN   Length
  )
{
  UINT32  Sum;
  UINT32  TmpValue;
  UINTN   Idx;
  UINT8   *TmpPtr;

  Sum    = 0;
  TmpPtr = (UINT8 *)Buffer;
  for (Idx = 0; Idx < Length; Idx++) {
    TmpValue = TmpPtr[Idx];
    if (Idx % 2 == 1) {
      TmpValue <<= 8;
    }

    Sum += TmpValue;

    // Wrap
    if (Sum >= 0x10000) {
      Sum = (Sum + (Sum >> 16)) & 0xFFFF;
    }
  }

  return (UINT16)((~Sum) & 0xFFFF);
}

/**
  Check the coreboot table if it is valid.

  @param  Header            Pointer to coreboot table

  @retval TRUE              The coreboot table is valid.
  @retval Others            The coreboot table is not valid.

**/
BOOLEAN
IsValidCbTable (
  IN struct cb_header  *Header
  )
{
  UINT16  CheckSum;

  if ((Header == NULL) || (Header->table_bytes == 0)) {
    return FALSE;
  }

  if (Header->signature != CB_HEADER_SIGNATURE) {
    return FALSE;
  }

  //
  // Check the checksum of the coreboot table header
  //
  CheckSum = CbCheckSum16 ((UINT16 *)Header, sizeof (*Header));
  if (CheckSum != 0) {
    DEBUG ((DEBUG_ERROR, "Invalid coreboot table header checksum\n"));
    return FALSE;
  }

  CheckSum = CbCheckSum16 ((UINT16 *)((UINT8 *)Header + sizeof (*Header)), Header->table_bytes);
  if (CheckSum != Header->table_checksum) {
    DEBUG ((DEBUG_ERROR, "Incorrect checksum of all the coreboot table entries\n"));
    return FALSE;
  }

  return TRUE;
}

/**
  This function retrieves the parameter base address from boot loader.

  This function will get bootloader specific parameter address for UEFI payload.
  e.g. HobList pointer for Slim Bootloader, and coreboot table header for Coreboot.

  @retval NULL            Failed to find the GUID HOB.
  @retval others          GUIDed HOB data pointer.

**/
VOID *
EFIAPI
GetParameterBase (
  VOID
  )
{
  struct cb_header  *Header;
  struct cb_record  *Record;
  UINT8             *TmpPtr;
  UINT8             *CbTablePtr;
  UINTN             Idx;
  EFI_STATUS        Status;

  //
  // coreboot could pass coreboot table to UEFI payload
  //
  Header = (struct cb_header *)(UINTN)GET_BOOTLOADER_PARAMETER ();
  if (IsValidCbTable (Header)) {
    return Header;
  }

  //
  // Find simplified coreboot table in memory range 0 ~ 4KB.
  // Some GCC version does not allow directly access to NULL pointer,
  // so start the search from 0x10 instead.
  //
  for (Idx = 16; Idx < 4096; Idx += 16) {
    Header = (struct cb_header *)Idx;
    if (Header->signature == CB_HEADER_SIGNATURE) {
      break;
    }
  }

  if (Idx >= 4096) {
    return NULL;
  }

  //
  // Check the coreboot header
  //
  if (!IsValidCbTable (Header)) {
    return NULL;
  }

  //
  // Find full coreboot table in high memory
  //
  CbTablePtr = NULL;
  TmpPtr     = (UINT8 *)Header + Header->header_bytes;
  for (Idx = 0; Idx < Header->table_entries; Idx++) {
    Record = (struct cb_record *)TmpPtr;
    if (Record->tag == CB_TAG_FORWARD) {
      CbTablePtr = (VOID *)(UINTN)((struct cb_forward *)(UINTN)Record)->forward;
      break;
    }

    TmpPtr += Record->size;
  }

  //
  // Check the coreboot header in high memory
  //
  if (!IsValidCbTable ((struct cb_header *)CbTablePtr)) {
    return NULL;
  }

  Status = PcdSet64S (PcdBootloaderParameter, (UINTN)CbTablePtr);
  ASSERT_EFI_ERROR (Status);

  return CbTablePtr;
}

/**
  Find coreboot record with given Tag.

  @param  Tag                The tag id to be found

  @retval NULL              The Tag is not found.
  @retval Others            The pointer to the record found.

**/
VOID *
FindCbTag (
  IN  UINT32  Tag
  )
{
  struct cb_header  *Header;
  struct cb_record  *Record;
  UINT8             *TmpPtr;
  UINT8             *TagPtr;
  UINTN             Idx;

  Header = (struct cb_header *)GetParameterBase ();

  TagPtr = NULL;
  TmpPtr = (UINT8 *)Header + Header->header_bytes;
  for (Idx = 0; Idx < Header->table_entries; Idx++) {
    Record = (struct cb_record *)TmpPtr;
    if (Record->tag == Tag) {
      TagPtr = TmpPtr;
      break;
    }

    TmpPtr += Record->size;
  }

  return TagPtr;
}

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
  )
{
  UINTN               Idx;
  BOOLEAN             IsImdEntry;
  struct cbmem_entry  *Entries;

  if ((Root == NULL) || (MemTable == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Check if the entry is CBMEM or IMD
  // and handle them separately
  //
  Entries = Root->entries;
  if (Entries[0].magic == CBMEM_ENTRY_MAGIC) {
    IsImdEntry = FALSE;
  } else {
    Entries = (struct cbmem_entry *)((struct imd_root *)Root)->entries;
    if (Entries[0].magic == IMD_ENTRY_MAGIC) {
      IsImdEntry = TRUE;
    } else {
      return RETURN_NOT_FOUND;
    }
  }

  for (Idx = 0; Idx < Root->num_entries; Idx++) {
    if (Entries[Idx].id == TableId) {
      if (IsImdEntry) {
        *MemTable = (VOID *)((UINTN)Entries[Idx].start + (UINTN)Root);
      } else {
        *MemTable = (VOID *)(UINTN)Entries[Idx].start;
      }

      if (MemTableSize != NULL) {
        *MemTableSize = Entries[Idx].size;
      }

      DEBUG ((
        DEBUG_INFO,
        "Find CbMemTable Id 0x%x, base %p, size 0x%x\n",
        TableId,
        *MemTable,
        Entries[Idx].size
        ));
      return RETURN_SUCCESS;
    }
  }

  return RETURN_NOT_FOUND;
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
ParseCbMemTable (
  IN  UINT32  TableId,
  OUT VOID    **MemTable,
  OUT UINT32  *MemTableSize
  )
{
  EFI_STATUS              Status;
  CB_MEMORY               *Rec;
  struct cb_memory_range  *Range;
  UINT64                  Start;
  UINT64                  Size;
  UINTN                   Index;
  struct cbmem_root       *CbMemRoot;

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
      CbMemRoot = (struct  cbmem_root *)(UINTN)(Start + Size - DYN_CBMEM_ALIGN_SIZE);
      Status    = FindCbMemTable (CbMemRoot, TableId, MemTable, MemTableSize);
      if (!EFI_ERROR (Status)) {
        break;
      }
    }
  }

  return Status;
}

/**
  Acquire the memory information from the coreboot table in memory.

  @param  MemInfoCallback     The callback routine
  @param  Params              Pointer to the callback routine parameter

  @retval RETURN_SUCCESS     Successfully find out the memory information.
  @retval RETURN_NOT_FOUND   Failed to find the memory information.

**/
RETURN_STATUS
EFIAPI
ParseMemoryInfo (
  IN  BL_MEM_INFO_CALLBACK  MemInfoCallback,
  IN  VOID                  *Params
  )
{
  CB_MEMORY               *Rec;
  struct cb_memory_range  *Range;
  UINTN                   Index;
  MEMORY_MAP_ENTRY        MemoryMap;

  //
  // Get the coreboot memory table
  //
  Rec = (CB_MEMORY *)FindCbTag (CB_TAG_MEMORY);
  if (Rec == NULL) {
    return RETURN_NOT_FOUND;
  }

  for (Index = 0; Index < MEM_RANGE_COUNT (Rec); Index++) {
    Range          = MEM_RANGE_PTR (Rec, Index);
    MemoryMap.Base = cb_unpack64 (Range->start);
    MemoryMap.Size = cb_unpack64 (Range->size);
    MemoryMap.Type = (UINT8)Range->type;
    MemoryMap.Flag = 0;
    DEBUG ((
      DEBUG_INFO,
      "%d. %016lx - %016lx [%02x]\n",
      Index,
      MemoryMap.Base,
      MemoryMap.Base + MemoryMap.Size - 1,
      MemoryMap.Type
      ));

    MemInfoCallback (&MemoryMap, Params);
  }

  return RETURN_SUCCESS;
}

/**
  Acquire SMBIOS table from coreboot.

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
  EFI_STATUS  Status;
  VOID        *MemTable;
  UINT32      MemTableSize;

  Status = ParseCbMemTable (SIGNATURE_32 ('T', 'B', 'M', 'S'), &MemTable, &MemTableSize);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  SmbiosTable->SmBiosEntryPoint = (UINT64)(UINTN)MemTable;

  return RETURN_SUCCESS;
}

/**
  Acquire ACPI table from coreboot.

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
  EFI_STATUS  Status;
  VOID        *MemTable;
  UINT32      MemTableSize;

  Status = ParseCbMemTable (SIGNATURE_32 ('I', 'P', 'C', 'A'), &MemTable, &MemTableSize);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  AcpiTableHob->Rsdp = (UINT64)(UINTN)MemTable;

  return RETURN_SUCCESS;
}

/**
  Find the serial port information

  @param  SerialPortInfo     Pointer to serial port info structure

  @retval RETURN_SUCCESS     Successfully find the serial port information.
  @retval RETURN_NOT_FOUND   Failed to find the serial port information .

**/
RETURN_STATUS
EFIAPI
ParseSerialInfo (
  OUT SERIAL_PORT_INFO  *SerialPortInfo
  )
{
  struct cb_serial  *CbSerial;

  CbSerial = FindCbTag (CB_TAG_SERIAL);
  if (CbSerial == NULL) {
    return RETURN_NOT_FOUND;
  }

  SerialPortInfo->BaseAddr    = CbSerial->baseaddr;
  SerialPortInfo->RegWidth    = CbSerial->regwidth;
  SerialPortInfo->Type        = CbSerial->type;
  SerialPortInfo->Baud        = CbSerial->baud;
  SerialPortInfo->InputHertz  = CbSerial->input_hertz;
  SerialPortInfo->UartPciAddr = CbSerial->uart_pci_addr;

  return RETURN_SUCCESS;
}

/**
  Find the video frame buffer information

  @param  GfxInfo             Pointer to the EFI_PEI_GRAPHICS_INFO_HOB structure

  @retval RETURN_SUCCESS     Successfully find the video frame buffer information.
  @retval RETURN_NOT_FOUND   Failed to find the video frame buffer information .

**/
RETURN_STATUS
EFIAPI
ParseGfxInfo (
  OUT EFI_PEI_GRAPHICS_INFO_HOB  *GfxInfo
  )
{
  struct cb_framebuffer                 *CbFbRec;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *GfxMode;

  if (GfxInfo == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  CbFbRec = FindCbTag (CB_TAG_FRAMEBUFFER);
  if (CbFbRec == NULL) {
    return RETURN_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "Found coreboot video frame buffer information\n"));
  DEBUG ((DEBUG_INFO, "physical_address: 0x%lx\n", CbFbRec->physical_address));
  DEBUG ((DEBUG_INFO, "x_resolution: 0x%x\n", CbFbRec->x_resolution));
  DEBUG ((DEBUG_INFO, "y_resolution: 0x%x\n", CbFbRec->y_resolution));
  DEBUG ((DEBUG_INFO, "bits_per_pixel: 0x%x\n", CbFbRec->bits_per_pixel));
  DEBUG ((DEBUG_INFO, "bytes_per_line: 0x%x\n", CbFbRec->bytes_per_line));

  DEBUG ((DEBUG_INFO, "red_mask_size: 0x%x\n", CbFbRec->red_mask_size));
  DEBUG ((DEBUG_INFO, "red_mask_pos: 0x%x\n", CbFbRec->red_mask_pos));
  DEBUG ((DEBUG_INFO, "green_mask_size: 0x%x\n", CbFbRec->green_mask_size));
  DEBUG ((DEBUG_INFO, "green_mask_pos: 0x%x\n", CbFbRec->green_mask_pos));
  DEBUG ((DEBUG_INFO, "blue_mask_size: 0x%x\n", CbFbRec->blue_mask_size));
  DEBUG ((DEBUG_INFO, "blue_mask_pos: 0x%x\n", CbFbRec->blue_mask_pos));
  DEBUG ((DEBUG_INFO, "reserved_mask_size: 0x%x\n", CbFbRec->reserved_mask_size));
  DEBUG ((DEBUG_INFO, "reserved_mask_pos: 0x%x\n", CbFbRec->reserved_mask_pos));

  GfxMode                       = &GfxInfo->GraphicsMode;
  GfxMode->Version              = 0;
  GfxMode->HorizontalResolution = CbFbRec->x_resolution;
  GfxMode->VerticalResolution   = CbFbRec->y_resolution;
  GfxMode->PixelsPerScanLine    = (CbFbRec->bytes_per_line << 3) / CbFbRec->bits_per_pixel;
  if ((CbFbRec->red_mask_pos == 0) && (CbFbRec->green_mask_pos == 8) && (CbFbRec->blue_mask_pos == 16)) {
    GfxMode->PixelFormat = PixelRedGreenBlueReserved8BitPerColor;
  } else if ((CbFbRec->blue_mask_pos == 0) && (CbFbRec->green_mask_pos == 8) && (CbFbRec->red_mask_pos == 16)) {
    GfxMode->PixelFormat = PixelBlueGreenRedReserved8BitPerColor;
  }

  GfxMode->PixelInformation.RedMask      = ((1 << CbFbRec->red_mask_size)      - 1) << CbFbRec->red_mask_pos;
  GfxMode->PixelInformation.GreenMask    = ((1 << CbFbRec->green_mask_size)    - 1) << CbFbRec->green_mask_pos;
  GfxMode->PixelInformation.BlueMask     = ((1 << CbFbRec->blue_mask_size)     - 1) << CbFbRec->blue_mask_pos;
  GfxMode->PixelInformation.ReservedMask = ((1 << CbFbRec->reserved_mask_size) - 1) << CbFbRec->reserved_mask_pos;

  GfxInfo->FrameBufferBase = CbFbRec->physical_address;
  GfxInfo->FrameBufferSize = CbFbRec->bytes_per_line *  CbFbRec->y_resolution;

  return RETURN_SUCCESS;
}

/**
  Find the video frame buffer device information

  @param  GfxDeviceInfo      Pointer to the EFI_PEI_GRAPHICS_DEVICE_INFO_HOB structure

  @retval RETURN_SUCCESS     Successfully find the video frame buffer information.
  @retval RETURN_NOT_FOUND   Failed to find the video frame buffer information.

**/
RETURN_STATUS
EFIAPI
ParseGfxDeviceInfo (
  OUT EFI_PEI_GRAPHICS_DEVICE_INFO_HOB  *GfxDeviceInfo
  )
{
  return RETURN_NOT_FOUND;
}

/**
  Parse and handle the misc info provided by bootloader

  @retval RETURN_SUCCESS           The misc information was parsed successfully.
  @retval RETURN_NOT_FOUND         Could not find required misc info.
  @retval RETURN_OUT_OF_RESOURCES  Insufficant memory space.

**/
RETURN_STATUS
EFIAPI
ParseMiscInfo (
  VOID
  )
{
  return RETURN_SUCCESS;
}

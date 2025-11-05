/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  FwBlockService.h

Abstract:

  Firmware volume block driver for Intel Firmware Hub (FWH) device

--*/

#ifndef _FW_BLOCK_SERVICE_H
#define _FW_BLOCK_SERVICE_H

//
// Fvb Protocol instance data
//
#define FVB_DEVICE_FROM_THIS(a)  CR (a, EFI_FW_VOL_BLOCK_DEVICE, FwVolBlockInstance, FVB_DEVICE_SIGNATURE)
#define FVB_DEVICE_SIGNATURE  SIGNATURE_32 ('F', 'V', 'B', 'N')

#pragma pack (1)

typedef struct {
  EFI_FIRMWARE_VOLUME_HEADER    FvHdr;
  EFI_FV_BLOCK_MAP_ENTRY        EndBlockMap;
  VARIABLE_STORE_HEADER         VarHdr;
} FVB_FV_HDR_AND_VARS_TEMPLATE;

typedef struct {
  MEMMAP_DEVICE_PATH          MemMapDevPath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevPath;
} FV_DEVICE_PATH;

#pragma pack ()

typedef struct {
  UINTN                                 Signature;
  FV_DEVICE_PATH                        DevicePath;
  VOID                                  *BufferPtr;
  UINTN                                 BlockSize;
  UINTN                                 Size;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    FwVolBlockInstance;
} EFI_FW_VOL_BLOCK_DEVICE;

//
// Constants
//
#define EMU_FVB_BLOCK_SIZE \
  EFI_PAGE_SIZE
#define EMU_FVB_NUM_SPARE_BLOCKS \
  EFI_SIZE_TO_PAGES ((UINTN)FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize))
#define EMU_FVB_NUM_TOTAL_BLOCKS \
  (2 * EMU_FVB_NUM_SPARE_BLOCKS)
#define EMU_FVB_SIZE \
  (EMU_FVB_NUM_TOTAL_BLOCKS * EMU_FVB_BLOCK_SIZE)
#define FTW_WRITE_QUEUE_SIZE \
  (FixedPcdGet32 (PcdFlashNvStorageFtwWorkingSize) - \
   sizeof (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER))
#define EMU_FV_HEADER_LENGTH  OFFSET_OF (FVB_FV_HDR_AND_VARS_TEMPLATE, VarHdr)

#define NOT_ERASED_BIT  0
#define ERASED_BIT      1
#define ERASED_UINT8    0xff
#define ERASED_UINT32   0xffffffff

//
// Protocol APIs
//
EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT       EFI_FVB_ATTRIBUTES_2                *Attributes
  )
;

EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN OUT    EFI_FVB_ATTRIBUTES_2                *Attributes
  )
;

EFI_STATUS
EFIAPI
FvbProtocolGetPhysicalAddress (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT       EFI_PHYSICAL_ADDRESS                *Address
  )
;

EFI_STATUS
EFIAPI
FvbProtocolGetBlockSize (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN        EFI_LBA                             Lba,
  OUT       UINTN                               *BlockSize,
  OUT       UINTN                               *NumberOfBlocks
  )
;

EFI_STATUS
EFIAPI
FvbProtocolRead (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN        EFI_LBA                             Lba,
  IN        UINTN                               Offset,
  IN OUT    UINTN                               *NumBytes,
  IN OUT    UINT8                               *Buffer
  )
;

EFI_STATUS
EFIAPI
FvbProtocolWrite (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN        EFI_LBA                             Lba,
  IN        UINTN                               Offset,
  IN OUT    UINTN                               *NumBytes,
  IN        UINT8                               *Buffer
  )
;

EFI_STATUS
EFIAPI
FvbProtocolEraseBlocks (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  ...
  )
;

#endif

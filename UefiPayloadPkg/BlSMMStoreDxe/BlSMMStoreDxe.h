/** @file  BlSMMStoreDxe.h

  Copyright (c) 2020, 9elements Agency GmbH<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __COREBOOT_SMM_STORE_DXE_H__
#define __COREBOOT_SMM_STORE_DXE_H__


#include <Base.h>
#include <PiDxe.h>

#include <Guid/EventGroup.h>

#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeLib.h>

#define SMMSTORE_SIGNATURE                       SIGNATURE_32('S', 'M', 'M', 'S')
#define INSTANCE_FROM_FVB_THIS(a)                CR(a, SMMSTORE_INSTANCE, FvbProtocol, SMMSTORE_SIGNATURE)

typedef struct _SMMSTORE_INSTANCE                SMMSTORE_INSTANCE;

#pragma pack (1)
typedef struct {
  VENDOR_DEVICE_PATH                  Vendor;
  UINT8                               Index;
  EFI_DEVICE_PATH_PROTOCOL            End;
} NOR_FLASH_DEVICE_PATH;
#pragma pack ()

struct _SMMSTORE_INSTANCE {
  UINT32                              Signature;
  EFI_HANDLE                          Handle;
  EFI_BLOCK_IO_MEDIA                  Media;

  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL FvbProtocol;

  NOR_FLASH_DEVICE_PATH               DevicePath;
};

//
// BlSMMStoreFvbDxe.c
//

EFI_STATUS
EFIAPI
SMMStoreFvbInitialize (
  IN SMMSTORE_INSTANCE*                            Instance
  );

EFI_STATUS
EFIAPI
FvbGetAttributes(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL     *This,
  OUT       EFI_FVB_ATTRIBUTES_2                    *Attributes
  );

EFI_STATUS
EFIAPI
FvbSetAttributes(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL     *This,
  IN OUT    EFI_FVB_ATTRIBUTES_2                    *Attributes
  );

EFI_STATUS
EFIAPI
FvbGetPhysicalAddress(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL     *This,
  OUT       EFI_PHYSICAL_ADDRESS                    *Address
  );

EFI_STATUS
EFIAPI
FvbGetBlockSize(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL     *This,
  IN        EFI_LBA                                 Lba,
  OUT       UINTN                                   *BlockSize,
  OUT       UINTN                                   *NumberOfBlocks
  );

EFI_STATUS
EFIAPI
FvbRead(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL     *This,
  IN        EFI_LBA                                 Lba,
  IN        UINTN                                   Offset,
  IN OUT    UINTN                                   *NumBytes,
  IN OUT    UINT8                                   *Buffer
  );

EFI_STATUS
EFIAPI
FvbWrite(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL     *This,
  IN        EFI_LBA                                 Lba,
  IN        UINTN                                   Offset,
  IN OUT    UINTN                                   *NumBytes,
  IN        UINT8                                   *Buffer
  );

EFI_STATUS
EFIAPI
FvbEraseBlocks(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL     *This,
  ...
  );


#endif /* __COREBOOT_SMM_STORE_DXE_H__ */

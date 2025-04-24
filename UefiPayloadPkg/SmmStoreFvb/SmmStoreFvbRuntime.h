/** @file  SmmStoreFvbRuntime.h

  Copyright (c) 2022, 9elements GmbH<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMM_STORE_DXE_H_
#define SMM_STORE_DXE_H_

#include <Base.h>
#include <PiDxe.h>

#include <Guid/EventGroup.h>

#include <Protocol/FirmwareVolumeBlock.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeLib.h>

#define SMMSTORE_SIGNATURE  SIGNATURE_32('S', 'M', 'M', 'S')
#define INSTANCE_FROM_FVB_THIS(a)  CR(a, SMMSTORE_INSTANCE, FvbProtocol, SMMSTORE_SIGNATURE)

typedef struct _SMMSTORE_INSTANCE SMMSTORE_INSTANCE;

typedef struct {
  MEMMAP_DEVICE_PATH          MemMapDevPath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevPath;
} FV_MEMMAP_DEVICE_PATH;

struct _SMMSTORE_INSTANCE {
  UINT32                                 Signature;
  EFI_HANDLE                             Handle;
  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL    FvbProtocol;
  UINTN                                  BlockSize;
  UINTN                                  LastBlock;
  EFI_PHYSICAL_ADDRESS                   MmioAddress;
  FV_MEMMAP_DEVICE_PATH                  DevicePath;
};

//
// SmmStoreFvbRuntimeDxe.c
//

EFI_STATUS
EFIAPI
FvbInitialize (
  IN SMMSTORE_INSTANCE  *Instance
  );

EFI_STATUS
EFIAPI
FvbGetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT       EFI_FVB_ATTRIBUTES_2                 *Attributes
  );

EFI_STATUS
EFIAPI
FvbSetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN OUT    EFI_FVB_ATTRIBUTES_2                 *Attributes
  );

EFI_STATUS
EFIAPI
FvbGetPhysicalAddress (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT       EFI_PHYSICAL_ADDRESS                 *Address
  );

EFI_STATUS
EFIAPI
FvbGetBlockSize (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  OUT       UINTN                                *BlockSize,
  OUT       UINTN                                *NumberOfBlocks
  );

EFI_STATUS
EFIAPI
FvbRead (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN OUT    UINTN                                *NumBytes,
  IN OUT    UINT8                                *Buffer
  );

EFI_STATUS
EFIAPI
FvbWrite (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN OUT    UINTN                                *NumBytes,
  IN        UINT8                                *Buffer
  );

EFI_STATUS
EFIAPI
FvbEraseBlocks (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  ...
  );

#endif // SMM_STORE_DXE_H_

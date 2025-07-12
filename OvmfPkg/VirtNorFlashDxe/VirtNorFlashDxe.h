/** @file  VirtNorFlashDxe.h

  Copyright (c) 2011 - 2021, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __VIRT_NOR_FLASH_DXE__
#define __VIRT_NOR_FLASH_DXE__

#include <Base.h>
#include <PiDxe.h>

#include <Guid/EventGroup.h>

#include <Protocol/FirmwareVolumeBlock.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/VirtNorFlashDeviceLib.h>
#include <Library/VirtNorFlashPlatformLib.h>

#define NOR_FLASH_SIGNATURE  SIGNATURE_32('n', 'o', 'r', '0')
#define INSTANCE_FROM_FVB_THIS(a)  CR(a, NOR_FLASH_INSTANCE, FvbProtocol, NOR_FLASH_SIGNATURE)

typedef struct _NOR_FLASH_INSTANCE NOR_FLASH_INSTANCE;

#pragma pack (1)
typedef struct {
  VENDOR_DEVICE_PATH          Vendor;
  UINT8                       Index;
  EFI_DEVICE_PATH_PROTOCOL    End;
} NOR_FLASH_DEVICE_PATH;
#pragma pack ()

struct _NOR_FLASH_INSTANCE {
  UINT32                                 Signature;
  EFI_HANDLE                             Handle;

  UINTN                                  DeviceBaseAddress;
  UINTN                                  RegionBaseAddress;
  UINTN                                  Size;
  EFI_LBA                                StartLba;
  EFI_LBA                                LastBlock;
  UINT32                                 BlockSize;

  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL    FvbProtocol;
  VOID                                   *ShadowBuffer;

  NOR_FLASH_DEVICE_PATH                  DevicePath;
};

//
// NorFlashFvbDxe.c
//

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

EFI_STATUS
ValidateFvHeader (
  IN  NOR_FLASH_INSTANCE  *Instance
  );

EFI_STATUS
InitializeFvAndVariableStoreHeaders (
  IN NOR_FLASH_INSTANCE  *Instance
  );

VOID
EFIAPI
FvbVirtualNotifyEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

//
// NorFlashDxe.c
//
EFI_STATUS
NorFlashCreateInstance (
  IN UINTN                NorFlashDeviceBase,
  IN UINTN                NorFlashRegionBase,
  IN UINTN                NorFlashSize,
  IN UINT32               Index,
  IN UINT32               BlockSize,
  IN BOOLEAN              SupportFvb,
  OUT NOR_FLASH_INSTANCE  **NorFlashInstance
  );

EFI_STATUS
EFIAPI
NorFlashFvbInitialize (
  IN NOR_FLASH_INSTANCE  *Instance
  );

#endif /* __VIRT_NOT_FLASH_DXE__ */

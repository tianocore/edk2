/**@file

  Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  Module Name:

    FwBlockService.h

  Abstract:

    Firmware volume block driver for Intel Firmware Hub (FWH) device

**/

#ifndef _FW_BLOCK_SERVICE_H
#define _FW_BLOCK_SERVICE_H

typedef struct {
  UINTN                       FvBase;
  UINTN                       NumOfBlocks;
  EFI_FIRMWARE_VOLUME_HEADER  VolumeHeader;
} EFI_FW_VOL_INSTANCE;

typedef struct {
  UINT32              NumFv;
  EFI_FW_VOL_INSTANCE *FvInstance;
} ESAL_FWB_GLOBAL;

extern ESAL_FWB_GLOBAL *mFvbModuleGlobal;

//
// Fvb Protocol instance data
//
#define FVB_DEVICE_FROM_THIS(a) CR (a, EFI_FW_VOL_BLOCK_DEVICE, \
                                  FwVolBlockInstance, FVB_DEVICE_SIGNATURE)

#define FVB_EXTEND_DEVICE_FROM_THIS(a) CR (a, EFI_FW_VOL_BLOCK_DEVICE, \
                                         FvbExtension, FVB_DEVICE_SIGNATURE)

#define FVB_DEVICE_SIGNATURE            SIGNATURE_32 ('F', 'V', 'B', 'N')

typedef struct {
  MEDIA_FW_VOL_DEVICE_PATH  FvDevPath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevPath;
} FV_PIWG_DEVICE_PATH;

typedef struct {
  MEMMAP_DEVICE_PATH          MemMapDevPath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevPath;
} FV_MEMMAP_DEVICE_PATH;

typedef struct {
  UINTN                               Signature;
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath;
  UINTN                               Instance;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  FwVolBlockInstance;
} EFI_FW_VOL_BLOCK_DEVICE;

EFI_STATUS
GetFvbInfo (
  IN  UINT64                            FvLength,
  OUT EFI_FIRMWARE_VOLUME_HEADER        **FvbInfo
  );

EFI_STATUS
FvbSetVolumeAttributes (
  IN UINTN                                Instance,
  IN OUT EFI_FVB_ATTRIBUTES_2             *Attributes,
  IN ESAL_FWB_GLOBAL                      *Global
  );

EFI_STATUS
FvbGetVolumeAttributes (
  IN UINTN                                Instance,
  OUT EFI_FVB_ATTRIBUTES_2                *Attributes,
  IN ESAL_FWB_GLOBAL                      *Global
  );

EFI_STATUS
FvbGetPhysicalAddress (
  IN UINTN                                Instance,
  OUT EFI_PHYSICAL_ADDRESS                *Address,
  IN ESAL_FWB_GLOBAL                      *Global
  );

EFI_STATUS
EFIAPI
FvbInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );


VOID
EFIAPI
FvbClassAddressChangeEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  );

EFI_STATUS
FvbGetLbaAddress (
  IN  UINTN                               Instance,
  IN  EFI_LBA                             Lba,
  OUT UINTN                               *LbaAddress,
  OUT UINTN                               *LbaLength,
  OUT UINTN                               *NumOfBlocks,
  IN  ESAL_FWB_GLOBAL                     *Global
  );

//
// Protocol APIs
//
EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  OUT EFI_FVB_ATTRIBUTES_2                              *Attributes
  );

EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN OUT EFI_FVB_ATTRIBUTES_2                           *Attributes
  );

EFI_STATUS
EFIAPI
FvbProtocolGetPhysicalAddress (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  OUT EFI_PHYSICAL_ADDRESS                        *Address
  );

EFI_STATUS
EFIAPI
FvbProtocolGetBlockSize (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN CONST EFI_LBA                                     Lba,
  OUT UINTN                                       *BlockSize,
  OUT UINTN                                       *NumOfBlocks
  );

EFI_STATUS
EFIAPI
FvbProtocolRead (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN CONST EFI_LBA                                      Lba,
  IN CONST UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  );

EFI_STATUS
EFIAPI
FvbProtocolWrite (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN       EFI_LBA                                      Lba,
  IN       UINTN                                        Offset,
  IN OUT   UINTN                                        *NumBytes,
  IN       UINT8                                        *Buffer
  );

EFI_STATUS
EFIAPI
FvbProtocolEraseBlocks (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *This,
  ...
  );

//
// The following functions have different implementations dependent on the
// module type chosen for building this driver.
//
VOID
InstallProtocolInterfaces (
  IN EFI_FW_VOL_BLOCK_DEVICE *FvbDevice
  );

VOID
InstallVirtualAddressChangeHandler (
  VOID
  );

EFI_STATUS
MarkIoMemoryRangeForRuntimeAccess (
  IN EFI_PHYSICAL_ADDRESS   BaseAddress,
  IN UINTN                  Length
  );

#endif

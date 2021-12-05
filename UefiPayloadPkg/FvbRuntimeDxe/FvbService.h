/** @file
The header file for Firmware volume block driver.

Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FW_BLOCK_SERVICE_H_
#define FW_BLOCK_SERVICE_H_

#include <Guid/EventGroup.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/VariableFormat.h>
#include <Protocol/DevicePath.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/FlashDeviceLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HobLib.h>
#include <Library/DxeServicesLib.h>
#include <Guid/NvVariableInfoGuid.h>
#include <Register/ArchitecturalMsr.h>

//
// Define two helper macro to extract the Capability field or Status field in FVB
// bit fields
//
#define EFI_FVB2_CAPABILITIES  (EFI_FVB2_READ_DISABLED_CAP |\
                              EFI_FVB2_READ_ENABLED_CAP | \
                              EFI_FVB2_WRITE_DISABLED_CAP | \
                              EFI_FVB2_WRITE_ENABLED_CAP | \
                              EFI_FVB2_LOCK_CAP \
                              )

#define EFI_FVB2_STATUS  (EFI_FVB2_READ_STATUS | EFI_FVB2_WRITE_STATUS | EFI_FVB2_LOCK_STATUS)

typedef struct {
  UINTN                         FvBase;
  UINTN                         NumOfBlocks;
  //
  // Note!!!: VolumeHeader must be the last element
  // of the structure.
  //
  EFI_FIRMWARE_VOLUME_HEADER    VolumeHeader;
} EFI_FW_VOL_INSTANCE;

typedef struct {
  EFI_FW_VOL_INSTANCE    *FvInstance;
  UINT32                 NumFv;
  UINT32                 Flags;
} FWB_GLOBAL;

//
// Fvb Protocol instance data
//
#define FVB_DEVICE_FROM_THIS(a)         CR(a, EFI_FW_VOL_BLOCK_DEVICE, FwVolBlockInstance, FVB_DEVICE_SIGNATURE)
#define FVB_EXTEND_DEVICE_FROM_THIS(a)  CR(a, EFI_FW_VOL_BLOCK_DEVICE, FvbExtension, FVB_DEVICE_SIGNATURE)
#define FVB_DEVICE_SIGNATURE  SIGNATURE_32('F','V','B','C')

typedef struct {
  MEDIA_FW_VOL_DEVICE_PATH    FvDevPath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevPath;
} FV_PIWG_DEVICE_PATH;

typedef struct {
  MEMMAP_DEVICE_PATH          MemMapDevPath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevPath;
} FV_MEMMAP_DEVICE_PATH;

typedef struct {
  UINT32                                Signature;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
  UINTN                                 Instance;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    FwVolBlockInstance;
} EFI_FW_VOL_BLOCK_DEVICE;

/**
  Get a heathy FV header used for variable store recovery

  @retval     The FV header.

**/
EFI_FIRMWARE_VOLUME_HEADER *
GetFvHeaderTemplate (
  VOID
  );

EFI_STATUS
InitVariableStore (
  VOID
  );

//
// Protocol APIs
//
EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT EFI_FVB_ATTRIBUTES_2                     *Attributes
  );

EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN OUT EFI_FVB_ATTRIBUTES_2                  *Attributes
  );

EFI_STATUS
EFIAPI
FvbProtocolGetPhysicalAddress (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  OUT EFI_PHYSICAL_ADDRESS                     *Address
  );

EFI_STATUS
EFIAPI
FvbProtocolGetBlockSize (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN  EFI_LBA                                  Lba,
  OUT UINTN                                    *BlockSize,
  OUT UINTN                                    *NumOfBlocks
  );

EFI_STATUS
EFIAPI
FvbProtocolRead (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN EFI_LBA                                   Lba,
  IN UINTN                                     Offset,
  IN OUT UINTN                                 *NumBytes,
  OUT UINT8                                    *Buffer
  );

EFI_STATUS
EFIAPI
FvbProtocolWrite (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN EFI_LBA                                   Lba,
  IN UINTN                                     Offset,
  IN OUT UINTN                                 *NumBytes,
  IN UINT8                                     *Buffer
  );

EFI_STATUS
EFIAPI
FvbProtocolEraseBlocks (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  ...
  );

EFI_FW_VOL_INSTANCE *
GetFvbInstance (
  IN  UINTN  Instance
  );

EFI_STATUS
InstallFvbProtocol (
  IN  EFI_FW_VOL_INSTANCE  *FwhInstance,
  IN  UINTN                InstanceNum
  );

EFI_STATUS
FvbInitialize (
  VOID
  );

extern FWB_GLOBAL               mFvbModuleGlobal;
extern EFI_FW_VOL_BLOCK_DEVICE  mFvbDeviceTemplate;
extern FV_MEMMAP_DEVICE_PATH    mFvMemmapDevicePathTemplate;
extern FV_PIWG_DEVICE_PATH      mFvPIWGDevicePathTemplate;

#endif

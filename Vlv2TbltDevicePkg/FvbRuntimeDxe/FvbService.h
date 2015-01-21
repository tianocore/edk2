/** @file
  The header file for Firmware volume block driver.

Copyright (c) 2006  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#ifndef _FW_BLOCK_SERVICE_H
#define _FW_BLOCK_SERVICE_H

#include <Guid/EventGroup.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/SystemNvDataGuid.h>
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

//
// Define two helper macro to extract the Capability field or Status field in FVB
// bit fields.
//
#define EFI_FVB2_CAPABILITIES (EFI_FVB2_READ_DISABLED_CAP | \
                              EFI_FVB2_READ_ENABLED_CAP | \
                              EFI_FVB2_WRITE_DISABLED_CAP | \
                              EFI_FVB2_WRITE_ENABLED_CAP | \
                              EFI_FVB2_LOCK_CAP \
                              )

#define EFI_FVB2_STATUS (EFI_FVB2_READ_STATUS | EFI_FVB2_WRITE_STATUS | EFI_FVB2_LOCK_STATUS)


typedef struct {
  UINTN                       FvBase;
  UINTN                       NumOfBlocks;
  //
  // Note!!!: VolumeHeader must be the last element
  // of the structure.
  //
  EFI_FIRMWARE_VOLUME_HEADER  VolumeHeader;
} EFI_FW_VOL_INSTANCE;

typedef struct {
  EFI_FW_VOL_INSTANCE         *FvInstance;
  UINT32                      NumFv;
} FWB_GLOBAL;

//
// Fvb Protocol instance data.
//
#define FVB_DEVICE_FROM_THIS(a) CR(a, EFI_FW_VOL_BLOCK_DEVICE, FwVolBlockInstance, FVB_DEVICE_SIGNATURE)
#define FVB_EXTEND_DEVICE_FROM_THIS(a) CR(a, EFI_FW_VOL_BLOCK_DEVICE, FvbExtension, FVB_DEVICE_SIGNATURE)
#define FVB_DEVICE_SIGNATURE       SIGNATURE_32('F','V','B','C')

typedef struct {
  MEDIA_FW_VOL_DEVICE_PATH  FvDevPath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevPath;
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

EFI_STATUS
GetFvbInfo (
  IN  EFI_PHYSICAL_ADDRESS              FvBaseAddress,
  OUT EFI_FIRMWARE_VOLUME_HEADER        **FvbInfo
  );

//
// Protocol APIs
//
EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  OUT EFI_FVB_ATTRIBUTES_2                      *Attributes
  );

EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN OUT EFI_FVB_ATTRIBUTES_2                   *Attributes
  );

EFI_STATUS
EFIAPI
FvbProtocolGetPhysicalAddress (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
       OUT EFI_PHYSICAL_ADDRESS                *Address
  );

EFI_STATUS
EFIAPI
FvbProtocolGetBlockSize (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN  EFI_LBA                            Lba,
  OUT UINTN                              *BlockSize,
  OUT UINTN                              *NumOfBlocks
  );

EFI_STATUS
EFIAPI
FvbProtocolRead (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN EFI_LBA                              Lba,
  IN UINTN                                Offset,
  IN OUT UINTN                            *NumBytes,
  OUT UINT8                                *Buffer
  );

EFI_STATUS
EFIAPI
FvbProtocolWrite (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN EFI_LBA                              Lba,
  IN UINTN                                Offset,
  IN OUT UINTN                            *NumBytes,
  IN UINT8                                *Buffer
  );

EFI_STATUS
EFIAPI
FvbProtocolEraseBlocks (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *This,
  ...
  );

EFI_FW_VOL_INSTANCE *
GetFvbInstance (
  IN  UINTN                              Instance
  );

BOOLEAN
IsFvHeaderValid (
  IN       EFI_PHYSICAL_ADDRESS          FvBase,
  IN CONST EFI_FIRMWARE_VOLUME_HEADER    *FwVolHeader
  );

VOID
InstallFvbProtocol (
  IN  EFI_FW_VOL_INSTANCE               *FwhInstance,
  IN  UINTN                             InstanceNum
  );

EFI_STATUS
FvbInitialize (
  VOID
  );

extern FWB_GLOBAL              mFvbModuleGlobal;
extern EFI_FW_VOL_BLOCK_DEVICE mFvbDeviceTemplate;
extern FV_MEMMAP_DEVICE_PATH   mFvMemmapDevicePathTemplate;
extern FV_PIWG_DEVICE_PATH     mFvPIWGDevicePathTemplate;
extern UINT32                  mPlatformFvBaseAddress[3];

#endif


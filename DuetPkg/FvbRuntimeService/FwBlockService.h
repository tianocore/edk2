/**@file
Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FwBlockService.h
  
Abstract:

  Firmware volume block driver for Intel Firmware Hub (FWH) device

**/
#ifndef _FW_BLOCK_SERVICE_H
#define _FW_BLOCK_SERVICE_H

//
// The package level header files this module uses
//
#include <PiDxe.h>

//
// The protocols, PPI and GUID defintions for this module
//
#include <Guid/EventGroup.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/BlockIo.h>
#include <Library/DevicePathLib.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/FlashMapHob.h>
#include <Guid/HobList.h>

//
// The Library classes this module consumes
//
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>


#define FWH_READ_LOCK                   (1 << 2)
#define FWH_LOCK_DOWN                   (1 << 1)
#define FWH_WRITE_LOCK                  1
#define FWH_WRITE_STATE_STATUS          (1 << 7)
#define FWH_ERASE_STATUS                (1 << 5)
#define FWH_PROGRAM_STATUS              (1 << 4)
#define FWH_VPP_STATUS                  (1 << 3)
#define STALL_TIME                      5
#define FWH_ERASE_STATUS_BITS           (FWH_ERASE_STATUS || FWH_VPP_STATUS)
#define FWH_WRITE_STATUS_BITS           (FWH_WRITE_STATUS || FWH_VPP_STATUS)

//
// BugBug: Add documentation here for data structure!!!!
//
#define FVB_PHYSICAL  0
#define FVB_VIRTUAL   1

#define EFI_FVB2_CAPABILITIES (EFI_FVB2_READ_DISABLED_CAP | \
                              EFI_FVB2_READ_ENABLED_CAP | \
                              EFI_FVB2_WRITE_DISABLED_CAP | \
                              EFI_FVB2_WRITE_ENABLED_CAP | \
                              EFI_FVB2_LOCK_CAP \
                              )
#define EFI_FVB2_STATUS (EFI_FVB2_READ_STATUS | EFI_FVB2_WRITE_STATUS | EFI_FVB2_LOCK_STATUS)

typedef struct {
  EFI_LOCK                    FvbDevLock;
  UINTN                       FvBase[2];
  //
  // We can treat VolumeSignature combined with MappedFile 
  //  as a unique key to locate the mapped file.
#define MAX_PATH 256
  UINT32                      VolumeId;
  CHAR16                      MappedFile[MAX_PATH];
  UINT32                      ActuralSize;
  UINT32                      Offset;
  
  EFI_DEVICE_PATH_PROTOCOL    *Device; // only used in BS period, won't use after memory map changed
  UINTN                       NumOfBlocks;
  BOOLEAN                     WriteEnabled;
  EFI_FIRMWARE_VOLUME_HEADER  VolumeHeader;
} EFI_FW_VOL_INSTANCE;

typedef struct {
  UINT32              NumFv;
  EFI_FW_VOL_INSTANCE *FvInstance[2];
  UINT8               *FvbScratchSpace[2];
} ESAL_FWB_GLOBAL;

//
// Fvb Protocol instance data
//
#define FVB_DEVICE_FROM_THIS(a)         CR (a, EFI_FW_VOL_BLOCK_DEVICE, FwVolBlockInstance, FVB_DEVICE_SIGNATURE)
#define FVB_EXTEND_DEVICE_FROM_THIS(a)  CR (a, EFI_FW_VOL_BLOCK_DEVICE, FvbExtension, FVB_DEVICE_SIGNATURE)
#define FVB_DEVICE_SIGNATURE            SIGNATURE_32 ('F', 'V', 'B', 'C')

typedef struct {
  MEMMAP_DEVICE_PATH        MemMapDevPath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevPath;
} FV_DEVICE_PATH;

typedef struct {
  UINTN                               Signature;
  FV_DEVICE_PATH                      DevicePath;
  UINTN                               Instance;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  FwVolBlockInstance;
} EFI_FW_VOL_BLOCK_DEVICE;

EFI_STATUS
GetFvbInfo (
  IN  EFI_PHYSICAL_ADDRESS              FvBaseAddress,
  OUT EFI_FIRMWARE_VOLUME_HEADER        **FvbInfo
  );

EFI_STATUS
EnableFvbWrites (
  IN  BOOLEAN   EnableWrites
  );

EFI_STATUS
PlatformGetFvbWriteBase (
  IN  UINTN     CurrentBaseAddress,
  IN  UINTN     *NewBaseAddress,
  IN  BOOLEAN   *WriteEnabled
  );

EFI_STATUS
EnablePlatformFvb (
  VOID
  );

BOOLEAN
SetPlatformFvbLock (
  IN UINTN  LbaAddress
  );

EFI_STATUS
FvbReadBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba,
  IN UINTN                                BlockOffset,
  IN OUT UINTN                            *NumBytes,
  IN UINT8                                *Buffer,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  );

EFI_STATUS
FvbWriteBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba,
  IN UINTN                                BlockOffset,
  IN OUT UINTN                            *NumBytes,
  IN UINT8                                *Buffer,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  );

EFI_STATUS
FvbEraseBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  );

EFI_STATUS
FvbSetVolumeAttributes (
  IN UINTN                                Instance,
  IN OUT EFI_FVB_ATTRIBUTES_2             *Attributes,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  );

EFI_STATUS
FvbGetVolumeAttributes (
  IN UINTN                                Instance,
  OUT EFI_FVB_ATTRIBUTES_2                *Attributes,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  );

EFI_STATUS
FvbGetPhysicalAddress (
  IN UINTN                                Instance,
  OUT EFI_PHYSICAL_ADDRESS                *Address,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
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
FvbSpecificInitialize (
  IN  ESAL_FWB_GLOBAL   *mFvbModuleGlobal
  );

EFI_STATUS
FvbGetLbaAddress (
  IN  UINTN                               Instance,
  IN  EFI_LBA                             Lba,
  OUT UINTN                               *LbaAddress,
  OUT UINTN                               *LbaLength,
  OUT UINTN                               *NumOfBlocks,
  IN  ESAL_FWB_GLOBAL                     *Global,
  IN  BOOLEAN                             Virtual
  );

//
// Protocol APIs
//
EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL          *This,
  OUT      EFI_FVB_ATTRIBUTES_2                        *Attributes
  );

EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL       *This,
  IN OUT   EFI_FVB_ATTRIBUTES_2                     *Attributes
  );

EFI_STATUS
EFIAPI
FvbProtocolGetPhysicalAddress (
  IN  CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL          *This,
  OUT       EFI_PHYSICAL_ADDRESS                        *Address
  );

EFI_STATUS
EFIAPI
FvbProtocolGetBlockSize (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN  EFI_LBA                                     Lba,
  OUT UINTN                                       *BlockSize,
  OUT UINTN                                       *NumOfBlocks
  );

EFI_STATUS
EFIAPI
FvbProtocolRead (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *This,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  );

EFI_STATUS
EFIAPI
FvbProtocolWrite (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *This,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  );

EFI_STATUS
EFIAPI
FvbProtocolEraseBlocks (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *This,
  ...  
  );

#endif

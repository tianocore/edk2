/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FwBlockService.h
  
Abstract:

  Firmware volume block driver for Intel Firmware Hub (FWH) device

--*/

#ifndef _FW_BLOCK_SERVICE_H
#define _FW_BLOCK_SERVICE_H

//
// Statements that include other header files
//
#include "Tiano.h"
#include "EfiFirmwareVolumeHeader.h"
#include "EfiRuntimeLib.h"
#include "EfiHobLib.h"
#include "EfiScriptLib.h"

#include EFI_PROTOCOL_PRODUCER (FirmwareVolumeBlock)
#include EFI_PROTOCOL_PRODUCER (FvbExtension)
#include EFI_GUID_DEFINITION (AlternateFvBlock)
#include EFI_GUID_DEFINITION (Hob)
#include EFI_GUID_DEFINITION (DxeServices)
#include EFI_PROTOCOL_CONSUMER (CpuIo)

#define FVB_MAX_RETRY_TIMES             10000000
#define FWH_BLOCK_ERASE_SETUP_COMMAND   0x20
#define FWH_BLOCK_ERASE_CONFIRM_COMMAND 0xd0
#define FWH_READ_STATUS_COMMAND         0x70
#define FWH_CLEAR_STATUS_COMMAND        0x50
#define FWH_READ_ARRAY_COMMAND          0xff
#define FWH_WRITE_SETUP_COMMAND         0x40
#define FWH_OPEN_FEATURE_SPACE_COMMAND  0x91
#define FWH_READ_LOCK                   (1 << 2)
#define FWH_WRITE_LOCK                  (1 << 1)
#define FWH_LOCK_DOWN                   1
#define FWH_WRITE_STATE_STATUS          (1 << 7)
#define FWH_ERASE_STATUS                (1 << 5)
#define FWH_PROGRAM_STATUS              (1 << 4)
#define FWH_VPP_STATUS                  (1 << 3)
#define STALL_TIME                      5
#define FWH_ERASE_STATUS_BITS           (FWH_ERASE_STATUS || FWH_VPP_STATUS)
#define FWH_WRITE_STATUS_BITS           (FWH_WRITE_STATUS || FWH_VPP_STATUS)
#define CFI_BLOCK_LOCK_UNLOCK           0x60
#define CFI_BLOCK_LOCK_CONFIRM          1
#define CFI_BLOCK_UNLOCK_CONFIRM        0xD0
#define CFI_QUERY                       0x98

//
// BugBug: Add documentation here for data structure!!!!
//
#define FVB_PHYSICAL  0
#define FVB_VIRTUAL   1

typedef struct {
  EFI_LOCK                    FvbDevLock;
  UINTN                       FvBase[2];
  UINTN                       FvWriteBase[2];
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
#define FVB_DEVICE_SIGNATURE            EFI_SIGNATURE_32 ('F', 'V', 'B', 'C')

typedef struct {
  MEMMAP_DEVICE_PATH        MemMapDevPath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevPath;
} FV_DEVICE_PATH;

typedef struct {
  UINTN                               Signature;
  FV_DEVICE_PATH                      DevicePath;
  UINTN                               Instance;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  FwVolBlockInstance;
  EFI_FVB_EXTENSION_PROTOCOL          FvbExtension;
} EFI_FW_VOL_BLOCK_DEVICE;

EFI_STATUS
GetFvbInfo (
  IN  EFI_PHYSICAL_ADDRESS              FvBaseAddress,
  OUT EFI_FIRMWARE_VOLUME_HEADER        **FvbInfo
  )
;

EFI_STATUS
EnableFvbWrites (
  IN  BOOLEAN   EnableWrites
  )
;

EFI_STATUS
PlatformGetFvbWriteBase (
  IN  UINTN     CurrentBaseAddress,
  IN  UINTN     *NewBaseAddress,
  IN  BOOLEAN   *WriteEnabled
  )
;

EFI_STATUS
EnablePlatformFvb (
  VOID
  )
;

BOOLEAN
SetPlatformFvbLock (
  IN UINTN  LbaAddress
  )
;

EFI_STATUS
FvbReadBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba,
  IN UINTN                                BlockOffset,
  IN OUT UINTN                            *NumBytes,
  IN UINT8                                *Buffer,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
;

EFI_STATUS
FvbWriteBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba,
  IN UINTN                                BlockOffset,
  IN OUT UINTN                            *NumBytes,
  IN UINT8                                *Buffer,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
;

EFI_STATUS
FvbEraseBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
;

EFI_STATUS
FvbSetVolumeAttributes (
  IN UINTN                                Instance,
  IN OUT EFI_FVB_ATTRIBUTES               *Attributes,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
;

EFI_STATUS
FvbGetVolumeAttributes (
  IN UINTN                                Instance,
  OUT EFI_FVB_ATTRIBUTES                  *Attributes,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
;

EFI_STATUS
FvbGetPhysicalAddress (
  IN UINTN                                Instance,
  OUT EFI_PHYSICAL_ADDRESS                *Address,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
;

EFI_STATUS
EFIAPI
FvbInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
;


VOID
EFIAPI
FvbClassAddressChangeEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
;

EFI_STATUS
FvbSpecificInitialize (
  IN  ESAL_FWB_GLOBAL   *mFvbModuleGlobal
  )
;

EFI_STATUS
FvbGetLbaAddress (
  IN  UINTN                               Instance,
  IN  EFI_LBA                             Lba,
  OUT UINTN                               *LbaAddress,
  OUT UINTN                               *LbaWriteAddress,
  OUT UINTN                               *LbaLength,
  OUT UINTN                               *NumOfBlocks,
  IN  ESAL_FWB_GLOBAL                     *Global,
  IN  BOOLEAN                             Virtual
  )
;

EFI_STATUS
FvbEraseCustomBlockRange (
  IN UINTN                                Instance,
  IN EFI_LBA                              StartLba,
  IN UINTN                                OffsetStartLba,
  IN EFI_LBA                              LastLba,
  IN UINTN                                OffsetLastLba,
  IN ESAL_FWB_GLOBAL                      *Global,
  IN BOOLEAN                              Virtual
  )
;

//
// Protocol APIs
//
EFI_STATUS
EFIAPI
FvbProtocolGetAttributes (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  OUT EFI_FVB_ATTRIBUTES                          *Attributes
  )
;

EFI_STATUS
EFIAPI
FvbProtocolSetAttributes (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN OUT EFI_FVB_ATTRIBUTES                       *Attributes
  )
;

EFI_STATUS
EFIAPI
FvbProtocolGetPhysicalAddress (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  OUT EFI_PHYSICAL_ADDRESS                        *Address
  )
;

EFI_STATUS
EFIAPI
FvbProtocolGetBlockSize (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN  EFI_LBA                                     Lba,
  OUT UINTN                                       *BlockSize,
  OUT UINTN                                       *NumOfBlocks
  )
;

EFI_STATUS
EFIAPI
FvbProtocolRead (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  )
;

EFI_STATUS
EFIAPI
FvbProtocolWrite (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL           *This,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  )
;

EFI_STATUS
EFIAPI
FvbProtocolEraseBlocks (
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *This,
  ...
  )
;

EFI_STATUS
EFIAPI
FvbExtendProtocolEraseCustomBlockRange (
  IN EFI_FVB_EXTENSION_PROTOCOL           *This,
  IN EFI_LBA                              StartLba,
  IN UINTN                                OffsetStartLba,
  IN EFI_LBA                              LastLba,
  IN UINTN                                OffsetLastLba
  )
;

#endif

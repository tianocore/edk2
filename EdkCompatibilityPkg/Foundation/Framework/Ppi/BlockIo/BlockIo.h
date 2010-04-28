/*++

Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  BlockIo.h

Abstract:

  BlockIo PPI as defined in EFI 2.0

  Used to access block-oriented storage devices

--*/

#ifndef _PEI_BLOCK_IO_H_
#define _PEI_BLOCK_IO_H_

#define PEI_BLOCK_IO_PPI_GUID \
  { \
    0x695d8aa1, 0x42ee, 0x4c46, {0x80, 0x5c, 0x6e, 0xa6, 0xbc, 0xe7, 0x99, 0xe3} \
  }

EFI_FORWARD_DECLARATION (PEI_RECOVERY_BLOCK_IO_INTERFACE);

typedef UINT64  PEI_LBA;

typedef enum {
  LegacyFloppy  = 0,
  IdeCDROM      = 1,
  IdeLS120      = 2,
  UsbMassStorage= 3,
  MaxDeviceType
} PEI_BLOCK_DEVICE_TYPE;

typedef struct {
  PEI_BLOCK_DEVICE_TYPE DeviceType;
  BOOLEAN               MediaPresent;
  UINTN                 LastBlock;
  UINTN                 BlockSize;
} PEI_BLOCK_IO_MEDIA;

typedef
EFI_STATUS
(EFIAPI *PEI_GET_NUMBER_BLOCK_DEVICES) (
  IN  EFI_PEI_SERVICES                         **PeiServices,
  IN PEI_RECOVERY_BLOCK_IO_INTERFACE           * This,
  OUT UINTN                                    *NumberBlockDevices
  );

typedef
EFI_STATUS
(EFIAPI *PEI_GET_DEVICE_MEDIA_INFORMATION) (
  IN  EFI_PEI_SERVICES                         **PeiServices,
  IN PEI_RECOVERY_BLOCK_IO_INTERFACE           * This,
  IN UINTN                                     DeviceIndex,
  OUT PEI_BLOCK_IO_MEDIA                       * MediaInfo
  );

typedef
EFI_STATUS
(EFIAPI *PEI_READ_BLOCKS) (
  IN  EFI_PEI_SERVICES                         **PeiServices,
  IN PEI_RECOVERY_BLOCK_IO_INTERFACE           * This,
  IN UINTN                                     DeviceIndex,
  IN PEI_LBA                                   StartLBA,
  IN UINTN                                     BufferSize,
  OUT VOID                                     *Buffer
  );

struct _PEI_RECOVERY_BLOCK_IO_INTERFACE {
  PEI_GET_NUMBER_BLOCK_DEVICES      GetNumberOfBlockDevices;
  PEI_GET_DEVICE_MEDIA_INFORMATION  GetBlockDeviceMediaInfo;
  PEI_READ_BLOCKS                   ReadBlocks;
};

extern EFI_GUID gPeiBlockIoPpiGuid;

#endif

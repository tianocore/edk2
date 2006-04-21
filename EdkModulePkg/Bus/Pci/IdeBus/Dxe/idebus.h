/*++
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    idebus.h

Abstract:

    Header file for IDE Bus Driver.

Revision History
++*/

// TODO: fix comment to end with --*/
#ifndef _IDE_BUS_H
#define _IDE_BUS_H


#include <IndustryStandard/Pci22.h>
#include "idedata.h"

//
// Extra Definition to porting
//
#define EFI_MIN(a, b) (((a) < (b)) ? (a) : (b))

#define MAX_IDE_DEVICE    4
#define MAX_IDE_CHANNELS  2
#define MAX_IDE_DRIVES    2

typedef struct {
  BOOLEAN HaveScannedDevice[MAX_IDE_DEVICE];
  BOOLEAN DeviceFound[MAX_IDE_DEVICE];
  BOOLEAN DeviceProcessed[MAX_IDE_DEVICE];
} IDE_BUS_DRIVER_PRIVATE_DATA;

#define IDE_BLK_IO_DEV_SIGNATURE  EFI_SIGNATURE_32 ('i', 'b', 'i', 'd')

typedef struct {
  UINT32                      Signature;

  EFI_HANDLE                  Handle;
  EFI_BLOCK_IO_PROTOCOL       BlkIo;
  EFI_BLOCK_IO_MEDIA          BlkMedia;
  EFI_DISK_INFO_PROTOCOL      DiskInfo;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  EFI_PCI_IO_PROTOCOL         *PciIo;
  IDE_BUS_DRIVER_PRIVATE_DATA *IdeBusDriverPrivateData;

  //
  // Local Data for IDE interface goes here
  //
  EFI_IDE_CHANNEL             Channel;
  EFI_IDE_DEVICE              Device;
  UINT16                      Lun;
  IDE_DEVICE_TYPE             Type;

  IDE_BASE_REGISTERS          *IoPort;
  UINT16                      AtapiError;

  INQUIRY_DATA                *pInquiryData;
  EFI_IDENTIFY_DATA           *pIdData;
  ATA_PIO_MODE                PioMode;
  ATA_UDMA_MODE               UDma_Mode;
  CHAR8                       ModelName[41];
  REQUEST_SENSE_DATA          *SenseData;
  UINT8                       SenseDataNumber;
  UINT8                       *Cache;

  EFI_UNICODE_STRING_TABLE    *ControllerNameTable;
} IDE_BLK_IO_DEV;

#include "ComponentName.h"

#define IDE_BLOCK_IO_DEV_FROM_THIS(a)           CR (a, IDE_BLK_IO_DEV, BlkIo, IDE_BLK_IO_DEV_SIGNATURE)
#define IDE_BLOCK_IO_DEV_FROM_DISK_INFO_THIS(a) CR (a, IDE_BLK_IO_DEV, DiskInfo, IDE_BLK_IO_DEV_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gIDEBusDriverBinding;

#include "ide.h"

//
// Prototypes
// Driver model protocol interface
//
EFI_STATUS
EFIAPI
IDEBusControllerDriverEntryPoint (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ImageHandle - TODO: add argument description
  SystemTable - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IDEBusDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This                - TODO: add argument description
  Controller          - TODO: add argument description
  RemainingDevicePath - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IDEBusDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This                - TODO: add argument description
  Controller          - TODO: add argument description
  RemainingDevicePath - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IDEBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL *This,
  IN  EFI_HANDLE                  Controller,
  IN  UINTN                       NumberOfChildren,
  IN  EFI_HANDLE                  *ChildHandleBuffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This              - TODO: add argument description
  Controller        - TODO: add argument description
  NumberOfChildren  - TODO: add argument description
  ChildHandleBuffer - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
// Block I/O Protocol Interface
//
EFI_STATUS
EFIAPI
IDEBlkIoReset (
  IN  EFI_BLOCK_IO_PROTOCOL       *This,
  IN  BOOLEAN                     ExtendedVerification
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This                  - TODO: add argument description
  ExtendedVerification  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IDEBlkIoReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL       *This,
  IN  UINT32                      MediaId,
  IN  EFI_LBA                     LBA,
  IN  UINTN                       BufferSize,
  OUT VOID                        *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  MediaId     - TODO: add argument description
  LBA         - TODO: add argument description
  BufferSize  - TODO: add argument description
  Buffer      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IDEBlkIoWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL       *This,
  IN  UINT32                      MediaId,
  IN  EFI_LBA                     LBA,
  IN  UINTN                       BufferSize,
  IN  VOID                        *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  MediaId     - TODO: add argument description
  LBA         - TODO: add argument description
  BufferSize  - TODO: add argument description
  Buffer      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IDEBlkIoFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL       *This
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
IDERegisterDecodeEnableorDisable (
  IN  EFI_PCI_IO_PROTOCOL         *PciIo,
  IN  BOOLEAN                     Enable
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIo   - TODO: add argument description
  Enable  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IDEDiskInfoInquiry (
  IN EFI_DISK_INFO_PROTOCOL       *This,
  IN OUT VOID                     *InquiryData,
  IN OUT UINT32                   *IntquiryDataSize
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This              - TODO: add argument description
  InquiryData       - TODO: add argument description
  IntquiryDataSize  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IDEDiskInfoIdentify (
  IN EFI_DISK_INFO_PROTOCOL       *This,
  IN OUT VOID                     *IdentifyData,
  IN OUT UINT32                   *IdentifyDataSize
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This              - TODO: add argument description
  IdentifyData      - TODO: add argument description
  IdentifyDataSize  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IDEDiskInfoSenseData (
  IN EFI_DISK_INFO_PROTOCOL       *This,
  IN OUT VOID                     *SenseData,
  IN OUT UINT32                   *SenseDataSize,
  OUT UINT8                       *SenseDataNumber
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This            - TODO: add argument description
  SenseData       - TODO: add argument description
  SenseDataSize   - TODO: add argument description
  SenseDataNumber - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
IDEDiskInfoWhichIde (
  IN EFI_DISK_INFO_PROTOCOL       *This,
  OUT UINT32                      *IdeChannel,
  OUT UINT32                      *IdeDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  IdeChannel  - TODO: add argument description
  IdeDevice   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#endif

/** @file
  Header file for IDE Bus Driver.

  Copyright (c) 2006 - 2007 Intel Corporation. <BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IDE_BUS_H
#define _IDE_BUS_H



#include <FrameworkDxe.h>

#include <Protocol/IdeControllerInit.h>
#include <Protocol/BlockIo.h>
#include <Protocol/PciIo.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/DevicePath.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PerformanceLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include <IndustryStandard/Pci22.h>
#include "IdeData.h"

//
// Extra Definition to porting
//
#define MAX_IDE_DEVICE    4
#define MAX_IDE_CHANNELS  2
#define MAX_IDE_DRIVES    2

#define INVALID_DEVICE_TYPE 0xff
#define ATA_DEVICE_TYPE     0x00
#define ATAPI_DEVICE_TYPE   0x01

#define EFI_IOB_ATA_BUS_SMART_ENABLE          (EFI_SUBCLASS_SPECIFIC | 0x00000000)
#define EFI_IOB_ATA_BUS_SMART_DISABLE         (EFI_SUBCLASS_SPECIFIC | 0x00000001)
#define EFI_IOB_ATA_BUS_SMART_OVERTHRESHOLD   (EFI_SUBCLASS_SPECIFIC | 0x00000002)
#define EFI_IOB_ATA_BUS_SMART_UNDERTHRESHOLD  (EFI_SUBCLASS_SPECIFIC | 0x00000003)

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

  ATAPI_INQUIRY_DATA                *pInquiryData;
  EFI_IDENTIFY_DATA           *pIdData;
  ATA_PIO_MODE                PioMode;
  EFI_ATA_MODE                UdmaMode;
  CHAR8                       ModelName[41];
  ATAPI_REQUEST_SENSE_DATA          *SenseData;
  UINT8                       SenseDataNumber;
  UINT8                       *Cache;

  //
  // ExitBootService Event, it is used to clear pending IDE interrupt
  //
  EFI_EVENT                   ExitBootServiceEvent;

  EFI_UNICODE_STRING_TABLE    *ControllerNameTable;
} IDE_BLK_IO_DEV;

#include "ComponentName.h"

#define IDE_BLOCK_IO_DEV_FROM_THIS(a)           CR (a, IDE_BLK_IO_DEV, BlkIo, IDE_BLK_IO_DEV_SIGNATURE)
#define IDE_BLOCK_IO_DEV_FROM_DISK_INFO_THIS(a) CR (a, IDE_BLK_IO_DEV, DiskInfo, IDE_BLK_IO_DEV_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL      gIDEBusDriverBinding;
extern EFI_DRIVER_DIAGNOSTICS_PROTOCOL  gIDEBusDriverDiagnostics;
extern EFI_DRIVER_DIAGNOSTICS2_PROTOCOL gIDEBusDriverDiagnostics2;

#include "Ide.h"

//
// Prototypes
// Driver model protocol interface
//
/**
  TODO: Add function description

  @param  ImageHandle TODO: add argument description
  @param  SystemTable TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
IDEBusControllerDriverEntryPoint (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
;

/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  Controller TODO: add argument description
  @param  RemainingDevicePath TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
IDEBusDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
;

/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  Controller TODO: add argument description
  @param  RemainingDevicePath TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
IDEBusDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
;

/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  Controller TODO: add argument description
  @param  NumberOfChildren TODO: add argument description
  @param  ChildHandleBuffer TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
IDEBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL *This,
  IN  EFI_HANDLE                  Controller,
  IN  UINTN                       NumberOfChildren,
  IN  EFI_HANDLE                  *ChildHandleBuffer
  )
;

//
// EFI Driver Configuration Functions
//
EFI_STATUS
IDEBusDriverConfigurationSetOptions (
  IN  EFI_DRIVER_CONFIGURATION_PROTOCOL                      *This,
  IN  EFI_HANDLE                                             ControllerHandle,
  IN  EFI_HANDLE                                             ChildHandle  OPTIONAL,
  IN  CHAR8                                                  *Language,
  OUT EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED               *ActionRequired
  );

EFI_STATUS
IDEBusDriverConfigurationOptionsValid (
  IN  EFI_DRIVER_CONFIGURATION_PROTOCOL               *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle  OPTIONAL
  );

EFI_STATUS
IDEBusDriverConfigurationForceDefaults (
  IN  EFI_DRIVER_CONFIGURATION_PROTOCOL                      *This,
  IN  EFI_HANDLE                                             ControllerHandle,
  IN  EFI_HANDLE                                             ChildHandle  OPTIONAL,
  IN  UINT32                                                 DefaultType,
  OUT EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED               *ActionRequired
  );

//
// EFI Driver Diagnostics Functions
//
EFI_STATUS
IDEBusDriverDiagnosticsRunDiagnostics (
  IN  EFI_DRIVER_DIAGNOSTICS_PROTOCOL               *This,
  IN  EFI_HANDLE                                    ControllerHandle,
  IN  EFI_HANDLE                                    ChildHandle  OPTIONAL,
  IN  EFI_DRIVER_DIAGNOSTIC_TYPE                    DiagnosticType,
  IN  CHAR8                                         *Language,
  OUT EFI_GUID                                      **ErrorType,
  OUT UINTN                                         *BufferSize,
  OUT CHAR16                                        **Buffer
  );

//
// Block I/O Protocol Interface
//
/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  ExtendedVerification TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
IDEBlkIoReset (
  IN  EFI_BLOCK_IO_PROTOCOL       *This,
  IN  BOOLEAN                     ExtendedVerification
  )
;

/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  MediaId TODO: add argument description
  @param  LBA TODO: add argument description
  @param  BufferSize TODO: add argument description
  @param  Buffer TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
IDEBlkIoReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL       *This,
  IN  UINT32                      MediaId,
  IN  EFI_LBA                     LBA,
  IN  UINTN                       BufferSize,
  OUT VOID                        *Buffer
  )
;

/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  MediaId TODO: add argument description
  @param  LBA TODO: add argument description
  @param  BufferSize TODO: add argument description
  @param  Buffer TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
IDEBlkIoWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL       *This,
  IN  UINT32                      MediaId,
  IN  EFI_LBA                     LBA,
  IN  UINTN                       BufferSize,
  IN  VOID                        *Buffer
  )
;

/**
  TODO: Add function description

  @param  This TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
IDEBlkIoFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL       *This
  )
;

/**
  TODO: Add function description

  @param  PciIo TODO: add argument description
  @param  Enable TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
IDERegisterDecodeEnableorDisable (
  IN  EFI_PCI_IO_PROTOCOL         *PciIo,
  IN  BOOLEAN                     Enable
  )
;

/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  InquiryData TODO: add argument description
  @param  IntquiryDataSize TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
IDEDiskInfoInquiry (
  IN EFI_DISK_INFO_PROTOCOL       *This,
  IN OUT VOID                     *InquiryData,
  IN OUT UINT32                   *IntquiryDataSize
  )
;

/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  IdentifyData TODO: add argument description
  @param  IdentifyDataSize TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
IDEDiskInfoIdentify (
  IN EFI_DISK_INFO_PROTOCOL       *This,
  IN OUT VOID                     *IdentifyData,
  IN OUT UINT32                   *IdentifyDataSize
  )
;

/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  SenseData TODO: add argument description
  @param  SenseDataSize TODO: add argument description
  @param  SenseDataNumber TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
IDEDiskInfoSenseData (
  IN EFI_DISK_INFO_PROTOCOL       *This,
  IN OUT VOID                     *SenseData,
  IN OUT UINT32                   *SenseDataSize,
  OUT UINT8                       *SenseDataNumber
  )
;

/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  IdeChannel TODO: add argument description
  @param  IdeDevice TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EFIAPI
IDEDiskInfoWhichIde (
  IN EFI_DISK_INFO_PROTOCOL       *This,
  OUT UINT32                      *IdeChannel,
  OUT UINT32                      *IdeDevice
  )
;

#endif

/** @file
BOT Transportation implementation.

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
  
This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_BOT_PEIM_H_
#define _PEI_BOT_PEIM_H_


#include <PiPei.h>

#include <Ppi/UsbIo.h>
#include <Ppi/UsbHostController.h>
#include <Ppi/BlockIo.h>

//#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>

#include <IndustryStandard/Atapi.h>

#pragma pack(1)
//
// Bulk Only device protocol
//
typedef struct {
  UINT32  Signature;
  UINT32  Tag;
  UINT32  DataTransferLength;
  UINT8   Flags;
  UINT8   Lun;
  UINT8   CmdLen;
  UINT8   CmdBlock[16];
} CBW;

typedef struct {
  UINT32  Signature;
  UINT32  Tag;
  UINT32  DataResidue;
  UINT8   Status;
} CSW;

#pragma pack()
//
// Status code, see Usb Bot device spec
//
#define CSWSIG  0x53425355
#define CBWSIG  0x43425355

/**
  Sends out ATAPI Inquiry Packet Command to the specified device. This command will
  return INQUIRY data of the device.

  @param PeiServices    The pointer of EFI_PEI_SERVICES.
  @param PeiBotDevice   The pointer to PEI_BOT_DEVICE instance.

  @retval EFI_SUCCESS       Inquiry command completes successfully.
  @retval EFI_DEVICE_ERROR  Inquiry command failed.

**/
EFI_STATUS
PeiUsbInquiry (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  PEI_BOT_DEVICE    *PeiBotDevice
  );

/**
  Sends out ATAPI Test Unit Ready Packet Command to the specified device
  to find out whether device is accessible.

  @param PeiServices    The pointer of EFI_PEI_SERVICES.
  @param PeiBotDevice   The pointer to PEI_BOT_DEVICE instance.

  @retval EFI_SUCCESS        TestUnit command executed successfully.
  @retval EFI_DEVICE_ERROR   Device cannot be executed TestUnit command successfully.

**/
EFI_STATUS
PeiUsbTestUnitReady (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  PEI_BOT_DEVICE    *PeiBotDevice
  );

/**
  Sends out ATAPI Request Sense Packet Command to the specified device.

  @param PeiServices    The pointer of EFI_PEI_SERVICES.
  @param PeiBotDevice   The pointer to PEI_BOT_DEVICE instance.
  @param SenseCounts    Length of sense buffer.
  @param SenseKeyBuffer Pointer to sense buffer.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
PeiUsbRequestSense (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  PEI_BOT_DEVICE    *PeiBotDevice,
  OUT UINTN             *SenseCounts,
  IN  UINT8             *SenseKeyBuffer
  );

/**
  Sends out ATAPI Read Capacity Packet Command to the specified device.
  This command will return the information regarding the capacity of the
  media in the device.

  @param PeiServices    The pointer of EFI_PEI_SERVICES.
  @param PeiBotDevice   The pointer to PEI_BOT_DEVICE instance.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
PeiUsbReadCapacity (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  PEI_BOT_DEVICE    *PeiBotDevice
  );

/**
  Sends out ATAPI Read Format Capacity Data Command to the specified device.
  This command will return the information regarding the capacity of the
  media in the device.

  @param PeiServices    The pointer of EFI_PEI_SERVICES.
  @param PeiBotDevice   The pointer to PEI_BOT_DEVICE instance.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
PeiUsbReadFormattedCapacity (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  PEI_BOT_DEVICE    *PeiBotDevice
  );

/**
  Execute Read(10) ATAPI command on a specific SCSI target.

  Executes the ATAPI Read(10) command on the ATAPI target specified by PeiBotDevice.

  @param PeiServices       The pointer of EFI_PEI_SERVICES.
  @param PeiBotDevice      The pointer to PEI_BOT_DEVICE instance.
  @param Buffer            The pointer to data buffer.
  @param Lba               The start logic block address of reading.
  @param NumberOfBlocks    The block number of reading.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
PeiUsbRead10 (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  PEI_BOT_DEVICE    *PeiBotDevice,
  IN  VOID              *Buffer,
  IN  EFI_PEI_LBA       Lba,
  IN  UINTN             NumberOfBlocks
  );

/**  
  Check if there is media according to sense data.

  @param  SenseData   Pointer to sense data.
  @param  SenseCounts Count of sense data.

  @retval TRUE    No media
  @retval FALSE   Media exists

**/
BOOLEAN
IsNoMedia (
  IN  ATAPI_REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                 SenseCounts
  );

/**  
  Check if there is media error according to sense data.

  @param  SenseData   Pointer to sense data.
  @param  SenseCounts Count of sense data.

  @retval TRUE    Media error
  @retval FALSE   No media error

**/
BOOLEAN
IsMediaError (
  IN  ATAPI_REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                 SenseCounts
  );

/**  
  Check if media is changed according to sense data.

  @param  SenseData   Pointer to sense data.
  @param  SenseCounts Count of sense data.

  @retval TRUE    There is media change event.
  @retval FALSE   media is NOT changed.

**/
BOOLEAN
IsMediaChange (
  IN  ATAPI_REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                 SenseCounts
  );

#endif

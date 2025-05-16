/** @file
Usb BOT Peim definition.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PEI_USB_BOT_PEIM_H_
#define _PEI_USB_BOT_PEIM_H_

#include <PiPei.h>

#include <Ppi/UsbIo.h>
#include <Ppi/BlockIo.h>
#include <Ppi/BlockIo2.h>

#include <Library/DebugLib.h>

#include <IndustryStandard/Usb.h>
#include <IndustryStandard/Atapi.h>

#define PEI_FAT_MAX_USB_IO_PPI  127

/**
  Gets the count of block I/O devices that one specific block driver detects.

  This function is used for getting the count of block I/O devices that one
  specific block driver detects.  To the PEI ATAPI driver, it returns the number
  of all the detected ATAPI devices it detects during the enumeration process.
  To the PEI legacy floppy driver, it returns the number of all the legacy
  devices it finds during its enumeration process. If no device is detected,
  then the function will return zero.

  @param[in]  PeiServices          General-purpose services that are available
                                   to every PEIM.
  @param[in]  This                 Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI
                                   instance.
  @param[out] NumberBlockDevices   The number of block I/O devices discovered.

  @retval     EFI_SUCCESS          Operation performed successfully.

**/
EFI_STATUS
EFIAPI
BotGetNumberOfBlockDevices (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  OUT UINTN                          *NumberBlockDevices
  );

/**
  Gets a block device's media information.

  This function will provide the caller with the specified block device's media
  information. If the media changes, calling this function will update the media
  information accordingly.

  @param[in]  PeiServices   General-purpose services that are available to every
                            PEIM
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI instance.
  @param[in]  DeviceIndex   Specifies the block device to which the function wants
                            to talk. Because the driver that implements Block I/O
                            PPIs will manage multiple block devices, the PPIs that
                            want to talk to a single device must specify the
                            device index that was assigned during the enumeration
                            process. This index is a number from one to
                            NumberBlockDevices.
  @param[out] MediaInfo     The media information of the specified block media.
                            The caller is responsible for the ownership of this
                            data structure.

  @retval EFI_SUCCESS        Media information about the specified block device
                             was obtained successfully.
  @retval EFI_DEVICE_ERROR   Cannot get the media information due to a hardware
                             error.

**/
EFI_STATUS
EFIAPI
BotGetMediaInfo (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  IN  UINTN                          DeviceIndex,
  OUT EFI_PEI_BLOCK_IO_MEDIA         *MediaInfo
  );

/**
  Reads the requested number of blocks from the specified block device.

  The function reads the requested number of blocks from the device. All the
  blocks are read, or an error is returned. If there is no media in the device,
  the function returns EFI_NO_MEDIA.

  @param[in]  PeiServices   General-purpose services that are available to
                            every PEIM.
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI instance.
  @param[in]  DeviceIndex   Specifies the block device to which the function wants
                            to talk. Because the driver that implements Block I/O
                            PPIs will manage multiple block devices, the PPIs that
                            want to talk to a single device must specify the device
                            index that was assigned during the enumeration process.
                            This index is a number from one to NumberBlockDevices.
  @param[in]  StartLBA      The starting logical block address (LBA) to read from
                            on the device
  @param[in]  BufferSize    The size of the Buffer in bytes. This number must be
                            a multiple of the intrinsic block size of the device.
  @param[out] Buffer        A pointer to the destination buffer for the data.
                            The caller is responsible for the ownership of the
                            buffer.

  @retval EFI_SUCCESS             The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the read operation.
  @retval EFI_INVALID_PARAMETER   The read request contains LBAs that are not
                                  valid, or the buffer is not properly aligned.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_BAD_BUFFER_SIZE     The BufferSize parameter is not a multiple of
                                  the intrinsic block size of the device.

**/
EFI_STATUS
EFIAPI
BotReadBlocks (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  IN  UINTN                          DeviceIndex,
  IN  EFI_PEI_LBA                    StartLBA,
  IN  UINTN                          BufferSize,
  OUT VOID                           *Buffer
  );

/**
  Gets the count of block I/O devices that one specific block driver detects.

  This function is used for getting the count of block I/O devices that one
  specific block driver detects.  To the PEI ATAPI driver, it returns the number
  of all the detected ATAPI devices it detects during the enumeration process.
  To the PEI legacy floppy driver, it returns the number of all the legacy
  devices it finds during its enumeration process. If no device is detected,
  then the function will return zero.

  @param[in]  PeiServices          General-purpose services that are available
                                   to every PEIM.
  @param[in]  This                 Indicates the EFI_PEI_RECOVERY_BLOCK_IO2_PPI
                                   instance.
  @param[out] NumberBlockDevices   The number of block I/O devices discovered.

  @retval     EFI_SUCCESS          Operation performed successfully.

**/
EFI_STATUS
EFIAPI
BotGetNumberOfBlockDevices2 (
  IN  EFI_PEI_SERVICES                **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO2_PPI  *This,
  OUT UINTN                           *NumberBlockDevices
  );

/**
  Gets a block device's media information.

  This function will provide the caller with the specified block device's media
  information. If the media changes, calling this function will update the media
  information accordingly.

  @param[in]  PeiServices   General-purpose services that are available to every
                            PEIM
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_BLOCK_IO2_PPI instance.
  @param[in]  DeviceIndex   Specifies the block device to which the function wants
                            to talk. Because the driver that implements Block I/O
                            PPIs will manage multiple block devices, the PPIs that
                            want to talk to a single device must specify the
                            device index that was assigned during the enumeration
                            process. This index is a number from one to
                            NumberBlockDevices.
  @param[out] MediaInfo     The media information of the specified block media.
                            The caller is responsible for the ownership of this
                            data structure.

  @retval EFI_SUCCESS        Media information about the specified block device
                             was obtained successfully.
  @retval EFI_DEVICE_ERROR   Cannot get the media information due to a hardware
                             error.

**/
EFI_STATUS
EFIAPI
BotGetMediaInfo2 (
  IN  EFI_PEI_SERVICES                **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO2_PPI  *This,
  IN  UINTN                           DeviceIndex,
  OUT EFI_PEI_BLOCK_IO2_MEDIA         *MediaInfo
  );

/**
  Reads the requested number of blocks from the specified block device.

  The function reads the requested number of blocks from the device. All the
  blocks are read, or an error is returned. If there is no media in the device,
  the function returns EFI_NO_MEDIA.

  @param[in]  PeiServices   General-purpose services that are available to
                            every PEIM.
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_BLOCK_IO2_PPI instance.
  @param[in]  DeviceIndex   Specifies the block device to which the function wants
                            to talk. Because the driver that implements Block I/O
                            PPIs will manage multiple block devices, the PPIs that
                            want to talk to a single device must specify the device
                            index that was assigned during the enumeration process.
                            This index is a number from one to NumberBlockDevices.
  @param[in]  StartLBA      The starting logical block address (LBA) to read from
                            on the device
  @param[in]  BufferSize    The size of the Buffer in bytes. This number must be
                            a multiple of the intrinsic block size of the device.
  @param[out] Buffer        A pointer to the destination buffer for the data.
                            The caller is responsible for the ownership of the
                            buffer.

  @retval EFI_SUCCESS             The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the read operation.
  @retval EFI_INVALID_PARAMETER   The read request contains LBAs that are not
                                  valid, or the buffer is not properly aligned.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_BAD_BUFFER_SIZE     The BufferSize parameter is not a multiple of
                                  the intrinsic block size of the device.

**/
EFI_STATUS
EFIAPI
BotReadBlocks2 (
  IN  EFI_PEI_SERVICES                **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO2_PPI  *This,
  IN  UINTN                           DeviceIndex,
  IN  EFI_PEI_LBA                     StartLBA,
  IN  UINTN                           BufferSize,
  OUT VOID                            *Buffer
  );

/**
  UsbIo installation notification function.

  This function finds out all the current USB IO PPIs in the system and add them
  into private data.

  @param  PeiServices      Indirect reference to the PEI Services Table.
  @param  NotifyDesc       Address of the notification descriptor data structure.
  @param  InvokePpi        Address of the PPI that was invoked.

  @retval EFI_SUCCESS      The function completes successfully.

**/
EFI_STATUS
EFIAPI
NotifyOnUsbIoPpi (
  IN  EFI_PEI_SERVICES           **PeiServices,
  IN  EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc,
  IN  VOID                       *InvokePpi
  );

/**
  Initialize the usb bot device.

  @param[in]  PeiServices   General-purpose services that are available to every
                            PEIM.
  @param[in]  UsbIoPpi      Indicates the PEI_USB_IO_PPI instance.

  @retval EFI_SUCCESS       The usb bot device is initialized successfully.
  @retval Other             Failed to initialize media.

**/
EFI_STATUS
InitUsbBot (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  PEI_USB_IO_PPI    *UsbIoPpi
  );

#define USBCDROM  1   // let the device type value equal to USBCDROM, which is defined by PI spec.
                      // Therefore the CdExpressPei module can do recovery on UsbCdrom.
#define USBFLOPPY   2 // for those that use ReadCapacity(0x25) command to retrieve media capacity
#define USBFLOPPY2  3 // for those that use ReadFormatCapacity(0x23) command to retrieve media capacity

//
// Bot device structure
//
#define PEI_BOT_DEVICE_SIGNATURE  SIGNATURE_32 ('U', 'B', 'O', 'T')
typedef struct {
  UINTN                             Signature;
  EFI_PEI_RECOVERY_BLOCK_IO_PPI     BlkIoPpi;
  EFI_PEI_RECOVERY_BLOCK_IO2_PPI    BlkIo2Ppi;
  EFI_PEI_PPI_DESCRIPTOR            BlkIoPpiList;
  EFI_PEI_PPI_DESCRIPTOR            BlkIo2PpiList;
  EFI_PEI_BLOCK_IO_MEDIA            Media;
  EFI_PEI_BLOCK_IO2_MEDIA           Media2;
  PEI_USB_IO_PPI                    *UsbIoPpi;
  EFI_USB_INTERFACE_DESCRIPTOR      *BotInterface;
  EFI_USB_ENDPOINT_DESCRIPTOR       *BulkInEndpoint;
  EFI_USB_ENDPOINT_DESCRIPTOR       *BulkOutEndpoint;
  UINTN                             AllocateAddress;
  UINTN                             DeviceType;
  ATAPI_REQUEST_SENSE_DATA          *SensePtr;
} PEI_BOT_DEVICE;

#define PEI_BOT_DEVICE_FROM_THIS(a)   CR (a, PEI_BOT_DEVICE, BlkIoPpi, PEI_BOT_DEVICE_SIGNATURE)
#define PEI_BOT_DEVICE2_FROM_THIS(a)  CR (a, PEI_BOT_DEVICE, BlkIo2Ppi, PEI_BOT_DEVICE_SIGNATURE)

/**
  Send ATAPI command using BOT protocol.

  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  PeiBotDev              The instance to PEI_BOT_DEVICE.
  @param  Command                The command to be sent to ATAPI device.
  @param  CommandSize            The length of the data to be sent.
  @param  DataBuffer             The pointer to the data.
  @param  BufferLength           The length of the data.
  @param  Direction              The direction of the data.
  @param  TimeOutInMilliSeconds  Indicates the maximum time, in millisecond, which the
                                 transfer is allowed to complete.

  @retval EFI_DEVICE_ERROR       Successful to get the status of device.
  @retval EFI_SUCCESS            Failed to get the status of device.

**/
EFI_STATUS
PeiAtapiCommand (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  PEI_BOT_DEVICE          *PeiBotDev,
  IN  VOID                    *Command,
  IN  UINT8                   CommandSize,
  IN  VOID                    *DataBuffer,
  IN  UINT32                  BufferLength,
  IN  EFI_USB_DATA_DIRECTION  Direction,
  IN  UINT16                  TimeOutInMilliSeconds
  );

#endif

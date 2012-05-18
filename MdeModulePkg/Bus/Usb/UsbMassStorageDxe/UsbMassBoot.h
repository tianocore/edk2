/** @file
  Definition of the command set of USB Mass Storage Specification
  for Bootability, Revision 1.0.

Copyright (c) 2007 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_USB_MASS_BOOT_H_
#define _EFI_USB_MASS_BOOT_H_

//
// The opcodes of various USB boot commands:
// INQUIRY/REQUEST_SENSE are "No Timeout Commands" as specified
// by Multi-Media Commands (MMC) set.
// Others are "Group 1 Timeout Commands". That is,
// they should be retried if driver is ready. 
//
#define USB_BOOT_INQUIRY_OPCODE         0x12
#define USB_BOOT_REQUEST_SENSE_OPCODE   0x03
#define USB_BOOT_MODE_SENSE10_OPCODE    0x5A
#define USB_BOOT_READ_CAPACITY_OPCODE   0x25
#define USB_BOOT_TEST_UNIT_READY_OPCODE 0x00
#define USB_BOOT_READ10_OPCODE          0x28
#define USB_BOOT_WRITE10_OPCODE         0x2A

#define USB_SCSI_MODE_SENSE6_OPCODE     0x1A

//
// The Sense Key part of the sense data. Sense data has three levels:
// Sense key, Additional Sense Code and Additional Sense Code Qualifier
//
#define USB_BOOT_SENSE_NO_SENSE         0x00 ///< No sense key
#define USB_BOOT_SENSE_RECOVERED        0x01 ///< Last command succeed with recovery actions
#define USB_BOOT_SENSE_NOT_READY        0x02 ///< Device not ready
#define USB_BOOT_SNESE_MEDIUM_ERROR     0X03 ///< Failed probably because flaw in the media
#define USB_BOOT_SENSE_HARDWARE_ERROR   0X04 ///< Non-recoverable hardware failure
#define USB_BOOT_SENSE_ILLEGAL_REQUEST  0X05 ///< Illegal parameters in the request
#define USB_BOOT_SENSE_UNIT_ATTENTION   0X06 ///< Removable medium may have been changed
#define USB_BOOT_SENSE_DATA_PROTECT     0X07 ///< Write protected
#define USB_BOOT_SENSE_BLANK_CHECK      0X08 ///< Blank/non-blank medium while reading/writing
#define USB_BOOT_SENSE_VENDOR           0X09 ///< Vendor specific sense key
#define USB_BOOT_SENSE_ABORTED          0X0B ///< Command aborted by the device
#define USB_BOOT_SENSE_VOLUME_OVERFLOW  0x0D ///< Partition overflow
#define USB_BOOT_SENSE_MISCOMPARE       0x0E ///< Source data mis-match while verfying.

#define USB_BOOT_ASC_NOT_READY          0x04
#define USB_BOOT_ASC_NO_MEDIA           0x3A
#define USB_BOOT_ASC_MEDIA_CHANGE       0x28

//
// Supported PDT codes, or Peripheral Device Type
//
#define USB_PDT_DIRECT_ACCESS           0x00       ///< Direct access device
#define USB_PDT_CDROM                   0x05       ///< CDROM
#define USB_PDT_OPTICAL                 0x07       ///< Non-CD optical disks
#define USB_PDT_SIMPLE_DIRECT           0x0E       ///< Simplified direct access device

//
// Other parameters, Max carried size is 512B * 128 = 64KB
//
#define USB_BOOT_IO_BLOCKS              128

//
// Retry mass command times, set by experience
//
#define USB_BOOT_COMMAND_RETRY          5

//
// Wait for unit ready command, set by experience
//
#define USB_BOOT_RETRY_UNIT_READY_STALL (500 * USB_MASS_1_MILLISECOND)

//
// Mass command timeout, refers to specification[USB20-9.2.6.1]
//
// USB2.0 Spec define the up-limit timeout 5s for all command. USB floppy, 
// USB CD-Rom and iPod devices are much slower than USB key when reponse 
// most of commands, So we set 5s as timeout here.
// 
#define USB_BOOT_GENERAL_CMD_TIMEOUT    (5 * USB_MASS_1_SECOND)

//
// The required commands are INQUIRY, READ CAPACITY, TEST UNIT READY,
// READ10, WRITE10, and REQUEST SENSE. The BLOCK_IO protocol uses LBA
// so it isn't necessary to issue MODE SENSE / READ FORMAT CAPACITY
// command to retrieve the disk gemotrics. 
//
#pragma pack(1)
typedef struct {
  UINT8             OpCode;
  UINT8             Lun;            ///< Lun (high 3 bits)
  UINT8             Reserved0[2];
  UINT8             AllocLen;
  UINT8             Reserved1;
  UINT8             Pad[6];
} USB_BOOT_INQUIRY_CMD;

typedef struct {
  UINT8             Pdt;            ///< Peripheral Device Type (low 5 bits)
  UINT8             Removable;      ///< Removable Media (highest bit)
  UINT8             Reserved0[2];
  UINT8             AddLen;         ///< Additional length
  UINT8             Reserved1[3];
  UINT8             VendorID[8];
  UINT8             ProductID[16];
  UINT8             ProductRevision[4];
} USB_BOOT_INQUIRY_DATA;

typedef struct {
  UINT8             OpCode;
  UINT8             Lun;
  UINT8             Reserved0[8];
  UINT8             Pad[2];
} USB_BOOT_READ_CAPACITY_CMD;

typedef struct {
  UINT8             LastLba[4];
  UINT8             BlockLen[4];
} USB_BOOT_READ_CAPACITY_DATA;

typedef struct {
  UINT8             OpCode;
  UINT8             Lun;
  UINT8             Reserved[4];
  UINT8             Pad[6];
} USB_BOOT_TEST_UNIT_READY_CMD;

typedef struct {
  UINT8             OpCode;
  UINT8             Lun;
  UINT8             PageCode;
  UINT8             Reserved0[4];
  UINT8             ParaListLenMsb;
  UINT8             ParaListLenLsb;
  UINT8             Reserved1;
  UINT8             Pad[2];
} USB_BOOT_MODE_SENSE10_CMD;

typedef struct {
  UINT8             ModeDataLenMsb;
  UINT8             ModeDataLenLsb;
  UINT8             Reserved0[4];
  UINT8             BlkDesLenMsb;
  UINT8             BlkDesLenLsb;
} USB_BOOT_MODE_SENSE10_PARA_HEADER;

typedef struct {
  UINT8             OpCode;
  UINT8             Lun;            ///< Lun (High 3 bits)
  UINT8             Lba[4];         ///< Logical block address
  UINT8             Reserved0;
  UINT8             TransferLen[2]; ///< Transfer length
  UINT8             Reserverd1;
  UINT8             Pad[2];
} USB_BOOT_READ10_CMD;

typedef struct {
  UINT8             OpCode;
  UINT8             Lun;
  UINT8             Lba[4];
  UINT8             Reserved0;
  UINT8             TransferLen[2];
  UINT8             Reserverd1;
  UINT8             Pad[2];
} USB_BOOT_WRITE10_CMD;

typedef struct {
  UINT8             OpCode;
  UINT8             Lun;            ///< Lun (High 3 bits)
  UINT8             Reserved0[2];
  UINT8             AllocLen;       ///< Allocation length
  UINT8             Reserved1;
  UINT8             Pad[6];
} USB_BOOT_REQUEST_SENSE_CMD;

typedef struct {
  UINT8             ErrorCode;
  UINT8             Reserved0;
  UINT8             SenseKey;       ///< Sense key (low 4 bits)
  UINT8             Infor[4];
  UINT8             AddLen;         ///< Additional Sense length, 10
  UINT8             Reserved1[4];
  UINT8             Asc;            ///< Additional Sense Code
  UINT8             Ascq;           ///< Additional Sense Code Qualifier
  UINT8             Reserverd2[4];
} USB_BOOT_REQUEST_SENSE_DATA;

typedef struct {
  UINT8             OpCode;
  UINT8             Lun;
  UINT8             PageCode;
  UINT8             Reserved0;
  UINT8             AllocateLen;
  UINT8             Control;
} USB_SCSI_MODE_SENSE6_CMD;

typedef struct {
  UINT8             ModeDataLen;
  UINT8             MediumType;
  UINT8             DevicePara;
  UINT8             BlkDesLen;
} USB_SCSI_MODE_SENSE6_PARA_HEADER;
#pragma pack()

//
// Convert a LUN number to that in the command
//
#define USB_BOOT_LUN(Lun) ((Lun) << 5)

//
// Get the removable, PDT, and sense key bits from the command data
//
#define USB_BOOT_REMOVABLE(RmbByte) (((RmbByte) & BIT7) != 0)
#define USB_BOOT_PDT(Pdt)           ((Pdt) & 0x1f)
#define USB_BOOT_SENSE_KEY(Key)     ((Key) & 0x0f)

/**
  Get the parameters for the USB mass storage media.

  This function get the parameters for the USB mass storage media,
  It is used both to initialize the media during the Start() phase
  of Driver Binding Protocol and to re-initialize it when the media is
  changed. Althought the RemoveableMedia is unlikely to change,
  it is also included here.

  @param  UsbMass                The device to retrieve disk gemotric.

  @retval EFI_SUCCESS            The disk gemotric is successfully retrieved.
  @retval Other                  Failed to get the parameters.

**/
EFI_STATUS
UsbBootGetParams (
  IN USB_MASS_DEVICE          *UsbMass
  );

/**
  Execute TEST UNIT READY command to check if the device is ready.

  @param  UsbMass                The device to test

  @retval EFI_SUCCESS            The device is ready.
  @retval Others                 Device not ready.

**/
EFI_STATUS
UsbBootIsUnitReady (
  IN USB_MASS_DEVICE          *UsbMass
  );

/**
  Detect whether the removable media is present and whether it has changed.

  @param  UsbMass                The device to check.

  @retval EFI_SUCCESS            The media status is successfully checked.
  @retval Other                  Failed to detect media.

**/
EFI_STATUS
UsbBootDetectMedia (
  IN  USB_MASS_DEVICE       *UsbMass
  );

/**
  Read some blocks from the device.

  @param  UsbMass                The USB mass storage device to read from
  @param  Lba                    The start block number
  @param  TotalBlock             Total block number to read
  @param  Buffer                 The buffer to read to

  @retval EFI_SUCCESS            Data are read into the buffer
  @retval Others                 Failed to read all the data

**/
EFI_STATUS
UsbBootReadBlocks (
  IN  USB_MASS_DEVICE         *UsbMass,
  IN  UINT32                  Lba,
  IN  UINTN                   TotalBlock,
  OUT UINT8                   *Buffer
  );

/**
  Write some blocks to the device.

  @param  UsbMass                The USB mass storage device to write to
  @param  Lba                    The start block number
  @param  TotalBlock             Total block number to write
  @param  Buffer                 Pointer to the source buffer for the data.

  @retval EFI_SUCCESS            Data are written into the buffer
  @retval Others                 Failed to write all the data

**/
EFI_STATUS
UsbBootWriteBlocks (
  IN  USB_MASS_DEVICE         *UsbMass,
  IN  UINT32                  Lba,
  IN  UINTN                   TotalBlock,
  IN  UINT8                   *Buffer
  );

/**
  Read some blocks from the device by SCSI 16 byte cmd.

  @param  UsbMass                The USB mass storage device to read from
  @param  Lba                    The start block number
  @param  TotalBlock             Total block number to read
  @param  Buffer                 The buffer to read to

  @retval EFI_SUCCESS            Data are read into the buffer
  @retval Others                 Failed to read all the data

**/
EFI_STATUS
UsbBootReadBlocks16 (
  IN  USB_MASS_DEVICE       *UsbMass,
  IN  UINT64                Lba,
  IN  UINTN                 TotalBlock,
  OUT UINT8                 *Buffer
  );

/**
  Write some blocks to the device by SCSI 16 byte cmd.

  @param  UsbMass                The USB mass storage device to write to
  @param  Lba                    The start block number
  @param  TotalBlock             Total block number to write
  @param  Buffer                 Pointer to the source buffer for the data.

  @retval EFI_SUCCESS            Data are written into the buffer
  @retval Others                 Failed to write all the data

**/
EFI_STATUS
UsbBootWriteBlocks16 (
  IN  USB_MASS_DEVICE         *UsbMass,
  IN  UINT64                  Lba,
  IN  UINTN                   TotalBlock,
  IN  UINT8                   *Buffer
  );


/**
  Use the USB clear feature control transfer to clear the endpoint stall condition.

  @param  UsbIo                  The USB I/O Protocol instance
  @param  EndpointAddr           The endpoint to clear stall for

  @retval EFI_SUCCESS            The endpoint stall condition is cleared.
  @retval Others                 Failed to clear the endpoint stall condition.

**/
EFI_STATUS
UsbClearEndpointStall (
  IN EFI_USB_IO_PROTOCOL    *UsbIo,
  IN UINT8                  EndpointAddr
  );

#endif


/** @file

The definition for SD media device driver model and blkio protocol routines.

Copyright (c) 2013-2016 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SD_MEDIA_DEVICE_H_
#define _SD_MEDIA_DEVICE_H_


#include <Uefi.h>

#include <Protocol/PciIo.h>
#include <Protocol/BlockIo.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <IndustryStandard/Pci22.h>

#include "ComponentName.h"
#include "SDHostIo.h"


extern EFI_DRIVER_BINDING_PROTOCOL   gSDMediaDeviceDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gSDMediaDeviceName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gSDMediaDeviceName2;

//
// Define the region of memory used for DMA memory
//
#define DMA_MEMORY_TOP          0x0000000001FFFFFFULL

#define CARD_DATA_SIGNATURE  SIGNATURE_32 ('c', 'a', 'r', 'd')

//
// Command timeout will be max 100 ms
//
#define  TIMEOUT_COMMAND     100
#define  TIMEOUT_DATA        5000

typedef enum{
  UnknownCard = 0,
  MMCCard,                // MMC card
  MMCCardHighCap,          // MMC Card High Capacity
  CEATACard,              // CE-ATA device
  SDMemoryCard,           // SD 1.1 card
  SDMemoryCard2,          // SD 2.0 or above standard card
  SDMemoryCard2High       // SD 2.0 or above high capacity card
}CARD_TYPE;


typedef struct {
  //
  //BlockIO
  //
  UINTN                     Signature;
  EFI_BLOCK_IO_PROTOCOL     BlockIo;

  EFI_BLOCK_IO_MEDIA        BlockIoMedia;

  EFI_SD_HOST_IO_PROTOCOL   *SDHostIo;
  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;
  CARD_TYPE                 CardType;

  UINT8                     CurrentBusWidth;
  BOOLEAN                   DualVoltage;
  BOOLEAN                   NeedFlush;
  UINT8                     Reserved[3];

  UINT16                    Address;
  UINT32                    BlockLen;
  UINT32                    MaxFrequency;
  UINT64                    BlockNumber;
  //
  //Common used
  //
  CARD_STATUS               CardStatus;
  OCR                       OCRRegister;
  CID                       CIDRegister;
  CSD                       CSDRegister;
  EXT_CSD                   ExtCSDRegister;
  UINT8                     *RawBufferPointer;
  UINT8                     *AlignedBuffer;
  //
  //CE-ATA specific
  //
  TASK_FILE                 TaskFile;
  IDENTIFY_DEVICE_DATA      IndentifyDeviceData;
  //
  //SD specific
  //
  SCR                       SCRRegister;
  SD_STATUS_REG             SDSattus;
  SWITCH_STATUS             SwitchStatus;
}CARD_DATA;

#define CARD_DATA_FROM_THIS(a) \
    CR(a, CARD_DATA, BlockIo, CARD_DATA_SIGNATURE)

/**
  Test to see if this driver supports ControllerHandle. Any
  ControllerHandle that has BlockIoProtocol installed will be supported.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          This driver supports this device.
  @return EFI_UNSUPPORTED      This driver does not support this device.

**/
EFI_STATUS
EFIAPI
SDMediaDeviceSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  );

/**
  Starting the SD Media Device Driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @retval EFI_SUCCESS          This driver supports this device.
  @retval EFI_UNSUPPORTED      This driver does not support this device.
  @retval EFI_DEVICE_ERROR     This driver cannot be started due to device Error.
                               EFI_OUT_OF_RESOURCES- Failed due to resource shortage.

**/
EFI_STATUS
EFIAPI
SDMediaDeviceStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  );

/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to stop driver on.
  @param  NumberOfChildren     Number of Children in the ChildHandleBuffer.
  @param  ChildHandleBuffer    List of handles for the children we need to stop.

  @return EFI_SUCCESS
  @return others

**/
EFI_STATUS
EFIAPI
SDMediaDeviceStop (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN UINTN                           NumberOfChildren,
  IN EFI_HANDLE                      *ChildHandleBuffer
  );

/**
  MMC/SD card init function

  @param  CardData             Pointer to CARD_DATA.

  @return EFI_SUCCESS
  @return others

**/
EFI_STATUS
MMCSDCardInit (
  IN  CARD_DATA    *CardData
  );

/**
  Send command by using Host IO protocol

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  CommandIndex          The command index to set the command index field of command register.
  @param  Argument              Command argument to set the argument field of command register.
  @param  DataType              TRANSFER_TYPE, indicates no data, data in or data out.
  @param  Buffer                Contains the data read from / write to the device.
  @param  BufferSize            The size of the buffer.
  @param  ResponseType          RESPONSE_TYPE.
  @param  TimeOut               Time out value in 1 ms unit.
  @param  ResponseData          Depending on the ResponseType, such as CSD or card status.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_UNSUPPORTED
  @retval EFI_DEVICE_ERROR

**/
EFI_STATUS
SendCommand (
  IN   CARD_DATA                  *CardData,
  IN   UINT16                     CommandIndex,
  IN   UINT32                     Argument,
  IN   TRANSFER_TYPE              DataType,
  IN   UINT8                      *Buffer, OPTIONAL
  IN   UINT32                     BufferSize,
  IN   RESPONSE_TYPE              ResponseType,
  IN   UINT32                     TimeOut,
  OUT  UINT32                     *ResponseData
  );

/**
  Send the card APP_CMD command with the following command indicated by CommandIndex

  @param  CardData              Pointer to CARD_DATA.
  @param  CommandIndex          The command index to set the command index field of command register.
  @param  Argument              Command argument to set the argument field of command register.
  @param  DataType              TRANSFER_TYPE, indicates no data, data in or data out.
  @param  Buffer                Contains the data read from / write to the device.
  @param  BufferSize            The size of the buffer.
  @param  ResponseType          RESPONSE_TYPE.
  @param  TimeOut               Time out value in 1 ms unit.
  @param  ResponseData          Depending on the ResponseType, such as CSD or card status.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER
  @retval EFI_UNSUPPORTED
  @retval EFI_DEVICE_ERROR

**/
EFI_STATUS
SendAppCommand (
  IN   CARD_DATA                  *CardData,
  IN   UINT16                     CommandIndex,
  IN   UINT32                     Argument,
  IN   TRANSFER_TYPE              DataType,
  IN   UINT8                      *Buffer, OPTIONAL
  IN   UINT32                     BufferSize,
  IN   RESPONSE_TYPE              ResponseType,
  IN   UINT32                     TimeOut,
  OUT  UINT32                     *ResponseData
  );

/**
  Send the card FAST_IO command

  @param  CardData               Pointer to CARD_DATA.
  @param  RegisterAddress        Register Address.
  @param  RegisterData           Pointer to register Data.
  @param  Write                  TRUE for write, FALSE for read.

  @retval EFI_SUCCESS
  @retval EFI_UNSUPPORTED
  @retval EFI_INVALID_PARAMETER
  @retval EFI_DEVICE_ERROR

**/
EFI_STATUS
FastIO (
  IN      CARD_DATA   *CardData,
  IN      UINT8       RegisterAddress,
  IN  OUT UINT8       *RegisterData,
  IN      BOOLEAN     Write
  );

/**
  Judge whether it is CE-ATA device or not.

  @param  CardData             Pointer to CARD_DATA.

  @retval TRUE
  @retval FALSE

**/
BOOLEAN
IsCEATADevice (
  IN  CARD_DATA    *CardData
  );

/**
  Send software reset

  @param  CardData             Pointer to CARD_DATA.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
SoftwareReset (
  IN  CARD_DATA    *CardData
  );

/**
  SendATACommand specificed in Taskfile

  @param  CardData             Pointer to CARD_DATA.
  @param  TaskFile             Pointer to TASK_FILE.
  @param  Write                TRUE means write, FALSE means read.
  @param  Buffer               If NULL, means no data transfer, neither read nor write.
  @param  SectorCount          Buffer size in 512 bytes unit.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
SendATACommand (
  IN  CARD_DATA   *CardData,
  IN  TASK_FILE   *TaskFile,
  IN  BOOLEAN     Write,
  IN  UINT8       *Buffer,
  IN  UINT16      SectorCount
  );

/**
  IDENTIFY_DEVICE command

  @param  CardData             Pointer to CARD_DATA.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
IndentifyDevice (
  IN  CARD_DATA    *CardData
  );

/**
  FLUSH_CACHE_EXT command

  @param  CardData             Pointer to CARD_DATA.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
FlushCache (
  IN  CARD_DATA    *CardData
  );

/**
  STANDBY_IMMEDIATE command

  @param  CardData             Pointer to CARD_DATA.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
StandByImmediate (
  IN  CARD_DATA    *CardData
  );

/**
  READ_DMA_EXT command

  @param  CardData             Pointer to CARD_DATA.
  @param  LBA                  The starting logical block address to read from on the device.
  @param  Buffer               A pointer to the destination buffer for the data. The caller
                               is responsible for either having implicit or explicit ownership
                               of the buffer.
  @param  SectorCount          Size in 512 bytes unit.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
ReadDMAExt (
  IN  CARD_DATA   *CardData,
  IN  EFI_LBA     LBA,
  IN  UINT8       *Buffer,
  IN  UINT16      SectorCount
  );

/**
  WRITE_DMA_EXT command

  @param  CardData             Pointer to CARD_DATA.
  @param  LBA                  The starting logical block address to read from on the device.
  @param  Buffer               A pointer to the destination buffer for the data. The caller
                               is responsible for either having implicit or explicit ownership
                               of the buffer.
  @param  SectorCount          Size in 512 bytes unit.

  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad

**/
EFI_STATUS
WriteDMAExt (
  IN  CARD_DATA   *CardData,
  IN  EFI_LBA     LBA,
  IN  UINT8       *Buffer,
  IN  UINT16      SectorCount
  );

/**
  CEATA card BlockIo init function.

  @param  CardData               Pointer to CARD_DATA.

  @retval EFI_SUCCESS
  @retval Others
**/
EFI_STATUS
CEATABlockIoInit (
  IN  CARD_DATA    *CardData
  );

/**
  MMC/SD card BlockIo init function.

  @param  CardData               Pointer to CARD_DATA.

  @retval EFI_SUCCESS
  @retval Others
**/
EFI_STATUS
MMCSDBlockIoInit (
  IN  CARD_DATA    *CardData
  );
#endif

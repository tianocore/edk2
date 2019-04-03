/** @file

The definition for SD host controller driver model and HC protocol routines.

Copyright (c) 2013-2016 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SD_CONTROLLER_H_
#define _SD_CONTROLLER_H_


#include <Uefi.h>


#include <Protocol/PciIo.h>

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


extern EFI_DRIVER_BINDING_PROTOCOL   gSDControllerDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gSDControllerName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gSDControllerName2;


#define SDHOST_DATA_SIGNATURE  SIGNATURE_32 ('s', 'd', 'h', 's')

#define BLOCK_SIZE   0x200
#define TIME_OUT_1S  1000

#pragma pack(1)
//
// PCI Class Code structure
//
typedef struct {
  UINT8 PI;
  UINT8 SubClassCode;
  UINT8 BaseCode;
} PCI_CLASSC;

#pragma pack()


typedef struct {
  UINTN                      Signature;
  EFI_SD_HOST_IO_PROTOCOL    SDHostIo;
  EFI_PCI_IO_PROTOCOL        *PciIo;
  BOOLEAN                    IsAutoStopCmd;
  UINT32                     BaseClockInMHz;
  UINT32                     CurrentClockInKHz;
  UINT32                     BlockLength;
  EFI_UNICODE_STRING_TABLE   *ControllerNameTable;
}SDHOST_DATA;

#define SDHOST_DATA_FROM_THIS(a) \
    CR(a, SDHOST_DATA, SDHostIo, SDHOST_DATA_SIGNATURE)

/**
  Test to see if this driver supports ControllerHandle. Any
  ControllerHandle that has SDHostIoProtocol installed will be supported.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          This driver supports this device.
  @return EFI_UNSUPPORTED      This driver does not support this device.

**/
EFI_STATUS
EFIAPI
SDControllerSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  );

/**
  Starting the SD Host Controller Driver.

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
SDControllerStart (
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
SDControllerStop (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN UINTN                           NumberOfChildren,
  IN EFI_HANDLE                      *ChildHandleBuffer
  );

/**
  The main function used to send the command to the card inserted into the SD host slot.
  It will assemble the arguments to set the command register and wait for the command
  and transfer completed until timeout. Then it will read the response register to fill
  the ResponseData.

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
  @retval EFI_OUT_OF_RESOURCES
  @retval EFI_TIMEOUT
  @retval EFI_DEVICE_ERROR

**/
EFI_STATUS
EFIAPI
SendCommand (
  IN   EFI_SD_HOST_IO_PROTOCOL    *This,
  IN   UINT16                     CommandIndex,
  IN   UINT32                     Argument,
  IN   TRANSFER_TYPE              DataType,
  IN   UINT8                      *Buffer, OPTIONAL
  IN   UINT32                     BufferSize,
  IN   RESPONSE_TYPE              ResponseType,
  IN   UINT32                     TimeOut,
  OUT  UINT32                     *ResponseData OPTIONAL
  );

/**
  Set max clock frequency of the host, the actual frequency may not be the same as MaxFrequency.
  It depends on the max frequency the host can support, divider, and host speed mode.

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  MaxFrequency          Max frequency in HZ.

  @retval EFI_SUCCESS
  @retval EFI_TIMEOUT

**/
EFI_STATUS
EFIAPI
SetClockFrequency (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     MaxFrequencyInKHz
  );

/**
  Set bus width of the host controller

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  BusWidth              Bus width in 1, 4, 8 bits.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
SetBusWidth (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     BusWidth
  );


/**
  Set voltage which could supported by the host controller.
  Support 0(Power off the host), 1.8V, 3.0V, 3.3V

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  Voltage               Units in 0.1 V.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI
SetHostVoltage (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     Voltage
  );


/**
  Reset the host controller.

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  ResetAll              TRUE to reset all.

  @retval EFI_SUCCESS
  @retval EFI_TIMEOUT

**/
EFI_STATUS
EFIAPI
ResetSDHost (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  RESET_TYPE                 ResetType
  );


/**
  Enable auto stop on the host controller.

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  Enable                TRUE to enable, FALSE to disable.

  @retval EFI_SUCCESS
  @retval EFI_TIMEOUT

**/
EFI_STATUS
EFIAPI
EnableAutoStopCmd (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  BOOLEAN                    Enable
  );

/**
  Find whether these is a card inserted into the slot. If so init the host.
  If not, return EFI_NOT_FOUND.

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND

**/
EFI_STATUS
EFIAPI
DetectCardAndInitHost (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This
  );

/**
  Set the Block length on the host controller.

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  BlockLength           card supportes block length.

  @retval EFI_SUCCESS
  @retval EFI_TIMEOUT

**/
EFI_STATUS
EFIAPI
SetBlockLength (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  UINT32                     BlockLength
  );

/**
  Enable/Disable High Speed transfer mode

  @param  This                  A pointer to the EFI_SD_HOST_IO_PROTOCOL instance.
  @param  Enable                TRUE to Enable, FALSE to Disable

  @return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
SetHighSpeedMode (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  BOOLEAN                    Enable
  );

EFI_STATUS
EFIAPI
SetDDRMode (
  IN  EFI_SD_HOST_IO_PROTOCOL    *This,
  IN  BOOLEAN                    Enable
  );
#endif

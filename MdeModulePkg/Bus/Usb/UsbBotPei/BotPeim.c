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

#include "UsbBotPeim.h"
#include "BotPeim.h"
#include "PeiUsbLib.h"

/**
  Reset the given usb device.

  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  PeiBotDev              The instance to PEI_BOT_DEVICE.

  @retval EFI_INVALID_PARAMETER  Can not get usb io ppi.
  @retval EFI_SUCCESS            Failed to reset the given usb device.

**/
EFI_STATUS
BotRecoveryReset (
  IN  EFI_PEI_SERVICES          **PeiServices,
  IN  PEI_BOT_DEVICE            *PeiBotDev
  )
{
  EFI_USB_DEVICE_REQUEST  DevReq;
  UINT32                  Timeout;
  PEI_USB_IO_PPI          *UsbIoPpi;
  UINT8                   EndpointAddr;
  EFI_STATUS              Status;

  UsbIoPpi = PeiBotDev->UsbIoPpi;

  if (UsbIoPpi == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  DevReq.RequestType  = 0x21;
  DevReq.Request      = 0xFF;
  DevReq.Value        = 0;
  DevReq.Index        = 0;
  DevReq.Length       = 0;

  Timeout             = 3000;

  Status = UsbIoPpi->UsbControlTransfer (
                      PeiServices,
                      UsbIoPpi,
                      &DevReq,
                      EfiUsbNoData,
                      Timeout,
                      NULL,
                      0
                      );

  //
  // clear bulk in endpoint stall feature
  //
  EndpointAddr = (PeiBotDev->BulkInEndpoint)->EndpointAddress;
  PeiUsbClearEndpointHalt (PeiServices, UsbIoPpi, EndpointAddr);

  //
  // clear bulk out endpoint stall feature
  //
  EndpointAddr = (PeiBotDev->BulkOutEndpoint)->EndpointAddress;
  PeiUsbClearEndpointHalt (PeiServices, UsbIoPpi, EndpointAddr);

  return Status;
}

/**
  Send the command to the device using Bulk-Out endpoint.

  This function sends the command to the device using Bulk-Out endpoint.
  BOT transfer is composed of three phases: Command, Data, and Status.
  This is the Command phase.

  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  PeiBotDev              The instance to PEI_BOT_DEVICE.
  @param  Command                The command to transfer to device.
  @param  CommandSize            The length of the command.
  @param  DataTransferLength     The expected length of the data.
  @param  Direction              The direction of the data.
  @param  Timeout                Indicates the maximum time, in millisecond, which the
                                 transfer is allowed to complete.

  @retval EFI_DEVICE_ERROR       Successful to send the command to device.
  @retval EFI_SUCCESS            Failed to send the command to device.

**/
EFI_STATUS
BotCommandPhase (
  IN  EFI_PEI_SERVICES          **PeiServices,
  IN  PEI_BOT_DEVICE            *PeiBotDev,
  IN  VOID                      *Command,
  IN  UINT8                     CommandSize,
  IN  UINT32                    DataTransferLength,
  IN  EFI_USB_DATA_DIRECTION    Direction,
  IN  UINT16                    Timeout
  )
{
  CBW             Cbw;
  EFI_STATUS      Status;
  PEI_USB_IO_PPI  *UsbIoPpi;
  UINTN           DataSize;

  UsbIoPpi = PeiBotDev->UsbIoPpi;

  ZeroMem (&Cbw, sizeof (CBW));

  //
  // Fill the command block, detailed see BOT spec
  //
  Cbw.Signature           = CBWSIG;
  Cbw.Tag                 = 0x01;
  Cbw.DataTransferLength  = DataTransferLength;
  Cbw.Flags               = (UINT8) ((Direction == EfiUsbDataIn) ? 0x80 : 0);
  Cbw.Lun                 = 0;
  Cbw.CmdLen              = CommandSize;

  CopyMem (Cbw.CmdBlock, Command, CommandSize);

  DataSize = sizeof (CBW);

  Status = UsbIoPpi->UsbBulkTransfer (
                      PeiServices,
                      UsbIoPpi,
                      (PeiBotDev->BulkOutEndpoint)->EndpointAddress,
                      (UINT8 *) &Cbw,
                      &DataSize,
                      Timeout
                      );
  if (EFI_ERROR (Status)) {
    //
    // Command phase fail, we need to recovery reset this device
    //
    BotRecoveryReset (PeiServices, PeiBotDev);
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Transfer the data between the device and host.

  This function transfers the data between the device and host.
  BOT transfer is composed of three phases: Command, Data, and Status.
  This is the Data phase.

  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  PeiBotDev              The instance to PEI_BOT_DEVICE.
  @param  DataSize               The length of the data.
  @param  DataBuffer             The pointer to the data.
  @param  Direction              The direction of the data.
  @param  Timeout                Indicates the maximum time, in millisecond, which the
                                 transfer is allowed to complete.

  @retval EFI_DEVICE_ERROR       Successful to send the data to device.
  @retval EFI_SUCCESS            Failed to send the data to device.

**/
EFI_STATUS
BotDataPhase (
  IN  EFI_PEI_SERVICES          **PeiServices,
  IN  PEI_BOT_DEVICE            *PeiBotDev,
  IN  UINT32                    *DataSize,
  IN  OUT VOID                  *DataBuffer,
  IN  EFI_USB_DATA_DIRECTION    Direction,
  IN  UINT16                    Timeout
  )
{
  EFI_STATUS      Status;
  PEI_USB_IO_PPI  *UsbIoPpi;
  UINT8           EndpointAddr;
  UINTN           Remain;
  UINTN           Increment;
  UINT32          MaxPacketLen;
  UINT8           *BufferPtr;
  UINTN           TransferredSize;

  UsbIoPpi        = PeiBotDev->UsbIoPpi;

  Remain          = *DataSize;
  BufferPtr       = (UINT8 *) DataBuffer;
  TransferredSize = 0;

  //
  // retrieve the the max packet length of the given endpoint
  //
  if (Direction == EfiUsbDataIn) {
    MaxPacketLen  = (PeiBotDev->BulkInEndpoint)->MaxPacketSize;
    EndpointAddr  = (PeiBotDev->BulkInEndpoint)->EndpointAddress;
  } else {
    MaxPacketLen  = (PeiBotDev->BulkOutEndpoint)->MaxPacketSize;
    EndpointAddr  = (PeiBotDev->BulkOutEndpoint)->EndpointAddress;
  }

  while (Remain > 0) {
    //
    // Using 15 packets to avoid Bitstuff error
    //
    if (Remain > 16 * MaxPacketLen) {
      Increment = 16 * MaxPacketLen;
    } else {
      Increment = Remain;
    }

    Status = UsbIoPpi->UsbBulkTransfer (
                        PeiServices,
                        UsbIoPpi,
                        EndpointAddr,
                        BufferPtr,
                        &Increment,
                        Timeout
                        );

    TransferredSize += Increment;

    if (EFI_ERROR (Status)) {
      PeiUsbClearEndpointHalt (PeiServices, UsbIoPpi, EndpointAddr);
      return Status;
    }

    BufferPtr += Increment;
    Remain -= Increment;
  }

  *DataSize = (UINT32) TransferredSize;

  return EFI_SUCCESS;
}

/**
  Get the command execution status from device.

  This function gets the command execution status from device.
  BOT transfer is composed of three phases: Command, Data, and Status.
  This is the Status phase.

  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  PeiBotDev              The instance to PEI_BOT_DEVICE.
  @param  TransferStatus         The status of the transaction.
  @param  Timeout                Indicates the maximum time, in millisecond, which the
                                 transfer is allowed to complete.

  @retval EFI_DEVICE_ERROR       Successful to get the status of device.
  @retval EFI_SUCCESS            Failed to get the status of device.

**/
EFI_STATUS
BotStatusPhase (
  IN  EFI_PEI_SERVICES          **PeiServices,
  IN  PEI_BOT_DEVICE            *PeiBotDev,
  OUT UINT8                     *TransferStatus,
  IN  UINT16                    Timeout
  )
{
  CSW             Csw;
  EFI_STATUS      Status;
  PEI_USB_IO_PPI  *UsbIoPpi;
  UINT8           EndpointAddr;
  UINTN           DataSize;

  UsbIoPpi = PeiBotDev->UsbIoPpi;

  ZeroMem (&Csw, sizeof (CSW));

  EndpointAddr  = (PeiBotDev->BulkInEndpoint)->EndpointAddress;

  DataSize      = sizeof (CSW);

  //
  // Get the status field from bulk transfer
  //
  Status = UsbIoPpi->UsbBulkTransfer (
                      PeiServices,
                      UsbIoPpi,
                      EndpointAddr,
                      &Csw,
                      &DataSize,
                      Timeout
                      );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Csw.Signature == CSWSIG) {
    *TransferStatus = Csw.Status;
  } else {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

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
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  PEI_BOT_DEVICE              *PeiBotDev,
  IN  VOID                        *Command,
  IN  UINT8                       CommandSize,
  IN  VOID                        *DataBuffer,
  IN  UINT32                      BufferLength,
  IN  EFI_USB_DATA_DIRECTION      Direction,
  IN  UINT16                      TimeOutInMilliSeconds
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  BotDataStatus;
  UINT8       TransferStatus;
  UINT32      BufferSize;

  BotDataStatus = EFI_SUCCESS;
  //
  // First send ATAPI command through Bot
  //
  Status = BotCommandPhase (
            PeiServices,
            PeiBotDev,
            Command,
            CommandSize,
            BufferLength,
            Direction,
            TimeOutInMilliSeconds
            );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Send/Get Data if there is a Data Stage
  //
  switch (Direction) {
  case EfiUsbDataIn:
  case EfiUsbDataOut:
    BufferSize = BufferLength;

    BotDataStatus = BotDataPhase (
                      PeiServices,
                      PeiBotDev,
                      &BufferSize,
                      DataBuffer,
                      Direction,
                      TimeOutInMilliSeconds
                      );
    break;

  case EfiUsbNoData:
    break;
  }
  //
  // Status Phase
  //
  Status = BotStatusPhase (
            PeiServices,
            PeiBotDev,
            &TransferStatus,
            TimeOutInMilliSeconds
            );
  if (EFI_ERROR (Status)) {
    BotRecoveryReset (PeiServices, PeiBotDev);
    return EFI_DEVICE_ERROR;
  }

  if (TransferStatus == 0x01) {
    return EFI_DEVICE_ERROR;
  }

  return BotDataStatus;
}

/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UsbMassBot.c

Abstract:

  Implementation of the USB mass storage Bulk-Only Transport protocol.

Revision History


**/

#include "UsbMass.h"
#include "UsbMassBot.h"

STATIC
EFI_STATUS
UsbBotResetDevice (
  IN  VOID                    *Context,
  IN  BOOLEAN                  ExtendedVerification
  );


/**
  Initialize the USB mass storage class BOT transport protocol.
  It will save its context which is a USB_BOT_PROTOCOL structure
  in the Context if Context isn't NULL.

  @param  UsbIo                 The USB IO protocol to use
  @param  Controller            The controller to init
  @param  Context               The variable to save the context to

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory
  @retval EFI_UNSUPPORTED       The transport protocol doesn't support the device.
  @retval EFI_SUCCESS           The device is supported and protocol initialized.

**/
STATIC
EFI_STATUS
UsbBotInit (
  IN  EFI_USB_IO_PROTOCOL       * UsbIo,
  IN  EFI_HANDLE                Controller,
  OUT VOID                      **Context OPTIONAL
  )
{
  USB_BOT_PROTOCOL              *UsbBot;
  EFI_USB_INTERFACE_DESCRIPTOR  *Interface;
  EFI_USB_ENDPOINT_DESCRIPTOR   EndPoint;
  EFI_STATUS                    Status;
  UINT8                         Index;

  //
  // Allocate the BOT context, append two endpoint descriptors to it
  //
  UsbBot = AllocateZeroPool (
             sizeof (USB_BOT_PROTOCOL) + 2 * sizeof (EFI_USB_ENDPOINT_DESCRIPTOR)
             );
  if (UsbBot == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UsbBot->UsbIo = UsbIo;

  //
  // Get the interface descriptor and validate that it
  // is a USB MSC BOT interface.
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &UsbBot->Interface);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbBotInit: Get invalid BOT interface (%r)\n", Status));
    goto ON_ERROR;
  }

  Interface = &UsbBot->Interface;

  if (Interface->InterfaceProtocol != USB_MASS_STORE_BOT) {
    Status = EFI_UNSUPPORTED;
    goto ON_ERROR;
  }

  //
  // Locate and save the first bulk-in and bulk-out endpoint
  //
  for (Index = 0; Index < Interface->NumEndpoints; Index++) {
    Status = UsbIo->UsbGetEndpointDescriptor (UsbIo, Index, &EndPoint);

    if (EFI_ERROR (Status) || !USB_IS_BULK_ENDPOINT (EndPoint.Attributes)) {
      continue;
    }

    if (USB_IS_IN_ENDPOINT (EndPoint.EndpointAddress) &&
       (UsbBot->BulkInEndpoint == NULL)) {

      UsbBot->BulkInEndpoint  = (EFI_USB_ENDPOINT_DESCRIPTOR *) (UsbBot + 1);
      CopyMem(UsbBot->BulkInEndpoint, &EndPoint, sizeof (EndPoint));
    }

    if (USB_IS_OUT_ENDPOINT (EndPoint.EndpointAddress) &&
       (UsbBot->BulkOutEndpoint == NULL)) {

      UsbBot->BulkOutEndpoint   = (EFI_USB_ENDPOINT_DESCRIPTOR *) (UsbBot + 1) + 1;
      CopyMem(UsbBot->BulkOutEndpoint, &EndPoint, sizeof(EndPoint));
    }
  }

  if ((UsbBot->BulkInEndpoint == NULL) || (UsbBot->BulkOutEndpoint == NULL)) {
    DEBUG ((EFI_D_ERROR, "UsbBotInit: In/Out Endpoint invalid\n"));
    Status = EFI_UNSUPPORTED;
    goto ON_ERROR;
  }

  //
  // The USB BOT protocol uses dCBWTag to match the CBW and CSW.
  //
  UsbBot->CbwTag = 0x01;

  if (Context != NULL) {
    *Context = UsbBot;
  } else {
    gBS->FreePool (UsbBot);
  }

  return EFI_SUCCESS;

ON_ERROR:
  gBS->FreePool (UsbBot);
  return Status;
}


/**
  Send the command to the device using Bulk-Out endpoint

  @param  UsbBot                The USB BOT device
  @param  Cmd                   The command to transfer to device
  @param  CmdLen                the length of the command
  @param  DataDir               The direction of the data
  @param  TransLen              The expected length of the data

  @retval EFI_NOT_READY         The device return NAK to the transfer
  @retval EFI_SUCCESS           The command is sent to the device.
  @retval Others                Failed to send the command to device

**/
STATIC
EFI_STATUS
UsbBotSendCommand (
  IN USB_BOT_PROTOCOL         *UsbBot,
  IN UINT8                    *Cmd,
  IN UINT8                    CmdLen,
  IN EFI_USB_DATA_DIRECTION   DataDir,
  IN UINT32                   TransLen
  )
{
  USB_BOT_CBW               Cbw;
  EFI_STATUS                Status;
  UINT32                    Result;
  UINTN                     DataLen;
  UINTN                     Timeout;

  ASSERT ((CmdLen > 0) && (CmdLen <= USB_BOT_MAX_CMDLEN));

  //
  // Fill in the CSW. Only the first LUN is supported now.
  //
  Cbw.Signature = USB_BOT_CBW_SIGNATURE;
  Cbw.Tag       = UsbBot->CbwTag;
  Cbw.DataLen   = TransLen;
  Cbw.Flag      = (UINT8) ((DataDir == EfiUsbDataIn) ? 0x80 : 0);
  Cbw.Lun       = 0;
  Cbw.CmdLen    = CmdLen;

  ZeroMem (Cbw.CmdBlock, USB_BOT_MAX_CMDLEN);
  CopyMem (Cbw.CmdBlock, Cmd, CmdLen);

  Result        = 0;
  DataLen       = sizeof (USB_BOT_CBW);
  Timeout       = USB_BOT_SEND_CBW_TIMEOUT / USB_MASS_1_MILLISECOND;

  //
  // Use the UsbIo to send the command to the device. The default
  // time out is enough.
  //
  Status = UsbBot->UsbIo->UsbBulkTransfer (
                            UsbBot->UsbIo,
                            UsbBot->BulkOutEndpoint->EndpointAddress,
                            &Cbw,
                            &DataLen,
                            Timeout,
                            &Result
                            );
  //
  // Respond to Bulk-Out endpoint stall with a Reset Recovery,
  // see the spec section 5.3.1
  //
  if (EFI_ERROR (Status)) {
    if (USB_IS_ERROR (Result, EFI_USB_ERR_STALL) && DataDir == EfiUsbDataOut) {
      UsbBotResetDevice (UsbBot, FALSE);
    } else if (USB_IS_ERROR (Result, EFI_USB_ERR_NAK)) {
      Status = EFI_NOT_READY;
    }
  }

  return Status;
}


/**
  Transfer the data between the device and host. BOT transfer
  is composed of three phase, command, data, and status.

  @param  UsbBot                The USB BOT device
  @param  DataDir               The direction of the data
  @param  Data                  The buffer to hold data
  @param  TransLen              The expected length of the data
  @param  Timeout               The time to wait the command to complete

  @retval EFI_SUCCESS           The data is transferred
  @retval Others                Failed to transfer data

**/
STATIC
EFI_STATUS
UsbBotDataTransfer (
  IN USB_BOT_PROTOCOL         *UsbBot,
  IN EFI_USB_DATA_DIRECTION   DataDir,
  IN OUT UINT8                *Data,
  IN OUT UINTN                *TransLen,
  IN UINT32                   Timeout
  )
{
  EFI_USB_ENDPOINT_DESCRIPTOR *Endpoint;
  EFI_STATUS                  Status;
  UINT32                      Result;

  //
  // It's OK if no data to transfer
  //
  if ((DataDir == EfiUsbNoData) || (*TransLen == 0)) {
    return EFI_SUCCESS;
  }

  //
  // Select the endpoint then issue the transfer
  //
  if (DataDir == EfiUsbDataIn) {
    Endpoint = UsbBot->BulkInEndpoint;
  } else {
    Endpoint = UsbBot->BulkOutEndpoint;
  }

  Result  = 0;
  Timeout = Timeout / USB_MASS_1_MILLISECOND;

  Status = UsbBot->UsbIo->UsbBulkTransfer (
                            UsbBot->UsbIo,
                            Endpoint->EndpointAddress,
                            Data,
                            TransLen,
                            Timeout,
                            &Result
                            );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbBotDataTransfer: (%r)\n", Status));
    if (USB_IS_ERROR (Result, EFI_USB_ERR_STALL)) {
      DEBUG ((EFI_D_ERROR, "UsbBotDataTransfer: DataIn Stall\n"));
      UsbClearEndpointStall (UsbBot->UsbIo, Endpoint->EndpointAddress);
    } else if (USB_IS_ERROR (Result, EFI_USB_ERR_NAK)) {
      Status = EFI_NOT_READY;
    }
  }

  return Status;
}


/**
  Get the command execution status from device. BOT transfer is
  composed of three phase, command, data, and status.
  This function return the transfer status of the BOT's CSW status,
  and return the high level command execution result in Result. So
  even it returns EFI_SUCCESS, the command may still have failed.

  @param  UsbBot                The USB BOT device
  @param  TransLen              The expected length of the data
  @param  Timeout               The time to wait the command to complete
  @param  CmdStatus             The result of the command execution.

  @retval EFI_DEVICE_ERROR      Failed to retrieve the command execute result
  @retval EFI_SUCCESS           Command execute result is retrieved and in the
                                Result.

**/
STATIC
EFI_STATUS
UsbBotGetStatus (
  IN  USB_BOT_PROTOCOL      *UsbBot,
  IN  UINT32                TransLen,
  OUT UINT8                 *CmdStatus
  )
{
  USB_BOT_CSW               Csw;
  UINTN                     Len;
  UINT8                     Endpoint;
  EFI_STATUS                Status;
  UINT32                    Result;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  UINT32                    Index;
  UINTN                     Timeout;

  *CmdStatus = USB_BOT_COMMAND_ERROR;
  Status     = EFI_DEVICE_ERROR;
  Endpoint   = UsbBot->BulkInEndpoint->EndpointAddress;
  UsbIo      = UsbBot->UsbIo;
  Timeout    = USB_BOT_RECV_CSW_TIMEOUT / USB_MASS_1_MILLISECOND;

  for (Index = 0; Index < USB_BOT_RECV_CSW_RETRY; Index++) {
    //
    // Attemp to the read CSW from bulk in endpoint
    //
    ZeroMem (&Csw, sizeof (USB_BOT_CSW));
    Result = 0;
    Len    = sizeof (USB_BOT_CSW);
    Status = UsbIo->UsbBulkTransfer (
                      UsbIo,
                      Endpoint,
                      &Csw,
                      &Len,
                      Timeout,
                      &Result
                      );
    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_ERROR, "UsbBotGetStatus (%r)\n", Status));
      if (USB_IS_ERROR (Result, EFI_USB_ERR_STALL)) {
        DEBUG ((EFI_D_ERROR, "UsbBotGetStatus: DataIn Stall\n"));
        UsbClearEndpointStall (UsbIo, Endpoint);
      }
      continue;
    }

    if (Csw.Signature != USB_BOT_CSW_SIGNATURE) {
      //
      // Invalid Csw need perform reset recovery
      //
      DEBUG ((EFI_D_ERROR, "UsbBotGetStatus: Device return a invalid signature\n"));
      Status = UsbBotResetDevice (UsbBot, FALSE);
    } else if (Csw.CmdStatus == USB_BOT_COMMAND_ERROR) {
      //
      // Respond phase error need perform reset recovery
      //
      DEBUG ((EFI_D_ERROR, "UsbBotGetStatus: Device return a phase error\n"));
      Status = UsbBotResetDevice (UsbBot, FALSE);
    } else {

      *CmdStatus = Csw.CmdStatus;
      break;
    }
  }
  //
  //The tag is increased even there is an error.
  //
  UsbBot->CbwTag++;

  return Status;
}


/**
  Call the Usb mass storage class transport protocol to issue
  the command/data/status circle to execute the commands

  @param  Context               The context of the BOT protocol, that is,
                                USB_BOT_PROTOCOL
  @param  Cmd                   The high level command
  @param  CmdLen                The command length
  @param  DataDir               The direction of the data transfer
  @param  Data                  The buffer to hold data
  @param  DataLen               The length of the data
  @param  Timeout               The time to wait command
  @param  CmdStatus             The result of high level command execution

  @retval EFI_DEVICE_ERROR      Failed to excute command
  @retval EFI_SUCCESS           The command is executed OK, and result in CmdStatus

**/
STATIC
EFI_STATUS
UsbBotExecCommand (
  IN  VOID                    *Context,
  IN  VOID                    *Cmd,
  IN  UINT8                   CmdLen,
  IN  EFI_USB_DATA_DIRECTION  DataDir,
  IN  VOID                    *Data,
  IN  UINT32                  DataLen,
  IN  UINT32                  Timeout,
  OUT UINT32                  *CmdStatus
  )
{
  USB_BOT_PROTOCOL          *UsbBot;
  EFI_STATUS                Status;
  UINTN                     TransLen;
  UINT8                     Result;

  *CmdStatus  = USB_MASS_CMD_FAIL;
  UsbBot      = (USB_BOT_PROTOCOL *) Context;

  //
  // Send the command to the device. Return immediately if device
  // rejects the command.
  //
  Status = UsbBotSendCommand (UsbBot, Cmd, CmdLen, DataDir, DataLen);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbBotExecCommand: UsbBotSendCommand (%r)\n", Status));
    return Status;
  }

  //
  // Transfer the data. Don't return immediately even data transfer
  // failed. The host should attempt to receive the CSW no matter
  // whether it succeeds or failed.
  //
  TransLen = (UINTN) DataLen;
  UsbBotDataTransfer (UsbBot, DataDir, Data, &TransLen, Timeout);

  //
  // Get the status, if that succeeds, interpret the result
  //
  Status = UsbBotGetStatus (UsbBot, DataLen, &Result);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbBotExecCommand: UsbBotGetStatus (%r)\n", Status));
    return Status;
  }

  if (Result == 0) {
    *CmdStatus = USB_MASS_CMD_SUCCESS;
  }

  return EFI_SUCCESS;
}


/**
  Reset the mass storage device by BOT protocol

  @param  Context               The context of the BOT protocol, that is,
                                USB_BOT_PROTOCOL

  @retval EFI_SUCCESS           The device is reset
  @retval Others                Failed to reset the device.

**/
STATIC
EFI_STATUS
UsbBotResetDevice (
  IN  VOID                    *Context,
  IN  BOOLEAN                  ExtendedVerification
  )
{
  USB_BOT_PROTOCOL        *UsbBot;
  EFI_USB_DEVICE_REQUEST  Request;
  EFI_STATUS              Status;
  UINT32                  Result;
  UINT32                  Timeout;

  UsbBot = (USB_BOT_PROTOCOL *) Context;

  if (ExtendedVerification) {
    //
    // If we need to do strictly reset, reset its parent hub port
    //
    Status = UsbBot->UsbIo->UsbPortReset (UsbBot->UsbIo);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Issue a class specific "Bulk-Only Mass Storage Reset reqest.
  // See the spec section 3.1
  //
  Request.RequestType = 0x21;
  Request.Request     = USB_BOT_RESET_REQUEST;
  Request.Value       = 0;
  Request.Index       = UsbBot->Interface.InterfaceNumber;
  Request.Length      = 0;
  Timeout             = USB_BOT_RESET_DEVICE_TIMEOUT / USB_MASS_1_MILLISECOND;

  Status = UsbBot->UsbIo->UsbControlTransfer (
                            UsbBot->UsbIo,
                            &Request,
                            EfiUsbNoData,
                            Timeout,
                            NULL,
                            0,
                            &Result
                            );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbBotResetDevice: (%r)\n", Status));
    return Status;
  }

  //
  // The device shall NAK the host's request until the reset is
  // complete. We can use this to sync the device and host. For
  // now just stall 100ms to wait the device.
  //
  gBS->Stall (USB_BOT_RESET_DEVICE_STALL);

  //
  // Clear the Bulk-In and Bulk-Out stall condition.
  //
  UsbClearEndpointStall (UsbBot->UsbIo, UsbBot->BulkInEndpoint->EndpointAddress);
  UsbClearEndpointStall (UsbBot->UsbIo, UsbBot->BulkOutEndpoint->EndpointAddress);
  return Status;
}


/**
  Clean up the resource used by this BOT protocol

  @param  Context               The context of the BOT protocol, that is,
                                USB_BOT_PROTOCOL

  @retval EFI_SUCCESS           The resource is cleaned up.

**/
STATIC
EFI_STATUS
UsbBotFini (
  IN  VOID                    *Context
  )
{
  gBS->FreePool (Context);
  return EFI_SUCCESS;
}

USB_MASS_TRANSPORT
mUsbBotTransport = {
  USB_MASS_STORE_BOT,
  UsbBotInit,
  UsbBotExecCommand,
  UsbBotResetDevice,
  UsbBotFini
};

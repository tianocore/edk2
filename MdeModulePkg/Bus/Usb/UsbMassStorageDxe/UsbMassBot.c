/** @file
  Implementation of the USB mass storage Bulk-Only Transport protocol,
  according to USB Mass Storage Class Bulk-Only Transport, Revision 1.0.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UsbMass.h"

//
// Definition of USB BOT Transport Protocol
//
USB_MASS_TRANSPORT mUsbBotTransport = {
  USB_MASS_STORE_BOT,
  UsbBotInit,
  UsbBotExecCommand,
  UsbBotResetDevice,
  UsbBotGetMaxLun,
  UsbBotCleanUp
};

/**
  Initializes USB BOT protocol.

  This function initializes the USB mass storage class BOT protocol.
  It will save its context which is a USB_BOT_PROTOCOL structure
  in the Context if Context isn't NULL.

  @param  UsbIo                 The USB I/O Protocol instance
  @param  Context               The buffer to save the context to

  @retval EFI_SUCCESS           The device is successfully initialized.
  @retval EFI_UNSUPPORTED       The transport protocol doesn't support the device.
  @retval Other                 The USB BOT initialization fails.

**/
EFI_STATUS
UsbBotInit (
  IN  EFI_USB_IO_PROTOCOL       *UsbIo,
  OUT VOID                      **Context OPTIONAL
  )
{
  USB_BOT_PROTOCOL              *UsbBot;
  EFI_USB_INTERFACE_DESCRIPTOR  *Interface;
  EFI_USB_ENDPOINT_DESCRIPTOR   EndPoint;
  EFI_STATUS                    Status;
  UINT8                         Index;

  //
  // Allocate the BOT context for USB_BOT_PROTOCOL and two endpoint descriptors.
  //
  UsbBot = AllocateZeroPool (sizeof (USB_BOT_PROTOCOL) + 2 * sizeof (EFI_USB_ENDPOINT_DESCRIPTOR));
  ASSERT (UsbBot != NULL);

  UsbBot->UsbIo = UsbIo;

  //
  // Get the interface descriptor and validate that it
  // is a USB Mass Storage BOT interface.
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &UsbBot->Interface);

  if (EFI_ERROR (Status)) {
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
      CopyMem (UsbBot->BulkOutEndpoint, &EndPoint, sizeof(EndPoint));
    }
  }

  //
  // If bulk-in or bulk-out endpoint is not found, report error.
  //
  if ((UsbBot->BulkInEndpoint == NULL) || (UsbBot->BulkOutEndpoint == NULL)) {
    Status = EFI_UNSUPPORTED;
    goto ON_ERROR;
  }

  //
  // The USB BOT protocol uses CBWTag to match the CBW and CSW.
  //
  UsbBot->CbwTag = 0x01;

  if (Context != NULL) {
    *Context = UsbBot;
  } else {
    FreePool (UsbBot);
  }

  return EFI_SUCCESS;

ON_ERROR:
  FreePool (UsbBot);
  return Status;
}

/**
  Send the command to the device using Bulk-Out endpoint.

  This function sends the command to the device using Bulk-Out endpoint.
  BOT transfer is composed of three phases: Command, Data, and Status.
  This is the Command phase.

  @param  UsbBot                The USB BOT device
  @param  Cmd                   The command to transfer to device
  @param  CmdLen                The length of the command
  @param  DataDir               The direction of the data
  @param  TransLen              The expected length of the data
  @param  Lun                   The number of logic unit

  @retval EFI_SUCCESS           The command is sent to the device.
  @retval EFI_NOT_READY         The device return NAK to the transfer
  @retval Others                Failed to send the command to device

**/
EFI_STATUS
UsbBotSendCommand (
  IN USB_BOT_PROTOCOL         *UsbBot,
  IN UINT8                    *Cmd,
  IN UINT8                    CmdLen,
  IN EFI_USB_DATA_DIRECTION   DataDir,
  IN UINT32                   TransLen,
  IN UINT8                    Lun
  )
{
  USB_BOT_CBW               Cbw;
  EFI_STATUS                Status;
  UINT32                    Result;
  UINTN                     DataLen;
  UINTN                     Timeout;

  ASSERT ((CmdLen > 0) && (CmdLen <= USB_BOT_MAX_CMDLEN));

  //
  // Fill in the Command Block Wrapper.
  //
  Cbw.Signature = USB_BOT_CBW_SIGNATURE;
  Cbw.Tag       = UsbBot->CbwTag;
  Cbw.DataLen   = TransLen;
  Cbw.Flag      = (UINT8) ((DataDir == EfiUsbDataIn) ? BIT7 : 0);
  Cbw.Lun       = Lun;
  Cbw.CmdLen    = CmdLen;

  ZeroMem (Cbw.CmdBlock, USB_BOT_MAX_CMDLEN);
  CopyMem (Cbw.CmdBlock, Cmd, CmdLen);

  Result  = 0;
  DataLen = sizeof (USB_BOT_CBW);
  Timeout = USB_BOT_SEND_CBW_TIMEOUT / USB_MASS_1_MILLISECOND;

  //
  // Use USB I/O Protocol to send the Command Block Wrapper to the device.
  //
  Status = UsbBot->UsbIo->UsbBulkTransfer (
                            UsbBot->UsbIo,
                            UsbBot->BulkOutEndpoint->EndpointAddress,
                            &Cbw,
                            &DataLen,
                            Timeout,
                            &Result
                            );
  if (EFI_ERROR (Status)) {
    if (USB_IS_ERROR (Result, EFI_USB_ERR_STALL) && DataDir == EfiUsbDataOut) {
      //
      // Respond to Bulk-Out endpoint stall with a Reset Recovery,
      // according to section 5.3.1 of USB Mass Storage Class Bulk-Only Transport Spec, v1.0.
      //
      UsbBotResetDevice (UsbBot, FALSE);
    } else if (USB_IS_ERROR (Result, EFI_USB_ERR_NAK)) {
      Status = EFI_NOT_READY;
    }
  }

  return Status;
}


/**
  Transfer the data between the device and host.

  This function transfers the data between the device and host.
  BOT transfer is composed of three phases: Command, Data, and Status.
  This is the Data phase.

  @param  UsbBot                The USB BOT device
  @param  DataDir               The direction of the data
  @param  Data                  The buffer to hold data
  @param  TransLen              The expected length of the data
  @param  Timeout               The time to wait the command to complete

  @retval EFI_SUCCESS           The data is transferred
  @retval EFI_SUCCESS           No data to transfer
  @retval EFI_NOT_READY         The device return NAK to the transfer
  @retval Others                Failed to transfer data

**/
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
  // If no data to transfer, just return EFI_SUCCESS.
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
    if (USB_IS_ERROR (Result, EFI_USB_ERR_STALL)) {
      DEBUG ((EFI_D_INFO, "UsbBotDataTransfer: (%r)\n", Status));
      DEBUG ((EFI_D_INFO, "UsbBotDataTransfer: DataIn Stall\n"));
      UsbClearEndpointStall (UsbBot->UsbIo, Endpoint->EndpointAddress);
    } else if (USB_IS_ERROR (Result, EFI_USB_ERR_NAK)) {
      Status = EFI_NOT_READY;
    } else {
      DEBUG ((EFI_D_ERROR, "UsbBotDataTransfer: (%r)\n", Status));
    }
    if(Status == EFI_TIMEOUT){
      UsbBotResetDevice(UsbBot, FALSE);
    }
  }

  return Status;
}


/**
  Get the command execution status from device.

  This function gets the command execution status from device.
  BOT transfer is composed of three phases: Command, Data, and Status.
  This is the Status phase.

  This function returns the transfer status of the BOT's CSW status,
  and returns the high level command execution result in Result. So
  even if EFI_SUCCESS is returned, the command may still have failed.

  @param  UsbBot         The USB BOT device.
  @param  TransLen       The expected length of the data.
  @param  CmdStatus      The result of the command execution.

  @retval EFI_SUCCESS    Command execute result is retrieved and in the Result.
  @retval Other          Error occurred when trying to get status.

**/
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
    // Attemp to the read Command Status Wrapper from bulk in endpoint
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
      if (USB_IS_ERROR (Result, EFI_USB_ERR_STALL)) {
        UsbClearEndpointStall (UsbIo, Endpoint);
      }
      continue;
    }

    if (Csw.Signature != USB_BOT_CSW_SIGNATURE) {
      //
      // CSW is invalid, so perform reset recovery
      //
      Status = UsbBotResetDevice (UsbBot, FALSE);
    } else if (Csw.CmdStatus == USB_BOT_COMMAND_ERROR) {
      //
      // Respond phase error also needs reset recovery
      //
      Status = UsbBotResetDevice (UsbBot, FALSE);
    } else {
      *CmdStatus = Csw.CmdStatus;
      break;
    }
  }
  //
  //The tag is increased even if there is an error.
  //
  UsbBot->CbwTag++;

  return Status;
}


/**
  Call the USB Mass Storage Class BOT protocol to issue
  the command/data/status circle to execute the commands.

  @param  Context               The context of the BOT protocol, that is,
                                USB_BOT_PROTOCOL
  @param  Cmd                   The high level command
  @param  CmdLen                The command length
  @param  DataDir               The direction of the data transfer
  @param  Data                  The buffer to hold data
  @param  DataLen               The length of the data
  @param  Lun                   The number of logic unit
  @param  Timeout               The time to wait command
  @param  CmdStatus             The result of high level command execution

  @retval EFI_SUCCESS           The command is executed successfully.
  @retval Other                 Failed to execute command

**/
EFI_STATUS
UsbBotExecCommand (
  IN  VOID                    *Context,
  IN  VOID                    *Cmd,
  IN  UINT8                   CmdLen,
  IN  EFI_USB_DATA_DIRECTION  DataDir,
  IN  VOID                    *Data,
  IN  UINT32                  DataLen,
  IN  UINT8                   Lun,
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
  Status = UsbBotSendCommand (UsbBot, Cmd, CmdLen, DataDir, DataLen, Lun);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbBotExecCommand: UsbBotSendCommand (%r)\n", Status));
    return Status;
  }

  //
  // Transfer the data. Don't return immediately even data transfer
  // failed. The host should attempt to receive the CSW no matter
  // whether it succeeds or fails.
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
  Reset the USB mass storage device by BOT protocol.

  @param  Context               The context of the BOT protocol, that is,
                                USB_BOT_PROTOCOL.
  @param  ExtendedVerification  If FALSE, just issue Bulk-Only Mass Storage Reset request.
                                If TRUE, additionally reset parent hub port.

  @retval EFI_SUCCESS           The device is reset.
  @retval Others                Failed to reset the device..

**/
EFI_STATUS
UsbBotResetDevice (
  IN  VOID                    *Context,
  IN  BOOLEAN                 ExtendedVerification
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
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // Issue a class specific Bulk-Only Mass Storage Reset request,
  // according to section 3.1 of USB Mass Storage Class Bulk-Only Transport Spec, v1.0.
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
    return EFI_DEVICE_ERROR;
  }

  //
  // The device shall NAK the host's request until the reset is
  // complete. We can use this to sync the device and host. For
  // now just stall 100ms to wait for the device.
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
  Get the max LUN (Logical Unit Number) of USB mass storage device.

  @param  Context          The context of the BOT protocol, that is, USB_BOT_PROTOCOL
  @param  MaxLun           Return pointer to the max number of LUN. (e.g. MaxLun=1 means LUN0 and
                           LUN1 in all.)

  @retval EFI_SUCCESS      Max LUN is got successfully.
  @retval Others           Fail to execute this request.

**/
EFI_STATUS
UsbBotGetMaxLun (
  IN  VOID                    *Context,
  OUT UINT8                   *MaxLun
  )
{
  USB_BOT_PROTOCOL        *UsbBot;
  EFI_USB_DEVICE_REQUEST  Request;
  EFI_STATUS              Status;
  UINT32                  Result;
  UINT32                  Timeout;

  if (Context == NULL || MaxLun == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbBot = (USB_BOT_PROTOCOL *) Context;

  //
  // Issue a class specific Bulk-Only Mass Storage get max lun reqest.
  // according to section 3.2 of USB Mass Storage Class Bulk-Only Transport Spec, v1.0.
  //
  Request.RequestType = 0xA1;
  Request.Request     = USB_BOT_GETLUN_REQUEST;
  Request.Value       = 0;
  Request.Index       = UsbBot->Interface.InterfaceNumber;
  Request.Length      = 1;
  Timeout             = USB_BOT_RESET_DEVICE_TIMEOUT / USB_MASS_1_MILLISECOND;

  Status = UsbBot->UsbIo->UsbControlTransfer (
                            UsbBot->UsbIo,
                            &Request,
                            EfiUsbDataIn,
                            Timeout,
                            (VOID *) MaxLun,
                            1,
                            &Result
                            );
  if (EFI_ERROR (Status) || *MaxLun > USB_BOT_MAX_LUN) {
    //
    // If the Get LUN request returns an error or the MaxLun is larger than
    // the maximum LUN value (0x0f) supported by the USB Mass Storage Class
    // Bulk-Only Transport Spec, then set MaxLun to 0.
    //
    // This improves compatibility with USB FLASH drives that have a single LUN
    // and either do not return a max LUN value or return an invalid maximum LUN
    // value.
    //
    *MaxLun = 0;
  }

  return EFI_SUCCESS;
}

/**
  Clean up the resource used by this BOT protocol.

  @param  Context         The context of the BOT protocol, that is, USB_BOT_PROTOCOL.

  @retval EFI_SUCCESS     The resource is cleaned up.

**/
EFI_STATUS
UsbBotCleanUp (
  IN  VOID                    *Context
  )
{
  FreePool (Context);
  return EFI_SUCCESS;
}

